#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

#include "include/mtdm.h"
#include "include/mtdm_proc.h"

/* 
* format: BTN_x $gpio $low_trig $type "${desc}" 
*/
static int init_all_gpio(void)
{
	int i, btn_gnum[2];
	char cdb_name[16];
#if 0	
	 /* init reset gpio, 6 */
	 f_write_string("/proc/cta-gpio-buttons", "stop\n", 0, 0);
	 f_write_string("/proc/cta-gpio-buttons", "set 0 6 1 1 \"reboot/reset\"\n", 0, 0);
	 f_write_string("/sys/class/gpio/export","6\n", 0, 0);
	 f_write_string("/sys/class/gpio/gpio6/direction", "in\n", 0, 0);
	 
	 /* init wps gpio, 12 */
	 f_write_string("/proc/cta-gpio-buttons", "set 1 12 1 1 \"wps\"\n", 0, 0);
	 f_write_string("/sys/class/gpio/export","12\n", 0, 0);
	 f_write_string("/sys/class/gpio/gpio12/direction", "in\n", 0, 0);
	 
	 /* init switch gpio, 8 */
	 f_write_string("/proc/cta-gpio-buttons", "set 2 8 1 1 \"switch\"\n", 0, 0);
	 f_write_string("/sys/class/gpio/export","8\n", 0, 0);
	 f_write_string("/sys/class/gpio/gpio8/direction", "in\n", 0, 0);
	 
	 /* start */
	 f_write_string("/proc/cta-gpio-buttons", "start\n", 0, 0);
#endif	 
	exec_cmd("rmmod gpio-button-hotplug.ko");
	for (i = 0; i < 2; i++)
	{
		sprintf(cdb_name, "$hw_gpio_btn%d", i);
		btn_gnum[i] = cdb_get_int(cdb_name, -1);
	}
	exec_cmd2("insmod /lib/modules/4.4.14/gpio-button-hotplug.ko btn0_gnum=%d btn1_gnum=%d",
                  btn_gnum[0], btn_gnum[1]);
	 
	 return 0;
}

/*
* initializing all madc
* format: set $chan $type
*/
static int init_all_madc(void)
{
#if 1
	 //diable madc
	 f_write_string("/proc/soc/power", "madc 0", 0, 0 );
#else	 		
	 /* format: set $chan $type , init chan 1, 2 */
	 f_write_string("/proc/madc", "total 2\n", 0, 0);
	 f_write_string("/proc/madc", "stop\n", 0, 0);
	 f_write_string("/proc/madc", "reset\n", 0, 0);
	 /* set $chan $type */
	 f_write_string("/proc/madc", "set 0 2\n", 0, 0);
	 f_write_string("/proc/madc", "set 1 2\n", 0, 0);
	 f_write_string("/proc/madc", "start\n", 0, 0);
#endif	 
	 return 0;
}

static int start(void)
{
    char codec[BUF_SIZE];

    f_write_string("/proc/eth", "sclk\n", 0, 0);
    f_write_string("/proc/eth", "an\n", 0, 0);

    if (setup_codec_info(codec, sizeof(codec)) == EXIT_SUCCESS) {
        cdb_set("$mpd_codec_info", codec);
    }
    else {
        MTDM_LOGGER(this, LOG_ERR, "setup_codec_info is failed!!");
    }

    init_all_gpio();
    init_all_madc();

#ifdef POWER_DOWN_USB
    // to disable usb default
    exec_cmd2("power-down -M usb");
    exec_cmd2("power-down -off");
#endif
#ifdef POWER_DOWN_EPHY
    // to disable ephy(mdio) default
    exec_cmd2("power-down -M mdio");
    exec_cmd2("power-down -off");
#endif
#ifdef POWER_DOWN_MADC
    // to disable madc default
    exec_cmd2("power-down -M madc");
    exec_cmd2("power-down -off");
#endif

    return 0;
}

static montage_sub_proc_t sub_proc[] = {
    { "boot",  start },
    { "start", start },
    { .cmd = NULL    }
};

int wdk_main(montage_proc_t *proc, char *cmd)
{
    MTDM_INIT_SUB_PROC(proc);
    MTDM_SUB_PROC(sub_proc, cmd);
    return 0;
}

