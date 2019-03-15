#include <linux/module.h>
#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/clk.h>
#include <linux/io.h>

#ifdef CONFIG_PROC_FS
#include <linux/proc_fs.h>
#include <linux/mmc/card.h>
#endif

#include <asm/mach-panther/panther.h>
#include <asm/mach-panther/pmu.h>

#include <linux/mmc/host.h>
#include <asm/bootinfo.h>

#include <asm/mach-panther/sdhci-mt.h>
#include "../../../../drivers/mmc/host/sdhci.h"

/* default enable system to change to big endian, not software */
/* 0xaf0050a4[5:4]:
 *               00:hw change little endian -> big endian
 *                  word:change to big endian
 *                  halfword and byte:shift and change to big endian
 *               10:hw change little endian -> little endian
 *                  word:no change
 *                  halfword and byte:shift
 *               X1:disabled AHB/AMB bus endian change
 * 0xaf00b070[1]:
 *               0:little endian
 *               1:big endian
 *
 * make sure that we always use big endian to access regs
 */
//#define SW_IMP_BIG_ENDIAN
//#define HW_IMP_BIG_ENDIAN

/* finally, AHB for SDIO still is little endian access word/halfword/byte,
 * but AMB change to big endian */


#ifdef CONFIG_MMC_SDHCI_IO_ACCESSORS
#ifdef SW_IMP_BIG_ENDIAN
/* 
 * sdio support byte/halfword/word read/write but not all regs, 
 * 0xaf00b070[1] = 1, make sdio operate in big endian mode
 * software uses word read/write to implement big endian
 */
static u8 sdhci_mt_readb(struct sdhci_host *host, int reg)
{
    return readb(host->ioaddr + reg);
}
static u16 sdhci_mt_readw(struct sdhci_host *host, int reg)
{
    return le16_to_cpu(readw(host->ioaddr + (reg & ~(1))));
}
static u32 sdhci_mt_readl(struct sdhci_host *host, int reg)
{
    u32 ret = *(const volatile u32 __force *) (host->ioaddr + (reg & ~(3)));
    return le32_to_cpu(ret);
}
static void sdhci_mt_writel(struct sdhci_host *host, u32 val, int reg)
{
    *(volatile u32 __force *) (host->ioaddr + (reg & ~(3))) = le32_to_cpu(val);
}
static void sdhci_mt_writew(struct sdhci_host *host, u16 val, int reg)
{
#if 1
    static u16 xfer_mode = 0;
    u32 tmp = sdhci_mt_readl(host, (reg & ~(3)));

    if(reg == SDHCI_TRANSFER_MODE)
    {
        xfer_mode = val;
        return;
    }
    else if(reg == SDHCI_COMMAND)
    {
        tmp = val << 16 | xfer_mode;
    }
    else
    {
        tmp &= (~(0xffff << ((reg & 0x2) * 8)));
        tmp |= (val << ((reg & 0x2) * 8));
    }
    sdhci_mt_writel(host, tmp, (reg & ~(3)));
#else
    // failed, only support word access
    writew(le16_to_cpu(val), host->ioaddr + (reg & ~(1)));
#endif
}
static void sdhci_mt_writeb(struct sdhci_host *host, u8 val, int reg)
{
#if 1
    u32 tmp = sdhci_mt_readl(host, (reg & ~(3)));
    tmp &= (~(0xff << ((reg & 0x3) * 8)));
    tmp |= (val << ((reg & 0x3) * 8));
    sdhci_mt_writel(host, tmp, (reg & ~(3)));
#else
    // failed, only support word access
    writeb(val, host->ioaddr + reg);
#endif
}
#elif defined(HW_IMP_BIG_ENDIAN)
/*
 * SDIO is little endian, but swap/shift to big endian in hw
 */
static u8 sdhci_mt_readb(struct sdhci_host *host, int reg)
{
    return readb(host->ioaddr + reg);
}
static u16 sdhci_mt_readw(struct sdhci_host *host, int reg)
{
    u16 ret = readw(host->ioaddr + (reg & ~(1)));
    return le16_to_cpu(ret);
}
static u32 sdhci_mt_readl(struct sdhci_host *host, int reg)
{
    u32 ret = *(const volatile u32 __force *) (host->ioaddr + (reg & ~(3)));
    return le32_to_cpu(ret);
}
static void sdhci_mt_writel(struct sdhci_host *host, u32 val, int reg)
{
    val = le32_to_cpu(val);
    *(volatile u32 __force *) (host->ioaddr + (reg & ~(3))) = val;
}
static void sdhci_mt_writew(struct sdhci_host *host, u16 val, int reg)
{
    val = le16_to_cpu(val);
    writew(val, host->ioaddr + (reg & ~(1)));
}
static void sdhci_mt_writeb(struct sdhci_host *host, u8 val, int reg)
{
    writeb(val, host->ioaddr + reg);
}
#else
/*
 * SDIO is little endian, but shift in hw when access halfword/byte
 * still little endian for linux
 */
