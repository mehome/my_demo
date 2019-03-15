/*
 * =====================================================================================
 *
 *       Filename:  mtdm_monitor.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  05/17/2016 03:12:26 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author: 
 *   Organization:  
 *
 * =====================================================================================
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <time.h>
#include <strings.h>
#include <stdlib.h>
#include <stdarg.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <sys/ioctl.h>
#include <net/if.h>
#include <netinet/in.h>
#include <linux/ethtool.h>
#include <linux/sockios.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <arpa/inet.h>

#include <ctype.h>
#include <errno.h>
#include <stdint.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <syslog.h>
#include <time.h>
#include <sys/time.h>
#include <mpd/client.h>
#include <mpd/connection.h>
#include <libutils/wlutils.h>
#include "include/mtdm_mon.h"
#include "include/mtdm.h"
#include "include/mtdm_proc.h"

#ifdef MONITOR_INPUTDEV
#include <linux/input.h>
#include <dirent.h>
#include <fcntl.h>
#endif

extern MtdmData *mtdm;


#define IF_DOWN  0
#define IF_UP    1

#define UPMPDCLI_BR0_PIDFILE "/var/run/upmpdcli.br0"
#define UPMPDCLI_BR1_PIDFILE "/var/run/upmpdcli.br1"

#define mon_error(msg) \
 do { perror(msg); exit(EXIT_FAILURE); } while (0)

#define PRO_MAP(_name, _handle, _cdb, _chk, _def)	\
{	\
	.restart	= 1,			\
	.def		= _def,			\
	.name		= (_name),		\
	.handler	= (_handle),	\
	.cdb		= (_cdb),		\
	.check		= (_chk),		\
}

unsigned int musicsrc = MONNONE;
static char ramode;
#ifdef MONITOR_INPUTDEV
static unsigned int mode;
struct timeval otimer;
#endif
struct mon_process
{
	char en;
	char restart;
    char def;
	const char *cdb;
	const char *name;
    void (*handler) (void);
    int (*check) (void);
};

static void mon_log(const char *format, ...)
{
	char buf[512];
	va_list args;
	
	va_start(args, format);
    vsnprintf(buf, sizeof (buf), format, args);
	va_end(args);
	
	openlog("Monitor", 0, 0);
	syslog(LOG_USER | LOG_NOTICE, "%s", buf);
	closelog();
}


static void uhttpd_handler(void)
{
	killall("uhttpd", SIGKILL);
	sleep(1);
	uhttpd_main(NULL, "start");
}


static int uhttpd_check(void)
{
	 	pid_t pid = 0;
		
		pid = get_pid_by_name("uhttpd"); 
		
		if( pid > 0 )
			return 0;
	
		return 1;
}

#if 0
static int proc_filter(const struct dirent *dir)
{
    if (isdigit(dir->d_name[0]))
        return 1;
    else
        return 0;
}

static int mtdaemon_check(void)
{
    struct dirent **namelist;
    int i, ndev;
    char fname[64] = { 0 };
    size_t taskNameSize = 128;
    char *taskName = NULL;
    static unsigned int mtdrst = 0;

    ndev = scandir(PROC_NAME, &namelist, proc_filter, alphasort);

    if (ndev <= 0)
        goto done;

    taskName = calloc(1, taskNameSize);

    for (i = 0; i < ndev; i++)
    {
        memset(fname, 0, sizeof (fname));
        snprintf(fname, sizeof (fname),
                 "%s/%s/cmdline", PROC_NAME, namelist[i]->d_name);

        FILE *cmdline = fopen(fname, "r");
        if (getline(&taskName, &taskNameSize, cmdline) > 0)
        {
            if (strstr(taskName, "mtdaemon") != 0)
            {
                int pid;
                pid = read_pid_file(MTDAEMON_PIDFILE);
                if (pid != atoi(namelist[i]->d_name))
                {
                    cdb_set_int("$mtd_reset", ++mtdrst);
                    fclose(cmdline);
                    kill(pid, SIGKILL);
                    break;
                }
            }
        }
        fclose(cmdline);
    }

    for (i = 0; i < ndev; i++)
    {
        free(namelist[i]);
    }

    free(namelist);
    free(taskName);

done:
    return 0;
}
#endif


#ifndef MONTAGE_BOARD
static void alexa_handler(void)
{
    killall("AlexaRequest", SIGKILL);
    sleep(1);
    exec_cmd("uartdfifo.sh tlkoff");
    exec_cmd("mpc volume 200");
    start_stop_daemon("/usr/bin/AlexaRequest", SSDF_START_BACKGROUND_DAEMON, NICE_DEFAULT, 0, NULL, "");
}


static int alexa_check(void)
{
    pid_t pid = 0;
		
		pid = get_pid_by_name("AlexaRequest"); 
		
		if( pid > 0 )
			return 0;
	
		return 1;
}
#endif

static void mpd_handler(void)
{
    static unsigned int mpdrst = 0;

	cdb_set_int("$mpd_reset", ++mpdrst);

    killall("mpdaemon", SIGKILL);
		sleep(1);
    mpd_main(NULL, "start");

}

static int mpd_check(void)
{
    struct mpd_status *status = NULL;
    struct mpd_connection *conn = NULL;
	int mpd_nocheck = cdb_get_int("$mpd_nocheck", 0);

    if (!mpd_nocheck)
		;
    else
    {
        if (conn)
			mpd_connection_free(conn);
        conn = NULL;
		return 0;
	}

    if (!conn)
		conn = mpd_connection_new("127.0.0.1", 0, 0);
	
	if (conn == NULL)
		goto done;
	
    if (!(status = mpd_run_status(conn)))
		goto done;
	mpd_status_free(status);
    mpd_connection_free(conn);

	return 0;

done:
    if (conn)
		mpd_connection_free(conn);
    conn = NULL;
	mon_log("MPD check fail need to restart");
	return 1;
} 
#if !defined(CONFIG_PACKAGE_shairport_sync_openssl)
static void air_handler(void)
{
    static unsigned int airset = 0;

	cdb_set_int("$air_reset", ++airset);

    shairport_main(NULL, "stop");
	sleep(1);
    //exec_cmd("mtc ra start");
    shairport_main(NULL, "start");
}

static int air_check(void)
{
	int cfd = -1;
	struct sockaddr_un c_addr;
	struct ctrl_param ctrl;
	struct timeval tv;
	int air_nocheck = cdb_get_int("$air_nocheck", 0);

    if (!air_nocheck)
			;
	else
		return 0;
	
    memset(&ctrl, 0, sizeof (struct ctrl_param));
			
	ctrl.type = 3;
	
    if ((cfd = socket(AF_UNIX, SOCK_DGRAM, 0)) == -1)
    {
		perror("fail to create socket");
		return 0;
	}

	tv.tv_sec = 3;
	tv.tv_usec = 0;

    setsockopt(cfd, SOL_SOCKET, SO_SNDTIMEO, (char *) &tv,
               sizeof (struct timeval));

    memset(&c_addr, 0, sizeof (struct sockaddr_un));
	c_addr.sun_family = AF_UNIX;
	strcpy(c_addr.sun_path, AIRS_SOCK);

    if (connect(cfd, (struct sockaddr *) &c_addr, sizeof (c_addr)) < 0)
    {
		mon_log("AirPlay Connect Fail");
		perror("fail to connect");
		goto done;
	}

    if (send(cfd, &ctrl, sizeof (struct ctrl_param) - 1 + ctrl.len, 0) == -1)
    {
		mon_log("AirPlay Send Fail");
		perror("fail to send");
		goto done;
	}
	
	if (cfd >= 0)
		close(cfd);
	
	return 0;

done:
	if (cfd >= 0)
		close(cfd);
	mon_log("AirPlay check fail need to restart");
	return 1;
}
#endif

static int process_nexist(char *pidfile) 
{
	int pid;
	
    if (f_exists(pidfile))
    {
        char buf[32] = { 0 };
		pid = read_pid_file(pidfile);
		sprintf(buf, "/proc/%d", pid);
        if (f_isdir(buf))
			return 0;
		else
			return 1;
	}
	else
		return 1;
}

static int dlna_check(void)
{
	int br0_nocheck = cdb_get_int("$dlnar_br0_nocheck", 0);
	int br1_nocheck = cdb_get_int("$dlnar_br1_nocheck", 0);

    if (br0_nocheck && br1_nocheck)
		return 0;
	
    if (ramode == 2)
    {
        if (mtdm->rule.HOSTAPD && !br0_nocheck)
            if (process_nexist(UPMPDCLI_BR0_PIDFILE))
				return 1;
        if (mtdm->rule.WPASUP)
        {
            if ((!br0_nocheck)
                && ((mtdm->rule.OMODE == OP_BR1_MODE)
                    || (mtdm->rule.OMODE == OP_BR2_MODE)))
            {
                if (process_nexist(UPMPDCLI_BR0_PIDFILE))
					return 1;
			}
            else if (!br1_nocheck)
                if (process_nexist(UPMPDCLI_BR1_PIDFILE))
					return 1;
		}
	}

	return 0;
}

#ifdef CONFIG_PACKAGE_duer_linux  
static void duer_handler(void)
{
    static unsigned int duer_reset = 0;
	cdb_set_int("$duer_reset", ++duer_reset);
    duer_main(NULL,"stop");
    sleep(1);
    duer_main(NULL,"start");
}

static int duer_check(void)
{
	int duer_check = cdb_get_int("$duer_nocheck", 0);
    if (!duer_check)
			;
	else
		return 0;
		
	pid_t pid = get_pid_by_name("duer_linux"); 
	
	if( pid > 0 )
		return 0;

	return 1;
}

#endif
static void dlna_handler(void)
{
    static unsigned int dlnarset = 0;

    upmpdcli_main(NULL, "stop");
	cdb_set_int("$dlnar_reset", ++dlnarset);
    //exec_cmd("mtc ra restart");
    upmpdcli_main(NULL, "start");
}

/* BEGIN: Added by Frog, 2018/6/20 */

