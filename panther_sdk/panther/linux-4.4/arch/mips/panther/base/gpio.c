/*
 *  Copyright (c) 2017    Montage Inc.    All rights reserved.
 */

#include <linux/module.h>
#include <linux/io.h>
#include <linux/irq.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/gpio/driver.h>
#include <asm/mach-panther/panther.h>
#include <asm/mach-panther/irq.h>
#include <asm/mach-panther/gpio.h>

static DEFINE_RAW_SPINLOCK(panther_gpio_lock);

#ifdef CONFIG_GPIOLIB
struct camelot_gpio {
    struct gpio_chip gpio_chip;
};
#endif

int panther_raw_gpio_get_value(unsigned gpio)
{
    if(gpio < PANTHER_GPIO_LINES)
        return (PMUREG_READ32(GPIO_ID) & (0x01 << gpio));

    return 0;
}

/* The values are boolean, zero for low, nonzero for high */
/* return 0 if the GPIO pin is set to output mode */
int camelot_gpio_get_value(unsigned gpio)
{
    //unsigned long flags;
    int value = 0;

    if(gpio < PANTHER_GPIO_LINES)
    {
        //raw_spin_lock_irqsave(&panther_gpio_lock, flags);

        if((PMUREG_READ32(GPIO_OEN) & (0x01 << gpio)))
            value = (PMUREG_READ32(GPIO_ID) & (0x01 << gpio));

        //raw_spin_unlock_irqrestore(&panther_gpio_lock, flags);
    }

    return value;
}
//EXPORT_SYMBOL(camelot_gpio_get_value);

void camelot_gpio_set_value(unsigned gpio, int value)
{
    //unsigned long flags;

    if (gpio < PANTHER_GPIO_LINES)
    {
        //raw_spin_lock_irqsave(&panther_gpio_lock, flags);

        if (value)
            PMUREG_WRITE32(GPIO_ODS, (0x01 << gpio));
        else
            PMUREG_WRITE32(GPIO_ODC, (0x01 << gpio));

        //raw_spin_unlock_irqrestore(&panther_gpio_lock, flags);
    }
}
//EXPORT_SYMBOL(camelot_gpio_set_value);

int camelot_gpio_direction_input(unsigned gpio)
{
    unsigned long flags;
    int err = -EINVAL;

    if (gpio < PANTHER_GPIO_LINES)
    {
        raw_spin_lock_irqsave(&panther_gpio_lock, flags);

        PMUREG_UPDATE32(GPIO_OEN, (0x01 << gpio), (0x01 << gpio));        /* set the corresponding bit to 1 for input */

        raw_spin_unlock_irqrestore(&panther_gpio_lock, flags);
        err = 0;
    }

    return err;
}
//EXPORT_SYMBOL(camelot_gpio_direction_input);

int camelot_gpio_direction_output(unsigned gpio, int value)
{
    unsigned long flags;
    int err = -EINVAL;

    if (gpio < PANTHER_GPIO_LINES)
    {
        raw_spin_lock_irqsave(&panther_gpio_lock, flags);

        camelot_gpio_set_value(gpio, value);
        PMUREG_UPDATE32(GPIO_OEN, 0, (0x01 << gpio));        /* clear the corresponding bit to 0 for output */

        raw_spin_unlock_irqrestore(&panther_gpio_lock, flags);
        err = 0;
    }

    return err;
}
//EXPORT_SYMBOL(camelot_gpio_direction_output);

int panther_raw_gpio_request(unsigned int gpio)
{
    unsigned long flags;

    if (gpio < PANTHER_GPIO_LINES)
    {
        raw_spin_lock_irqsave(&panther_gpio_lock, flags);

        PMUREG_UPDATE32(GPIO_FUNC_EN, (0x01 << gpio), (0x01 << gpio));

        raw_spin_unlock_irqrestore(&panther_gpio_lock, flags);

        return 0;
    }

    return -ENXIO;
}

#ifdef CONFIG_GPIOLIB
static int cta_gpio_request(struct gpio_chip *chip, unsigned offset)
{
    return panther_raw_gpio_request(offset);
}

