/*
 * hotplug service
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * $Id: hotplug.c 417161 2015-11-19 14:50:17Z $
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/signal.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/reboot.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <cdb.h>

/* Optimize performance */
#define LOCK_FILE      "/tmp/hotplug_lock"
#define PARTITION_FILE "/proc/partitions"

//#define HOTPLUG_DBG 					1
//#define HOTPLUG_CCHIP_CODE 		1

#define AP_MODE                 1
#define WISP_MODE               3
#define STA_MODE                9

#define WIFI_CONNECT            1
#define WIFI_NOT_CONNECT        0
#define NETWORK_CONNECT         1
#define NETWORK_NOT_CONNECT     0

#define output_console(fmt, args...) (										\
        { \
	  FILE *log = fopen ("/dev/console", "a");							\
	  if (log != NULL) {												\
		fprintf(log, "%s, ", __FUNCTION__); \
		fprintf(log, fmt, ##args); \
		fclose(log);													\
	  }																	\
        } \
)

#ifdef HOTPLUG_DBG
#define dbgprintf output_console
#else
#define dbgprintf(fmt, args...)
#endif /* HOTPLUG_DBG */


int check_partition_num(char *devname)
{
	FILE *fp;
	char buf[128];
	int part = 0;

	fp = fopen(PARTITION_FILE, "r");
	if (fp == NULL) {
		perror("fopen");
		return 0;
	}

	memset(buf, 0, sizeof(buf));
	while (fgets(buf, sizeof(buf), fp) != NULL) {
		unsigned int major, minor, blocks;
		char name[16];

		memset(name, 0, sizeof(name));
		if (sscanf(buf, "%u %u %u %[^\n ]", &major, &minor, &blocks, name) != 4)
			continue;

		//output_console("ID = %u-%u, BLK = %u, NM = %s\n", major,minor, blocks, name );
		if (((devname[0] == 's' || devname[0] == 'h') && !strncmp(name, devname, 3)) ||
			((devname[0] == 'm') && !strncmp(name, devname, 7)))
		{
			part++;
			continue;
		}
	}
	fclose(fp);

	return part;
}
/*
 * Concatenates NULL-terminated list of arguments into a single
 * commmand and executes it
 * @param	argv	argument list
 * @param	path	NULL, ">output", or ">>output"
 * @param	timeout	seconds to wait before timing out or 0 for no timeout
 * @param	ppid	NULL to wait for child termination or pointer to pid
 * @return	return value of executed command or errno
 */
int
_evalpid(char *const argv[], char *path, int timeout, int *ppid)
{
    pid_t pid;
    int   status;
    int   fd;
    int   flags;
    int   sig;

    switch (pid = fork()) {
    case -1:    /* error */
        dbgprintf("failed to fork\n");
        return errno;
    case 0:     /* child */
        /* Reset signal handlers set for parent process */
        for (sig = 0; sig < (_NSIG-1); sig++)
            signal(sig, SIG_DFL);

        /* Clean up */
        ioctl(0, TIOCNOTTY, 0);
        close(STDIN_FILENO);
        setsid();

        /* Redirect stdout to <path> */
        if (path) {
            flags = O_WRONLY | O_CREAT;
            if (!strncmp(path, ">>", 2)) {
                /* append to <path> */
                flags |= O_APPEND;
                path += 2;
			}
			else if (!strncmp(path, ">", 1)) {
                /* overwrite <path> */
                flags |= O_TRUNC;
                path += 1;
            }
            if ((fd = open(path, flags, 0644)) < 0) {
                perror(path);
			}
			else {
                dup2(fd, STDOUT_FILENO);
                close(fd);
            }
        }

        /* execute command */
        setenv("PATH", "/sbin:/bin:/usr/sbin:/usr/bin", 1);
        alarm(timeout);
        execvp(argv[0], argv);
        perror(argv[0]);
        exit(errno);
    default:    /* parent */
        if (ppid) {
            *ppid = pid;
            return 0;
        }
        else
        {
            if (waitpid(pid, &status, 0) == -1)
            {
                if (errno == ECHILD)
                    return 0;
				//dbgprintf("waitpid %d failed\n.", pid );
                return errno;
            }
            if (WIFEXITED(status))
                return WEXITSTATUS(status);
            else
                return status;
        }
    }
}

/*
 * Simple version of _evalpid() (no timeout and wait for child termination, output to console) 
 */
#define eval(cmd, args...) ({ \
	char *argv[] = { cmd, ## args, NULL }; \
	_evalpid(argv, ">/dev/console", 0, NULL); \
})


int mpc_stop()
{
	eval("mpc", "stop");

	return 0;
}

