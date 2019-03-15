/* 
 *  Copyright (c) 2017	Montage Inc.	All rights reserved. 
 */
#include <linux/pm.h>
#include <linux/delay.h>
#include <asm/io.h>
#include <asm/reboot.h>
#include <asm/bootinfo.h>
#include <asm/mach-panther/panther.h>
#include <asm/mach-panther/reg.h>
#include <asm/mach-panther/pmu.h>

static DEFINE_SPINLOCK(pmu_lock);

static void __pmu_reset_devices(unsigned long *device_ids, int use_spinlock)
{
    unsigned long flags;
    int i = 0;
    unsigned long curr_id;
    u32 pmu_reset_reg24_mask = 0;
    u32 pmu_reset_reg25_mask = 0;

    if(device_ids==NULL)
        return;

    while(1)
    {
        curr_id = device_ids[i];
        if((curr_id/100)==24)
            pmu_reset_reg24_mask |= (0x01 << (curr_id%100));
        else if((curr_id/100)==25)
            pmu_reset_reg25_mask |= (0x01 << (curr_id%100));
        else
            break;

        i++;
    }

    if(use_spinlock)
        spin_lock_irqsave(&pmu_lock, flags);

    if(pmu_reset_reg24_mask)
        REG_UPDATE32(PMU_RESET_REG24, 0x0, pmu_reset_reg24_mask);

    if(pmu_reset_reg25_mask)
        REG_UPDATE32(PMU_RESET_REG25, 0x0, pmu_reset_reg25_mask);

    udelay(1);

    if(pmu_reset_reg25_mask)
        REG_UPDATE32(PMU_RESET_REG25, 0xffffffff, pmu_reset_reg25_mask);

    if(pmu_reset_reg24_mask)
        REG_UPDATE32(PMU_RESET_REG24, 0xffffffff, pmu_reset_reg24_mask);

    if(use_spinlock)
        spin_unlock_irqrestore(&pmu_lock, flags);
}

void pmu_reset_devices(unsigned long *device_ids)
{
    __pmu_reset_devices(device_ids, 1);
}

void pmu_reset_devices_no_spinlock(unsigned long *device_ids)
{
    __pmu_reset_devices(device_ids, 0);
}

void pmu_system_restart(void)
{
    unsigned long flags;

    spin_lock_irqsave(&pmu_lock, flags);

    if(REG_READ32(PMU_SOFTWARE_GPREG))
    {
        REG_WRITE32(PMU_SLP_TMR_CTRL, 0x01000001);
        REG_WRITE32(PMU_CTRL, 0x00000081);
    }
    else
    {
        REG_WRITE32(PMU_WATCHDOG, 0x01000100);
    }

    while(1)
        ;
}

void pmu_system_halt(void)
{
    unsigned long flags;

#ifdef CONFIG_SMP
	//smp_send_stop();
#endif

    spin_lock_irqsave(&pmu_lock, flags);

    REG_WRITE32(PMU_WATCHDOG, 0x0);

    while(1)
        ;
}

void pmu_set_gpio_driving_strengh(int *gpio_ids, unsigned long *gpio_vals)
{
    unsigned long flags;
    int i = 0;
    int curr_id;
    unsigned long value;
    u32 gpio_driver_0_15_mask = 0;
    u32 gpio_driver_0_15_val = 0;
    u32 gpio_driver_16_31_mask = 0;
    u32 gpio_driver_16_31_val = 0;
    u32 gpio_driver_32_47_mask = 0;
    u32 gpio_driver_32_47_val = 0;

    if(gpio_ids==NULL)
        return;

    while(1)
    {
        curr_id = gpio_ids[i];
        value = gpio_vals[i];
        if(curr_id < 0)
        {
            break;
        }
        else if(curr_id < 16)
        {
            gpio_driver_0_15_mask |= (0x03 << (curr_id * 2));
            gpio_driver_0_15_val |=  ((0x03 & value) << (curr_id * 2));
        }
        else if(curr_id < 32)
        {
            gpio_driver_16_31_mask |= (0x03 << ((curr_id - 16) * 2));
            gpio_driver_16_31_val |= ((0x03 & value) << ((curr_id - 16) * 2));
        }
        else if(curr_id < 48)
        {
            gpio_driver_32_47_mask |= (0x03 << ((curr_id - 32) * 2));
            gpio_driver_32_47_val |= ((0x03 & value) << ((curr_id - 32) * 2));
        }

        i++;
    }

    spin_lock_irqsave(&pmu_lock, flags);

    if(gpio_driver_0_15_mask)
        REG_UPDATE32(PMU_GPIO_DRIVER_0_15, gpio_driver_0_15_val, gpio_driver_0_15_mask);

    if(gpio_driver_16_31_mask)
        REG_UPDATE32(PMU_GPIO_DRIVER_16_31, gpio_driver_16_31_val, gpio_driver_16_31_mask);

    if(gpio_driver_32_47_mask)
        REG_UPDATE32(PMU_GPIO_DRIVER_32_47, gpio_driver_32_47_val, gpio_driver_32_47_mask);

    spin_unlock_irqrestore(&pmu_lock, flags);
}

