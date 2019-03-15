#include <linux/io.h>
#include <linux/module.h>
#include <linux/uio_driver.h>
#include <linux/platform_device.h>
#include <linux/slab.h>

#include <asm/mach-panther/panther.h>
#include <asm/mach-panther/pmu.h>
#include <asm/mach-panther/pinmux.h>

#define TSI_REGBANK_BASE        (PANTHER_REG_BASE + 0x9000)
#define TSI_REGBANK_SIZE        0x1000
#define TSI_MAX_BUFFER_SIZE     0x60000

#define PHYSICAL_ADDR(x)        ((u32) (x) & 0x1fffffffUL)

/* TSI register */
#define TSI_CTRL       0x00
#define TSI_INTR_EN    0x40

#define TSIREG(offset)   (*(volatile unsigned int*)(((unsigned long)TSI_REGBANK_BASE)+offset))

#define DRV_NAME "uio-tsi"
#define DRV_VERSION "1.0.0"

void *tsi_buf0;
void *tsi_buf1;

static irqreturn_t uio_tsi_irq(int irq, struct uio_info *dev_info)
{
    TSIREG(TSI_INTR_EN) = 0;

    return IRQ_HANDLED;
}

static int __init tsi_probe(struct platform_device *dev)
{
    struct uio_info *info;
    int ret;

    info = kzalloc(sizeof(struct uio_info), GFP_KERNEL);
    if (unlikely(!info)) {
        dev_err(&dev->dev, "Could not allocate info\n");
        return -ENOMEM;
    }

    tsi_buf0 = kmalloc(TSI_MAX_BUFFER_SIZE, GFP_KERNEL);
    tsi_buf1 = kmalloc(TSI_MAX_BUFFER_SIZE, GFP_KERNEL);
    if((tsi_buf0==NULL)||(tsi_buf1==NULL))
        goto Error;

    dma_cache_wback_inv((unsigned long) tsi_buf0, TSI_MAX_BUFFER_SIZE);
    dma_cache_wback_inv((unsigned long) tsi_buf1, TSI_MAX_BUFFER_SIZE);

    info->mem[0].addr = PHYSICAL_ADDR(TSI_REGBANK_BASE);
    info->mem[0].size = TSI_REGBANK_SIZE;
    info->mem[0].internal_addr = (void *) (TSI_REGBANK_BASE);
    info->mem[0].memtype = UIO_MEM_PHYS;

    info->mem[1].addr = PHYSICAL_ADDR(tsi_buf0);
    info->mem[1].size = TSI_MAX_BUFFER_SIZE;
    info->mem[1].memtype = UIO_MEM_PHYS;

    info->mem[2].addr = PHYSICAL_ADDR(tsi_buf1);
    info->mem[2].size = TSI_MAX_BUFFER_SIZE;
    info->mem[2].memtype = UIO_MEM_PHYS;

    info->version = DRV_VERSION;
    info->name = DRV_NAME;
    info->irq = IRQ_TSI;
    info->handler = uio_tsi_irq;

    ret = uio_register_device(&dev->dev, info);
    if (ret)
    {
        kfree(tsi_buf0);
        tsi_buf0 = NULL;
        kfree(tsi_buf1);
        tsi_buf1 = NULL;
        kfree(info);
        return -ENODEV;
    }

    platform_set_drvdata(dev, info);
    return 0;

Error:
    if(tsi_buf0)
    {
        kfree(tsi_buf0);
        tsi_buf0 = NULL;
    }
    if(tsi_buf1)
    {
        kfree(tsi_buf1);
        tsi_buf1 = NULL;
    }
    return -ENOMEM;
}

static int __exit tsi_remove(struct platform_device *dev)
{
    struct uio_info *info = platform_get_drvdata(dev);

    uio_unregister_device(info);
    platform_set_drvdata(dev, NULL);
    kfree(info);

    if(tsi_buf0)
    {
        kfree(tsi_buf0);
        tsi_buf0 = NULL;
    }
    if(tsi_buf1)
    {
        kfree(tsi_buf1);
        tsi_buf1 = NULL;
    }

    return 0;
}

static struct platform_driver tsi_driver = {
    .probe = tsi_probe,
    .remove = tsi_remove,
    .driver = {
        .name = DRV_NAME,
        .owner = THIS_MODULE,
    },
};

static int __init tsi_init(void)
{
    int ret;

    if (POWERCTL_STATIC_OFF==pmu_get_tsi_powercfg())
        return -ENODEV;

    if(!tsi_pinmux_selected())
        return -ENODEV;

    ret = platform_driver_register(&tsi_driver);
    if (ret)
        printk(KERN_ERR "%s: probe failed: %d\n", DRV_NAME, ret);

    return ret;
}

static void __exit tsi_exit(void)
{
    platform_driver_unregister(&tsi_driver);
}
late_initcall(tsi_init);
module_exit(tsi_exit);

MODULE_DESCRIPTION("TSI module for userspace io");
MODULE_AUTHOR("Montage Taiwan");
MODULE_ALIAS("TSI module");

