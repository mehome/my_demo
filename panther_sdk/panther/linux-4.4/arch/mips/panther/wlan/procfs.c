/*=============================================================================+
|                                                                              |
| Copyright 2017                                                               |
| Montage Inc. All right reserved.                                             |
|                                                                              |
+=============================================================================*/
/*
*   \file 
*   \brief
*   \author Montage
*/

#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/sched.h>
#include <linux/mutex.h>
#include <linux/sort.h>
#include <linux/skbuff.h>
#include <linux/ctype.h>

#include "os_compat.h"
#include "rf.h"
#include "bb.h"
#include "dragonite_main.h"
#include "mac.h"
#include "wla_ctrl.h"
#include "mac_regs.h"

#define WLA_PROCFS_NAME	"wla"

#define SEQ_FILE_BUFSIZE    PAGE_SIZE
#define MAX_CMD_STRING_LENGTH   256

static char ___procfs_cmd[MAX_CMD_STRING_LENGTH];
struct mac80211_dragonite_data *global_data;

struct wla_config
{
    char *name;
    void *data;
    int array_size;
    int (*config_set)(void *, void *, int);
    int (*config_get)(void *, struct seq_file *, int);
    int (*update)(void *, struct seq_file *, int);
    char *hint;
};

int config_set_int(void *cfg, void *input, int index)
{
    struct wla_config *working_cfg = (struct wla_config *) cfg;
    int *working_data = (int *) working_cfg->data;

    if(working_cfg->array_size)
    {
        if(index<working_cfg->array_size)
        {
            working_data[index] = simple_strtoul(input, NULL, 10);
        
            printk(KERN_DEBUG "wla: set %s.%d=%d\n", working_cfg->name, index, working_data[index]);    
        }
    }
    else
    {
        *working_data = simple_strtoul(input, NULL, 10);
    
        printk(KERN_DEBUG "wla: set %s=%d\n", working_cfg->name, *working_data);
    }

    return 0;
}

int config_get_int(void *cfg, struct seq_file *s, int index)
{
    struct wla_config *working_cfg = (struct wla_config *) cfg;
    int *working_data = (int *) working_cfg->data;

    if(working_cfg->array_size)
    {
        if(index<working_cfg->array_size)
            seq_printf(s, "%d\n", working_data[index]);
        else
            seq_printf(s, "\n");
    }
    else
    {
        seq_printf(s, "%d\n", *working_data);
		printk("wla: %s=%d\n",working_cfg->name, *working_data);
    }

    return 0;
}

int config_set_u64(void *cfg, void *input, int index)
{
    struct wla_config *working_cfg = (struct wla_config *) cfg;
    u64 *working_data = (u64 *) working_cfg->data;

    if(working_cfg->array_size)
    {
        if(index<working_cfg->array_size)
        {
            working_data[index] = simple_strtoull(input, NULL, 16);
        
            printk(KERN_DEBUG "wla: set %s.%d=%llx\n", working_cfg->name, index, working_data[index]);
        }
    }
    else
    {
        *((u32 *) working_cfg->data) = simple_strtoull(input, NULL, 16);
    
        printk(KERN_DEBUG "wla: set %s=%llx\n", working_cfg->name, *((u64 *) working_cfg->data));
    }

    return 0;
}

int config_get_u64(void *cfg, struct seq_file *s, int index)
{
    struct wla_config *working_cfg = (struct wla_config *) cfg;
    u64 *working_data = (u64 *) working_cfg->data;

    if(working_cfg->array_size)
    {
        if(index<working_cfg->array_size)
            seq_printf(s, "%llx\n", working_data[index]);
        else
            seq_printf(s, "\n");
    }
    else
    {
        seq_printf(s, "%llx\n", *working_data);
    }

    return 0;
}

int config_set_u32(void *cfg, void *input, int index)
{
    struct wla_config *working_cfg = (struct wla_config *) cfg;
    u32 *working_data = (u32 *) working_cfg->data;

    if(working_cfg->array_size)
    {
        if(index<working_cfg->array_size)
        {
            working_data[index] = simple_strtoul(input, NULL, 16);
        
            printk(KERN_DEBUG "wla: set %s.%d=%x\n", working_cfg->name, index, working_data[index]);
        }
    }
    else
    {
        *((u32 *) working_cfg->data) = simple_strtoul(input, NULL, 16);
    
        printk(KERN_DEBUG "wla: set %s=%x\n", working_cfg->name, *((u32 *) working_cfg->data));
    }

    return 0;
}

int config_get_u32(void *cfg, struct seq_file *s, int index)
{
    struct wla_config *working_cfg = (struct wla_config *) cfg;
    u32 *working_data = (u32 *) working_cfg->data;

    if(working_cfg->array_size)
    {
        if(index<working_cfg->array_size)
            seq_printf(s, "%x\n", working_data[index]);
        else
            seq_printf(s, "\n");
    }
    else
    {
        seq_printf(s, "%x\n", *working_data);
    }

    return 0;
}

