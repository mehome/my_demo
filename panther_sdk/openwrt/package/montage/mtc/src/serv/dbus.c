#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

#include "include/mtdm.h"
#include "include/mtdm_proc.h"

static int start(void)
{
    mkdir("/var/lib", 0755);
    mkdir("/var/lib/dbus", 0755);
    mkdir("/var/run/dbus", 0755);

    if(!f_exists("/var/lib/dbus/machine-id"))
        exec_cmd("/usr/bin/dbus-uuidgen --ensure");

    if(!f_exists("/var/run/dbus.pid"))
        unlink("/var/run/dbus.pid");

    start_stop_daemon("/usr/sbin/dbus-daemon", SSDF_START_DAEMON, NICE_DEFAULT, 0, NULL, 
        "--system");
    return 0;
}

static montage_sub_proc_t sub_proc[] = {
    { "boot",  start },
    { "start", start },
    { .cmd = NULL    }
};

int dbus_main(montage_proc_t *proc, char *cmd)
{
    MTDM_INIT_SUB_PROC(proc);
    MTDM_SUB_PROC(sub_proc, cmd);
    return 0;
}

