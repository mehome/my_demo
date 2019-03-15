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

#define LLD2_NAME        "lld2d"
#define LLD2_CMDS        "/usr/sbin/"LLD2_NAME
#define LLD2_PIDFILE     "/var/run/"LLD2_NAME"-%s.pid"

static int start(void)
{
    char lld2_pid[PATH_MAX];
    int lld2_enable = cdb_get_int("$lld2_enable", 0);

    if (lld2_enable == 1) {
        snprintf(lld2_pid, sizeof(lld2_pid), LLD2_PIDFILE, mtdm->rule.LAN);
        if (f_exists(LLD2_CMDS)) {
            start_stop_daemon(LLD2_CMDS, SSDF_START_BACKGROUND_DAEMON, NICE_DEFAULT, 0, lld2_pid, 
                "%s", mtdm->rule.LAN);
        }
    }

    return 0;
}

static int stop(void)
{
    char lld2_pid[PATH_MAX];
    snprintf(lld2_pid, sizeof(lld2_pid), LLD2_PIDFILE, mtdm->rule.LAN);

    if( f_exists(lld2_pid) ) {
        start_stop_daemon(LLD2_CMDS, SSDF_STOP_DAEMON, 0, SIGKILL, lld2_pid, NULL);
        unlink(lld2_pid);
    }

    return 0;
}

static montage_sub_proc_t sub_proc[] = {
    { "boot",  start },
    { "start", start },
    { "stop",  stop  },
    { .cmd = NULL    }
};

int lld2_main(montage_proc_t *proc, char *cmd)
{
    MTDM_INIT_SUB_PROC(proc);
    MTDM_SUB_PROC(sub_proc, cmd);
    return 0;
}