int config_set_rfc(void *cfg, void *input, int index)
{
    struct wla_config *working_cfg = (struct wla_config *) cfg;
    char *input_str = (char *) input;
    char temp_str[3];
    int i;
    int input_bytes = 0;
    int max_input_bytes = MAX_RFC_REG_NUM * 2;
    u8* rfc_data = (u8 *) &rfc_tbl[0];

    char *rf_setting;
    u32  reg, val;

    memset(&rfc_tbl, 0, sizeof(struct bb_regs) * MAX_RFC_REG_NUM);
    memset(&rf_tbl, 0, sizeof(struct rf_regs) *  MAX_RF_REG_NUM);

    rf_setting = input_str;
    i = 0;
    while((rf_setting = strchr(rf_setting, ',')))
    {
        *rf_setting = '\0';
        rf_setting++;

        reg = simple_strtoul(rf_setting, NULL, 16);

        if((rf_setting = strchr(rf_setting, '=')))
        {
            *rf_setting = '\0';
            rf_setting++;
    
            val = simple_strtoul(rf_setting, NULL, 16);

            if(i<MAX_RF_REG_NUM)
            {
                //printk("RF %d %x %x\n", i, reg, val);

                rf_tbl[i].num = reg;
                rf_tbl[i].val = val;
                i++;
            }
        }
    }

    //printk("RFC %s\n", input_str);

    if((strlen(input_str)>2) && (input_str[2]==' '))
    {
        for(i=0;i<strlen(input_str);i+=3)
        {
            if(input_bytes>=max_input_bytes)
                break;

            *rfc_data = (u8) simple_strtoul(&input_str[i], NULL, 16);
            rfc_data++;
            input_bytes++;
        }
    }
    else
    {
        memset(temp_str, 0, sizeof(temp_str));
        for(i=0;i<strlen(input_str);i+=2)
        {
            if(input_bytes>=max_input_bytes)
                break;

            temp_str[0] = input_str[i];
            temp_str[1] = input_str[i + 1];
            *rfc_data = (u8) simple_strtoul(temp_str, NULL, 16);
            rfc_data++;
            input_bytes++;
        }
    }

    printk(KERN_DEBUG "wla: set %s=%s\n", working_cfg->name, (char *) input);    

    return 0;
}

int config_get_rfc(void *cfg, struct seq_file *s, int index)
{
    int i;

    for(i=0;i<MAX_RFC_REG_NUM;i++)
    {
        if(rfc_tbl[i].num==0)
            break;

        seq_printf(s, "%02x%02x", rfc_tbl[i].num, rfc_tbl[i].val);
    }

    seq_printf(s, "\n");

    return 0;
}

#define TS0_WAKE_TIME_L         ( LMAC_REG_OFFSET + 0x200 )
#define TS0_WAKE_TIME_H         ( LMAC_REG_OFFSET + 0x204 )
#define TS0_WAKE_TIME_INFO      ( LMAC_REG_OFFSET + 0x208 )
    #define TS_WAKE_TIMEOUT_COUNT     0x00FF0000UL
    #define TS_WAKE_TIME_DELTA        0x0000FFFFUL
#define TS0_WAKE_DTIM           ( LMAC_REG_OFFSET + 0x20C )

#define TS_WAKE_TIME_L(x)       ( TS0_WAKE_TIME_L + ((x) * 0x10) )
#define TS_WAKE_TIME_H(x)       ( TS0_WAKE_TIME_H + ((x) * 0x10) )
#define TS_WAKE_TIME_INFO(x)    ( TS0_WAKE_TIME_INFO + ((x) * 0x10) )
#define TS_WAKE_DTIM(x)         ( TS0_WAKE_DTIM + ((x) * 0x10) )

#define TS_WAKE_CTRL            ( LMAC_REG_OFFSET + 0x240 )
    #define TS_AID0_MATCH_EN    0x00000002UL
    #define TS_LISTEN_DROP_EN   0x00000001UL
#define TS_WAKE_STATUS          ( LMAC_REG_OFFSET + 0x244 )
    #define BSS_AID0_HIT(x)     (0x01 << ((x) * 8))
    #define BSS_AID_HIT(x)      (0x02 << ((x) * 8))
    #define BSS_BEACON_LOST(x)  (0x10 << ((x) * 8))

#define TS_WAKE_MASK            ( LMAC_REG_OFFSET + 0x248 )

#define TSF0_NEXT_L             ( LMAC_REG_OFFSET + 0x260 )
#define TSF0_NEXT_H             ( LMAC_REG_OFFSET + 0x264 )

#define TSF_NEXT_L(x)           ( TSF0_NEXT_L + ((x) * 0x08) )
#define TSF_NEXT_H(x)           ( TSF0_NEXT_H + ((x) * 0x08) )

#define PMU_CTRL                        (0x04)
    #define PMU_CTRL_SLP_ON             0x0001
    #define PMU_CTRL_WMAC_RESET_DISABLE 0x0010
    #define PMU_CTRL_BB_RESET_DISABLE   0x0020
#define PMU_WIFI_TMR_CTRL               (0x18)
    #define PMU_SLP_WIFI_TMR_EN         0x01000000
#define PMU_SLP_WIFI_TIM_TMR            (0x5C)

#define PMU_WIFI_RXPE_WAIT_TIMER        (0xEC)
    #define RXPE_WAIT_TIM_EN            0x00010000

#define PMU_SOFTWARE_GPREG              (0xFC)

u32 microsecond_to_32k_ticks(u32 us)
{
    // 32Khz ~ 31.25 micro-seconds
    return ((us * 100) / 3125);
}

#define TIME_UNIT                   1024
#define POST_TBTT_INTERVAL          33
#define PMU_TUNRON_US               1703
#define WAKEUP_INTERVAL             20

#define UNION_U64_2U32 union {  \
    long long val_64;           \
    u32 val_32[2];              \
}

#if defined(CONFIG_MIPS_MT_SMP) && defined(CONFIG_HOTPLUG_CPU)
#define SECOND_CPU_ID   1
extern int __ref cpu_down(unsigned int cpu);
//extern int disable_nonboot_cpus(void);
#endif

extern void pmu_reset_devices(unsigned long *device_ids);