void pmu_set_gpio_input_enable(int *gpio_ids, unsigned long *gpio_vals)
{
    pmu_set_gpio_driving_strengh(gpio_ids, gpio_vals);
}

void pmu_get_gpio_function(int *gpio_ids, unsigned long *gpio_funcs)
{
    unsigned long flags;
    int i = 0;
    int curr_id;
    unsigned long func;
    u32 gpio_func_0_7_val;
    u32 gpio_func_8_15_val;
    u32 gpio_func_16_23_val;
    u32 gpio_func_24_31_val;
    u32 gpio_func_32_39_val;
    u32 gpio_func_0_7_mask = 0;
    u32 gpio_func_8_15_mask = 0;
    u32 gpio_func_16_23_mask = 0;
    u32 gpio_func_24_31_mask = 0;
    u32 gpio_func_32_39_mask = 0;

    if(gpio_ids==NULL)
        return;

    spin_lock_irqsave(&pmu_lock, flags);

    gpio_func_0_7_val = REG_READ32(PMU_GPIO_FUNC_0_7);
    gpio_func_8_15_val = REG_READ32(PMU_GPIO_FUNC_8_15);
    gpio_func_16_23_val = REG_READ32(PMU_GPIO_FUNC_16_23);
    gpio_func_24_31_val = REG_READ32(PMU_GPIO_FUNC_24_31);
    gpio_func_32_39_val = REG_READ32(PMU_GPIO_FUNC_32_39);

    spin_unlock_irqrestore(&pmu_lock, flags);

    while(1)
    {
        curr_id = gpio_ids[i];
        if(curr_id < 0)
        {
            break;
        }
        else if(curr_id < 8)
        {
            gpio_func_0_7_mask |= (0x0F << (curr_id * 4));
            func = ((gpio_func_0_7_val & gpio_func_0_7_mask) >> (curr_id * 4));
        }
        else if(curr_id < 16)
        {
            gpio_func_8_15_mask |= (0x0F << ((curr_id - 8) * 4));
            func = ((gpio_func_8_15_val & gpio_func_8_15_mask) >> ((curr_id - 8) * 4));
        }
        else if(curr_id < 24)
        {
            gpio_func_16_23_mask |= (0x0F << ((curr_id - 16) * 4));
            func = ((gpio_func_16_23_val & gpio_func_16_23_mask) >> ((curr_id - 16) * 4));
        }
        else if(curr_id < 32)
        {
            gpio_func_24_31_mask |= (0x0F << ((curr_id - 24) * 4));
            func = ((gpio_func_24_31_val & gpio_func_24_31_mask) >> ((curr_id - 24) * 4));
        }
        else if(curr_id < 40)
        {
            gpio_func_32_39_mask |= (0x0F << ((curr_id - 32) * 4));
            func = ((gpio_func_32_39_val & gpio_func_32_39_mask) >> ((curr_id - 32) * 4));
        }
        else
        {
            func = 10;
        }
        gpio_funcs[i] = func;

        i++;
    }
}

