/* 
 *  Copyright (c) 2016	Montage Inc.	All rights reserved. 
 */
#include <linux/init.h>
#include <linux/kobject.h>
#include <linux/suspend.h>
#include <linux/sysfs.h>
#include <linux/pm.h>
#include <linux/interrupt.h>
#include <linux/compiler.h>
#include <linux/smp.h>

#include <asm/atomic.h>
#include <asm/cacheflush.h>
#include <asm/tlbflush.h>
#include <asm/cpu.h>
#include <asm/processor.h>

#include <asm/io.h>
#include <asm/reboot.h>
#include <asm/r4kcache.h>
#include <asm/mach-panther/panther.h>
#include <asm/mach-panther/reg.h>
#include <asm/mach-panther/pmu.h>

//#include "fastboot.h"
#include "str.h"
//#include "cache.h"

static DEFINE_SPINLOCK(system_suspend_lock);

extern void panther_irq_resume(void);

extern void uart_resume(void);

extern void pdma_suspend(void);
extern void pdma_resume(void);

extern void symphony_spi_suspend(void);
extern void symphony_spi_resume(void);

extern void dragonite_bb_suspend(void);
extern void dragonite_bb_resume(void);

extern void audio_suspend(void);
extern void audio_resume(void);

extern void i2c_mt_suspend(void);
extern void i2c_mt_resume(void);

extern void sram_suspend(void);
extern void sram_resume(void);

void cheetah_serial_outc(unsigned char c);

#if defined(CONFIG_MIPS_MT_SMP)
void vsmp_resume(void);
void vsmp_suspend(void);
#endif

struct reg_backup
{
    unsigned long reg_addr;
    unsigned long reg_val;
};

static struct reg_backup soc_backup_regs[] =
{
    { 0xBF004820, 0 }, // WDOG_TMR_CTRL_REG
};

static void soc_suspend(void)
{
    int i;

    for(i=0;i<sizeof(soc_backup_regs)/sizeof(struct reg_backup);i++)
        soc_backup_regs[i].reg_val = *(volatile unsigned long *) soc_backup_regs[i].reg_addr;
}

static void soc_resume(void)
{
    int i;

    for(i=0;i<sizeof(soc_backup_regs)/sizeof(struct reg_backup);i++)
        *(volatile unsigned long *) soc_backup_regs[i].reg_addr = soc_backup_regs[i].reg_val;
}

static void peripheral_suspend(void)
{
    soc_suspend();

#if defined(CONFIG_MONTAGE_I2C)
    i2c_mt_suspend();
#endif

#if defined(CONFIG_MTD_MT_SYMPHONY_SPI_FLASH)
    symphony_spi_suspend();
#endif

#if defined(CONFIG_PANTHER_PDMA)
    pdma_suspend();
#endif

#if defined(CONFIG_PANTHER_WLAN)
    dragonite_bb_suspend();
#endif
}

static void peripheral_resume(void)
{
    soc_resume();

    uart_resume();

#if defined(CONFIG_MONTAGE_I2C)
    i2c_mt_resume();
#endif

#if defined(CONFIG_PANTHER_WLAN)
    dragonite_bb_resume();
#endif

#if defined(CONFIG_PANTHER_PDMA)
    pdma_resume();
#endif

#if defined(CONFIG_MTD_MT_SYMPHONY_SPI_FLASH)
    symphony_spi_resume();
#endif
}

/* 
   NOTE:
   
   1. test suspend function with following command
      The system will offline all CPUs except CPU0 and then suspend
      echo mem > /sys/power/state
   
   2. stand-alone test cpu1 offline
      echo 0 > /sys/devices/system/cpu/cpu1/online 

   3. stand-alone test cpu1 online
      echo 1 > /sys/devices/system/cpu/cpu1/online  
*/
void panther_machine_suspend(void)
{
    void (*func) (void);
    unsigned long flags;

    spin_lock_irqsave(&system_suspend_lock, flags);

#if defined(CONFIG_PANTHER_SND)
    audio_suspend();
#endif
    peripheral_suspend();

    sram_suspend();
    //memcpy((void *)0x90000000, fastboot, sizeof(fastboot));
    memcpy((void *)0x90000000, str_fw, sizeof(str_fw));

    #define SUSP_TIME_IN_MS 100
    REG_WRITE32(PMU_SLP_TMR_CTRL, SUSP_TIME_IN_MS);  // str_fw will convert ms to tick

    local_flush_tlb_all();
    //flush_tlb_all();
    //HAL_DCACHE_WB_INVALIDATE_ALL();
    //HAL_ICACHE_INVALIDATE_ALL();

#if 1
    r4k_blast_dcache();
    r4k_blast_icache();
#else
    __flush_cache_all();
#endif

    flush_icache_all();

    REG_WRITE32(PMU_WATCHDOG, 0x0);

#if defined(CONFIG_MIPS_MT_SMP)
    vsmp_suspend();
#endif

    func = (void *) 0x90000000UL;
    func();
    asm volatile ("nop;nop;nop;");

    //printk("BACK!!!!\n");
    sram_resume();

    peripheral_resume();

    panther_irq_resume();

#if defined(CONFIG_MIPS_MT_SMP)
    vsmp_resume();
#endif

    //cheetah_serial_outc('B'); cheetah_serial_outc('a'); cheetah_serial_outc('c'); cheetah_serial_outc('k');
    //cheetah_serial_outc('!'); cheetah_serial_outc('\r'); cheetah_serial_outc('\n');

    spin_unlock_irqrestore(&system_suspend_lock, flags);
}

static int panther_pm_enter(suspend_state_t state)
{
    //printk("panther_pm_enter\n");

    panther_machine_suspend();

    return 0;
}

#if defined(CONFIG_MIPS_MT_SMP)
extern int _cpu_down(unsigned int cpu, int tasks_frozen);
int _cpu_up(unsigned int cpu, int tasks_frozen);
#endif

#define SECOND_CPU_ID   1
#if defined(CONFIG_MIPS_MT_SMP)
static int need_resume_2nd_cpu;
#endif
static int panther_pm_begin(suspend_state_t state)
{
    //printk("panther_pm_begin\n");

#if defined(CONFIG_MIPS_MT_SMP)
    if(cpu_online(SECOND_CPU_ID))
    {
        _cpu_down(SECOND_CPU_ID, 1);
        need_resume_2nd_cpu = 1;
    }
    else
    {
        need_resume_2nd_cpu = 0;
    }

    vsmp_suspend();
#endif
    
    return 0;
}

static void panther_pm_end(void)
{
    //printk("panther_pm_end\n");

#if defined(CONFIG_PANTHER_SND)
    audio_resume();
#endif

#if defined(CONFIG_MIPS_MT_SMP)
    if(need_resume_2nd_cpu)
    {
        _cpu_up(SECOND_CPU_ID, 1);
        need_resume_2nd_cpu = 0;
    }
#endif
}

static struct platform_suspend_ops panther_pm_ops = {
	.valid		= suspend_valid_only_mem,
	.begin		= panther_pm_begin,
	.enter		= panther_pm_enter,
	.end		= panther_pm_end,
};

static int __init pm_init(void)
{
    suspend_set_ops(&panther_pm_ops);

    //return sysfs_create_group(power_kobj, &panther_pmattr_group);
    return 0;
}

late_initcall(pm_init);
