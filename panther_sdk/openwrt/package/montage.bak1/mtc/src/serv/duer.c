#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

#include "include/mtdm.h"
#include "include/mtdm_proc.h"

#define DUER_BIN_NAME "duer_linux"
#define DUER_LOG_CFG  "l=111000"
#define DUER_LOG_OUT  "2> /dev/null 1> /dev/null"
static int start(void)
{
	cdb_set("$duer_nocheck","0");
	 exec_cmd("/usr/bin/"DUER_BIN_NAME" "DUER_LOG_OUT" &");
//	 cslog("/usr/bin/"DUER_BIN_NAME" "DUER_LOG_OUT" &");
    return 0;
}
static int stop(void)
{
	cdb_set("$duer_nocheck","1");
	exec_cmd("killall -9 > "DUER_BIN_NAME);
    return 0;
}

static montage_sub_proc_t sub_proc[] = {
    { "boot",  start },
    { "start", start },
    { .cmd = NULL    }
};

int duer_main(montage_proc_t *proc, char *cmd)
{
    MTDM_INIT_SUB_PROC(proc);
    MTDM_SUB_PROC(sub_proc, cmd);
    return 0;
}