void reset_pmu(void)
{
#define DEVICE_ID_GPIO          2431
#define DEVICE_ID_PWM           2430
#define DEVICE_ID_OTP           2429
#define DEVICE_ID_PDMA          2428
#define DEVICE_ID_UART2         2427
#define DEVICE_ID_UART1         2426
#define DEVICE_ID_UART0         2425
#define DEVICE_ID_SMI           2424
#define DEVICE_ID_TIMER         2423
#define DEVICE_ID_DISPLAY       2422
#define DEVICE_ID_GRAPHIC       2421
#define DEVICE_ID_DDR           2420
#define DEVICE_ID_USBOTG        2419
#define DEVICE_ID_USB           2418
#define DEVICE_ID_SDIO          2417
#define DEVICE_ID_AES           2416
#define DEVICE_ID_TSI           2415
#define DEVICE_ID_GSPI          2414
#define DEVICE_ID_HNAT          2413
#define DEVICE_ID_SWP2          2412
#define DEVICE_ID_SWP1          2411
#define DEVICE_ID_SWP0          2410
#define DEVICE_ID_SPDIF         2409
#define DEVICE_ID_PCM           2408
#define DEVICE_ID_RTC           2407
#define DEVICE_ID_BBP           2406
#define DEVICE_ID_GDMA          2405
#define DEVICE_ID_SRAM_CTRL     2404
#define DEVICE_ID_PBUS          2403
#define DEVICE_ID_RBUS          2402
#define DEVICE_ID_DBUS          2401
#define DEVICE_ID_CPU           2400
#define DEVICE_ID_WIFIMAC       2531
#define DEVICE_ID_SWP1_PORT     2530
#define DEVICE_ID_SWP0_PORT     2529
    unsigned long reset_device_ids[] = { DEVICE_ID_USBOTG, 0 };
//  unsigned long reset_device_ids[] = { DEVICE_ID_USBOTG, DEVICE_ID_USB, DEVICE_ID_WIFIMAC, 0 };
    pmu_reset_devices(reset_device_ids);
}

#if 0
#define SDRAM_REG_BASE       0xBF000000
#define DDR3_CAL_BASE_ADDR   0xBF000800
void wd_write(unsigned long addr, unsigned long data)
{
    *(volatile unsigned long *)addr = data;
}

void wd_read(unsigned long addr, unsigned long *data)
{
    *(volatile unsigned long *)data = *(volatile unsigned long *)addr;
}

void srf_enter(void)
{
	unsigned long data;

#if 0
	unsigned long clr_msk, set_msk;

	// Mask off all interrupt except wakeup intr
	data = ~(1<<25); /* data[25] = 0 */
	wd_write(GCI0_MASK ,data);
	wd_write(GCI1_MASK ,data);

	set_msk = (1<<25);
	wd_read(GCI0_STATUS,&data);
	if (data & set_msk) 
		PHY_INFO("!!! ERROR: GCI0_STATUS %0h Wake up status is active",data);
	wd_read(GCI1_STATUS,&data);
	if (data & set_msk) 
		PHY_INFO("!!! ERROR: GCI1_STATUS %0h Wake up status is active",data);

	// Enable BUCK control by PMU register
	wd_read(RF_CTRL_34_REG, &data);
	data |= (1<<7); /* data[7] = 1; */
	wd_write(RF_CTRL_34_REG, data);

	// Set Power down control in stndby mode
	wd_read(PMU_STDBY_PD_REG, &data);
	/*
	data[26:22] = 60x01e; data[18:17] = 20x02;
	data[12] = 1; data[10] = 1;
	data[5:0] = 60x021; */
	// turn on psw_ddr
	clr_msk = ~((0x3f<<22) & (0x3<<17) & (0x3f));
	set_msk =  (0x1e << 22)| (0x2<<17) | (1<<12) | (1<<10) | (0x21);
	data = (data & clr_msk) | set_msk;
	wd_write(PMU_STDBY_PD_REG, data);

	// Set Power down control in SLP mode
	wd_read(PMU_SLP_PD_REG, &data);
	data = (data & (~0x3f)) | 0x20; /* data[5:0] = 60x020; // turn on psw_ddr */
	wd_write(PMU_SLP_PD_REG, data);

	/*  ??? TODO
			// Set SLP timer
			data = 0x01000040;
			wd_write(SLP_TMR_REG, data);
			//$display ("Setting SLP_TMR_REG Enable and 0x40");
	*/
	// ??? TODO
	data = 0x00001e00;
	wd_write(PMU_INT_REG, data);
	//PHY_INFO(PMU_INT_REG, data);
	//PHY_INFO ("read dta: %8h",data) ;
	//PHY_INFO ("Setting PMU_INT_REG - Disabling Non Sleep Timer INT function");
#endif

	// MC enter DRAM Self refresh
	wd_read(SDRAM_REG_BASE + 0x0c, &data);
	data = data | 0x1 | (1<<8); /* data[0] = 1; data [8] = 1; */
	wd_write(SDRAM_REG_BASE + 0x0c, data);
	//~ PHY_INFO ("Set MC to make DRAM into Self refresh mode");

#if 1
	// DDR PHY Power Down
	wd_write(DDR3_CAL_BASE_ADDR + 0x04, 0x00000010);
#else
	// DDR PHY goes to power off
	wd_read(DDR3_CAL_BASE_ADDR + 0x04, &data);
	clr_msk = ~(0x0f & 0x20); /* data[3:0] = 0; data[5] = 0; */
	data = data & clr_msk;
	wd_write(DDR3_CAL_BASE_ADDR + 0x04, data);
#endif

#if 0
	// Enter power saving
	// Set Bit6 DDR PHY REG Reset Disable
	data = 0x00000045;
	wd_write(PMU_CTRL_REG, data);
	//wd_read(PMU_CTRL_REG, &data);
	//$display ("read dta: %8h",data) ;
	//PHY_INFO("Setting PMU_CTRL_REG - Set SLP_ON_REG to enter Sleep Mode");
#endif
}

