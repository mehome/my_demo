/* 
 *  Copyright (c) 2013	Montage Inc.	All rights reserved. 
 */
#include <linux/cpu.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/ioport.h>
#include <linux/irq.h>
#include <linux/pci.h>
#include <linux/screen_info.h>
#include <linux/time.h>

#include <asm/bootinfo.h>
#include <asm/mach-panther/generic.h>
#include <asm/mach-panther/prom.h>
#include <asm/dma.h>
#include <asm/traps.h>
#include <asm/prom.h>
#ifdef CONFIG_VT
#include <linux/console.h>
#endif

const char *get_system_type(void)
{
	return "Panther";
}

int chip_revision;
static void chip_revision_detection(void)
{
	if((*(volatile unsigned long *)0xbfc03aa0UL)==0x27bdffd8)
		chip_revision = 1;
	else if((*(volatile unsigned long *)0xbfc03aa0UL)==0xac627ff8)
		chip_revision = 2;
}

#ifdef CONFIG_BLK_DEV_IDE
/* check this later */
#endif

extern void panther_reboot_setup(void);
extern void sram_init(void);
void __init plat_mem_setup(void)
{
	char machine_name[64];

	chip_revision_detection();

	sprintf(machine_name, "A%d", chip_revision);
	mips_set_machine_name(machine_name);

	sram_init();

#ifdef CONFIG_DMA_COHERENT
#error CONFIG_DMA_COHERENT not applicable!
#endif

	panther_reboot_setup();

	/* bus error handler setup */
	board_be_init = NULL;
	board_be_handler = NULL;
}


