#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <ctype.h>

#include "include/mtdm.h"
#include "include/mtdm_misc.h"
#include "include/mtdm_proc.h"

extern MtdmData *mtdm;

#define SNAPCLIENT_NAME     "snapclient"
#define SNAPCLIENT_CMDS     "/usr/bin/"SNAPCLIENT_NAME
#define SNAPCLIENT_PIDFILE  "/var/run/"SNAPCLIENT_NAME".pid"
#define SNAPCLIENT_CONF     "/etc/"SNAPCLIENT_NAME".default"

#define SNAPSERVER_NAME     "snapserver"
#define SNAPSERVER_CMDS     "/usr/bin/"SNAPSERVER_NAME
#define SNAPSERVER_PIDFILE  "/var/run/"SNAPSERVER_NAME".pid"
#define SNAPSERVER_CONF     "/etc/"SNAPSERVER_NAME".default"

enum SNAP_MODE
{
    SNAP_DISABLE,
    SNAP_SERVER,
    SNAP_CLIENT
};

#ifdef CONFIG_PACKAGE_snapclient
/*
	get arg from file /etc/snap*.default
	retrun 1 means auto-start or 0 not auto-start
*/
#if 0
static int get_snapclient_opt(char* args)
{
    FILE *fp;
    char *ptr;
    char tmp[BUF_SIZE];
    int auto_start = 0;

    if ((fp = fopen(SNAPCLIENT_CONF, "r"))) {
        while ( new_fgets(tmp, sizeof(tmp), fp) != NULL ) {
            if (!strncmp(tmp, "START_SNAPCLIENT=", 17)) {
                if (strstr(tmp + 17, "true"))
                    auto_start = 1;
            }
            if (!strncmp(tmp, "SNAPCLIENT_OPTS=", 16)) {
                ptr = tmp;
                while (*ptr) {
                    if (*ptr++ == '\"') {
                        strcpy(args, ptr);
                        if (args[strlen(args)-1] == '\"')
                            args[strlen(args)-1] = '\0';
                        break;
                    }
                }
            }
        }
        fclose(fp);
    }

    return auto_start;
}
#endif

static int get_snapclient_opt_cdb(char* arg)
{
    char srv_ip[SSBUF_LEN] = {0};

    if (cdb_get_int("$ra_snaptype", 0) != SNAP_CLIENT)
        goto stop;

    cdb_get_str("$snap_target_ip", srv_ip, sizeof(srv_ip), NULL);
    if ((srv_ip != NULL) && strcmp(srv_ip, "0.0.0.0"))
        arg += sprintf(arg, "-h %s", srv_ip);
    else
        goto stop;

    return 1;
stop:
    return 0;
}
#endif

#ifdef CONFIG_PACKAGE_snapserver
/*
	get arg from file /etc/snap*.default
	retrun 1 means auto-start or 0 not auto-start
*/
#if 0
static int get_snapserver_opt(char* args)
{
    FILE *fp;
    char *ptr;
    char tmp[BUF_SIZE];
    int auto_start = 0;

    if ((fp = fopen(SNAPSERVER_CONF, "r"))) {
        while ( new_fgets(tmp, sizeof(tmp), fp) != NULL ) {
            if (!strncmp(tmp, "START_SNAPSERVER=", 17)) {
                if (strstr(tmp + 17, "true"))
                    auto_start = 1;
            }
            if (!strncmp(tmp, "SNAPSERVER_OPTS=", 16)) {
                ptr = tmp;
                while (*ptr) {
                    if (*ptr++ == '\"') {
                        strcpy(args, ptr);
                        if (args[strlen(args)-1] == '\"')
                            args[strlen(args)-1] = '\0';
                        break;
                    }
                }
            }
        }
        fclose(fp);
    }

    return auto_start;
}
#endif

static int get_snapserver_opt_cdb(char* arg)
{
    char srv_name[BUF_SIZE] = {0};

    if (cdb_get_int("$ra_snaptype", 0) != SNAP_SERVER)
        goto stop;

    cdb_get_str("$snap_srv_name", srv_name, sizeof(srv_name), NULL);
    if (srv_name != NULL)
        arg += sprintf(arg, "-s pipe:///tmp/snapfifo?name=%s", srv_name);
    else
        goto stop;

    arg += sprintf(arg, " --streamBuffer 50");

    return 1;
stop:
    return 0;
}
#endif

static int start(void)
{
#if defined(CONFIG_PACKAGE_snapclient) || defined(CONFIG_PACKAGE_snapserver)
    char args[BUF_SIZE];
#endif

#ifdef CONFIG_PACKAGE_snapclient
    memset(args, 0, BUF_SIZE);
    if (get_snapclient_opt_cdb(args)) {
        start_stop_daemon(SNAPCLIENT_CMDS, SSDF_START_BACKGROUND_DAEMON, NICE_DEFAULT, 0, SNAPCLIENT_PIDFILE, args);
    }
#endif

#ifdef CONFIG_PACKAGE_snapserver
    memset(args, 0, BUF_SIZE);
    if (get_snapserver_opt_cdb(args)) {
        start_stop_daemon(SNAPSERVER_CMDS, SSDF_START_BACKGROUND_DAEMON, NICE_DEFAULT, 0, SNAPSERVER_PIDFILE, args);
        exec_cmd("cp /etc/snapcast/asound.conf.fifo /etc/asound.conf");
    }
#endif

    return 0;
}

static int stop(void)
{
#ifdef CONFIG_PACKAGE_snapclient
    if( f_exists(SNAPCLIENT_PIDFILE) ) {
        start_stop_daemon(SNAPCLIENT_CMDS, SSDF_STOP_DAEMON, 0, SIGKILL, SNAPCLIENT_PIDFILE, NULL);
        unlink(SNAPCLIENT_PIDFILE);
    }
#endif

#ifdef CONFIG_PACKAGE_snapserver
    if( f_exists(SNAPSERVER_PIDFILE) ) {
        start_stop_daemon(SNAPSERVER_CMDS, SSDF_STOP_DAEMON, 0, SIGKILL, SNAPSERVER_PIDFILE, NULL);
        unlink(SNAPSERVER_PIDFILE);
        exec_cmd("cp /etc/snapcast/asound.conf.dmix /etc/asound.conf");
    }
#endif
    return 0;
}

static montage_sub_proc_t sub_proc[] = {
    { "boot",  start },
    { "start", start },
    { "stop",  stop  },
    { .cmd = NULL    }
};

int snapcast_main(montage_proc_t *proc, char *cmd)
{
    MTDM_INIT_SUB_PROC(proc);
    MTDM_SUB_PROC(sub_proc, cmd);
    return 0;
}