int suspend(void)
{
//#if defined(ADD_DELAY_BEFORE_RESUME)
//    backup_bb_result();
//#endif
//
//    HAL_DCACHE_WB_INVALIDATE_ALL();
//    HAL_ICACHE_INVALIDATE_ALL();

//#if defined(TEST_MEMORY)
//    *((volatile unsigned long *) 0xA0000000) = 0x12345678;
//    test_mem_dump(0xA0000000, 16);
//    udelay(1000000);
//#endif

//#if defined(CSUM_MEMORY)
//    mem_cksum();
//#endif

    printk("Suspend\n");
#if !defined(CONFIG_FPGA)
//  srf_enter();
#endif

    *((volatile unsigned long *) 0xbf0048fc) = 0x8000002A;//(unsigned long) &resume_asm;

//  REG_WRITE32(0xbf004810, REG_READ32(0xbf004810));
//  if((REG_READ32(0xbf004814) & 0xFFFFFF))
//      REG_UPDATE32(0xbf004814, (REG_READ32(0xbf004814) * 32) | (0x01 << 24), 0x01FFFFFFUL);
//  else
        *((volatile unsigned long *) 0xbf004814) = 0x01017d00;

//  *((volatile unsigned long *) 0xbf004808) = 0x7ff3ebfe;
//  *((volatile unsigned long *) 0xbf004f38) = 0x0053f108;

    printk("Go Sleep\n");
    udelay(100000);
    *((volatile unsigned long *) 0xbf004804) = 0xF5;
//  REG_WRITE32(0xbf004808, 0x7ff3ebfe);    // Wi-Fi listen mode
//  REG_WRITE32(0xbf004f38, 0x0053f108);
//
//  REG_WRITE32(0xbf004804,0x00F5);

    while (1)
        ;

    return 0;
}

#include <linux/suspend.h>
#include <linux/export.h>
#include <linux/kobject.h>
#include <linux/string.h>
#include <linux/pm-trace.h>
#include <linux/workqueue.h>
#include <linux/debugfs.h>
#include <linux/seq_file.h>
#include <trace/events/power.h>
#include <linux/syscalls.h>
#include <linux/reboot.h>
#include <linux/device.h>
#include <linux/async.h>
#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/mount.h>
#include <linux/pm.h>
#include <linux/console.h>
#include <linux/cpu.h>
#include <linux/freezer.h>
#include <linux/gfp.h>
#include <linux/syscore_ops.h>
#include <linux/ctype.h>
#include <linux/genhd.h>
#include <linux/ktime.h>

#include <linux/init.h>
#include <linux/kobject.h>
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
#if defined(CONFIG_MIPS_MT_SMP)
void vsmp_resume(void);
void vsmp_suspend(void);
#endif

#include "../../../../kernel/power/power.h"
//#include "../base/fastboot.h"

//void resume_back(void)
//{
//    printk("resume_back\n");
//    udelay(100000);
//}

#define SUSP_TIME_IN_MS 3000
void start_sleep(void)
{
    u32 status;

    printk("start_sleep\n");

    *((volatile unsigned long *) 0xbf004814) = SUSP_TIME_IN_MS;

    *((volatile unsigned long *) 0xbf004808) = 0x7ff3ebfe;  // BUCK power down [17]
    *((volatile unsigned long *) 0xbf004f38) = 0x0053f108;  // avoid POR [21]

    local_flush_tlb_all();

//    memcpy((void *)0x90000000, fastboot, sizeof(fastboot));
#if 1
    r4k_blast_dcache();
    r4k_blast_icache();
#else
    __flush_cache_all();
#endif

    flush_icache_all();

//  REG_WRITE32(PMU_WATCHDOG, 0x0);
    *((volatile unsigned long *) 0xbf004820) = 0x0;

#if defined(CONFIG_MIPS_MT_SMP)
    vsmp_suspend();
#endif

    //#if defined(ADD_DELAY_BEFORE_RESUME)
//    backup_bb_result();
//#endif
//
//    HAL_DCACHE_WB_INVALIDATE_ALL();
//    HAL_ICACHE_INVALIDATE_ALL();

//#if defined(TEST_MEMORY)
//    *((volatile unsigned long *) 0xA0000000) = 0x12345678;
//    test_mem_dump(0xA0000000, 16);
//    udelay(1000000);
//#endif

//#if defined(CSUM_MEMORY)
//    mem_cksum();
//#endif

#if !defined(CONFIG_FPGA)
//  srf_enter();
#endif

//  *((volatile unsigned long *) 0xbf0048fc) = 0x0000002A;
    *((volatile unsigned long *) 0xbf0048fc) = 0x8003d750;

    status = *((volatile unsigned long *) 0xbf004810);
    *((volatile unsigned long *) 0xbf004810) = status;
    *((volatile unsigned long *) 0xbf004814) = 0x01017d00;

    printk("Go Sleep\n");
    udelay(100000);
    *((volatile unsigned long *) 0xbf004804) = 0xF5;

    while(1)
    {
    }
}

extern int _suspend_devices_and_enter(suspend_state_t state);
extern int _enter_state(suspend_state_t state);
extern int pm_suspend(suspend_state_t state);
extern void panther_machine_suspend(void);