static u8 sdhci_mt_readb(struct sdhci_host *host, int reg)
{
    return readb(host->ioaddr + reg);
}
static u16 sdhci_mt_readw(struct sdhci_host *host, int reg)
{
    return readw(host->ioaddr + (reg & ~(1)));
}
static u32 sdhci_mt_readl(struct sdhci_host *host, int reg)
{
    return *(const volatile u32 __force *) (host->ioaddr + (reg & ~(3)));
}
static void sdhci_mt_writel(struct sdhci_host *host, u32 val, int reg)
{
    *(volatile u32 __force *) (host->ioaddr + (reg & ~(3))) = val;
}
static void sdhci_mt_writew(struct sdhci_host *host, u16 val, int reg)
{
    writew(val, host->ioaddr + (reg & ~(1)));
}
static void sdhci_mt_writeb(struct sdhci_host *host, u8 val, int reg)
{
    writeb(val, host->ioaddr + reg);
}
#endif
#endif

static unsigned int sdhci_mt_get_min_clock(struct sdhci_host *host)
{
	return 400000;
}

static unsigned int sdhci_mt_get_max_clock(struct sdhci_host *host)
{
#ifdef CONFIG_PANTHER_FPGA
    return 50000000;
#else
    return 100000000;
#endif
}

static void sdhci_mt_platform_reset_enter(struct sdhci_host *host, u8 mask)
{
#ifdef CONFIG_PANTHER_MMCSPI
    if (!(GPREG(PINMUX) & EN_SDIO_FNC))
        return;
#endif
    if (mask & SDHCI_RESET_ALL)
    {
#if defined(CONFIG_TODO)
        /* enable SD clock */
        ANAREG(CLKEN) |= SD_CLK_EN;

        /* SD driving adjustment */
        GPREG(DRIVER1) &= ~(DRV_SDIO);
#endif

        /* 
         *  bit 27, 26, 25, 24 : 
         *  bit 23, 22, 21, 20 : MDIO_AUX, SWI2C_AUX,    SWI2C, RF_LEGA
         *  bit 19, 18, 17, 16 : JTAG_AUX,   LED_AUX, UART_AUX, I2C_AUX
         *  bit 15, 14, 13, 12 :  PWM_AUX,      ROM1,     JTAG,     AGC
         *  bit 11, 10,  9,  8 :      PWM,       LED,     UART,    MDIO
         *  bit  7,  6,  5,  4 :     PCM1,      PCM0,     ETH1,    ETH0
         *  bit  3,  2,  1,  0 :     SDIO,        W6,      SIP,     SDR
         *
         *  For PINMUX, Enable SDIO function. Max(SDIO_FNC + ETH0_FNC) <=1
         *  ETH0 must be disabled, when SDIO is enabled
         *  MDIO must be disabled, when SDIO is enabled
         */

#if defined(CONFIG_TODO)
        /* init voltage control by sw or hw */
        if (host->mmc->caps3 & MMC_CAP3_CONTROL_VOLTAGE)
            ANAREG(VOLCTRL) &= ~(3<<0);
        else
            ANAREG(VOLCTRL) |= (1<<1);

        /* disable external BB mode */
        GPREG(GDEBUG) &= ~(0x90<<24);
        /* enable W6 */
        GPREG(GDEBUG) |= (1<<22);
        /* enable sdio module */
        GPREG(GDEBUG) |= (1<<21);
        /* turn on sdio gated clock */
        GPREG(SWRST) &= ~(PAUSE_SDIO);
        /* reset sdio module */
        GPREG(SWRST) &= ~(SWRST_SDIO);
        mdelay(1);
        GPREG(SWRST) |= (SWRST_SDIO);
#endif
    }
}

