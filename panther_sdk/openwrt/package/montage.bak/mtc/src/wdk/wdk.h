
#ifndef WDK_H
#define WDK_H

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <sys/reboot.h>
#include <fcntl.h>
#include <errno.h>
#include <dirent.h>
#include <syslog.h>
#include <time.h>
#include <signal.h>

#include <cdb.h>
#include <shutils.h>
#include <strutils.h>
#include <netutils.h>
#include <linklist.h>

#define WDK_BIN_VERSION 1
//#define PRINT_TO_CONSOLE 1


#define STDOUT printf

#define cprintf(fmt, args...) do { \
        FILE *fp = fopen("/dev/console", "w"); \
        if (fp) { \
                fprintf(fp, fmt , ## args); \
                fclose(fp); \
        } \
} while (0)



#if PRINT_TO_CONSOLE
#define LOG(fmt, args...) \
	do { \
		if (g_log_on) {\
			cprintf(fmt, ## args); \
			cprintf("\n"); \
		} \
	} while (0)
#else
#define LOG(fmt, args...) \
	do { \
		if (g_log_on) \
			syslog(LOG_INFO, fmt, ## args); \
	} while (0)
#endif


struct wdk_program {
    char *name;
    char *description;
    int (*function)(int argc, char **argv);

};



extern int g_log_on;


enum web_state {
    WEB_STATE_DEFAULT=0,
    WEB_STATE_ONGOING=1,
    WEB_STATE_SUCCESS=2,
    WEB_STATE_FAILURE=3,
    WEB_STATE_FWSTART=4,
};

#define WEB_UPLOAD_STATE "/tmp/upload_state"

#define WORK_MODE_AP               1
#define WORK_MODE_WR               2
#define WORK_MODE_WISP             3
#define WORK_MODE_RP               4
#define WORK_MODE_BR               5
#define WORK_MODE_P2P              8
#define WORK_MODE_CLIENT           9

#define OMNICFG_LOG_CLEAN          0
#define OMNICFG_LOG_START          1
#define OMNICFG_LOG_STOP           2
#define OMNICFG_LOG_BACKUP         3

#define OMNICFG_R_UNKNOWN          0 
#define OMNICFG_R_CONNECTING       1
#define OMNICFG_R_SUCCESS          2
#define OMNICFG_R_AP_NOT_FOUND     3
#define OMNICFG_R_PASS_ERROR       4
#define OMNICFG_R_IP_NOT_AVAILABLE 5
#define OMNICFG_R_IP_NOT_USABLE    6
#define OMNICFG_R_CONNECT_TIMEOUT  7

#define WIFI_SCAN_AP_NOT_FOUND     0
#define WIFI_SCAN_AP_FOUND         1
#define WIFI_SCAN_AP_PASS_ERROR    2
#define WIFI_SCAN_AP_SEC_ERROR     3

void dump_file(char *file_path);
int str_equal(char *str1, char *str2);
int wdk_sleep(int argc, char **argv);
int wdk_ap(int argc, char **argv);
int wdk_cdbreset(int argc, char **argv);
int wdk_debug(int argc, char **argv);
int wdk_dhcps(int argc, char **argv);
int wdk_get_channel(int argc, char **argv);
int wdk_logout(int argc, char **argv);
int wdk_ping(int argc, char **argv);
int wdk_reboot(int argc, char **argv);
int wdk_route(int argc, char **argv);
int wdk_wan(int argc, char **argv);
int wdk_mangment(int argc, char **argv);
int wdk_sta(int argc, char **argv);
int wdk_stor(int argc, char **argv);
int wdk_stapoll(int argc, char **argv);
int wdk_status(int argc, char **argv);
int wdk_system(int argc, char **argv);
int wdk_sysupgrade(int argc, char **argv);
int wdk_http(int argc, char **argv);
int wdk_sysupgrade_before(int argc, char **argv);
int wdk_sysupgrade_after(int argc, char **argv);
int wdk_time(int argc, char **argv);
int wdk_upnp(int argc, char **argv);
int wdk_cdbupload(int argc, char **argv);
int wdk_log(int argc, char **argv);
int wdk_logconf(int argc, char **argv);
int wdk_upload(int argc, char **argv);
int wdk_wps(int argc, char **argv);
int wdk_smbc(int argc, char **argv);
int wdk_commit(int argc, char **argv);
int wdk_save(int argc, char **argv);
int wdk_rakey(int argc, char **argv);
int wdk_mpc(int argc, char **argv);
int wdk_pandora(int argc, char **argv);
int wdk_omnicfg(int argc, char **argv);
int wdk_omnicfg_ctrl(int argc, char **argv);
int wdk_omnicfg_apply(int argc, char **argv);
int wdk_hotplug(int argc, char **argv);




int readline(char *path, char* buffer, int buffer_size);
int readline2(char *base_path, char *ext_path, char* buffer, int buffer_size);
int writeline(char *path, char* buffer);
int appendline(char *path, char* buffer);
void update_upload_state(int state);
int get_self_path(char *buf, int len);
int find_pid_by_name( char* proc_name, int* foundpid);
int print_shell_result(char *cmd);

int omnicfg_cdb_save(void);
int omnicfg_simply_commit(void);

/*
shutils:
int exec_cmd(const char *cmd)
int exec_cmd2(const char *cmd, ...)
int exec_cmd3(char *rbuf, unsigned int rbuflen, const char *cmd, ...)
*/


#endif
