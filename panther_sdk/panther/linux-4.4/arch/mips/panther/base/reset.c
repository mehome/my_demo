/* 
 *  Copyright (c) 2016, 2017	Montage Inc.	All rights reserved. 
 */
#include <linux/pm.h>
#include <linux/delay.h>
#include <asm/io.h>
#include <asm/reboot.h>
#include <asm/mach-panther/panther.h>
#include <asm/mach-panther/reg.h>
#include <asm/mach-panther/pmu.h>

static void panther_machine_restart(char *command);
static void panther_machine_halt(void);
static void panther_machine_power_off(void);

static void panther_machine_restart(char *command)
{
    pmu_system_restart();
}

static void panther_machine_halt(void)
{
    pmu_system_halt();
}

static void panther_machine_power_off(void)
{
    printk("System halted. Please turn off power.\n");
    panther_machine_halt();
}

void panther_reboot_setup(void)
{
	_machine_restart = panther_machine_restart;
	_machine_halt = panther_machine_halt;
	pm_power_off = panther_machine_power_off;
}

