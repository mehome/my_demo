#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

#include "include/mtdm.h"
#include "include/mtdm_proc.h"

static int start(void)
{
    unlink("/etc/crontabs/root");
    f_write_string("/etc/crontabs/root", "# m h dom mon dow command", FW_NEWLINE, 0);
    start_stop_daemon("/usr/sbin/crond", SSDF_START_DAEMON, NICE_DEFAULT, 0, NULL, NULL);
    return 0;
}

static montage_sub_proc_t sub_proc[] = {
    { "boot",  start },
    { "start", start },
    { .cmd = NULL    }
};

int crond_main(montage_proc_t *proc, char *cmd)
{
    MTDM_INIT_SUB_PROC(proc);
    MTDM_SUB_PROC(sub_proc, cmd);
    return 0;
}

