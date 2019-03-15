#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

#include "include/mtdm.h"
#include "include/mtdm_proc.h"

static int start(void)
{
	cdb_set("$uartd_nocheck","0");
    exec_cmd("/usr/bin/uartd &");
    return 0;
}
static int stop(void)
{
	cdb_set("$uartd_nocheck","1");
    exec_cmd("killall uartd");
    return 0;
}

static montage_sub_proc_t sub_proc[] = {
    { "boot",  start },
    { "start", start },
    { "stop", stop },
    { .cmd = NULL    }
};

int uartd_main(montage_proc_t *proc, char *cmd)
{
    //system("wavplayer /tmp/res/starting_up.wav");
    MTDM_INIT_SUB_PROC(proc);
    MTDM_SUB_PROC(sub_proc, cmd);
    return 0;
}

