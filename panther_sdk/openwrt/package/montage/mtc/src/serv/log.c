#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

#include "include/mtdm.h"
#include "include/mtdm_proc.h"

static int start(void)
{
    char sys_ip[64] = { 0 };
    int enable = cdb_get_int("$log_rm_enable", 0);

    if ((enable != 0) && f_exists("/sbin/syslogd")) {
        if ((cdb_get_str("$log_sys_ip", sys_ip, sizeof(sys_ip), NULL) != NULL) &&
            (strcmp(sys_ip, ""))){
            start_stop_daemon("/sbin/syslogd", SSDF_START_DAEMON, NICE_DEFAULT, 0, NULL, 
                "-s 1 -L -R %s:514", sys_ip);
        }
        else {
            start_stop_daemon("/sbin/syslogd", SSDF_START_DAEMON, NICE_DEFAULT, 0, NULL, 
                "-s 1");
        }
    }

    return 0;
}

static int stop(void)
{
    return 0;
}

static montage_sub_proc_t sub_proc[] = {
    { "boot",  start },
    { "start", start },
    { "stop",  stop },
    { .cmd = NULL    }
};

int log_main(montage_proc_t *proc, char *cmd)
{
    MTDM_INIT_SUB_PROC(proc);
    MTDM_SUB_PROC(sub_proc, cmd);
    return 0;
}

