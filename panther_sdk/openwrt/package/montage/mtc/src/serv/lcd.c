#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

#include "include/mtdm.h"
#include "include/mtdm_proc.h"

#define MCC_PIDFILE    "/var/run/mcc.pid"
#define MCC_BIN        "/usr/bin/mcc"

#define PWMETH "/proc/pwm"

static int start(void)
{
    if(f_exists("/etc/_volume"))
        cp("/etc/_volume", "/etc/hotplug.d/volume/action");

    if(f_exists("/proc/pwm")) {
        f_write_string(PWMETH, "set A ", 0, 0);
        f_write_string(PWMETH, "pol 1 ", 0, 0);
        f_write_string(PWMETH, "mode 0 ", 0, 0);
        f_write_string(PWMETH, "duty 0 ", 0, 0);
        f_write_string(PWMETH, "en 1 ", 0, 0);
    }
    start_stop_daemon(MCC_BIN, SSDF_START_DAEMON, NICE_DEFAULT, 0, NULL, NULL);
    return 0;
}

static int stop(void)
{
    start_stop_daemon(MCC_BIN, SSDF_STOP_DAEMON, 0, SIGTERM, MCC_PIDFILE, NULL);
    
    if(f_exists("/proc/pwm")) {
        f_write_string(PWMETH, "set A ", 0, 0);
        f_write_string(PWMETH, "en 0 ", 0, 0);
    }
    return 0;
}

static montage_sub_proc_t sub_proc[] = {
    { "boot",  start },
    { "start", start },
    { "stop",  stop  },
    { .cmd = NULL    }
};

int lcd_main(montage_proc_t *proc, char *cmd)
{
    MTDM_INIT_SUB_PROC(proc);
    MTDM_SUB_PROC(sub_proc, cmd);
    return 0;
}

