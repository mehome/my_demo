#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mount.h>
#include <dirent.h>
#include <sys/types.h> 
#include <sys/stat.h>

#include "include/mtdm.h"
#include "include/mtdm_proc.h"
#include "include/mtdm_misc.h"
#include "shutils.h"


static int start()
{
    printf("--------------------------------------------------------------------start-hotplug---\n");
    if(f_isdir("/proc/bus/usb"))
		mount("usbfs", "/proc/bus/usb", "usbfs", MS_MGC_VAL, NULL);

	if( f_exists("/dev/mmcblk0p1"))
		hotplug_block("mmc", "add", "mmcblk0p1");
	else if( f_exists("/dev/mmcblk0p0"))
		hotplug_block("mmc", "add", "mmcblk0p0");
	else if( f_exists("/dev/mmcblk0"))
		hotplug_block("mmc", "add", "mmcblk0");

	if( f_exists("/dev/sda1"))
		hotplug_block("usb", "add", "sda1");
	else if( f_exists("/dev/sda"))
		hotplug_block("usb", "add", "sda");	

	save_diskinfo();
    return 0;
}

static montage_sub_proc_t sub_proc[] = {
    { "boot",  start },
    { "start", start },
    { .cmd = NULL    }
};

int usb_main(montage_proc_t *proc, char *cmd)
{
    MTDM_INIT_SUB_PROC(proc);
    MTDM_SUB_PROC(sub_proc, cmd);
    return 0;
}