//设备发现用的  utf-8
static void airkiss_discover_handler(void)
{
    //unlink("/tmp/airkissDis.pid");

    eval("killall", "-9", "airkissDis");
    killall("airkissDis", SIGKILL);
    sleep(1);
    eval_nowait("airkissDis");
}

//如果已经连接到了路由器上就一直发广播包，保证用户绑定设备时可以发现设备 utf-8
static int airkiss_discover_check(void)
{
    pid_t pid = 0;

    int iot_nocheck = cdb_get_int("$iot_nocheck", 0);

    if (iot_nocheck || !f_exists("/usr/bin/airkissDis"))
        return 0;
    
    int wanif_state = cdb_get_int("$wanif_state", 0);
    if(wanif_state != 2) 
    {
        pid = get_pid_by_name("airkissDis");
        if(pid > 0) 
        {
            killall("airkissDis", SIGKILL);
            eval("killall", "-9", "airkissDis");
        }
        return 0;
    }
    pid = get_pid_by_name("airkissDis"); 

    if( pid > 0 )
        return 0;

    return 1;
}

static void turing_handler(void)
{
	unlink("/tmp/turing_iot.pid");

	eval("killall", "-9","turingIot");
	killall("turingIot", SIGKILL);
	sleep(1);
	eval_nowait("turingIot");
	
}

static int turing_check(void)
{
	pid_t pid = 0;
	int omi_result ;
	int iot_nocheck = cdb_get_int("$iot_nocheck", 0);

	if (iot_nocheck || !f_exists("/usr/bin/turingIot"))
	return 0;

	pid = get_pid_by_name("turingIot"); 

	if( pid > 0 )
		return 0;

	return 1;
}
/* END:   Added by Frog, 2018/6/20 */