static DEFINE_SPINLOCK(pmu_sleep_test_lock);
void pmu_sleep_test(void)
{
//#define PM_SUSPEND_ON       ((__force suspend_state_t) 0)
//#define PM_SUSPEND_FREEZE	((__force suspend_state_t) 1)
//#define PM_SUSPEND_STANDBY	((__force suspend_state_t) 2)
//#define PM_SUSPEND_MEM		((__force suspend_state_t) 3)
//#define PM_SUSPEND_MIN		PM_SUSPEND_FREEZE
//#define PM_SUSPEND_MAX		((__force suspend_state_t) 4)
    suspend_state_t state = PM_SUSPEND_MEM;
    unsigned long irqflags;
	int error;

	error = pm_autosleep_lock();
	if (error)
    {
        printk("error 1\n");
        return;
    }

	if (pm_autosleep_state() > PM_SUSPEND_ON) {
		error = -EBUSY;
        printk("pm_autosleep_state() > PM_SUSPEND_ON\n");
		goto out;
	}

    printk("Before Before Before Before Before\n");

    _enter_state(state);
    reset_pmu();
//  pm_suspend(state);

out:
    pm_autosleep_unlock();
    spin_lock_irqsave(&pmu_sleep_test_lock, irqflags);

    local_flush_tlb_all();

#if 1
    r4k_blast_dcache();
    r4k_blast_icache();
#else
    __flush_cache_all();
#endif

    flush_icache_all();

//  REG_WRITE32(PMU_WATCHDOG, 0x0);
    *((volatile unsigned long *) 0xbf004820) = 0x0;

#if defined(CONFIG_MIPS_MT_SMP)
    vsmp_suspend();
#endif

//  printk("QQQQQ\n");
//  panther_machine_suspend();
//  printk("RRRRR\n");

    start_sleep();
    printk("After After After After After After \n");
//  resume_uart();
//  resume_panther_irq();

#if defined(CONFIG_MIPS_MT_SMP)
    vsmp_resume();
#endif

    spin_unlock_irqrestore(&pmu_sleep_test_lock, irqflags);
    pm_autosleep_unlock();
}
#else

