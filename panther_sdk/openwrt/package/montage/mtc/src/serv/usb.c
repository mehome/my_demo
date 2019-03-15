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


int update_usb_status()
{
    if( f_exists("/dev/mmcblk0p1"))
        mount_tf_and_usb("mmc", "add", "mmcblk0p1");
    else if( f_exists("/dev/mmcblk0p0"))
        mount_tf_and_usb("mmc", "add", "mmcblk0p0");
    else if( f_exists("/dev/mmcblk0"))
        mount_tf_and_usb("mmc", "add", "mmcblk0");
    
    if( f_exists("/dev/sda1"))
        mount_tf_and_usb("usb", "add", "sda1");
    else if( f_exists("/dev/sda"))
        mount_tf_and_usb("usb", "add", "sda"); 
}

static int start()
{
    printf("--------------------------------------------------------------------start-hotplug---\n");
    if(f_isdir("/proc/bus/usb"))
		mount("usbfs", "/proc/bus/usb", "usbfs", MS_MGC_VAL, NULL);

    update_usb_status();


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