#ifdef HOTPLUG_CCHIP_CODE
void mpc_create_sda_usb_playlist()
{
	char tempbuff[64];
	eval("mpc", "clear");
	sleep(2);
	//eval("mpc", "update", "1");

	eval("mpc", "update");
	//system("mpc listall | grep sda > /usr/script/playlists/usbPlaylist.m3u");
	eval("mpc", "listall", "|", "grep", "sda", ">", "/usr/script/playlists/usbPlaylist.m3u");
	usleep(500000);
	//  system("creatPlayList modifyConf xzxwifiaudio.config.usb_status 1");
	//	eval("playlist.sh", "replace","/usr/script/playlists/usbPlaylist.m3u");
	eval("mpc", "load", "/usr/script/playlists/usbPlaylist.m3u");
	eval("mpc", "play");
	memset(tempbuff, 0, sizeof(tempbuff) / sizeof(char));
	sprintf(tempbuff, "%s", "1");
	WIFIAudio_UciConfig_SearchAndSetValueString("xzxwifiaudio.config.usb_status", tempbuff);

	WIFIAudio_UciConfig_SearchAndSetValueString("xzxwifiaudio.config.playmode", "003");
	eval("uartdfifo.sh", "usbint");
	//	usleep(500000);
	return;
}

void mpc_create_sdb_usb_playlist()
{
	char tempbuff[64];
	eval("mpc", "clear");
	sleep(2);
	//eval("mpc", "update", "1");

	eval("mpc", "update");
	//system("mpc listall | grep sdb > /usr/script/playlists/usbPlaylist.m3u");
	eval("mpc", "listall", "|", "grep", "sdb", ">", "/usr/script/playlists/usbPlaylist.m3u");
	usleep(500000);
	//	eval("playlist.sh", "replace","/usr/script/playlists/usbPlaylist.m3u");
	eval("mpc", "load", "/usr/script/playlists/usbPlaylist.m3u");
	eval("mpc", "play");
	memset(tempbuff, 0, sizeof(tempbuff) / sizeof(char));
	sprintf(tempbuff, "%s", "1");
	WIFIAudio_UciConfig_SearchAndSetValueString("xzxwifiaudio.config.usb_status", tempbuff);

	WIFIAudio_UciConfig_SearchAndSetValueString("xzxwifiaudio.config.playmode", "003");
	eval("uartdfifo.sh", "usbint");
	//	usleep(500000);
	return;
}

void mpc_create_tf_playlist()
{
	char tempbuff[64];
	eval("mpc", "clear");
	sleep(2);
	//eval("mpc", "update", "1");

	eval("mpc", "update");
	//system("mpc listall | grep mmcblk > /usr/script/playlists/tfPlaylist.m3u");
	eval("mpc", "listall", "|", "grep", "mmcblk", ">", "/usr/script/playlists/tfPlaylist.m3u");
	usleep(500000);
	eval("mpc", "load", "/usr/script/playlists/tfPlaylist.m3u");
	eval("mpc", "play");
	memset(tempbuff, 0, sizeof(tempbuff) / sizeof(char));
	sprintf(tempbuff, "%s", "1");
	WIFIAudio_UciConfig_SearchAndSetValueString("xzxwifiaudio.config.tf_status", tempbuff);

	WIFIAudio_UciConfig_SearchAndSetValueString("xzxwifiaudio.config.playmode", "004");
	//eval("uartdfifo.sh", "tfint");
	// system("creatPlayList modifyConf xzxwifiaudio.config.tf_status 1");

	//	usleep(500000);
        return;
}
#endif