static void cta_gpio_free(struct gpio_chip *chip, unsigned offset)
{
    unsigned long flags;

    if (offset < PANTHER_GPIO_LINES)
    {
        raw_spin_lock_irqsave(&panther_gpio_lock, flags);

        PMUREG_UPDATE32(GPIO_FUNC_EN, 0, (0x01 << offset));

        raw_spin_unlock_irqrestore(&panther_gpio_lock, flags);
    }
}

static inline int cta_gpio_direction_out(struct gpio_chip *chip,
                     unsigned offset, int value)
{
    return camelot_gpio_direction_output(offset, value);
}

static inline int cta_gpio_direction_in(struct gpio_chip *chip, unsigned offset)
{
    return camelot_gpio_direction_input(offset);
}

static inline int cta_gpio_get(struct gpio_chip *chip, unsigned offset)
{
   return camelot_gpio_get_value(offset);
}

static inline void cta_gpio_set(struct gpio_chip *chip, unsigned offset, int value)
{
   return camelot_gpio_set_value(offset, value);
}

static struct gpio_chip gc =
{
    .label            = "panther_gpio",
    .owner            = THIS_MODULE,
    .request          = cta_gpio_request,
    .free             = cta_gpio_free,
    .direction_input  = cta_gpio_direction_in,
    .get              = cta_gpio_get,
    .direction_output = cta_gpio_direction_out,
    .set              = cta_gpio_set,
	.can_sleep        = false,
};

void panther_raw_gpio_irq_mask(int gpio)
{
    unsigned long flags;

    raw_spin_lock_irqsave(&panther_gpio_lock, flags);

    PMUREG_UPDATE32(GPIO_INT_MASK, (0x01 << gpio), (0x01 << gpio));

    raw_spin_unlock_irqrestore(&panther_gpio_lock, flags);
}

void panther_raw_gpio_irq_unmask(int gpio)
{
    unsigned long flags;

    raw_spin_lock_irqsave(&panther_gpio_lock, flags);

    PMUREG_UPDATE32(GPIO_INT_MASK, 0, (0x01 << gpio));

    raw_spin_unlock_irqrestore(&panther_gpio_lock, flags);
}

void panther_raw_gpio_irq_clear_unmask(int gpio)
{
    unsigned long flags;

    raw_spin_lock_irqsave(&panther_gpio_lock, flags);

    PMUREG_WRITE32(GPIO_INT_STATUS, (0x01 << gpio));
    PMUREG_UPDATE32(GPIO_INT_MASK, 0, (0x01 << gpio));

    raw_spin_unlock_irqrestore(&panther_gpio_lock, flags);
}

static void panther_gpio_irq_mask(struct irq_data *d)
{
    int gpio = d->hwirq;

    if (gpio < PANTHER_GPIO_LINES)
        panther_raw_gpio_irq_mask(gpio);
}

static void panther_gpio_irq_unmask(struct irq_data *d)
{
    int gpio = d->hwirq;

    if (gpio < PANTHER_GPIO_LINES)
        panther_raw_gpio_irq_unmask(gpio);
}

static int panther_gpio_irq_set_type(struct irq_data *d, unsigned int type)
{
    unsigned long flags;
    int offset = d->hwirq;
    u32 pmureg1 = 0;
    u32 pmureg2 = 0;
    u32 value;

    if (offset >= PANTHER_GPIO_LINES)
        return -ENXIO;

    value = (0x01 << offset);
    switch (type)
    {
        case IRQ_TYPE_EDGE_BOTH:
            pmureg1 = GPIO_INT_RAISING;
            pmureg2 = GPIO_INT_FALLING;
            break;
        case IRQ_TYPE_EDGE_RISING:
            pmureg1 = GPIO_INT_RAISING;
            break;
        case IRQ_TYPE_EDGE_FALLING:
            pmureg1 = GPIO_INT_FALLING;
            break;
        case IRQ_TYPE_LEVEL_LOW:
            pmureg1 = GPIO_INT_LOW;
            break;
        case IRQ_TYPE_LEVEL_HIGH:
            pmureg1 = GPIO_INT_HIGH;
            break;
        case IRQ_TYPE_NONE:
            pmureg1 = GPIO_INT_RAISING;
            pmureg2 = GPIO_INT_FALLING;
            value = 0;
            break;
    }

    raw_spin_lock_irqsave(&panther_gpio_lock, flags);

    if(pmureg1)
        PMUREG_UPDATE32(pmureg1, value, (0x01 << offset));

    if(pmureg2)
        PMUREG_UPDATE32(pmureg2, value, (0x01 << offset));

    raw_spin_unlock_irqrestore(&panther_gpio_lock, flags);

    return 0;
}