static void uartd_handler(void){
	static int uartd_reset=0;
	uartd_main(NULL,"stop");
	cdb_set_int("$uartd_reset",++uartd_reset);
    uartd_main(NULL,"start");
}
static int uartd_check(void){
	pid_t pid = 0;
	int uartd_nocheck = cdb_get_int("$uartd_nocheck", 0);
	if (uartd_nocheck)return 0;
	pid = get_pid_by_name("uartd"); 
	if( pid > 0 )return 0;
	return 1;
}

static struct mon_process proc_set[] = {
#ifdef CONFIG_PACKAGE_mpd-mini
    PRO_MAP("mpdaemon", mpd_handler, "$ra_func", mpd_check, 0),
#endif 	
#if !defined(CONFIG_PACKAGE_shairport_sync_openssl)
    PRO_MAP("shairport", air_handler, "$ra_func", air_check, 0),
#endif
    PRO_MAP("upmpdcli", dlna_handler, "$ra_func", dlna_check, 0),
#ifndef MONTAGE_BOARD    
    PRO_MAP("alexa", alexa_handler, "$ra_func", alexa_check, 0),
#endif
	PRO_MAP("uhttpd", uhttpd_handler, "$ra_func", uhttpd_check,0),
#ifdef CONFIG_PACKAGE_duer_linux    
		PRO_MAP("duer", duer_handler, "$ra_func", duer_check, 0),
#endif

#ifdef CONFIG_PACKAGE_airkiss
    PRO_MAP("airkiss_discover",airkiss_discover_handler, "$ra_func",airkiss_discover_check,0),
#endif

#ifdef CONFIG_PACKAGE_iot
    PRO_MAP("turingIot", turing_handler, "$ra_func", turing_check,0),
#endif
#ifdef CONFIG_PACKAGE_uartd 
	PRO_MAP("uartd", uartd_handler, "$ra_func", uartd_check,0),
#endif
};

static void check_process(void)
{
    int i, val = 0;

    for (i = 0; i < ARRAY_SIZE(proc_set); i++)
    {
        val = cdb_get_int(proc_set[i].cdb, proc_set[i].def);

        if (!val)
            proc_set[i].en = 0;
        else
            proc_set[i].en = 1;
    }
}
#ifdef CONFIG_PACKAGE_mpd-mini
static int last_volume = 0;
static void monitor_volume_handler(void)
{
	int volume;
	volume = cdb_get_int("$ra_vol", 0);

    if (volume != last_volume)
    {

        last_volume = volume;
        volume = 201 + volume;
#ifdef SHAIRPORT_UPDATE_VOLUME
		double svolume;
		double svol = volume * 1.0;
		double shairport_volume_max = 0.0;
		double shairport_volume_min = -30.0;

        svolume =
            (double) (((svol / 100.0) *
                       (shairport_volume_max - shairport_volume_min) +
                       shairport_volume_min) + 0.5);
#endif
        char cmd[64] = { 0 };
        char name[32] = { 0 };
        if (cdb_get_str("$current_volume_change", name, sizeof (name), NULL) !=
            NULL)
        {
            if (strcmp(name, "mpc") == 0)
            {
                char cmd[64] = { 0 };
#ifdef SHAIRPORT_UPDATE_VOLUME
                bzero(cmd, sizeof (cmd));
                snprintf(cmd, sizeof (cmd), "air_cli v %f > /dev/null 2>&1",
                         svolume);
                exec_cmd(cmd);
#endif

                bzero(cmd, sizeof (cmd));
                snprintf(cmd, sizeof (cmd),
                         "mspotify_cli v %d  > /dev/null 2>&1", volume);
                exec_cmd(cmd);
            }
            else if (strcmp(name, "spotify") == 0)
            {
#ifdef SHAIRPORT_UPDATE_VOLUME
                bzero(cmd, sizeof (cmd));
                snprintf(cmd, sizeof (cmd), "air_cli v %f > /dev/null 2>&1",
                         svolume);
                exec_cmd(cmd);
#endif
                bzero(cmd, sizeof (cmd));
                snprintf(cmd, sizeof (cmd), "mpc volume %d > /dev/null 2>&1",
                         volume);
                exec_cmd(cmd);
            }
            else if (strcmp(name, "shairport") == 0)
            {
                bzero(cmd, sizeof (cmd));
                snprintf(cmd, sizeof (cmd),
                         "mspotify_cli v %d > /dev/null 2>&1", volume);
                exec_cmd(cmd);

                bzero(cmd, sizeof (cmd));
                snprintf(cmd, sizeof (cmd), "mpc volume %d > /dev/null 2>&1",
                         volume);
                exec_cmd(cmd);
            }
            else
	{
#ifdef SHAIRPORT_UPDATE_VOLUME
                bzero(cmd, sizeof (cmd));
                snprintf(cmd, sizeof (cmd), "air_cli v %f > /dev/null 2>&1",
                         svolume);
                exec_cmd(cmd);
#endif

                bzero(cmd, sizeof (cmd));
                snprintf(cmd, sizeof (cmd),
                         "mspotify_cli v %d > /dev/null 2>&1", volume);
                exec_cmd(cmd);

                bzero(cmd, sizeof (cmd));
                snprintf(cmd, sizeof (cmd), "mpc volume %d > /dev/null 2>&1",
                         volume);
                exec_cmd(cmd);
            }
        }
		else
        {
#ifdef SHAIRPORT_UPDATE_VOLUME
            bzero(cmd, sizeof (cmd));
            snprintf(cmd, sizeof (cmd), "air_cli v %f > /dev/null 2>&1",
                     svolume);
            exec_cmd(cmd);
#endif

            bzero(cmd, sizeof (cmd));
            snprintf(cmd, sizeof (cmd), "mspotify_cli v %d > /dev/null 2>&1",
                     volume);
            exec_cmd(cmd);

            bzero(cmd, sizeof (cmd));
            snprintf(cmd, sizeof (cmd), "mpc volume %d > /dev/null 2>&1",
                     volume);
            exec_cmd(cmd);
        }
	}
}
#endif
static void monitor_process_handler(void)
{
	int i;
	check_process();

    for (i = 0; i < ARRAY_SIZE(proc_set); i++)
	{
		int ret;
						
        if (proc_set[i].restart == 0)
			continue;

		if ((ret = proc_set[i].check()) == 0)
			proc_set[i].restart = 0;
	}

    /*Restart Process */
    for (i = 0; i < ARRAY_SIZE(proc_set); i++)
	{
        if (proc_set[i].en == 0)
			proc_set[i].restart = 0;
        else if (proc_set[i].restart == 0)
			proc_set[i].restart = 1;
		else
        {
            if (proc_set[i].handler)
			proc_set[i].handler();
	}
    }
}
//#ifdef CONFIG_PACKAGE_upmpdcli
static unsigned int parse_handle_msg(const char *input)
{
    struct mon_param *monctrl = (struct mon_param *) input;
    int ret = 0;

    mon_log("MODE:%d\n", monctrl->type);
    switch (monctrl->type)
    {
		case MONDLNA:
			ret = ctrl_play_dlna(musicsrc);
			break;
		case MONAIRPLAY:
			ret = ctrl_play_airplay(musicsrc);
			break;
		case MONSPOTIFY:
			ret = ctrl_play_spotify(musicsrc);
			break;
		default:
            if (musicsrc == MONNONE)
				ret = 0;
            else
            {
				ret = 1; 
				perror("Not support now");
			}
			break;
	}
	musicsrc = monctrl->type;
	cdb_set_int("$current_play_mode", musicsrc);
	
	//exec_cmd("cdb commit");
	return ret;
}