static void sdhci_mt_platform_reset_exit(struct sdhci_host *host, u8 mask)
{
    if (mask & SDHCI_RESET_ALL)
    {
#if defined(CONFIG_TODO)
        /* system endian setting */
#ifdef SW_IMP_BIG_ENDIAN
        // AHB bus: no change, AMB bus: no change
        // but enable SDIO to big endian
        GPREG(SDIO) |= (0x3UL << SDIO_ENDIAN_PIN);
        writel(readl(host->ioaddr + SDHCI_HOST_CONTROL3) | SDHCI_CTRL_BIG_ENDIAN, host->ioaddr + SDHCI_HOST_CONTROL3);
#elif defined(HW_IMP_BIG_ENDIAN)
        // AHB bus: little to big, AMB bus: little to big
        GPREG(SDIO) &= ~(0x3UL << SDIO_ENDIAN_PIN);
#else
        // AHB bus: little to little, AMB bus: little to big
        GPREG(SDIO) = (GPREG(SDIO) & ~(0x3UL << SDIO_ENDIAN_PIN)) | (0x2UL << SDIO_ENDIAN_PIN);
#endif
#endif
        if (host->mmc->caps3 & MMC_CAP3_INVERTED_SDCLK)
        {
            writel(readl(host->ioaddr + SDHCI_HOST_CONTROL3) | SDHCI_INVERSE_SD_CLOCK, host->ioaddr + SDHCI_HOST_CONTROL3);
        }
    }
}

static struct sdhci_host *sdhci_host = NULL;
static struct sdhci_ops sdhci_mt_ops = {
#ifdef CONFIG_MMC_SDHCI_IO_ACCESSORS
    .read_l = sdhci_mt_readl,
    .read_w = sdhci_mt_readw,
    .read_b = sdhci_mt_readb,
    .write_l = sdhci_mt_writel,
    .write_w = sdhci_mt_writew,
    .write_b = sdhci_mt_writeb,
#endif
    .set_clock = sdhci_set_clock,
    .set_bus_width = sdhci_set_bus_width,
    .get_max_clock = sdhci_mt_get_max_clock,
    .get_min_clock = sdhci_mt_get_min_clock,
    .reset = sdhci_reset,
    .set_uhs_signaling = sdhci_set_uhs_signaling,   
    .platform_reset_enter = sdhci_mt_platform_reset_enter,
    .platform_reset_exit  = sdhci_mt_platform_reset_exit,
};