void pmu_set_gpio_function(int *gpio_ids, unsigned long *gpio_funcs)
{
    unsigned long flags;
    int i = 0;
    int curr_id;
    unsigned long func;
    u32 gpio_func_0_7_val = 0;
    u32 gpio_func_8_15_val = 0;
    u32 gpio_func_16_23_val = 0;
    u32 gpio_func_24_31_val = 0;
    u32 gpio_func_32_39_val = 0;
    u32 gpio_func_0_7_mask = 0;
    u32 gpio_func_8_15_mask = 0;
    u32 gpio_func_16_23_mask = 0;
    u32 gpio_func_24_31_mask = 0;
    u32 gpio_func_32_39_mask = 0;

    if(gpio_ids==NULL)
        return;

    while(1)
    {
        curr_id = gpio_ids[i];
        func = gpio_funcs[i];
        if(curr_id < 0)
        {
            break;
        }
        else if(curr_id < 8)
        {
            gpio_func_0_7_mask |= (0x0F << (curr_id * 4));
            gpio_func_0_7_val |=  ((0x0F & func) << (curr_id * 4));
        }
        else if(curr_id < 16)
        {
            gpio_func_8_15_mask |= (0x0F << ((curr_id - 8) * 4));
            gpio_func_8_15_val |=  ((0x0F & func) << ((curr_id - 8) * 4));
        }
        else if(curr_id < 24)
        {
            gpio_func_16_23_mask |= (0x0F << ((curr_id - 16) * 4));
            gpio_func_16_23_val |=  ((0x0F & func) << ((curr_id - 16) * 4));
        }
        else if(curr_id < 32)
        {
            gpio_func_24_31_mask |= (0x0F << ((curr_id - 24) * 4));
            gpio_func_24_31_val |=  ((0x0F & func) << ((curr_id - 24) * 4));
        }
        else if(curr_id < 40)
        {
            gpio_func_32_39_mask |= (0x0F << ((curr_id - 32) * 4));
            gpio_func_32_39_val |=  ((0x0F & func) << ((curr_id - 32) * 4));
        }

        i++;
    }

    spin_lock_irqsave(&pmu_lock, flags);

    if(gpio_func_0_7_mask)
        REG_UPDATE32(PMU_GPIO_FUNC_0_7, gpio_func_0_7_val, gpio_func_0_7_mask);

    if(gpio_func_8_15_mask)
        REG_UPDATE32(PMU_GPIO_FUNC_8_15, gpio_func_8_15_val, gpio_func_8_15_mask);

    if(gpio_func_16_23_mask)
        REG_UPDATE32(PMU_GPIO_FUNC_16_23, gpio_func_16_23_val, gpio_func_16_23_mask);

    if(gpio_func_24_31_mask)
        REG_UPDATE32(PMU_GPIO_FUNC_24_31, gpio_func_24_31_val, gpio_func_24_31_mask);

    if(gpio_func_32_39_mask)
        REG_UPDATE32(PMU_GPIO_FUNC_32_39, gpio_func_32_39_val, gpio_func_32_39_mask);

    spin_unlock_irqrestore(&pmu_lock, flags);
}

void pmu_update_pcm_clk(int clk_sel)
{
    unsigned long flags;
    u32 val = 0;
    if(clk_sel)
    {
        val = PCM_CLK_SEL;
    }

    spin_lock_irqsave(&pmu_lock, flags);
    REG_UPDATE32(PMU_CPLL_XDIV_REG_0_31, val, PCM_CLK_SEL);
    spin_unlock_irqrestore(&pmu_lock, flags);
}

void pmu_update_i2s_clk(int domain_sel, int bypass_en, unsigned long clk_div_sel, unsigned long ndiv_sel)
{
    unsigned long flags;
    u32 xdiv_reg_0_31_val = 0;
    u32 xdiv_reg_32_47_val = 0;
    u32 xdiv_reg2_0_31_val = 0;

    if(domain_sel)
    {
        xdiv_reg_32_47_val |= I2S_DOMAIN_SEL;
    }

    if(bypass_en)
    {
        xdiv_reg2_0_31_val |= I2S_BYPASS_EN;
    }

    xdiv_reg2_0_31_val |= ((clk_div_sel << I2S_CLK_SEL_SHIFT) & I2S_CLK_SEL_MASK);
    xdiv_reg_0_31_val |= ((ndiv_sel << I2S_NDIV_SEL_SHIFT) & I2S_NDIV_SEL_MASK);

    spin_lock_irqsave(&pmu_lock, flags);
    REG_UPDATE32(PMU_CPLL_XDIV_REG_0_31, xdiv_reg_0_31_val, I2S_NDIV_SEL_MASK);
    REG_UPDATE32(PMU_CPLL_XDIV_REG_32_47, xdiv_reg_32_47_val, I2S_DOMAIN_SEL);
    REG_UPDATE32(PMU_CPLL_XDIV_REG2_0_31, xdiv_reg2_0_31_val, I2S_CLK_SEL_MASK|I2S_BYPASS_EN);
    spin_unlock_irqrestore(&pmu_lock, flags);

}