static void handle_mon(int fd)
{
	struct sockaddr_un from;
	char msg[MAX_BUF_LEN];
    socklen_t fromlen = sizeof (from);
	ssize_t recvsize;
	int ret;

		memset(msg , 0 , sizeof(msg));
    recvsize = recvfrom(fd, msg, MAX_BUF_LEN, 0, (struct sockaddr *) &from,
		     &fromlen);

    if (recvsize <= 0)
    {
		perror("Recv size less 0");
	}

	ret = parse_handle_msg(msg);

    if (sendto(fd, &ret, 1, 0, (struct sockaddr *) &from, fromlen) == -1)
    {
		perror("ctrl socket fail to send");
	}
}
//#endif
#ifdef MONITOR_INPUTDEV
static int is_event_device(const struct dirent *dir)
{
	return strncmp(EVENT_DEV_NAME, dir->d_name, 5) == 0;
}

#ifdef MONITOR_GPIO_INPUTDEV
static void rst_ctrl(struct input_event *ev)
{
	unsigned int code = ev->code;
	unsigned int value = ev->value;
    struct timeval *ntimer = (struct timeval *) &ev->time;
	
    if (value == BTN_PRESS)
    {
		otimer.tv_sec = ev->time.tv_sec + RST_TO_DFT;
		otimer.tv_usec = ev->time.tv_usec;
	}
    else if (value == BTN_RELEASE)
    {
        if (timercmp(&otimer, ntimer, <=))
        {
            exec_cmd("/lib/wdk/cdbreset");
            killall("upmpdcli", SIGUSR1);
            reboot(RB_AUTOBOOT);
		}
        else
        {
            killall("upmpdcli", SIGUSR1);
            reboot(RB_AUTOBOOT);
		}
	}
    else if (value == BTN_CONTINUE)
    {
        if (timercmp(&otimer, ntimer, <=))
        {
            exec_cmd("/lib/wdk/cdbreset");
            killall("upmpdcli", SIGUSR1);
            reboot(RB_AUTOBOOT);
		}
	}
}

static void wps_ctrl(struct input_event *ev)
{
	unsigned int code = ev->code;
	unsigned int value = ev->value;
    struct timeval *ntimer = (struct timeval *) &ev->time;
	
    if (value == BTN_PRESS)
    {
		otimer.tv_sec = ev->time.tv_sec + OMNI_TRI;
		otimer.tv_usec = ev->time.tv_usec;
	}
    else if (value == BTN_RELEASE)
    {
        if (timercmp(&otimer, ntimer, <=))
        {
            exec_cmd("/lib/wdk/omnicfg_ctrl trigger");
		}
        else
        {
            exec_cmd("hostapd_cli wps_pbc");
            exec_cmd("wpa_cli wps_pbc");
		}
	}
    else if (value == BTN_CONTINUE)
    {
        if (timercmp(&otimer, ntimer, <=))
        {
            exec_cmd("/lib/wdk/omnicfg_ctrl trigger");
		}
	}
}

static void sw_ctrl(struct input_event *ev)
{
	unsigned int code = ev->code;
	unsigned int value = ev->value;
	static unsigned int cnt = 1;
    struct timeval *ntimer = (struct timeval *) &ev->time;
	
    if (value == BTN_PRESS)
    {
		otimer.tv_sec = ev->time.tv_sec + 1;
		otimer.tv_usec = ev->time.tv_usec;
	}
    else if (value == BTN_RELEASE)
    {
		unsigned int val;
		int ra_func = cdb_get_int("$ra_func");
	
        mode = mode ? mode : ra_func;
		mode += cnt;
        val = mode % 3;
        mode = (val) ? val : 3;
        if (mode == ra_func)
			mode = 0;
		cnt = 1;
	}
    else if (value == BTN_CONTINUE)
    {
        if (timercmp(&otimer, ntimer, <=))
        {
			otimer.tv_sec = ntimer->tv_sec + 1;		
			otimer.tv_usec = ntimer->tv_usec;
			cnt++;
		}
	}
}

