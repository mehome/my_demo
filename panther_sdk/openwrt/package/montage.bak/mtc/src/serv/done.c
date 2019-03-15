#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

#include "include/mtdm.h"
#include "include/mtdm_proc.h"

static int start(void)
{
    char cpuclk[5] = {0};
    char sysclk[5] = {0};
#if 0 
    if (f_exists("/etc/init.d/usb")) {
            exec_cmd("/etc/init.d/usb start");
    }   
    if (f_exists("/etc/rc.local")) {
        exec_cmd("/etc/rc.local");
    }
#endif
    cdb_get("$chip_cpu", cpuclk);
    cdb_get("$chip_sys", sysclk);
    /* don't change system clock running time */
#if 0
    f_write_sprintf("/proc/soc/clk", 0, 0, "%s %s ", cpuclk, sysclk);
#endif
  
    f_write_string("/proc/eth", "sclk", 0, 0);
    f_write_string("/proc/eth", "an", 0, 0);

    f_write_string("/tmp/boot_done", "1", FW_NEWLINE, 0);

    return 0;
}

static montage_sub_proc_t sub_proc[] = {
    { "boot",  start },
    { "start", start },
    { .cmd = NULL    }
};

int done_main(montage_proc_t *proc, char *cmd)
{
    MTDM_INIT_SUB_PROC(proc);
    MTDM_SUB_PROC(sub_proc, cmd);
    return 0;
}