static int sdhci_mt_probe(struct platform_device *pdev)
{
    struct device *dev = &pdev->dev;
    struct sdhci_host *host;
    struct sdhci_mt *sc;
    struct resource *res;
    int ret, irq;
#if !defined(CONFIG_PANTHER_FPGA)
    int gpio_ids[] = { 25, 26, 27, 28, 29, 30, 31, -1, };
    unsigned long gpio_vals[] = { 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x0, };
#endif

    if (POWERCTL_STATIC_OFF==pmu_get_sdio_powercfg())
        return -ENODEV;

    irq = platform_get_irq(pdev, 0);
    if (irq < 0) {
        dev_err(dev, "no irq specified\n");
        return irq;
    }

    res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    if (!res) {
        dev_err(dev, "no memory specified\n");
        return -ENOENT;
    }

    host = sdhci_alloc_host(dev, sizeof(struct sdhci_mt));
    if (IS_ERR(host)) {
        dev_err(dev, "sdhci_alloc_host() failed\n");
        return PTR_ERR(host);
    }

    sc = sdhci_priv(host);

    sc->host = host;
    sc->pdev = pdev;

#ifdef CONFIG_PANTHER_FPGA
    printk(KERN_WARNING "Default Max SDCLK 25MHz\n");
    sc->max_clock = 25000000;
    //printk(KERN_WARNING "Default Max SDCLK 12.5MHz\n");
    //sc->max_clock = 12500000;
#else
    printk(KERN_WARNING "Default Max SDCLK 50MHz\n");
    sc->max_clock = 50000000;

    //*((volatile unsigned long *)0xbf004a30UL) = 0x45540000UL;  /* replaced by pmu_set_gpio_input_enable() */
    pmu_set_gpio_input_enable(gpio_ids, gpio_vals);
#endif

    platform_set_drvdata(pdev, host);

    sc->ioarea = request_mem_region(res->start, resource_size(res),
                    mmc_hostname(host->mmc));
    if (!sc->ioarea) {
        dev_err(dev, "failed to reserve register area\n");
        ret = -ENXIO;
        goto err_req_regs;
    }

    host->ioaddr = ioremap_nocache(res->start, resource_size(res));

    if (!host->ioaddr) {
        dev_err(dev, "failed to map registers\n");
        ret = -ENXIO;
        goto err_req_regs;
    }

    host->hw_name = "montage-hsmmc";
    host->ops = &sdhci_mt_ops;
    host->quirks = 0;
    host->irq = irq;

    /* for debug only */
    sdhci_host = host;

    /* Setup quirks for the controller */
#if 1
    /* Controller has unreliable card detection */
//  host->quirks |= SDHCI_QUIRK_BROKEN_CARD_DETECTION;
    /* Controller has unreliable card detection
     * don't polling card, using /proc/sdhc to trigger */
    host->quirks2 |= SDHCI_QUIRK2_TRIGGER_CARD_DETECTION;

    /* for debug, dynamically change max SD clock */
    host->quirks2 |= SDHCI_QUIRK2_MAX_CLOCK_VAL;

    /* force using 1 bit mode */
//  host->quirks |= SDHCI_QUIRK_FORCE_1_BIT_DATA;

    /* Controller is missing device caps. Use caps provided by host */
    host->quirks |= (SDHCI_QUIRK_MISSING_CAPS | SDHCI_QUIRK_CAP_CLOCK_BASE_BROKEN);
    host->caps = 0x21ecd081;
#ifdef CONFIG_PANTHER_FPGA
    host->caps1 = 0x100; /* no SDR50/DDR50/SDR104 Support */
#else
    //host->caps1 = 0x100 | SDHCI_SUPPORT_DDR50; /* no SDR50/SDR104 Support */
    host->caps1 = 0x100; /* no SDR50/DDR50/SDR104 Support */
    //host->caps1 = 0x105; /* mask SDR104 Support */
#endif    

    /* Controller uses Auto CMD12 command to stop the transfer */
    host->quirks |= SDHCI_QUIRK_MULTIBLOCK_READ_ACMD12;

    /* Controller provides an incorrect timeout value for transfers */
//  host->quirks |= SDHCI_QUIRK_BROKEN_TIMEOUT_VAL;

#if 1   // new ADMA
    host->quirks2 |= (SDHCI_QUIRK2_NO_ADMA_ALIGNMENT | SDHCI_QUIRK2_PRESET_VALUE_BROKEN);
#elif 0 // old ADMA
//  host->quirks |= SDHCI_QUIRK_32BIT_ADMA_SIZE;
    host->quirks2 |= SDHCI_QUIRK2_64BIT_ADMA_ADDR;
#elif 0 // SDMA
    host->quirks |= SDHCI_QUIRK_BROKEN_ADMA;
    host->quirks |= (SDHCI_QUIRK_32BIT_DMA_ADDR |
             SDHCI_QUIRK_32BIT_DMA_SIZE);
    host->quirks2 |= SDHCI_QUIRK2_64BIT_ADMA_ADDR;
#else   // PIO
    host->quirks |= SDHCI_QUIRK_BROKEN_ADMA;
    host->quirks |= SDHCI_QUIRK_BROKEN_DMA;
#endif

    /* Controller doesn't support UHS-I SDHC3.0 */
//  host->quirks2 |= SDHCI_QUIRK2_NO_UHSI;

    /* Our Controller need to reset SD clock after preset value enabled */
    host->quirks2 |= SDHCI_QUIRK2_RESET_CLOCK_AFTER_PRESET;


#if defined(CONFIG_TODO)
    /* Can the host do 8 bit transfers */
    host->mmc->caps |= MMC_CAP_8_BIT_DATA;
#endif

    /* CMD14/CMD19 bus width ok */
//  host->mmc->caps |= MMC_CAP_BUS_WIDTH_TEST;

#if defined(CONFIG_TODO)
    /* Patch Marvell SDIO WIFI card slowdown clock */
    host->mmc->caps3 |= MMC_CAP3_SD8686_SLOWDOWN;
#endif

#if defined(CONFIG_PANTHER_FPGA)
    /* invert sdclk in default */
    host->mmc->caps3 |= MMC_CAP3_INVERTED_SDCLK;
#endif

#ifdef CONFIG_PANTHER_SDHCI_VOLTAGE_CTRL
    /* Software need to control voltage 3.3V or 1.8V */
    host->mmc->caps3 |= MMC_CAP3_CONTROL_VOLTAGE;
#endif

#ifdef CONFIG_PANTHER_MMCSPI
    /* for debug and test usage */
    host->mmc->caps3 |= MMC_CAP3_FORCE_REMOVE_CARD;
#endif

#endif
    ret = sdhci_add_host(host);
    if (ret) {
        dev_err(dev, "sdhci_add_host() failed\n");
        goto err_add_host;
    }

    return 0;

 err_add_host:
    iounmap(host->ioaddr);
    release_mem_region(sc->ioarea->start, resource_size(sc->ioarea));

 err_req_regs:
    sdhci_free_host(host);

    return ret;
}