static DEFINE_SPINLOCK(pmu_sleep_test_lock);
void pmu_sleep_test(void)
{
    /* test parameters */
    int tsf_idx = 3;
    int bss_idx = 1;
    u32 aid = 1999;
    u32 consecutive_beacon_lost_wakeup_threshold = 10;
    /* end of test parameters */

    u64 ts_now, ts_sleep, ts_wakeup;

    u32 first_timer_tick;
    u32 pmu_timer_tick, wakeup_delta_reload_wifi, wakeup_delta_reload_pmu;
    u32 wakeup_delta_next_reload_wifi;
    u32 bcn_interval_32;
    UNION_U64_2U32 next_tbtt, next_tbtt_tmp, next_tbtt_final;
    unsigned long irqflags;

#if defined(CONFIG_MIPS_MT_SMP) && defined(CONFIG_HOTPLUG_CPU)
    cpu_down(SECOND_CPU_ID);
    //disable_nonboot_cpus();
#endif

    spin_lock_irqsave(&pmu_sleep_test_lock, irqflags);

    //printk("0__pmu_sleep_test\n");

#if 0
    /* clear tsf_tout_cnt hardware counter */
    MACREG_WRITE32(TS_WAKE_TIME_INFO(tsf_idx), 0);
    msleep(120);
#endif

#if 1 // calculate ts_wakeup
    ts_now = MACREG_READ64(TS_O(tsf_idx));
    bcn_interval_32 = (global_data->dragonite_bss_info[bss_idx].beacon_interval * TIME_UNIT);
    next_tbtt.val_64 = (ts_now + (bcn_interval_32 - 1));
    //printk("1_%016llx,%08x,%016llx\n",ts_now, bcn_interval_32, next_tbtt.val_64);

#ifdef __BIG_ENDIAN
    next_tbtt_tmp.val_64 = ((((next_tbtt.val_32[0] % bcn_interval_32) * ((UINT_MAX % bcn_interval_32) + 1))
                             % bcn_interval_32) + (next_tbtt.val_32[1] % bcn_interval_32)) % bcn_interval_32;
#else
    next_tbtt_tmp.val_64 = ((((next_tbtt.val_32[1] % bcn_interval_32) * ((UINT_MAX % bcn_interval_32) + 1))
                             % bcn_interval_32) + (next_tbtt.val_32[0] % bcn_interval_32)) % bcn_interval_32;
#endif
    next_tbtt_final.val_64 = next_tbtt.val_64 - next_tbtt_tmp.val_64;
    ts_sleep = next_tbtt_final.val_64 + (POST_TBTT_INTERVAL * TIME_UNIT);
    next_tbtt_final.val_64 = next_tbtt_final.val_64 + bcn_interval_32;
    ts_wakeup = next_tbtt_final.val_64 - (WAKEUP_INTERVAL * TIME_UNIT);
    //printk("2_%016llx,%016llx,%016llx\n", ts_sleep, ts_wakeup, next_tbtt_final.val_64);
#endif

#if 1 // calculate wakeup_delta_reload timestamp
    wakeup_delta_reload_wifi = (global_data->dragonite_bss_info[bss_idx].beacon_interval - POST_TBTT_INTERVAL - WAKEUP_INTERVAL) * TIME_UNIT;
    if (USHRT_MAX <= wakeup_delta_reload_wifi)
    {
        wakeup_delta_reload_wifi = USHRT_MAX;
    }
    wakeup_delta_reload_pmu = wakeup_delta_reload_wifi - PMU_TUNRON_US;
    if (wakeup_delta_reload_pmu > wakeup_delta_reload_wifi)
    {
        wakeup_delta_reload_pmu = 0;
    }
    wakeup_delta_next_reload_wifi = global_data->dragonite_bss_info[bss_idx].beacon_interval * TIME_UNIT;
    //printk("3_%08x,%08x\n", wakeup_delta_reload_wifi, wakeup_delta_reload_pmu);
#endif

    pmu_timer_tick = microsecond_to_32k_ticks(wakeup_delta_reload_pmu);
    first_timer_tick = pmu_timer_tick;

    do
    {
        ts_now = MACREG_READ64(TS_O(tsf_idx));
    } while (ts_now < ts_sleep);
    //printk("4_%016llx_set\n", ts_now);

    /* set AID for detect associated ID in TIM */
    MACREG_WRITE32(AID_BSSID_0_1, ((aid << 16) | aid));
    MACREG_WRITE32(AID_BSSID_2_3, ((aid << 16) | aid));

#if 1
    /* first timer */
    //PMUREG_WRITE32(PMU_WIFI_TMR_CTRL, PMU_SLP_WIFI_TMR_EN | 0x00FFFFFF);
    PMUREG_WRITE32(PMU_WIFI_TMR_CTRL, PMU_SLP_WIFI_TMR_EN | first_timer_tick);

    /* reload timer */
    PMUREG_WRITE32(PMU_SLP_WIFI_TIM_TMR, pmu_timer_tick);

    MACREG_WRITE32(TS_WAKE_CTRL, (TS_AID0_MATCH_EN|TS_LISTEN_DROP_EN));
    MACREG_WRITE32(TS_WAKE_STATUS, 0xFFFFFFFF);

    MACREG_WRITE32(TSF_NEXT_L(tsf_idx), next_tbtt_final.val_64);
    MACREG_WRITE32(TSF_NEXT_H(tsf_idx), (next_tbtt_final.val_64 >> 32));

    //MACREG_WRITE32(TS_WAKE_TIME_L(tsf_idx), 0);
    //MACREG_WRITE32(TS_WAKE_TIME_H(tsf_idx), 0);
    MACREG_WRITE32(TS_WAKE_TIME_L(tsf_idx), ts_wakeup);
    MACREG_WRITE32(TS_WAKE_TIME_H(tsf_idx), (ts_wakeup>>32));

    consecutive_beacon_lost_wakeup_threshold |= 0x01;  /* hardware supports odd number only */
    MACREG_WRITE32(TS_WAKE_TIME_INFO(tsf_idx), (consecutive_beacon_lost_wakeup_threshold << 16) | ((wakeup_delta_reload_wifi/2) & 0x0ffffUL));
    MACREG_WRITE32(TS_WAKE_DTIM(tsf_idx), ((wakeup_delta_next_reload_wifi/2)<<16));

    MACREG_UPDATE32(TS_WAKE_MASK, 0, (BSS_AID0_HIT(bss_idx) | BSS_BEACON_LOST(bss_idx) | BSS_AID_HIT(bss_idx)));
    //MACREG_UPDATE32(TS_WAKE_MASK, 0, (BSS_AID0_HIT(bss_idx) | BSS_AID_HIT(bss_idx)));
    //MACREG_UPDATE32(TS_WAKE_MASK, 0, BSS_BEACON_LOST(bss_idx));

    PMUREG_UPDATE32(PMU_WIFI_RXPE_WAIT_TIMER, RXPE_WAIT_TIM_EN, RXPE_WAIT_TIM_EN);

    //PMUREG_WRITE32(PMU_SOFTWARE_GPREG, MAGIC_NUM_FOR_REBOOT_TEST);
#endif

    //printk("%016llx__pmu_sleep_test__2\n", MACREG_READ64(TS_O(tsf_idx)));

    #if 1
    //*((volatile unsigned int *) 0xbf004808) = 0xdf1d8afe;
    //*((volatile unsigned int *) 0xbf004860) = 0x800000fe;
    *((volatile unsigned int *) 0xbf004810) = 0x0f00;
    *((volatile unsigned int *) 0xbf004808) = 0xfffdebfe;
    *((volatile unsigned int *) 0xbf004860) = 0xfc41e8fe;
    #else
    *((volatile unsigned int *) 0xbf004810) = 0x0f00;
    *((volatile unsigned int *) 0xbf004808) = 0x7ff3ebfe;
    *((volatile unsigned int *) 0xbf004f38) = 0x0053f108;
    *((volatile unsigned int *) 0xbf004860) = 0xfc43e9fe;
    #endif

    reset_pmu();

    //*((volatile unsigned int *) 0xbf0048fc) = 0x8003d750;//0x8000002A;

    PMUREG_WRITE32(PMU_CTRL, 0xF5);
    //PMUREG_UPDATE32(0x10, 0x00000C00, 0x00000C00);
    //PMUREG_UPDATE32(PMU_CTRL, PMU_CTRL_SLP_ON|PMU_CTRL_WMAC_RESET_DISABLE|PMU_CTRL_BB_RESET_DISABLE,
    //                            PMU_CTRL_SLP_ON|PMU_CTRL_WMAC_RESET_DISABLE|PMU_CTRL_BB_RESET_DISABLE);
    //  printk("%016llx %016llx %016llx, %08x %08x\n", ts_sleep, ts_wakeup, next_tbtt_final.val_64,
    //                                                   wakeup_delta_reload_pmu, wakeup_delta_reload_wifi);

    while(1)
    {
        //printk("Sleep Failed !\n");
        //printk(".\n");
        //printk("[%016llx]\n", MACREG_READ64(TS_O(tsf_idx)));
        printk("(%08x) %08x %08x\n",  MACREG_READ32(TS_BEACON_COUNT(tsf_idx)),
                MACREG_READ32(TS_WAKE_STATUS), *((volatile unsigned int *) 0xbf004810));
    }

    spin_unlock_irqrestore(&pmu_sleep_test_lock, irqflags);
    return;
}
#endif