static int input_gpio_action(struct input_event *ev)
{
	unsigned int code = ev->code;

    switch (code)
    {
		case RB_RST:
			rst_ctrl(ev);
			break;
		case WPS:
			wps_ctrl(ev);
			break;
		case SWITCH:
			sw_ctrl(ev);
			break;
		default:
            mon_log("Not support code:%d\n", code);
			break;
	}
	return 0;
}
#endif
#ifdef MONITOR_MADC_INPUTDEV
static void vol_ctrl(struct input_event *ev)
{
	unsigned int code = ev->code;
	unsigned int value = ev->value;
    struct timeval *ntimer = (struct timeval *) &ev->time;

    if (value == BTN_PRESS)
    {
		otimer.tv_sec = ev->time.tv_sec + 1;		
		otimer.tv_usec = ev->time.tv_usec;		
	}
    else if (value == BTN_RELEASE)
    {
        if (code == VOLUP)
        {
            if (musicsrc == MONDLNA)
                exec_cmd("mpc volume +5");
            else if (musicsrc == MONAIRPLAY)
                exec_cmd("air_cli d volumeup");
		}
        else
        {
            if (musicsrc == MONDLNA)
                exec_cmd("mpc volume -5");
            else if (musicsrc == MONAIRPLAY)
                exec_cmd("air_cli d volumedown");
		}
	}
    else if (value == BTN_CONTINUE)
    {
        if (timercmp(&otimer, ntimer, <=))
        {
			otimer.tv_sec = ntimer->tv_sec + 1;		
			otimer.tv_usec = ntimer->tv_usec;
            if (code == VOLUP)
            {
                if (musicsrc == MONDLNA)
                    exec_cmd("mpc volume +5");
                else if (musicsrc == MONAIRPLAY)
                    exec_cmd("air_cli d volumeup");
			}
            else
            {
                if (musicsrc == MONDLNA)
                    exec_cmd("mpc volume -5");
                else if (musicsrc == MONAIRPLAY)
                    exec_cmd("air_cli d volumedown");
			}
		}
	}
	else
		mon_log("Vol ctrl not support\n");
	
}

static void seq_ctrl(struct input_event *ev)
{
	unsigned int code = ev->code;
	unsigned int value = ev->value;
    struct timeval *ntimer = (struct timeval *) &ev->time;

    if (value == BTN_RELEASE)
    {
        if (code == FORWARD)
        {
            if (musicsrc == MONDLNA)
                exec_cmd("mpc next");
            else if (musicsrc == MONAIRPLAY)
                exec_cmd("air_cli d nextitem");
		}
        else
        {
            if (musicsrc == MONDLNA)
                exec_cmd("mpc prev");
            else if (musicsrc == MONAIRPLAY)
                exec_cmd("air_cli d previtem");
		}
	}
}

static void play_ctrl(struct input_event *ev)
{
	unsigned int code = ev->code;
	unsigned int value = ev->value;
    struct timeval *ntimer = (struct timeval *) &ev->time;

    if (value == BTN_RELEASE)
    {
        if (musicsrc == MONDLNA)
            exec_cmd("mpc toggle");
        else if (musicsrc == MONAIRPLAY)
            exec_cmd("air_cli d playpause");
	}
}

static int input_madc_action(struct input_event *ev)
{
	unsigned int code = ev->code;

    switch (code)
    {
		case VOLUP:
		case VOLDWN:
			vol_ctrl(ev);
			break;
		case FORWARD:
		case BACKWARD:
			seq_ctrl(ev);
			break;
		case TOGGLE:
			play_ctrl(ev);
			break;
		default:
            mon_log("Not support code:%d\n", code);
			break;
	}
	
	return 0;
}
#endif
#endif


#if defined(__PANTHER__)
static void mpd_mode_chk(void)
#else
static void mpd_mode_chk(int signum, siginfo_t * bar, void *baz)
#endif
{
	int cmode = cdb_get_int("$ra_func", 0);
	
    mon_log("o:%d c:%d\n", ramode, cmode);
	
    if (ramode != cmode)
    {
        struct mpd_connection *session = NULL;
		
        ramode = cmode;
		
		session = mpd_connection_new("127.0.0.1", 0, 0);
		
		if (session == NULL)
			return;
		mpd_run_clear(session);
		mpd_connection_free(session);
	}
	return;
}

extern void aplist_monitor_timer_enable(int from_omnicfg, int sec, char *msg);
extern void mtc_delay_commit_timer_enable(void);

static void get_peer_ap_info_from_aplist(int *ta_mode, char *ta_ssid, char *ta_pass, int *ta_type, int *ta_cipher)
{
    char target[MSBUF_LEN] = { 0 };
    char wl_aplist_info[MSBUF_LEN] = { 0 };
    char wl_aplist_pass[MSBUF_LEN] = { 0 };
    char *argv[10];
    char *ssid;
    char *pass = wl_aplist_pass;
    char *mode;
    char *tc;
    int num;
    int i;

    exec_cmd3(target, sizeof(target), "iw dev %s link | sed -n 's/^.*SSID: //p'", mtdm->rule.STA);
    if (strlen(target) == 0) {
        return;
    }
	
    for (i=1;i<11;i++) {
        if (cdb_get_array_str("$wl_aplist_info", i, wl_aplist_info, sizeof(wl_aplist_info), NULL) == NULL) {
			break;
        }
        else if (strlen(wl_aplist_info) == 0) {
            break;
        }
        else {
            cdb_get_array_str("$wl_aplist_pass", i, wl_aplist_pass, sizeof(wl_aplist_pass), "");

            num = str2argv(wl_aplist_info, argv, '&');
            if((num < 7)               || !str_arg(argv, "bssid=")  || 
               !str_arg(argv, "sec=")  || !str_arg(argv, "cipher=") || 
               !str_arg(argv, "last=") || !str_arg(argv, "mode=")   || 
               !str_arg(argv, "tc=")   || !str_arg(argv, "ssid=")) {
                continue;
            }
            ssid = str_arg(argv, "ssid=");
            mode = str_arg(argv, "mode=");
            tc   = str_arg(argv, "tc=");
            if ((strlen(ssid) == 0) || (strlen(mode) == 0)) {
                continue;
            }

            if (strcmp(ssid, target) == 0) {
                if (strlen(pass) != 0) {
                    strcpy(ta_pass, pass);
                }
                strcpy(ta_ssid, ssid);
                if (sscanf(mode, "%d", ta_mode) != 1) {
                    MTDM_LOGGER(this, LOG_INFO, "%s: skip, mode is not digtal", __func__);
                    *ta_mode = OP_UNKNOWN;
                    continue;
                }
                if (sscanf(tc, "%d:%d", ta_type, ta_cipher) != 2) {
                    MTDM_LOGGER(this, LOG_INFO, "%s: skip, tc is not digtal", __func__);
                    *ta_mode = OP_UNKNOWN;
                    continue;
                }
                break;
            } 
        }
    }
}