void pmu_update_spdif_clk(int domain_sel, int bypass_en, unsigned long clk_div_sel, unsigned long ndiv_sel)
{
    unsigned long flags;
    u32 xdiv_reg_0_31_val = 0;
    u32 xdiv_reg_32_47_val = 0;
    u32 xdiv_reg2_0_31_val = 0;

    if(domain_sel)
    {
        xdiv_reg_32_47_val |= SPDIF_DOMAIN_SEL;
    }

    if(bypass_en)
    {
        xdiv_reg2_0_31_val |= SPDIF_BYPASS_EN;
    }

    xdiv_reg2_0_31_val |= ((clk_div_sel << SPDIF_CLK_SEL_SHIFT) & SPDIF_CLK_SEL_MASK);
    xdiv_reg_0_31_val |= ((ndiv_sel << SPDIF_NDIV_SEL_SHIFT) & SPDIF_NDIV_SEL_MASK);

    spin_lock_irqsave(&pmu_lock, flags);
    REG_UPDATE32(PMU_CPLL_XDIV_REG_0_31, xdiv_reg_0_31_val, SPDIF_NDIV_SEL_MASK);
    REG_UPDATE32(PMU_CPLL_XDIV_REG_32_47, xdiv_reg_32_47_val, SPDIF_DOMAIN_SEL);
    REG_UPDATE32(PMU_CPLL_XDIV_REG2_0_31, xdiv_reg2_0_31_val, SPDIF_CLK_SEL_MASK|SPDIF_BYPASS_EN);
    spin_unlock_irqrestore(&pmu_lock, flags);
}

void pmu_initial_dac(void)
{
    unsigned long flags;

    spin_lock_irqsave(&pmu_lock, flags);

    /* set as suggestion from QiuYu. */
    REG_UPDATE32(PMU_AUDIO_DAC_REG0_REG1, DAC_POWER_1_4_V, DAC_POWER_LVL_MASK);
    REG_WRITE32(PMU_AUDIO_DAC_REG2, 0x55428E00);
    REG_WRITE32(PMU_AUDIO_DAC_REG3, 0x03040200);
    REG_WRITE32(PMU_AUDIO_DAC_REG4, 0x01B2000F);
    REG_WRITE32(PMU_AUDIO_DAC_REG2, 0x5540A555);

    spin_unlock_irqrestore(&pmu_lock, flags);
}

void pmu_internal_audio_enable(int en)
{
    unsigned long flags;
    u32 val = 0;
    if(1 == en)
    {
        val = 1;
    }
    spin_lock_irqsave(&pmu_lock, flags);
    REG_UPDATE32(PMU_RF_REG_CTRL, val, 0x00000001);
    spin_unlock_irqrestore(&pmu_lock, flags);
}

void pmu_internal_dac_enable(int en)
{
    unsigned long flags;
    u32 val = 0;
    if(1 == en)
    {
        val = INTERNAL_DAC_EN;
    }
    spin_lock_irqsave(&pmu_lock, flags);
    REG_UPDATE32(PMU_ADC_DAC_REG_CTRL, val, INTERNAL_DAC_EN);
    spin_unlock_irqrestore(&pmu_lock, flags);
}

void pmu_internal_adc_enable(int en)
{
    unsigned long flags;
    u32 val = 0;
    if(1 == en)
    {
        val = INTERNAL_ADC_EN;
    }
    spin_lock_irqsave(&pmu_lock, flags);
    REG_UPDATE32(PMU_ADC_DAC_REG_CTRL, val, INTERNAL_ADC_EN);
    spin_unlock_irqrestore(&pmu_lock, flags);
}

