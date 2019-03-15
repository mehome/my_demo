#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

#include "include/mtdm.h"
#include "include/mtdm_proc.h"

#define TELNETD_BIN "/usr/sbin/telnetd"
#define TELNETD_NAME "telnetd"
#define TELNETD_PID_FILE "/var/run/telnetd.pid"

static int start(void)
{
#if defined(__PANTHER__)
    start_stop_daemon(TELNETD_BIN, SSDF_START_BACKGROUND_DAEMON, NICE_DEFAULT, 0, TELNETD_PID_FILE, 
        "-F -l /bin/login.sh");
#else
    start_stop_daemon(TELNETD_BIN, SSDF_START_BACKGROUND_DAEMON, NICE_DEFAULT, 0, TELNETD_PID_FILE, 
        "-F -l /bin/login");
#endif
    return 0;
}

static montage_sub_proc_t sub_proc[] = {
    { "boot",  start },
    { "start", start },
    { .cmd = NULL    }
};

int telnet_main(montage_proc_t *proc, char *cmd)
{
    MTDM_INIT_SUB_PROC(proc);
    MTDM_SUB_PROC(sub_proc, cmd);
    return 0;
}