static int panther_gpio_irq_set_wake(struct irq_data *d, u32 enable)
{
    return 0;
}

static void panther_gpio_irq_handler(struct irq_desc *desc)
{
	struct irq_chip *irqchip = irq_desc_get_chip(desc);
    unsigned long intr_status;
    int hwirq;
	int handled = 0;

	chained_irq_enter(irqchip, desc);

    intr_status = PMUREG_READ32(GPIO_INT_STATUS);
    PMUREG_WRITE32(GPIO_INT_STATUS, intr_status);

#define IRQS_PER_WORD		32

    for_each_set_bit(hwirq, &intr_status, IRQS_PER_WORD) {
        generic_handle_irq((GPIO_VIRQ_BASE + hwirq));
		handled++;
    }

	/* No interrupts were flagged */
	if (handled == 0)
		handle_bad_irq(desc);

	chained_irq_exit(irqchip, desc);
}

static struct irq_chip panther_gpio_irq_chip = {
	.name		= "GPIO",
	.irq_mask	= panther_gpio_irq_mask,
	.irq_unmask	= panther_gpio_irq_unmask,
	.irq_set_type	= panther_gpio_irq_set_type,
	.irq_set_wake	= panther_gpio_irq_set_wake,
	//.irq_startup	= NULL,
	//.irq_shutdown	= NULL,
};

static int camelot_gpio_probe(struct platform_device *pdev)
{
    struct camelot_gpio *camelot_gpio;
    int ret;

    camelot_gpio = kzalloc(sizeof(*camelot_gpio), GFP_KERNEL);
    if (camelot_gpio == NULL)
    {
        return -ENOMEM;
    }
    camelot_gpio->gpio_chip = gc;
    camelot_gpio->gpio_chip.ngpio = PANTHER_GPIO_LINES;
    camelot_gpio->gpio_chip.dev = &pdev->dev;
    camelot_gpio->gpio_chip.base = -1;

    ret = gpiochip_add(&camelot_gpio->gpio_chip);
    if (ret < 0)
    {
        dev_err(&pdev->dev, "Could not register gpiochip, %d\n", ret);
        goto err;
    }

    ret = gpiochip_irqchip_add(&camelot_gpio->gpio_chip,
                    &panther_gpio_irq_chip,
                    GPIO_VIRQ_BASE,
                    handle_simple_irq,
                    IRQ_TYPE_NONE);
    if (ret) {
        dev_err(&pdev->dev, "Could not connect irqchip to gpiochip, %d\n", ret);
        gpiochip_remove(&camelot_gpio->gpio_chip);
        goto err;
    }

    gpiochip_set_chained_irqchip(&camelot_gpio->gpio_chip,
                     &panther_gpio_irq_chip,
                     IRQ_GPIO,
                     panther_gpio_irq_handler);

    platform_set_drvdata(pdev, camelot_gpio);

    return ret;

err:
    kfree(camelot_gpio);
    return ret;
}

static int camelot_gpio_remove(struct platform_device *pdev)
{
    struct camelot_gpio *camelot_gpio = platform_get_drvdata(pdev);
    int err = -EINVAL;

    if (camelot_gpio)
    {
        gpiochip_remove(&camelot_gpio->gpio_chip);
        kfree(camelot_gpio);
        err = 0;
    }

    return err;
}

static struct platform_driver camelot_gpio_driver =
{
    .driver.name  = PLATFORM_GPIO_NAME,
    .driver.owner = THIS_MODULE,
    .probe        = camelot_gpio_probe,
    .remove       = camelot_gpio_remove,
};
#endif

static int __init camelot_gpio_init(void)
{
#ifdef CONFIG_GPIOLIB
    return platform_driver_register(&camelot_gpio_driver);
#else
    return 0;
#endif
}
postcore_initcall(camelot_gpio_init);
//arch_initcall(camelot_gpio_init);
//subsys_initcall(camelot_gpio_init);

static void __exit camelot_gpio_exit(void)
{
#ifdef CONFIG_GPIOLIB
    platform_driver_unregister(&camelot_gpio_driver);
#endif
}
module_exit(camelot_gpio_exit);

