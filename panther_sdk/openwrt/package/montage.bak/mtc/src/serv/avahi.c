#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

#include "include/mtdm.h"
#include "include/mtdm_proc.h"

static int start(void)
{
    exec_cmd("sysctl -w net.ipv4.conf.default.force_igmp_version=2");
    mkdir("/var/run/avahi-daemon", 0755);
    start_stop_daemon("/usr/sbin/avahi-daemon", SSDF_START_DAEMON, NICE_DEFAULT, 0, NULL, "-D");
    MTDM_LOGGER(this, LOG_INFO, "avahi-daemon is running");
    return 0;
}

static montage_sub_proc_t sub_proc[] = {
    { "boot",  start },
    { "start", start },
    { .cmd = NULL    }
};

int avahi_main(montage_proc_t *proc, char *cmd)
{
    MTDM_INIT_SUB_PROC(proc);
    MTDM_SUB_PROC(sub_proc, cmd);
    return 0;
}