void pmu_internal_adc_clock_en(int en)
{
    unsigned long flags;
    u32 val = AADC_RESET_MASK;
    if(1 == en)
    {
        val = 0;
    }
    spin_lock_irqsave(&pmu_lock, flags);
    REG_UPDATE32(PMU_ADC_REG0_1, val, AADC_RESET_MASK);
    spin_unlock_irqrestore(&pmu_lock, flags);
}

void pmu_update_dac_clk(int dac_fc)
{
    unsigned long flags;
    u32 xdiv_reg_0_31_val = 0;
    xdiv_reg_0_31_val |= ((dac_fc << DAC_FS_SHIFT) & DAC_FS_MASK);

    spin_lock_irqsave(&pmu_lock, flags);
    REG_UPDATE32(PMU_CPLL_XDIV_REG_0_31, xdiv_reg_0_31_val, DAC_FS_MASK);
    spin_unlock_irqrestore(&pmu_lock, flags);
}

void pmu_update_adc_clk(int mode_type)
{
    unsigned long flags;
	u32 val = 0;
    /* adc support fs 16k(bandwidth 4k) & 64k(bandwidth 16k)
       in PMU_ADC_REG2[2:3], it means bandwidth 16k(0) and 4k(1) in Document */
    if(AADC_FS_16k == mode_type)
        val = AADC_FS_MASK;
    spin_lock_irqsave(&pmu_lock, flags);
    REG_UPDATE32(PMU_ADC_REG2, val, AADC_FS_MASK);
    spin_unlock_irqrestore(&pmu_lock, flags);
}

static unsigned int powercfg = 0x0FFFF;
void load_powercfg_from_boot_cmdline(void)
{
    char *str;

    if((str = strstr(arcs_cmdline, "powercfg=")))
        sscanf(&str[9], "%08x", &powercfg);
}

int pmu_get_usb_powercfg(void)
{
    if(powercfg & POWERCTL_FLAG_USB_DYNAMIC)
        return POWERCTL_DYNAMIC;

    if(0==(powercfg & POWERCTL_FLAG_USB_STATIC))
        return POWERCTL_STATIC_OFF;

    return POWERCTL_STATIC_ON;
}

int pmu_get_sdio_powercfg(void)
{
    if(powercfg & POWERCTL_FLAG_SDIO_DYNAMIC)
        return POWERCTL_DYNAMIC;

    if(0==(powercfg & POWERCTL_FLAG_SDIO_STATIC))
        return POWERCTL_STATIC_OFF;

    return POWERCTL_STATIC_ON;
}

int pmu_get_ethernet_powercfg(void)
{
    if(powercfg & POWERCTL_FLAG_ETHERNET_DYNAMIC)
        return POWERCTL_DYNAMIC;

    if(0==(powercfg & POWERCTL_FLAG_ETHERNET_STATIC))
        return POWERCTL_STATIC_OFF;

    return POWERCTL_STATIC_ON;
}

int pmu_get_audio_codec_powercfg(void)
{
    if(powercfg & POWERCTL_FLAG_AUDIO_CODEC_DYNAMIC)
        return POWERCTL_DYNAMIC;

    if(0==(powercfg & POWERCTL_FLAG_AUDIO_CODEC_STATIC))
        return POWERCTL_STATIC_OFF;

    return POWERCTL_STATIC_ON;
}

int pmu_get_tsi_powercfg(void)
{
    if(powercfg & POWERCTL_FLAG_TSI_DYNAMIC)
        return POWERCTL_DYNAMIC;

    if(0==(powercfg & POWERCTL_FLAG_TSI_STATIC))
        return 1;

    return 0;
}

int pmu_get_wifi_powercfg(void)
{
    if(powercfg & POWERCTL_FLAG_WIFI_DYNAMIC)
        return POWERCTL_DYNAMIC;

    if(0==(powercfg & POWERCTL_FLAG_WIFI_STATIC))
        return POWERCTL_STATIC_OFF;

    return POWERCTL_STATIC_ON;
}