static int sdhci_mt_remove(struct platform_device *pdev)
{
    struct sdhci_host *host =  platform_get_drvdata(pdev);
    struct sdhci_mt *sc = sdhci_priv(host);

    sdhci_remove_host(host, 1);

    iounmap(host->ioaddr);
    release_mem_region(sc->ioarea->start, resource_size(sc->ioarea));

    sdhci_free_host(host);
    platform_set_drvdata(pdev, NULL);

    return 0;
}

#ifdef CONFIG_PM

static int sdhci_mt_suspend(struct platform_device *dev, pm_message_t state)
{
    return 0;
}

static int sdhci_mt_resume(struct platform_device *dev)
{
    return 0;
}

#else /* CONFIG_PM */
#define sdhci_mt_suspend NULL
#define sdhci_mt_resume NULL
#endif /* CONFIG_PM */

static struct platform_driver sdhci_mt_driver = {
    .driver = {
        .name   = "sdhci",
        .owner  = THIS_MODULE,
    },
    .probe      = sdhci_mt_probe,
    .remove     = sdhci_mt_remove,
    .suspend    = sdhci_mt_suspend,
    .resume     = sdhci_mt_resume,
};

#ifdef CONFIG_PROC_FS

#ifdef CONFIG_PANTHER_MMCSPI
static int spi_cd = 0;
int sdhci_mmc_spi_get_cd(void)
{
    return spi_cd;
}
#endif

#if defined(CONFIG_TODO)
static void sdhci_mt_clear_set_irqs(struct sdhci_host *host, u32 clear, u32 set)
{
	u32 ier;

	ier = sdhci_readl(host, SDHCI_INT_ENABLE);
	ier &= ~clear;
	ier |= set;
	sdhci_writel(host, ier, SDHCI_INT_ENABLE);
	sdhci_writel(host, ier, SDHCI_SIGNAL_ENABLE);
}

static void sdhci_mt_unmask_irqs(struct sdhci_host *host, u32 irqs)
{
	sdhci_mt_clear_set_irqs(host, 0, irqs);
}

static void sdhci_mt_mask_irqs(struct sdhci_host *host, u32 irqs)
{
	sdhci_mt_clear_set_irqs(host, irqs, 0);
}
#endif

enum sdhc_pin_type {
    SDHC_PIN_SPI = 0,
    SDHC_PIN_SDHC,
    SDHC_PIN_UNKNOWN,
    SDHC_PIN_COUNT,
}; 

static enum sdhc_pin_type sdhci_mt_get_pinmux(void)
{
#if !defined(CONFIG_TODO)
    return SDHC_PIN_SDHC;  
#else
    if (GPREG(PINMUX) & EN_SDIO_FNC) {
        return SDHC_PIN_SDHC;
    }
    else {
#ifdef CONFIG_PANTHER_MMCSPI
        return SDHC_PIN_SPI;
#else
        return SDHC_PIN_UNKNOWN;
#endif
    }
#endif
}

static int sdhci_mt_pinmux_switch(enum sdhc_pin_type type)
{
#if defined(CONFIG_TODO)
    /*
     * SDHC forcely disable mdio pin and eth0 function
     * So if DDR mode(SIP mode) and MDIO is used by external ether phy, 
     * ether will lose its function
     * (DDR mode + external ETH0 or ETH1 will be failed)
     * (DDR mode + internal ether will be OK)
     */
    /* disable PINMUX mdio function */
    GPREG(PINMUX) &= ~(EN_MDIO_FNC);
    /* disable PINMUX eth0 function */
    GPREG(PINMUX) &= ~(EN_ETH0_FNC);
    if (type == SDHC_PIN_SPI) {
#ifdef CONFIG_PANTHER_MMCSPI
        /* disable PINMUX sdio function */
        GPREG(PINMUX) &= ~(EN_SDIO_FNC);

        /* disable sdio module */
        GPREG(GDEBUG) &= ~(1<<21);
        printk("Switch To SPI Mode\n");
#else
        printk("Not Support SPI Mode\n");
#endif
    }
    else if (type == SDHC_PIN_SDHC) {
        /* enable sdio module */
        GPREG(GDEBUG) |= (1<<21);

        /* enable PINMUX sdio function */
        GPREG(PINMUX) |= (EN_SDIO_FNC);
        printk("Switch To SDHC Mode\n");
    }
    else {
        printk(KERN_ERR "undefined sdhc_pin_type(%d)\n", type);
    }
#endif
    return 0;
}

