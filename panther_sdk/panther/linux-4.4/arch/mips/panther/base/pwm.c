/*
 * Montage Technologies Pulse Width Modulator driver
 *
 * Copyright (c) 2014-2015, Montage Technologies
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License.
 */

#include <linux/clk.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/pwm.h>
#include <linux/regmap.h>
#include <linux/slab.h>

/* PWM registers */
#define PWM_PRE_SCALAR	0x0
#define PWM_C0_C1_CFG	0x4
#define PWM_C2_C3_CFG	0x8
#define PWM_C4_C5_CFG	0xC

#define C_EN_BIT        13
#define AUTO_BIT        12
#define POL_BIT         11
#define DEFAULT_CLK     156250
#define IN_610_20_CLK   4882 // 156250/32=4882
#define UNDER_20_CLK    13   // 156250/12000=13
#define CLK_32_DIV      0x07
#define CLK_12000_DIV   0x01

struct panther_pwm_chip {
	struct device *dev;
	struct pwm_chip chip;
	void __iomem *base;
    u8 pre_scalar[3]; //0->C0,C1 1->C2,C3 2->C4,C5
    u8 mode; //0->normal, 1->auto
    u8 sweep[6];
};

enum panther_pwm_mode
{
    MODE_NORMAL,
    MODE_AUTO,
};

static struct panther_pwm_chip *to_panther_pwm_chip(struct pwm_chip *chip)
{
	return container_of(chip, struct panther_pwm_chip, chip);
}

static void panther_pwm_writel(struct panther_pwm_chip *panther_pwm, u32 reg, u32 val)
{
	writel(val, panther_pwm->base + reg);
}

static u32 panther_pwm_readl(struct panther_pwm_chip *panther_pwm, u32 reg)
{
	return readl(panther_pwm->base + reg);
}

static int panther_pwm_config(struct pwm_chip *chip, struct pwm_device *pwm, int duty_ns, int period_ns)
{
    struct panther_pwm_chip *panther_pwm = to_panther_pwm_chip(chip);
    u32 freq, offset = (pwm->pwm/2)*4, val;
    u8 pre_scalar, duty;

    //setup freq
    if((period_ns > 1000000000) || (period_ns < 10000))
    {
        return -EINVAL;
    }
    else
    {
        freq = 1000000000/period_ns;
        if((freq < 610) && (panther_pwm->mode == MODE_NORMAL))
        {
            if(panther_pwm->mode == MODE_AUTO)
            {
                return -EINVAL;
            }
            else
            {
                if(freq < 20)
                {
                    //156250 / 12000 = 13, freq = 13 / pre_scalar
                    pre_scalar = (UNDER_20_CLK / freq) - 1; 
                    val = panther_pwm_readl(panther_pwm, PWM_PRE_SCALAR);
                    val = (val & ~(0xFF << (offset*2))) | (pre_scalar << (offset*2));  //offset = offset*8 = offset*4*2
                    panther_pwm_writel(panther_pwm, PWM_PRE_SCALAR, val);
                    val = (panther_pwm_readl(panther_pwm, PWM_C0_C1_CFG+offset) & ~(0x0700 << (pwm->pwm%2)*16)) |
                                           (CLK_12000_DIV << (8+(pwm->pwm%2)*16));
                    panther_pwm_writel(panther_pwm, PWM_C0_C1_CFG+offset, val);
                }
                else
                {
                    //156250 / 32 = 4882, freq = 4882 / pre_scalar
                    pre_scalar = (IN_610_20_CLK / freq) - 1;
                    val = panther_pwm_readl(panther_pwm, PWM_PRE_SCALAR);
                    val = (val & ~(0xFF << (offset*2))) | (pre_scalar << (offset*2));  //offset = offset*8 = offset*4*2
                    panther_pwm_writel(panther_pwm, PWM_PRE_SCALAR, val);
                    val = (panther_pwm_readl(panther_pwm, PWM_C0_C1_CFG+offset) & ~(0x0700 << (pwm->pwm%2)*16)) |
                                           (CLK_32_DIV << (8+(pwm->pwm%2)*16));
                    panther_pwm_writel(panther_pwm, PWM_C0_C1_CFG+offset, val);
                }
            }
        }
        else
        {
            //freq = 156250 /pre_scalar
            pre_scalar = (DEFAULT_CLK / freq) - 1;
            val = panther_pwm_readl(panther_pwm, PWM_PRE_SCALAR);
            val = (val & ~(0xFF << (offset*2))) | (pre_scalar << (offset*2));  //offset = offset*8 = offset*4*2
            panther_pwm_writel(panther_pwm, PWM_PRE_SCALAR, val);
            val = panther_pwm_readl(panther_pwm, PWM_C0_C1_CFG+offset) & ~(0x0700 << (pwm->pwm%2)*16);
            panther_pwm_writel(panther_pwm, PWM_C0_C1_CFG+offset, val);
        }
    }

    //setup duty
    if(duty_ns > 0)
    {
        if(panther_pwm->mode == MODE_NORMAL)
        {
            duty = (duty_ns/1000*0xFF)/(period_ns/1000);
            val = (panther_pwm_readl(panther_pwm, PWM_C0_C1_CFG+offset) & ~(0xFF << (pwm->pwm%2)*16)) | (duty << (pwm->pwm%2)*16);
            panther_pwm_writel(panther_pwm, PWM_C0_C1_CFG+offset, val);
        }
        else if(panther_pwm->mode == MODE_AUTO)
        {
            if(duty_ns > 8)
            {
                return -EINVAL;
            }
            else
            {
                duty = duty_ns - 1;
                val = (panther_pwm_readl(panther_pwm, PWM_C0_C1_CFG+offset) & ~(0x0700 << (pwm->pwm%2)*16)) | (duty << (8+(pwm->pwm%2)*16));
                panther_pwm_writel(panther_pwm, PWM_C0_C1_CFG+offset, val);
            }
        }
    }

	return 0;
}