static void update_wan(void)
{       
    if (mtdm->rule.WIFLINK == IF_UP) {
        logger(LOG_INFO, "%s: link up", mtdm->rule.IFPLUGD);
    } else {
        logger(LOG_INFO, "%s: link down", mtdm->rule.IFPLUGD); 
    }

    if (cdb_get_int("$wl_aplist_en", 0) == 1) {
        if (mtdm->rule.WIFLINK == IF_UP) {
            char ssid[128] = {0};
            char pass[128] = {0};
            int mode = OP_UNKNOWN;
            int type = 0;
            int cipher = 0;
            get_peer_ap_info_from_aplist(&mode, ssid, pass, &type, &cipher);
            if ((mode != OP_UNKNOWN) && 
                (strlen(ssid) > 0) && 
                (mode != mtdm->rule.OMODE)) {

                logger(LOG_ERR, "aplist: work mode[%d] not match [%d][%s][%s][%d][%d], RESTART!!", 
                    mtdm->rule.OMODE, mode, ssid, pass, type, cipher);

                cdb_set_int("$op_work_mode", mode);

                cdb_set_int("$wl_bss_cipher2", cipher);
                cdb_set_int("$wl_bss_sec_type2", type);
                if (type & 4) {
                    cdb_set("$wl_bss_wep_1key2", pass);
                    cdb_set("$wl_bss_wep_2key2", pass);
                    cdb_set("$wl_bss_wep_3key2", pass);
                    cdb_set("$wl_bss_wep_4key2", pass);
    }
    else {
                    cdb_set("$wl_bss_wpa_psk2", pass);
                }
                cdb_set("$wl_bss_ssid2", ssid);
                mtc_delay_commit_timer_enable();
                return;
            }
        }
        else {
            if(get_pid_by_name("airkiss") > 0) {
                ;
            }
            else if (cdb_get_int("$omi_result", 0) != OMNICFG_R_CONNECTING) {
                /* NOT under omnicfg, 
                 * to wait wpa_supplicant to reconnect other AP in aplist
                 * start a timer of 20 seconds to determine rollback AP mode
                 */
                if ((mtdm->rule.STA[0] != 0) && (mtdm->misc.reconf_wpa != 0)) {
                    mtdm->misc.reconf_wpa = 0;
                    start_stop_daemon("/usr/sbin/wpa_cli", SSDF_START_BACKGROUND_DAEMON, NICE_DEFAULT, 0, 
                        NULL, "-i %s reconfigure", mtdm->rule.STA);
                    logger(LOG_ERR, "%s: reconfigure wpa_supplicant by new wpa.conf", __func__);
                }
                aplist_monitor_timer_enable(0, 20, "wifi disconnected");
            }
        }
    }

    if (mtdm->rule.WIFLINK == IF_UP) {
        wan_main(NULL,"start");
    } else {
        wan_main(NULL,"stop");
    }
}

static int ifplugd_wifi_mon(void)
{
	int fd;
	struct sockaddr_nl addr;

    if ((fd = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE)) == -1)
	    return -1;
	
    memset(&addr, 0, sizeof (addr));
	addr.nl_family = AF_NETLINK;
	/*
		Currently Disable RTMGRP_IPV4_IFADDR
		coz DHCPC will do this
	*/
	//addr.nl_groups = RTMGRP_LINK | RTMGRP_IPV4_IFADDR;
	addr.nl_groups = RTMGRP_LINK;
	
    if ((bind(fd, (struct sockaddr *) &addr, sizeof (addr))) == -1)
	    return -1;

	return fd;

}