#if defined(CONFIG_PROC_FS)
#define MAX_ARGV 8
static int get_args(const char *string, char *argvs[])
{
    char *p;
    int n;

    argvs[0]=0;
    n = 0;
//  memset ((void*)argvs, 0, MAX_ARGV * sizeof (char *));
    p = (char *) string;
    while (*p == ' ')
        p++;
    while (*p)
    {
        argvs[n] = p;
        while (*p != ' ' && *p)
            p++;
        if (0==*p)
            goto out;
        *p++ = '\0';
        while (*p == ' ' && *p)
            p++;
out:
        n++;
        if (n == MAX_ARGV)
            break;
    }
    return n;
}

static int strtoul(char *str, void *v)
{   
    return(sscanf(str,"%d", (unsigned *)v)==1);
}

static int hextoul(char *str, void *v)
{   
    return(sscanf(str,"%x", (unsigned *)v)==1);
}

static int sdhc_cmd(int argc, char *argv[])
{
    struct mmc_host *mmc;
    struct sdhci_host *host;
    struct sdhci_mt *sc;
    unsigned int val;
    int reinstall = 0;

    if (!sdhci_host)
        goto err;

    mmc = sdhci_host->mmc;
    host = sdhci_host;
    sc = sdhci_priv(host);

    if (argc < 1)
    {
        goto err;
    }
    else if (argc == 1)
    {
        if (!strcmp(argv[0], "sdhc"))
        {
            sdhci_mt_pinmux_switch(SDHC_PIN_SDHC);
        }
        else if (!strcmp(argv[0], "spi"))
        {
            sdhci_mt_pinmux_switch(SDHC_PIN_SPI);
        }
#if defined(CONFIG_TODO)
        else if (!strcmp(argv[0], "card"))
        {
            mmc->caps3 &= ~MMC_CAP3_FORCE_REMOVE_CARD;
            if (sdhci_mt_get_pinmux() == SDHC_PIN_SDHC) {
                if (!(host->quirks & SDHCI_QUIRK_BROKEN_CARD_DETECTION)) {
                    u32 present = sdhci_readl(host, SDHCI_PRESENT_STATE) & SDHCI_CARD_PRESENT;
                    u8 ctrl = sdhci_readb(host, SDHCI_HOST_CONTROL);

                    if (present && !mmc->card)
                        tasklet_schedule(&host->card_tasklet);
                    else {
                        sdhci_mt_mask_irqs(host, present ? SDHCI_INT_CARD_INSERT : SDHCI_INT_CARD_REMOVE);
                        sdhci_mt_unmask_irqs(host, present ? SDHCI_INT_CARD_REMOVE : SDHCI_INT_CARD_INSERT);

                        ctrl |= (present ? 0x80 : 0xc0);
                        sdhci_writeb(host, ctrl, SDHCI_HOST_CONTROL);
                    }
                }
                else {
                    tasklet_schedule(&host->card_tasklet);
                }
            }
#ifdef CONFIG_PANTHER_MMCSPI
            else {
                spi_cd = 1;
            }
#endif
        }
        else if (!strcmp(argv[0], "nocard"))
        {
            mmc->caps3 |= MMC_CAP3_FORCE_REMOVE_CARD;
            if (sdhci_mt_get_pinmux() == SDHC_PIN_SDHC) {
                sdhci_mt_mask_irqs(host, SDHCI_INT_CARD_INSERT);
                if (mmc->card)
                    mmc_card_set_removed(mmc->card);
                tasklet_schedule(&host->card_tasklet);
            }
#ifdef CONFIG_PANTHER_MMCSPI
            else {
                spi_cd = 0;
                if (mmc->card)
                    mmc_card_set_removed(mmc->card);
            }
#endif
        }
#endif
        else if (!strcmp(argv[0], "pio"))
        {
            host->quirks &= ~(SDHCI_QUIRK_BROKEN_ADMA | SDHCI_QUIRK_BROKEN_DMA | SDHCI_QUIRK_32BIT_DMA_ADDR | SDHCI_QUIRK_32BIT_DMA_SIZE);
            host->quirks2 &= ~(SDHCI_QUIRK2_64BIT_ADMA_ADDR | SDHCI_QUIRK2_NO_ADMA_ALIGNMENT);
            host->quirks |= SDHCI_QUIRK_BROKEN_ADMA;
            host->quirks |= SDHCI_QUIRK_BROKEN_DMA;
            reinstall = 1;
        }
        else if (!strcmp(argv[0], "sdma"))
        {
            host->quirks &= ~(SDHCI_QUIRK_BROKEN_ADMA | SDHCI_QUIRK_BROKEN_DMA | SDHCI_QUIRK_32BIT_DMA_ADDR | SDHCI_QUIRK_32BIT_DMA_SIZE);
            host->quirks2 &= ~(SDHCI_QUIRK2_64BIT_ADMA_ADDR | SDHCI_QUIRK2_NO_ADMA_ALIGNMENT);
            host->quirks |= SDHCI_QUIRK_BROKEN_ADMA;
            host->quirks |= (SDHCI_QUIRK_32BIT_DMA_ADDR | SDHCI_QUIRK_32BIT_DMA_SIZE);
            host->quirks2 |= SDHCI_QUIRK2_64BIT_ADMA_ADDR;
            reinstall = 1;
        }
        else if (!strcmp(argv[0], "adma"))
        {
            host->quirks &= ~(SDHCI_QUIRK_BROKEN_ADMA | SDHCI_QUIRK_BROKEN_DMA | SDHCI_QUIRK_32BIT_DMA_ADDR | SDHCI_QUIRK_32BIT_DMA_SIZE);
            host->quirks2 &= ~(SDHCI_QUIRK2_64BIT_ADMA_ADDR | SDHCI_QUIRK2_NO_ADMA_ALIGNMENT);
            host->quirks2 |= SDHCI_QUIRK2_NO_ADMA_ALIGNMENT;
            reinstall = 1;
        }

        if (reinstall) {
            sdhci_remove_host(host, 0);
            host->flags = 0;
            if (sdhci_add_host(host)) {
                dev_err(host->mmc->parent, "sdhci_add_host() failed\n");
            }
        }
    }
    else if (argc == 2)
    {
        if (!strcmp(argv[0], "flags"))
        {
            if (!hextoul(argv[1], &val))
                goto err;
            host->flags = val;
        }
        else if (!strcmp(argv[0], "caps1"))
        {
            if (!hextoul(argv[1], &val))
                goto err;
            mmc->caps = val;
        }
        else if (!strcmp(argv[0], "caps2"))
        {
            if (!hextoul(argv[1], &val))
                goto err;
            mmc->caps2 = val;
        }
        else if (!strcmp(argv[0], "caps3"))
        {
            if (!hextoul(argv[1], &val))
                goto err;
            mmc->caps3 = val;
        }
        else if (!strcmp(argv[0], "uhs"))
        {
            if (!hextoul(argv[1], &val))
                goto err;
            if (val)
                mmc->caps |= (MMC_CAP_UHS_SDR12 | MMC_CAP_UHS_SDR25 |
                                MMC_CAP_UHS_SDR50 | MMC_CAP_UHS_DDR50);
            else
                mmc->caps &= ~(MMC_CAP_UHS_SDR12 | MMC_CAP_UHS_SDR25 |
                                MMC_CAP_UHS_SDR50 | MMC_CAP_UHS_SDR104 |
                                MMC_CAP_UHS_DDR50);
        }
        else if (!strcmp(argv[0], "bit"))
        {
            if (!hextoul(argv[1], &val))
                goto err;
            if ( val & 0x800 )
                mmc->caps |= MMC_CAP_8_BIT_DATA;
            else
                mmc->caps &= ~MMC_CAP_8_BIT_DATA;
            if ( val & 0x40 ) {
                mmc->caps |= MMC_CAP_4_BIT_DATA;
            }
            else if ( val & 0x1 )
                mmc->caps &= ~MMC_CAP_4_BIT_DATA;
            else {
                mmc->caps |= MMC_CAP_4_BIT_DATA;
            }
        }
        else if (!strcmp(argv[0], "max_clock"))
        {
            if (!strtoul(argv[1], &val))
                goto err;
            sc->max_clock = val;
        }
    }
    return 0;

err:
    printk(KERN_ERR "sdhc cmd is failed!!\n");
    return 0;
}