static int panther_pwm_enable(struct pwm_chip *chip, struct pwm_device *pwm)
{
    struct panther_pwm_chip *panther_pwm = to_panther_pwm_chip(chip);
    u32 offset = (pwm->pwm/2)*4, val;

    val = panther_pwm_readl(panther_pwm, PWM_C0_C1_CFG+offset) | (1 << (C_EN_BIT + (pwm->pwm%2)*16));
    panther_pwm_writel(panther_pwm, PWM_C0_C1_CFG+offset, val);    

	return 0;
}

static void panther_pwm_disable(struct pwm_chip *chip, struct pwm_device *pwm)
{
    struct panther_pwm_chip *panther_pwm = to_panther_pwm_chip(chip);
    u32 offset = (pwm->pwm/2)*4, val;

    val = panther_pwm_readl(panther_pwm, PWM_C0_C1_CFG+offset) & ~(1 << (C_EN_BIT + (pwm->pwm%2)*16));
    panther_pwm_writel(panther_pwm, PWM_C0_C1_CFG+offset, val);    
}

static int panther_pwm_set_polarity(struct pwm_chip *chip, struct pwm_device *pwm, enum pwm_polarity polarity)
{
    struct panther_pwm_chip *panther_pwm = to_panther_pwm_chip(chip);
    u32 offset = (pwm->pwm/2)*4, val;

    if(polarity == PWM_POLARITY_NORMAL) //duty is high
    {
        val = panther_pwm_readl(panther_pwm, PWM_C0_C1_CFG+offset) | (1 << (POL_BIT + (pwm->pwm%2)*16));
        panther_pwm_writel(panther_pwm, PWM_C0_C1_CFG+offset, val);
    }
    else if (polarity == PWM_POLARITY_INVERSED) //duty is low
    {
        val = panther_pwm_readl(panther_pwm, PWM_C0_C1_CFG+offset) & ~(1 << (POL_BIT + (pwm->pwm%2)*16));
        panther_pwm_writel(panther_pwm, PWM_C0_C1_CFG+offset, val);
    }
    else if (polarity == PWM_POLARITY_AUTOINC)
    {
        val = panther_pwm_readl(panther_pwm, PWM_C0_C1_CFG+offset) | (1 << (AUTO_BIT + (pwm->pwm%2)*16));
        panther_pwm_writel(panther_pwm, PWM_C0_C1_CFG+offset, val);
        panther_pwm->mode = MODE_AUTO;
    }
    else if (polarity == PWM_POLARITY_DISAUTO)
    {
        val = panther_pwm_readl(panther_pwm, PWM_C0_C1_CFG+offset) & ~(1 << (AUTO_BIT + (pwm->pwm%2)*16));
        panther_pwm_writel(panther_pwm, PWM_C0_C1_CFG+offset, val);
        panther_pwm->mode = MODE_NORMAL;
    }

    return 0;
}

static const struct pwm_ops panther_pwm_ops = {
	.config = panther_pwm_config,
	.enable = panther_pwm_enable,
	.disable = panther_pwm_disable,
    .set_polarity = panther_pwm_set_polarity,
	.owner = THIS_MODULE,
};

static int panther_pwm_probe(struct platform_device *pdev)
{
    int ret;
    struct resource *res;
    struct panther_pwm_chip *panther_pwm;

    panther_pwm = devm_kzalloc(&pdev->dev, sizeof(struct panther_pwm_chip), GFP_KERNEL);
    if (panther_pwm == NULL)
    {
        return -ENOMEM;
    }

    res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    panther_pwm->base = devm_ioremap_resource(&pdev->dev, res);
    if (IS_ERR(panther_pwm->base))
    {
        return PTR_ERR(panther_pwm->base);
    }

    panther_pwm->dev = &pdev->dev;
    panther_pwm->chip.dev = &pdev->dev;
    panther_pwm->chip.ops = &panther_pwm_ops;
    panther_pwm->chip.base = -1;
    panther_pwm->chip.npwm = 6;

    ret = pwmchip_add(&panther_pwm->chip);
    if (ret < 0) 
    {
		dev_err(&pdev->dev, "panther_pwmchip_add failed: %d\n", ret);
        return -EIO;
	}

    //reset all pwm, polarity set to 1
    panther_pwm_writel(panther_pwm, PWM_C0_C1_CFG, 0x08000800);
    panther_pwm_writel(panther_pwm, PWM_C2_C3_CFG, 0x08000800);
    panther_pwm_writel(panther_pwm, PWM_C4_C5_CFG, 0x08000800);

	return 0;

}

static int panther_pwm_remove(struct platform_device *pdev)
{
	struct panther_pwm_chip *pwm_chip = platform_get_drvdata(pdev);

	return pwmchip_remove(&pwm_chip->chip);
}

static struct platform_driver panther_pwm_driver = {
	.driver = {
		.name = "mt-pwm",
	},
	.probe = panther_pwm_probe,
	.remove = panther_pwm_remove,
};

module_platform_driver(panther_pwm_driver);