int hotplug_block(char *type, char *action, char *name)
{
	char mntdev[128] = { 0 };
	char mntpath[128] = { 0 };
	char tempbuff[16];
	if (!action)
		return -1;

	sprintf(mntdev, "/dev/%s", name);
	sprintf(mntpath, "/media/%s", name);

	dbgprintf("%sing disk %s to %s\n", action, mntdev, mntpath);
	if (!strcmp(action, "add"))
	{
		mkdir(mntpath, 0777);

		eval("mount", mntdev, mntpath);
#ifdef HOTPLUG_DBG		
		dbgprintf("mount ret = = = = = = %d\n", ret);

		dbgprintf("mount type = %s\n", type);
		dbgprintf("mount devname = %s\n", name);
		dbgprintf("mount mntpath = %s\n", mntpath);
#endif

#ifdef HOTPLUG_CCHIP_CODE	
		if(name[0] == 's' )
		{
			//	mpc_create_sda_usb_playlist();
			eval("uartdfifo.sh", "usbint");
			cdb_set("$usb_status", "1");
			eval("playlist.sh", "usb");
    }
		else if(name[0] == 'm' )
		{		
			//eval("mpc", "listall",">","/usr/script/playlists/tfPlaylist.m3u");
			//mpc_create_tf_playlist();
			//cdb_set("$tf_status","1");

			eval("uartdfifo.sh", "tfint");
			cdb_set("$tf_status", "1");
			eval("playlist.sh", "tf");			
		}
#endif		
	}
	else if (!strcmp(action, "remove"))
	{

#ifdef HOTPLUG_CCHIP_CODE	
		char *value;
		value = WIFIAudio_UciConfig_SearchValueString("xzxwifiaudio.config.playmode");
		//if(strcmp(value,"0") == 0)
		// {
		//mpc_stop();
		//eval("mpc", "clear");
		// }
		output_console("usb playmode  = %s\n", value);
#endif

		eval("umount", "-f", mntpath);
		rmdir(mntpath);
		
#ifdef HOTPLUG_CCHIP_CODE			
		if(name[0] == 's' )
		{
			eval("uartdfifo.sh", "usbout");
			cdb_set("$usb_status", "0");
			if (strcmp(value, "003") == 0)
			{

				mpc_stop();
				eval("mpc", "clear");
				eval("creatPlayList", "wifimode");
    }

			//system("creatPlayList modifyConf xzxwifiaudio.config.usb_status 0");
			memset(tempbuff, 0, sizeof(tempbuff) / sizeof(char));

			sprintf(tempbuff, "%s", "0");
			WIFIAudio_UciConfig_SearchAndSetValueString("xzxwifiaudio.config.usb_status", tempbuff);
		}
		else if(name[0] == 'm' )
		{
			eval("uartdfifo.sh", "tfout");
			cdb_set("$tf_status", "0");
			if (strcmp(value, "004") == 0)
			{

				eval("echo", "remove mmcblk");
				mpc_stop();
				eval("mpc", "clear");
				eval("creatPlayList", "wifimode");
        }
			//output_console("tf playmode = %s\n", getPlayerCmd:switchmode);

			//system("creatPlayList modifyConf xzxwifiaudio.config.tf_status 0");

			memset(tempbuff, 0, sizeof(tempbuff) / sizeof(char));
			sprintf(tempbuff, "%s", "0");
			WIFIAudio_UciConfig_SearchAndSetValueString("xzxwifiaudio.config.tf_status", tempbuff);
        }
#endif		
    }

	return 0;
}



/* hotplug block, called by LINUX26 */
int main(int argc, char **argv)
{
	char *action = NULL;
	char *hotplug_type = NULL;
	char *devpath = NULL, *devname = NULL, *devtype = NULL;
	int   retry = 3, lock_fd = -1, ret = 0;
	struct flock lk_info = { 0 };
	
    /* hotplug [event] */
    action = getenv("ACTION");
	
#ifdef HOTPLUG_DBG
	dbgprintf("hotplug [%s], argc = %d\n", action, argc);
#endif

    if (argc >= 2)
    {
        hotplug_type = argv[1];
    }
	if (!strcmp(hotplug_type, "compat_firmware"))
    {
        hotplug_type = "firmware";
    }

	if (!hotplug_type)
    {
        dbgprintf("no hotplug event.\n");
        return EINVAL;
    }

	if ((lock_fd = open(LOCK_FILE, O_RDWR | O_CREAT, 0666)) < 0)
    {
        dbgprintf("Failed opening lock file LOCK_FILE: %s\n", strerror(errno));
        return -1;
    }

    while (--retry)
    {
        lk_info.l_type = F_WRLCK;
        lk_info.l_whence = SEEK_SET;
        lk_info.l_start = 0;
        lk_info.l_len = 0;
        if (!fcntl(lock_fd, F_SETLKW, &lk_info))
        {
            break;
        }
    }


    if (!retry)
    {
        dbgprintf("Failed locking LOCK_FILE: %s\n", strerror(errno));
        return -1;
    }
	
	if (!strcmp(hotplug_type, "block") || !strcmp(hotplug_type, "mmc") || !strcmp(hotplug_type, "usb"))
    {
		int part = 0;

		devpath = getenv("DEVPATH");
		devname = getenv("DEVNAME");
		devtype = getenv("DEVTYPE");
#ifdef HOTPLUG_DBG
		dbgprintf("hotplug_type=%s, action=%s,devname=%s\n", hotplug_type, action, devname);
#endif
		part = check_partition_num(devname);
		if ((!strcmp(devtype, "partition") || (!strcmp(devtype, "disk") && part < 2)) && (!strcmp(action, "add") || !strcmp(action, "remove")))
		{
			ret = hotplug_block(hotplug_type, action, devname);
    }
	}


    close(lock_fd);
    unlink(LOCK_FILE);
	return ret;
}