static ssize_t
sdhc_mt_proc_read(struct file *file, char __user *user_buf,
				     size_t count, loff_t *ppos)
{
    struct mmc_host *mmc;
    struct sdhci_host *host;
    struct sdhci_mt *sc;
    char *buff;
    int desc = 0;
    int ret;
    static const char *const pin_types[SDHC_PIN_COUNT] = {
        [SDHC_PIN_SPI]     = "SPI",
        [SDHC_PIN_SDHC]    = "SDHC",
        [SDHC_PIN_UNKNOWN] = "Unknown"
    };
    enum sdhc_pin_type type = sdhci_mt_get_pinmux();

    if (!sdhci_host)
        return 0;

    buff = kmalloc(1024, GFP_KERNEL);
    if (!buff)
        return -ENOMEM;

    mmc = sdhci_host->mmc;
    host = sdhci_host;
    sc = sdhci_priv(host);
    desc += sprintf(buff + desc, "pin   = %s\n", pin_types[type]);
    desc += sprintf(buff + desc, "flags = 0x%x\n", (u32)host->flags);
    desc += sprintf(buff + desc, "caps1 = 0x%x\n", (u32)mmc->caps);
    desc += sprintf(buff + desc, "caps2 = 0x%x\n", (u32)mmc->caps2);
    desc += sprintf(buff + desc, "caps3 = 0x%x\n", (u32)mmc->caps3);
    desc += sprintf(buff + desc, "uhs   = %d\n", mmc_host_uhs(mmc) ? 1 : 0);
    desc += sprintf(buff + desc, "bit   = %s%s%s\n", (mmc->caps & MMC_CAP_8_BIT_DATA) ? "8" : "" , 
                                        (mmc->caps & MMC_CAP_4_BIT_DATA) ? "4" : "" , 
                                        (mmc->caps & MMC_CAP_4_BIT_DATA) ? ""  : "1" );
    desc += sprintf(buff + desc, "max_clock = %dHz\n", (u32)sc->max_clock);

    if ( host->quirks2 & SDHCI_QUIRK2_NO_ADMA_ALIGNMENT )
        desc += sprintf(buff + desc, "adma = 1\n");
    else if( (host->quirks & (SDHCI_QUIRK_BROKEN_ADMA | SDHCI_QUIRK_BROKEN_DMA)) == (SDHCI_QUIRK_BROKEN_ADMA | SDHCI_QUIRK_BROKEN_DMA) )
        desc += sprintf(buff + desc, "pio = 1\n");
    else
        desc += sprintf(buff + desc, "sdma = 1\n");

    ret = simple_read_from_buffer(user_buf, count, ppos, buff, desc);
    kfree(buff);
    return ret;
}

