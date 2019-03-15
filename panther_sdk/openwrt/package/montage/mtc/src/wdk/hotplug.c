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

#include "include/mtdm.h"
#include "include/mtdm_proc.h"
#include "include/mtdm_misc.h"
#include "shutils.h"

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

//#define HOTPLUG_DBG

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
	//my_system("mpc listall | grep sda > /usr/script/playlists/usbPlaylist.m3u");
	eval("mpc", "listall", "|", "grep", "sda", ">", "/usr/script/playlists/usbPlaylist.m3u");
	usleep(500000);
	//  my_system("creatPlayList modifyConf xzxwifiaudio.config.usb_status 1");
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
	//my_system("mpc listall | grep sdb > /usr/script/playlists/usbPlaylist.m3u");
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
	//my_system("mpc listall | grep mmcblk > /usr/script/playlists/tfPlaylist.m3u");
	eval("mpc", "listall", "|", "grep", "mmcblk", ">", "/usr/script/playlists/tfPlaylist.m3u");
	usleep(500000);
	eval("mpc", "load", "/usr/script/playlists/tfPlaylist.m3u");
	eval("mpc", "play");
	memset(tempbuff, 0, sizeof(tempbuff) / sizeof(char));
	sprintf(tempbuff, "%s", "1");
	WIFIAudio_UciConfig_SearchAndSetValueString("xzxwifiaudio.config.tf_status", tempbuff);

	WIFIAudio_UciConfig_SearchAndSetValueString("xzxwifiaudio.config.playmode", "004");
	//eval("uartdfifo.sh", "tfint");
	// my_system("creatPlayList modifyConf xzxwifiaudio.config.tf_status 1");

	//	usleep(500000);
	return;
}
#endif



/* hotplug block, called by LINUX26 */
int wdk_hotplug(int argc, char **argv)
{
	char *action = NULL;
	char *hotplug_type = NULL;
	char *devname = NULL, *devtype = NULL;
	char *button = NULL;
	char *seen = NULL;
	int   retry = 3, lock_fd = -1, ret = 0;
	struct flock lk_info = { 0 };

	/* hotplug [event] */
	action = getenv("ACTION");

#ifdef HOTPLUG_DBG
	//dbgprintf("hotplug [%s], argc = %d\n", action, argc);
#endif

	if (argc >= 1)
	{
		hotplug_type = argv[0];
	}
	if (!strcmp(hotplug_type, "compat_firmware"))
	{
		hotplug_type = "firmware";
	}
	if (!hotplug_type)
	{
		//dbgprintf("no hotplug event.\n");
		return EINVAL;
	}

	if ((lock_fd = open(LOCK_FILE, O_RDWR | O_CREAT, 0666)) < 0)
	{
		//dbgprintf("Failed opening lock file LOCK_FILE: %s\n", strerror(errno));
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
		//dbgprintf("Failed locking LOCK_FILE: %s\n", strerror(errno));
		close(lock_fd);
		unlink(LOCK_FILE);
		return -1;
	}

	/* (add or remove) && (block or mmc or usb) */
	if ( (!strcmp(action, "add") || !strcmp(action, "remove")) &&
		 (!strcmp(hotplug_type, "block") || !strcmp(hotplug_type, "mmc") || !strcmp(hotplug_type, "usb")) )
	{
		int part = 0;
		
		devtype = getenv("DEVTYPE");
		devname = getenv("DEVNAME");
		if (devname == NULL || strlen(devname) == 0)
		{
			//dbgprintf("devname == NULL\n");
			return 0;
		}

		part = check_partition_num(devname);
#ifdef HOTPLUG_DBG
		dbgprintf("adding devices %s, part = %d\n", devname, part );
#endif
		/* neither partition nor disk */
		if( (!strcmp(devtype, "partition") || (!strcmp(devtype, "disk") && part == 1 )) ) 
        {
            //mpd还没有启动，就先不处理U盘、TF卡事件了
            if(access("/tmp/mpd.pid",0))
            {
                output_console("Booting now.............\n");
                goto EXIT;
            }
#if 0            
            if(!strcmp(action, "add"))
            {
                mount_tf_and_usb(hotplug_type, action, devname);
            }
            else if(!strcmp(action, "remove"))
            {
                umount_tf_and_usb(hotplug_type, action, devname);
            }
#endif
			ret = hotplug_block(hotplug_type, action, devname);		//判断是否有TF卡
		}

	}
    else if (!strcmp(hotplug_type, "button"))
    {
		button = getenv("BUTTON");
		seen = getenv("SEEN");
		hotplug_button(button, action, seen);
		ret = 0;
	}
EXIT:
	close(lock_fd);
	unlink(LOCK_FILE);
	return ret;
}
