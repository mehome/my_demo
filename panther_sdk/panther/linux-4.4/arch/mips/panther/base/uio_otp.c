#include <linux/io.h>
#include <linux/module.h>

#include <asm/mach-panther/panther.h>
#include <asm/mach-panther/otp.h>
#include <linux/uio_driver.h>
#include <linux/platform_device.h>
#include <linux/slab.h>

#define OTP_REGBANK_BASE    (PANTHER_REG_BASE + 0x1000)
#define OTP_REGBANK_SIZE    0x1000

#define PHYSICAL_ADDR(x)    ((u32) (x) & 0x1fffffffUL)

#define DRV_NAME "uio-otp"
#define DRV_VERSION "0.0.1"

static int __init otp_probe(struct platform_device *dev)
{
    struct uio_info *info;
    int ret;

    info = kzalloc(sizeof(struct uio_info), GFP_KERNEL);
    if (unlikely(!info)) {
        dev_err(&dev->dev, "Could not allocate info\n");
        return -ENOMEM;
    }

    info->mem[0].addr = PHYSICAL_ADDR(OTP_REGBANK_BASE);
    info->mem[0].size = OTP_REGBANK_SIZE;
    info->mem[0].internal_addr = (void *) (OTP_REGBANK_BASE);
    info->mem[0].memtype = UIO_MEM_PHYS;
    info->version = DRV_VERSION;
    info->name = DRV_NAME;
    info->irq = UIO_IRQ_NONE;

    ret = uio_register_device(&dev->dev, info);
    if (ret)
    {
        kfree(info);
        return -ENODEV;
    }

    platform_set_drvdata(dev, info);
    return 0;
}

static int __exit otp_remove(struct platform_device *dev)
{
    struct uio_info *info = platform_get_drvdata(dev);

    uio_unregister_device(info);
    platform_set_drvdata(dev, NULL);
    kfree(info);
    return 0;
}

static struct platform_driver otp_driver = {
    .probe = otp_probe,
    .remove = otp_remove,
    .driver = {
        .name = DRV_NAME,
        .owner = THIS_MODULE,
    },
};

static int __init otp_init(void)
{
    int ret;

    ret = platform_driver_register(&otp_driver);
    if (ret)
        printk(KERN_ERR "%s: probe failed: %d\n", DRV_NAME, ret);

    return ret;
}

static void __exit otp_exit(void)
{
    platform_driver_unregister(&otp_driver);
}
late_initcall(otp_init);
module_exit(otp_exit);

MODULE_DESCRIPTION("OTP module for userspace io");
MODULE_AUTHOR("Edden Tsai");
MODULE_ALIAS("OTP module");

