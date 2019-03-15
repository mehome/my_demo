#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <linux/limits.h>

#include "include/mtdm.h"
#include "include/mtdm_proc.h"

extern MtdmData *mtdm;

#define DLNA_NAME        "minidlna"
#define DLNA_CMDS        "/usr/bin/"DLNA_NAME
#define DLNA_CONFILE     "/tmp/"DLNA_NAME".conf"
#define DLNA_PIDFILE     "/var/run/"DLNA_NAME".pid"
#define DLNA_ARGS        "-f "DLNA_CONFILE" -P "DLNA_PIDFILE

#define STOR_ROOT_PATH  "/media"
#define STOR_ROOT_PATH_LEN  (sizeof(STOR_ROOT_PATH)-1)
#define DISK_INFO_FILE  "/var/diskinfo.txt"

static int start(void)
{
    FILE *fp;
    char info[PATH_MAX];
    int dlna_enable = cdb_get_int("$dlna_enable", 0);

    if (dlna_enable == 1) {
        dlna_enable = 0;

        if (!f_isdir("/var/run/"DLNA_NAME)) {
            mkdir("/var/run/"DLNA_NAME, 0755);
        }

        if (f_exists(DISK_INFO_FILE) && ((fp = fopen(DISK_INFO_FILE, "r")) != NULL)) {
            while (new_fgets(info, sizeof(info), fp) != NULL) {
                if (!strncmp(info, STOR_ROOT_PATH, STOR_ROOT_PATH_LEN)) {
                    snprintf(info, sizeof(info), "%s/.dlna", info);
                    if (!f_isdir(info)) {
                        mkdir(info, 0755);
                    }
                    dlna_enable = 1;
                    break;
                }
            }
            fclose(fp);
        }
    }

    if (dlna_enable == 1) {
        fp = fopen(DLNA_CONFILE, "w");
        if (fp) {
            fprintf(fp, "port=8200\n");
            fprintf(fp, "network_interface=%s\n", mtdm->rule.LAN);
            fprintf(fp, "friendly_name=DMS\n");
            fprintf(fp, "inotify=yes\n");
            fprintf(fp, "media_dir=/media\n");
            fprintf(fp, "db_dir=%s\n", info);
            fprintf(fp, "log_dir=/dev/null\n");
            fclose(fp);
        }
        start_stop_daemon(DLNA_CMDS, SSDF_START_BACKGROUND_DAEMON, NICE_DEFAULT, 0, NULL, 
            "%s", DLNA_ARGS);
    }

    return 0;
}

static int stop(void)
{
    if( f_exists(DLNA_PIDFILE) ) {
        start_stop_daemon(DLNA_CMDS, SSDF_STOP_DAEMON, 0, SIGKILL, DLNA_PIDFILE, NULL);
        unlink(DLNA_PIDFILE);
    }

    return 0;
}

static montage_sub_proc_t sub_proc[] = {
    { "boot",  start },
    { "start", start },
    { "stop",  stop  },
    { .cmd = NULL    }
};

int dlna_main(montage_proc_t *proc, char *cmd)
{
    MTDM_INIT_SUB_PROC(proc);
    MTDM_SUB_PROC(sub_proc, cmd);
    return 0;
}