static void parsenetlinkstatus(struct nlmsghdr *nlh)
{
    int len = nlh->nlmsg_len - sizeof (*nlh);
    struct ifinfomsg *ifi;
    struct rtattr *rta;

    if (sizeof (struct ifinfomsg) > (size_t) len)
        return;

    ifi = (struct ifinfomsg *) NLMSG_DATA(nlh);
    if ((ifi->ifi_flags & IFF_LOOPBACK) != 0)
        return;

    rta =
        (struct rtattr *) ((char *) ifi +
                           NLMSG_ALIGN(sizeof (struct ifinfomsg)));
    len = NLMSG_PAYLOAD(nlh, sizeof (struct ifinfomsg));

    for (;RTA_OK(rta, len); rta = RTA_NEXT(rta, len))
    {
        if (rta->rta_type == IFLA_IFNAME)
        {   
#if defined(__PANTHER__)
            /*  detect link up with NL events found to have problem on first boot after upgrading firmware
                The IFF_RUNNING flag does not set, even the sta0 interface is up&running.
                Using SIOCGIFFLAGS ioctl , however, can detect the IFF_RUNNING properly
                Thus, we need this work-around for safer sta0 link-up detection
             */

#ifndef IFF_LOWER_UP
#define IFF_LOWER_UP  (0x01 << 16)
#endif

            char name[IFNAMSIZ];
            char link;

            memset(name, 0, IFNAMSIZ);
            if_indextoname(ifi->ifi_index, name);

            /* 1.use (IFF_UP|IFF_RUNNING|IFF_LOWER_UP) flags to check link up/down can avoid
             *   when rebooting on AP MODE DUT will send dhcp packet.
             * 2.check interface name: only allow sta0 interface to check link up/down.
             * 3.check link: to avoid repeat update_wan.
             */
            link = (((ifi->ifi_flags & (IFF_UP|IFF_RUNNING|IFF_LOWER_UP)) == ((IFF_UP|IFF_RUNNING|IFF_LOWER_UP))))?IF_UP:IF_DOWN;
            if (!(strcmp(mtdm->rule.IFPLUGD, name)) && (mtdm->rule.WIFLINK != link))
            {
                mtdm->rule.WIFLINK = link;
                update_wan();
            }
#else
            char name[IFNAMSIZ];
            char link;
            
            memset(name, 0, IFNAMSIZ);

            if_indextoname(ifi->ifi_index, name);
			link = (((ifi->ifi_flags & (IFF_UP|IFF_RUNNING)) == ((IFF_UP|IFF_RUNNING)))) ? IF_UP : IF_DOWN;
            if (!(strcmp(mtdm->rule.IFPLUGD, name)) && (mtdm->rule.WIFLINK != link))
            {
				mtdm->rule.WIFLINK = link;
                update_wan();
            }
#endif
        }
    }
}

static void parsenetlinkaddr(struct nlmsghdr *nlh)
{       
    struct ifaddrmsg *ifa = (struct ifaddrmsg *) NLMSG_DATA(nlh);
    struct rtattr *rta = IFA_RTA(ifa);
    int len = IFA_PAYLOAD(nlh);
                
    while (len && RTA_OK(rta, len))
    {
        if (rta->rta_type == IFA_LOCAL)
        {
            unsigned int ipaddr = htonl(*((unsigned int *) RTA_DATA(rta)));
            char name[IFNAMSIZ];

						memset(name, 0, IFNAMSIZ);
            if_indextoname(ifa->ifa_index, name);
            if (!strcmp(mtdm->rule.IFPLUGD, name))
            {
                cprintf("%s GOT/CHANGE IP:%d.%d.%d.%d\n",
                    name,
                    (ipaddr >> 24) & 0xff,
                    (ipaddr >> 16) & 0xff,
                        (ipaddr >> 8) & 0xff, ipaddr & 0xff);
            }
        }
        rta = RTA_NEXT(rta, len);
    }
}

static void handle_ifplugd_wifi(int fd)
{
	int len;
	char buffer[4096];
	struct nlmsghdr *nlh;

    memset(buffer, 0 , sizeof(buffer));
    len = recv(fd, buffer, sizeof(buffer), 0);
    //cprintf(" @@@@ recv %d, errno %d\n", len, errno);

    nlh = (struct nlmsghdr *) buffer;
    while ((NLMSG_OK(nlh, len)) && (nlh->nlmsg_type != NLMSG_DONE))
    {
        if ((nlh->nlmsg_type == RTM_NEWADDR))
			parsenetlinkaddr(nlh);
        else if ((nlh->nlmsg_type == RTM_NEWLINK)
                 || (nlh->nlmsg_type == RTM_DELLINK))
			parsenetlinkstatus(nlh);
	
		nlh = NLMSG_NEXT(nlh, len);
	}
}

