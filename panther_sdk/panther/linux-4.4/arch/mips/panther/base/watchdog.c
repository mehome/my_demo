/* 
 *  Copyright (c) 2008, 2009, 2016, 2017    Montage Inc.    All rights reserved.
 *
 *  Watchdog driver for Montage SoC
 *
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/watchdog.h>
#include <linux/init.h>
#include <linux/bitops.h>
#include <linux/uaccess.h>

#include <asm/mach-panther/panther.h>
#include <asm/mach-panther/reg.h>
#include <asm/mach-panther/pmu.h>

static DEFINE_SPINLOCK(watchdog_lock);

static int nowayout = WATCHDOG_NOWAYOUT;
static int heartbeat = 60;  /* (secs) Default is 1 minute */
static unsigned long wdt_status;
static unsigned long boot_status;

#define	WDT_IN_USE          0
#define	WDT_OK_TO_CLOSE     1

static void clear_boot_failure_indication(void)
{
#define IPL_BOOT_FAILURE_CHECK_MAGIC   0x45A9C637UL
#define IPL_BOOT_FAILURE_CHECK_MAGIC2  0xFF0000FFUL
#define IPL_BOOT_FAILURE_CHECK_ADDR    0xB0007FF8UL

#define BOOT2_BOOT_FAILURE_CHECK_MAGIC   0x45A84637UL
#define BOOT2_BOOT_FAILURE_CHECK_MAGIC2  0xFF8001FFUL
#define BOOT2_BOOT_FAILURE_CHECK_MAGIC2_STOP_IN_BOOT2 0xFF4002FFUL
#define BOOT2_BOOT_FAILURE_CHECK_MAGIC2_BOOT_RECOVERY 0xFF2004FFUL
#define BOOT2_BOOT_FAILURE_CHECK_ADDR    0xB0007FF0UL
    unsigned long *ipl_checkval = (unsigned long *) IPL_BOOT_FAILURE_CHECK_ADDR;
    unsigned long *boot2_checkval = (unsigned long *) BOOT2_BOOT_FAILURE_CHECK_ADDR;

    /* clear IPL boot failure check value, IPL->BOOT2 considered to be success */
    if(ipl_checkval[0] == IPL_BOOT_FAILURE_CHECK_MAGIC)
        ipl_checkval[1] = 0;

    /* BOOT2->OpenWRT considered to be success */
    if((boot2_checkval[0] == BOOT2_BOOT_FAILURE_CHECK_MAGIC)
        && (boot2_checkval[1] == BOOT2_BOOT_FAILURE_CHECK_MAGIC2))
        boot2_checkval[1] = 0;
}

void panther_watchdog_enable(void)
{
    unsigned long flags;
    u32 period;

    spin_lock_irqsave(&watchdog_lock, flags);

    // 32KHz watchdog counter
    period = (0x0ffffff & (heartbeat * 32000));
    REG_WRITE32(PMU_WATCHDOG, (0x01000000 | period) );

    clear_boot_failure_indication();

    spin_unlock_irqrestore(&watchdog_lock, flags);
}

void panther_watchdog_disable(void)
{
    unsigned long flags;

    spin_lock_irqsave(&watchdog_lock, flags);

    REG_WRITE32(PMU_WATCHDOG, 0x0);

    spin_unlock_irqrestore(&watchdog_lock, flags);
}

static int panther_watchdog_open(struct inode *inode, struct file *file)
{
    if (test_and_set_bit(WDT_IN_USE, &wdt_status))
        return -EBUSY;

    clear_bit(WDT_OK_TO_CLOSE, &wdt_status);
    panther_watchdog_enable();
    return nonseekable_open(inode, file);
}

static ssize_t
panther_watchdog_write(struct file *file, const char *data, size_t len, loff_t *ppos)
{
    if (len)
    {
        if (!nowayout)
        {
            size_t i;

            clear_bit(WDT_OK_TO_CLOSE, &wdt_status);

            for (i = 0; i != len; i++)
            {
                char c;

                if (get_user(c, data + i))
                    return -EFAULT;
                if (c == 'V')
                    set_bit(WDT_OK_TO_CLOSE, &wdt_status);
            }
        }
        panther_watchdog_enable();
    }
    return len;
}

static struct watchdog_info ident = {
    .options    = WDIOF_CARDRESET | WDIOF_MAGICCLOSE |
                  WDIOF_SETTIMEOUT | WDIOF_KEEPALIVEPING,
    .identity   = "Panther Watchdog",
};


static long panther_watchdog_ioctl(struct file *file, unsigned int cmd,
                              unsigned long arg)
{
    int ret = -ENOTTY;
    int time;

    switch (cmd)
    {
    case WDIOC_GETSUPPORT:
        ret = copy_to_user((struct watchdog_info *)arg, &ident,
                           sizeof(ident)) ? -EFAULT : 0;
        break;

    case WDIOC_GETSTATUS:
        ret = put_user(0, (int *)arg);
        break;

    case WDIOC_GETBOOTSTATUS:
        ret = put_user(boot_status, (int *)arg);
        break;

    case WDIOC_KEEPALIVE:
        panther_watchdog_enable();
        ret = 0;
        break;

    case WDIOC_SETTIMEOUT:
        ret = get_user(time, (int *)arg);
        if (ret)
            break;

        if (time <= 0 || time > 60)
        {
            ret = -EINVAL;
            break;
        }

        heartbeat = time;
        panther_watchdog_enable();
        /* Fall through */

    case WDIOC_GETTIMEOUT:
        ret = put_user(heartbeat, (int *)arg);
        break;
    }
    return ret;
}

static int panther_watchdog_release(struct inode *inode, struct file *file)
{
    if (test_bit(WDT_OK_TO_CLOSE, &wdt_status))
        panther_watchdog_disable();
    else
        printk(KERN_CRIT "WATCHDOG: Device closed unexpectedly - "
               "timer will not stop\n");
    clear_bit(WDT_IN_USE, &wdt_status);
    clear_bit(WDT_OK_TO_CLOSE, &wdt_status);

    return 0;
}


static const struct file_operations panther_watchdog_fops = {
    .owner          = THIS_MODULE,
    .llseek         = no_llseek,
    .write          = panther_watchdog_write,
    .unlocked_ioctl = panther_watchdog_ioctl,
    .open           = panther_watchdog_open,
    .release        = panther_watchdog_release,
};

static struct miscdevice panther_watchdog_miscdev = {
    .minor      = WATCHDOG_MINOR,
    .name       = "watchdog",
    .fops       = &panther_watchdog_fops,
};

static int __init panther_watchdog_init(void)
{
    int ret;

    spin_lock_init(&watchdog_lock);

    /* boot status support in HW? */
    boot_status = 0;

    ret = misc_register(&panther_watchdog_miscdev);
    if (ret == 0)
        printk(KERN_INFO "Panther Watchdog Timer: heartbeat %d sec\n",
               heartbeat);

    return ret;
}

static void __exit panther_watchdog_exit(void)
{
    misc_deregister(&panther_watchdog_miscdev);
}


module_init(panther_watchdog_init);
module_exit(panther_watchdog_exit);

module_param(heartbeat, int, 0);
MODULE_PARM_DESC(heartbeat, "Watchdog heartbeat in seconds (default 60s)");

module_param(nowayout, int, 0);
MODULE_PARM_DESC(nowayout, "Watchdog cannot be stopped once started");

MODULE_ALIAS_MISCDEV(WATCHDOG_MINOR);