static struct wla_config config[] =
{
    { "wifi_recover", (u32 *)&drg_wifi_recover, 0, config_set_u32, config_get_u32, NULL ,"                Recover wifi", },
    { "wifi_recover_times", &drg_wifi_recover_cnt, 0, NULL, config_get_int, NULL ,"        Wifi recover times", },
    { "wifi_poweroff_enable", &drg_wifi_poweroff_enable, 0, config_set_int, config_get_int, NULL ,"        Wifi power-off enable", },
    { "rf_bb_init_once_enable", &drg_rf_bb_init_once, 0, config_set_u32, config_get_u32, NULL ,"RF BB init once", },
    { "tx_delay_kick_count", &drg_tx_acq_delay_max_cnt, 0, config_set_int, config_get_int, NULL ,"        Delay Tx kick max count", },
    { "tx_delay_kick_interval", &drg_tx_acq_delay_interval, 0, config_set_int, config_get_int, NULL ,"Delay Tx kick interval (us)", },
    { "tx_stats_enable", &drg_tx_stats.stats_enable, 0, config_set_int, config_get_int, NULL ,"        Enable Tx statistic", },
    { "tx_time_space", &drg_tx_stats.time_space, 0, config_set_u32, config_get_u32, NULL ,"        Tx time space", },
    { "tx_per_ac_max_buf", &dragonite_tx_per_ac_max_buf, 0, config_set_int, config_get_int, NULL ,"        Tx per ac max buffer size", },
    { "tx_ampdu_enable", &dragonite_tx_ampdu_enable, 0, config_set_int, config_get_int, NULL ,"        Enable Tx ampdu", },
    { "rx_ampdu_enable", &dragonite_rx_ampdu_enable, 0, config_set_int, config_get_int, NULL ,"        Enable Rx ampdu", },
    { "tx_ampdu_vo_enable", &dragonite_tx_ampdu_vo_enable, 0, config_set_int, config_get_int, NULL ,"        Enable Tx ACQ-VO setup ampdu", },
    { "tx_debug_dump", &dragonite_tx_debug_dump, 0, config_set_int, config_get_int, NULL ,"        Enable Tx debug dump", },
    { "rx_debug_dump", &dragonite_rx_debug_dump, 0, config_set_int, config_get_int, NULL ,"        Enable Rx debug dump", },
    { "debug_flag", &dragonite_debug_flag, 0, config_set_u32, config_get_u32, NULL ,"                Run time debug flag", },
    { "tx_winsz", &drg_tx_winsz, 0, config_set_int, config_get_int, NULL ,"                Tx window size", },
    { "rx_winsz", &drg_rx_winsz, 0, config_set_int, config_get_int, NULL ,"                Rx window size", },
    { "tx_protect", &drg_tx_protect, 0, config_set_int, config_get_int, NULL ,"                Tx protection enable", },
    { "dynamic_ps_timeout", &dynamic_ps_timeout_override, 0, config_set_int, config_get_int, NULL ,"        Dynamic ps timeout", },
    { "dynamic_ps_postpone_time", &dynamic_ps_postpone_time, 0, config_set_int, config_get_int, NULL ,"Dynamic ps postpone time", },
    { "tcp_tracking_debug", &tcp_active_session_tracking_debug, 0, config_set_int, config_get_int, NULL ,"        Enable Tcp active session tracking debug", },
#if defined(CONFIG_BRIDGE)
    { "swbr_shortcut_enable", &enable_swbr_shortcut, 0, config_set_int, config_get_int, NULL ,"        Enable shortcut for software bridge", },
    { "swbr_debug_enable", &enable_swbr_debug, 0, config_set_int, config_get_int, NULL ,"        Enable shortcut debug message", },
#endif
    { "pmu_sleep_test", NULL, 0, (int (*)(void *, void *, int)) pmu_sleep_test, NULL, NULL ,"        Test PMU sleep mode", },
};

#define NUM_OF_CONFIGS  (sizeof(config)/sizeof(struct wla_config))
#undef isdigit
#define isdigit(c)	('0' <= (c) && (c) <= '9')

static int wla_procfs_exec_set_command(char* cmd)
{
    char *p;
    int i;
    int k;
    int index = 0;

    if(0==strlen(cmd))
        return 0;

    p = strchr(cmd, '=');
    if(p==NULL)
        return 0;

    *p = '\0';
    p++;

    k = strlen(cmd) - 1;
    if(k>=0 && isdigit(cmd[k]))
    {
        index = cmd[k] - '0';
        k--;

        if(k>=0 && isdigit(cmd[k]))
        {
            index += (cmd[k] - '0') * 10;
            k--;
        }

        if((k>=0)&&(cmd[k]=='.'))
            cmd[k] = '\0';
    }

    for(i=0;i<NUM_OF_CONFIGS;i++)
    {
        if(!strcmp(config[i].name, cmd))
        {
            if(config[i].config_set)
                config[i].config_set(&config[i], p, index);

            break;
        }
    }

    return 1;
}

void wla_procfs_show_usage(struct seq_file *s)
{
    int i;

    seq_printf(s, "Configurable attributes list\n");

    for(i=0;i<NUM_OF_CONFIGS;i++)
    {
        if(config[i].array_size)
        {
            seq_printf(s, "   %s.[0~%d]", config[i].name, config[i].array_size - 1);
        }
        else
        {
            seq_printf(s, "   %s", config[i].name);
        }

        if(config[i].hint)
            seq_printf(s, "\t\t%s", config[i].hint);

        seq_printf(s, "\n");
    }

    seq_printf(s, "\nExample:\n");
    seq_printf(s, "   echo wifi_recover=1 > /proc/wla\n");
}

