#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

#include "include/mtdm.h"
#include "include/mtdm_proc.h"

extern MtdmData *mtdm;

#define FTP_NAME        "stupid-ftpd"
#define FTP_CMDS        "/usr/sbin/"FTP_NAME
#define FTP_CONFILE     "/etc/"FTP_NAME"/"FTP_NAME".conf"
#define FTP_NEW_CONFILE "/tmp/"FTP_NAME".conf"
#define FTP_PIDFILE     "/var/run/"FTP_NAME".pid"
#define FTP_ARGS        "-f "FTP_NEW_CONFILE" -p "FTP_PIDFILE

static int start(void)
{
    int ftp_enable = cdb_get_int("$ftp_enable", 0);

    if (ftp_enable == 1) {
        cp(FTP_CONFILE, FTP_NEW_CONFILE);
        start_stop_daemon(FTP_CMDS, SSDF_START_BACKGROUND_DAEMON, NICE_DEFAULT, 0, NULL, 
            "%s", FTP_ARGS);
    }

    return 0;
}

static int stop(void)
{
    if( f_exists(FTP_PIDFILE) ) {
        start_stop_daemon(FTP_CMDS, SSDF_STOP_DAEMON, 0, SIGKILL, FTP_PIDFILE, NULL);
        unlink(FTP_PIDFILE);
    }
    return 0;
}

static montage_sub_proc_t sub_proc[] = {
    { "boot",  start },
    { "start", start },
    { "stop",  stop  },
    { .cmd = NULL    }
};

int ftp_main(montage_proc_t *proc, char *cmd)
{
    MTDM_INIT_SUB_PROC(proc);
    MTDM_SUB_PROC(sub_proc, cmd);
    return 0;
}