static ssize_t
sdhc_mt_proc_write(struct file *file,
				      const char __user *user_buf,
				      size_t count, loff_t *ppos)
{
    int argc;
    char *argv[MAX_ARGV];
    char *buf;

    if(count>=1024)
        return -EFAULT;

    buf = kmalloc(1024, GFP_KERNEL);
	if (!buf)
        return -ENOMEM;

    if (copy_from_user(buf, user_buf, count))
    {
        kfree(buf);
        return -EFAULT;
    }

    buf[count-1] = '\0';

    argc = get_args( (const char *)buf, argv);
    sdhc_cmd(argc, argv);

    kfree(buf);

    return count;
}
#endif
#endif

#ifdef CONFIG_PROC_FS
static const struct file_operations sdhc_mt_fops = {
    .read       = sdhc_mt_proc_read,
    .write      = sdhc_mt_proc_write,
};
#endif

static int __init sdhci_mt_init(void)
{
    //char *init_info;

#ifdef CONFIG_PROC_FS
    if(NULL==proc_create("sdhc", S_IWUSR | S_IRUGO, NULL, &sdhc_mt_fops))
        return -ENOMEM;
#endif

    sdhci_mt_pinmux_switch(SDHC_PIN_SDHC);

    return platform_driver_register(&sdhci_mt_driver);
}

static void __exit sdhci_mt_exit(void)
{
#ifdef CONFIG_PROC_FS
    remove_proc_entry("sdhc", NULL);
#endif
    platform_driver_unregister(&sdhci_mt_driver);
}

module_init(sdhci_mt_init);
module_exit(sdhci_mt_exit);

MODULE_DESCRIPTION("Montage Cheetah SOC MMC/SD Card Interface driver");
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Nady Chen <nady.chen@montage-tech.com>");