void do_monitor_timer(void);
static void mon_loop(void)
{
	int monfd = -1;
	int wififd = -1;
	int maxfd = -1;
	fd_set fds;
	struct timeval tv;
	int ret;
	
	struct sockaddr_un addr;

#ifdef MONITOR_INPUTDEV
	struct dirent **namelist;
	int i, ndev;
	int madcfd = -1;
	int gpiofd = -1;
	struct input_event ev;
#endif

    if ((monfd = socket(AF_UNIX, SOCK_DGRAM, 0)) == -1)
    {
		perror("Fail to create mon socket");
		return;
	}

    memset(&addr, 0, sizeof (struct sockaddr_un));
	addr.sun_family = AF_UNIX;
	strcpy(addr.sun_path, MONS_SOCK);
	unlink(addr.sun_path);

    if (bind(monfd, (struct sockaddr *) &addr, sizeof (addr)) == -1)
    {
		perror("Fail to bind");
		goto done;
	}

    if ((wififd = ifplugd_wifi_mon()) < 0)
		goto done;

#ifdef MONITOR_INPUTDEV
	ndev = scandir(DEV_INPUT_EVENT, &namelist, is_event_device, alphasort);
	
	if (ndev <= 0)
		goto done;
	
	for (i = 0; i < ndev; i++)
	{
		char fname[64];
		int fd =  -1;
		char name[256] = "???";

        snprintf(fname, sizeof (fname),
			 "%s/%s", DEV_INPUT_EVENT, namelist[i]->d_name);
		fd = open(fname, O_RDONLY);
		if (fd < 0)
			continue;
        ioctl(fd, EVIOCGNAME(sizeof (name)), name);

#ifdef MONITOR_MADC_INPUTDEV
        if (!strcmp(name, "madc-key"))
        {
			madcfd = fd;
			fprintf(stderr, "%s:	%s\n", fname, name);
		}
#endif
#ifdef MONITOR_GPIO_INPUTDEV
        if (!strcmp(name, "cta-gpio-buttons"))
        {
			gpiofd = fd;
			fprintf(stderr, "%s:	%s\n", fname, name);
		}
#endif
	}

	free(namelist[i]);
#endif

    while (1)
    {
		do_monitor_timer();

		tv.tv_sec = 2;
		tv.tv_usec = 0;

		FD_ZERO(&fds);

		FD_SET(monfd, &fds);
		FD_SET(wififd, &fds);
#ifdef MONITOR_INPUTDEV
        if (madcfd > 0)
			FD_SET(madcfd, &fds);
        if (gpiofd > 0)
			FD_SET(gpiofd, &fds);
#ifdef MONITOR_GPIO_INPUTDEV
		maxfd = MAX(maxfd, gpiofd);
#endif
#ifdef MONITOR_MADC_INPUTDEV
		maxfd = MAX(maxfd, madcfd);
#endif
#endif
		maxfd = MAX(maxfd, wififd);
		maxfd = MAX(maxfd, monfd);

		ret = select(maxfd + 1, &fds, 0, 0, &tv);

		if(mtdm->terminate)
			break;

		if (ret == 0)
			continue;
        else if (ret == -1)
        {
			if (errno == EINTR)
				continue;
			break;
		}

#if defined(__PANTHER__)
#if defined(CONFIG_PACKAGE_mpd_mini)
        if(mtdm->exec_mpd_mode_chk)
        {
            mtdm->exec_mpd_mode_chk = 0;
            mpd_mode_chk();
        }

        if (FD_ISSET(monfd, &fds))
        {
			handle_mon(monfd);
		}
#endif
#endif		
        if (FD_ISSET(wififd, &fds))
        {
			handle_ifplugd_wifi(wififd);
		}
#ifdef MONITOR_MADC_INPUTDEV
        if (FD_ISSET(madcfd, &fds))
        {
            if (safe_read(madcfd, &ev, sizeof (ev)) > 0)
            {
                input_madc_action((struct input_event *) &ev);
			}
		}
#endif
#ifdef MONITOR_GPIO_INPUTDEV
        if (FD_ISSET(gpiofd, &fds))
        {
            if (safe_read(gpiofd, &ev, sizeof (ev)) > 0)
            {
                input_gpio_action((struct input_event *) &ev);
			}
		}
#endif
	}

done:
    if (monfd >= 0)
		close(monfd);
    if (wififd >= 0)
		close(wififd);
#ifdef MONITOR_INPUTDEV
    if (madcfd >= 0)
		close(madcfd);
#endif
#ifdef MONITOR_GPIO_INPUTDEV
    if (gpiofd >= 0)
		close(gpiofd);
#endif
}

static int detect_eif_up_down(char *ifname, int skfd)
{
    struct ifreq ifr;
    struct ethtool_value edata;

    edata.cmd = ETHTOOL_GLINK;
    memset(&ifr, 0, sizeof (struct ifreq));
                    
    ifr.ifr_data = (void *) &edata;
        
    strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
    ioctl(skfd, SIOCETHTOOL, &ifr);
	
	return edata.data ? IF_UP : IF_DOWN;
}

static void ifplugd_eth_monitor(void)
{	
    int skfd;

    if ((skfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("socket");
        return;
    }

    if ((mtdm->rule.OMODE == OP_DB_MODE)
        || (mtdm->rule.OMODE == OP_WIFIROUTER_MODE)
#if 0 // defined(__PANTHER__)  // enable this would cause two update_wan() call racing
        || (mtdm->rule.OMODE == OP_WISP_MODE)
        || (mtdm->rule.OMODE == OP_STA_MODE)
#endif
        || (mtdm->rule.OMODE == OP_P2P_MODE))
    {
        char link;
        if (mtdm->rule.WIFLINK != IF_UP)
        {
            ifconfig(mtdm->rule.IFPLUGD, IFUP, NULL, NULL);
        }
        link = detect_eif_up_down(mtdm->rule.IFPLUGD, skfd);
        if (link != mtdm->rule.WIFLINK)
        {
            mtdm->rule.WIFLINK = link;
            update_wan();
        }
    }

    if (mtdm->rule.OMODE != OP_P2P_MODE)
        detect_eif_up_down("eth0.3", skfd);

    if (skfd)
        close(skfd);
}

void do_monitor_timer(void)
{
    static struct timespec lastrun = { 0, 0 };
    struct timespec now;

    if(mtdm->bootState != bootState_done)
        return;

    clock_gettime(CLOCK_MONOTONIC, &now);
    if((now.tv_sec - lastrun.tv_sec)>=2)
    {
        lastrun.tv_sec = now.tv_sec;
    }
    else
    {
        return;
    }

	/*
		To monitor ethernet interface link status
		using polling instead of netlink because we cannot detect phy link/unlink
	*/
	ifplugd_eth_monitor();

	monitor_process_handler();

//	monitor_volume_handler();

	return;
}

int mtdm_monitor_init(char *argv0)
{
    pid_t child;

    child = fork();
    if(child < 0)
    {
        logger(LOG_ERR, "monitor thread is NOT running!!");
        return 0;
    }
    else if(child > 0) {
        return 1;
    }

    memset(argv0, 0, strlen(argv0));
    strcpy(argv0, "monitor");

    ramode = cdb_get_int("$ra_func", 0);
#ifdef CONFIG_PACKAGE_mpd-mini
    cdb_set("$mpd_nocheck", "1");
#endif
#if !defined(CONFIG_PACKAGE_shairport_sync_openssl)
    cdb_set("$air_nocheck", "1");
#endif
#ifdef CONFIG_PACKAGE_upmpdcli
    cdb_set("$dlnar_br0_nocheck", "1");
    cdb_set("$dlnar_br1_nocheck", "1");
#endif
#ifdef CONFIG_PACKAGE_duer_linux
    cdb_set("$duer_nocheck", "1");
#endif
#ifdef CONFIG_PACKAGE_uartd 
	cdb_set("$uartd_nocheck", "1");
#endif
    //signal_setup();

    mon_loop();

    exit(0);

    return 0;
}