static int wla_procfs_exec_get_command(char* cmd, struct seq_file *s)
{
    int i;
    int k;
    int index = 0;

    //printk(KERN_DEBUG "wla: get %s\n", cmd);

    if(0==strlen(cmd))
    {   
        wla_procfs_show_usage(s);
        return 0;
    }

    k = strlen(cmd) - 1;
    if(k>=0 && isdigit(cmd[k]))
    {
        index = cmd[k] - '0';
        k--;

        if(k>=0 && isdigit(cmd[k]))
        {
            index += (cmd[k] - '0') * 10;
            k--;
        }

        if((k>=0)&&(cmd[k]=='.'))
            cmd[k] = '\0';
    }

    for(i=0;i<NUM_OF_CONFIGS;i++)
    {
        if(!strcmp(config[i].name, cmd))
        {
            if(config[i].config_get)
                config[i].config_get(&config[i], s, 0);

            break;
        }
    }

    return 0;
}

static int wla_procfs_show(struct seq_file *s, void *priv)
{
    char user_cmd[MAX_CMD_STRING_LENGTH];

    mutex_lock(&global_data->mutex);
    strcpy(user_cmd, ___procfs_cmd);
    ___procfs_cmd[0] = '\0';
    mutex_unlock(&global_data->mutex);

    wla_procfs_exec_get_command(user_cmd, s);
    return 0;
}

static int wla_procfs_open(struct inode *inode, struct file *file)
{
    int ret;
    struct seq_file *p;

    ret = single_open(file, wla_procfs_show, NULL);
    if(ret==0)
    {
        p = (struct seq_file *) file->private_data;
    
        if(p->buf==NULL)
        {
            p->buf = kmalloc(SEQ_FILE_BUFSIZE, GFP_KERNEL);

            if(p->buf)
                p->size = SEQ_FILE_BUFSIZE;
        }
    }

    return ret;
}

static ssize_t wla_procfs_write(struct file *file, const char __user *buffer, size_t count, loff_t *data)
{
    int copy_count;
    char user_cmd[MAX_CMD_STRING_LENGTH];

    copy_count = (count > (MAX_CMD_STRING_LENGTH - 1)) ? (MAX_CMD_STRING_LENGTH - 1):count;

    if(0==copy_from_user(user_cmd, buffer, copy_count))
    {
        if(copy_count>0)
        {
            user_cmd[copy_count] = '\0';
            if(user_cmd[copy_count-1]=='\n')
                user_cmd[copy_count-1] = '\0';
        }
        else
        {
            user_cmd[0] = '\0';
        }

        if(!wla_procfs_exec_set_command(user_cmd))
        {
            mutex_lock(&global_data->mutex);
            strcpy(___procfs_cmd, user_cmd);
            mutex_unlock(&global_data->mutex);
        }

        return copy_count;
    }
    else
    {
        mutex_lock(&global_data->mutex); 
        ___procfs_cmd[0] = '\0';
        mutex_unlock(&global_data->mutex);

        return -EIO;
    }
}

/* keep track of how many times it is mmapped */
void mmap_open(struct vm_area_struct *vma)
{
		;
}

void mmap_close(struct vm_area_struct *vma)
{
		;
}

/* 
	nopage is called the first time a memory area is accessed which is not in memory,
	it does the actual mapping between kernel and user space memory
*/
static int mmap_fault(struct vm_area_struct *vma, struct vm_fault *vmf)
{
	struct page *page;
	MAC_INFO *info;
	int ret = 0;

	/* is the address valid? */
	if ((unsigned long)vmf->virtual_address > vma->vm_end) {
		printk("invalid address\n");
		return VM_FAULT_SIGBUS;
	}
	/* the data is in vma->vm_private_data */
	info = (MAC_INFO *)vma->vm_private_data;

	if (!info) {
		printk("no data\n");
		return VM_FAULT_SIGBUS;
	}

	/* get the page */
	page = virt_to_page(info);

	/* increment the reference count of this page */
	get_page(page);

	vmf->page = page;

	return ret | VM_FAULT_LOCKED;;
}

struct vm_operations_struct mmap_vm_ops = {
	.open	= mmap_open,
	.close	= mmap_close,
	.fault	= mmap_fault,
};

static const struct file_operations wla_procfs_fops = {
    .open       = wla_procfs_open,
    .read       = seq_read,
    .llseek     = seq_lseek,
    .release    = single_release,
    .write      = wla_procfs_write,
};

int debug_mmap(struct file *filp, struct vm_area_struct *vma)
{
	unsigned long offset = vma->vm_pgoff << PAGE_SHIFT;
	unsigned long size = vma->vm_end - vma->vm_start;
	
	(void)size;
	if (offset & ~PAGE_MASK)
	{
		printk("offset not aligned: %ld\n", offset);
		return -ENXIO;
	}
		
	vma->vm_ops = &mmap_vm_ops;
	vma->vm_flags |= VM_LOCKED;
	/* assign the file private data to the vm private data */
	vma->vm_private_data = filp->private_data;
	mmap_open(vma);
	return 0;
}

int debug_close(struct inode *inode, struct file *filp)
{
	filp->private_data = NULL;

	return 0;
}

int debug_open(struct inode *inode, struct file *filp)
{
	MAC_INFO *info = global_data->mac_info;

	filp->private_data = (char *)info;

	return 0;
}

static const struct file_operations wla_debug_fops = {
	.open		= debug_open,
	.release	= debug_close,
	.mmap		= debug_mmap,
};


int wla_procfs_init(struct mac80211_dragonite_data *wlan_data)
{
    if(NULL==proc_create(WLA_PROCFS_NAME, S_IWUSR, NULL, &wla_procfs_fops))
    {
        return -EIO;
    }
    
    if(NULL==proc_create("wla_debug", S_IWUSR, NULL, &wla_debug_fops))
    {
        return -EIO;
    }

    global_data = wlan_data;

    return 0;
}

void wla_procfs_exit(void)
{
    remove_proc_entry(WLA_PROCFS_NAME, NULL);

    global_data = NULL;
}
