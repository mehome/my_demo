/*
 *
 * ALSA SoC Audio Layer - Panther S/PDIF Controller driver
 *
 */

#include <linux/clk.h>
#include <linux/io.h>
#include <linux/module.h>

#include <sound/soc.h>
#include <sound/pcm_params.h>

#include <asm/mach-panther/panther.h>
#include <asm/mach-panther/pmu.h>

#include "codec/spdif.h"
#include "purin-audio.h"
#include "purin-dma.h"

#define PURIN_DEBUG
#ifdef PURIN_DEBUG
#undef pr_debug
#define pr_debug(fmt, ...) \
    printk(KERN_EMERG pr_fmt(fmt), ##__VA_ARGS__)

#define dp(fmt, ...) \
    printk(KERN_EMERG pr_fmt(fmt), ##__VA_ARGS__)
#endif

static int sample_rate_table[SAMPLE_RATE_SUPPORT] = {
    8000,  16000, 22050, 24000,  32000,
    44100, 48000, 88200, 96000, 128000, 176400, 192000
};

static int spdif_rate_table[SAMPLE_RATE_SUPPORT] = {
    -1, 1, 1, 1,  1,
     1, 1, 1, 1,  1, 1, 1
};

#ifndef CONFIG_PANTHER_FPGA
static struct purin_clk_setting spdif_rate_setting_table[SAMPLE_RATE_SUPPORT] = {
    {0, 0, 2, 24}, {0, 1, 2, 24}, {1, 1, 3,  4}, {0, 1, 2, 16}, {0, 1, 2, 12},
    {1, 1, 2,  4}, {0, 1, 2,  8}, {1, 1, 1,  4}, {0, 1, 2,  4}, {0, 1, 2,  3},
    {1, 1, 0,  4}, {0, 1, 2,  2}
};
#endif

#define ENABLE_INTERFACE()                                      \
    do {                                                        \
        if(!(AIREG_READ32(SPDIF_CONF) & BIT0))                  \
            AIREG_UPDATE32(SPDIF_CONF, BIT0, BIT0);             \
    } while (0);

#define DISABLE_INTERFACE()                                     \
    do { AIREG_UPDATE32(SPDIF_CONF, 0, BIT0); } while (0);

static void purin_enable_spdif(void)
{
    int tmp = 0;
    /* SPDIF Channel status */
    CFGREG(tmp, (2 << 18), (BIT18|BIT19));
    CFGREG(tmp, (4 << 10), (BIT13|BIT12|BIT11|BIT10));
    AIREG_WRITE32(SPDIF_CH_STA, tmp);

    /* SPDIF Channel status 1 */
    tmp = 0;
    AIREG_WRITE32(SPDIF_CH_STA_1, tmp);

    // channel 0, 1 conf
    if(chip_revision < 2) {
        tmp = 0;
        tmp |= (BIT12| BIT13);

        AIREG_WRITE32(CH0_CONF, tmp);
        AIREG_WRITE32(CH1_CONF, tmp);
    }
    pr_debug("purin_enable_spdif 0x<%x>, 0x<%x>, 0x<%x>, 0x<%x>, 0x<%x>\n",
        AIREG_READ32(SPDIF_CH_STA), AIREG_READ32(SPDIF_CH_STA_1), AIREG_READ32(SPDIF_CONF), AIREG_READ32(CH0_CONF), AIREG_READ32(CH1_CONF));
}

static int spdif_set_sysclk(struct snd_soc_dai *dai,
                int clk_id, unsigned int freq, int dir)
{
    pr_debug("in function %s %d\n", __func__, freq);
    return 0;
}

static int spdif_prepare(struct snd_pcm_substream *substream,
                struct snd_soc_dai *dai)
{
    struct purin_master *master = snd_soc_dai_get_drvdata(dai);
    unsigned long irqflags;
    pr_debug("in function %s\n", __func__);
    spin_lock_irqsave(&(master->reg_lock), irqflags);
    purin_desc_specific(substream);
    purin_enable_spdif();

    /* unmask Interrupt */
    AIREG_UPDATE32(INTR_MASK, 0, INTR_STATUS_TX);
    spin_unlock_irqrestore(&(master->reg_lock), irqflags);
    return 0;
}

static int spdif_trigger(struct snd_pcm_substream *substream, int cmd,
                struct snd_soc_dai *dai)
{
    int ret = 0;
    struct purin_master *master = snd_soc_dai_get_drvdata(dai);
    unsigned long irqflags;

    switch (cmd) {
    case SNDRV_PCM_TRIGGER_START:
    case SNDRV_PCM_TRIGGER_RESUME:
    case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
        spin_lock_irqsave(&(master->reg_lock), irqflags);
        /* spdif is playback only */
        ENABLE_TXDMA();
        TXDMA_READY();
        if(chip_revision < 2) {
            if (0 == master->ch_enabled) {
                /* reset spdif frame counter, must call it here to make sure
                   passing left-right channel test */
                PURIN_RESET_HW_SPDIF();
                ENABLE_DUAL_CH();
                master->ch_enabled = 1;
            }
        }
        ENABLE_INTERFACE();
        spin_unlock_irqrestore(&(master->reg_lock), irqflags);
        break;
    case SNDRV_PCM_TRIGGER_STOP:
    case SNDRV_PCM_TRIGGER_SUSPEND:
    case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
        spin_lock_irqsave(&(master->reg_lock), irqflags);
        DISABLE_TXDMA();
        DISABLE_INTERFACE();
        if(chip_revision < 2) {
            PURIN_RESET_HW_SPDIF();
            master->ch_enabled = 0;
        }
        AIREG_UPDATE32(INTR_MASK, INTR_STATUS_TX, INTR_STATUS_TX);
        spin_unlock_irqrestore(&(master->reg_lock), irqflags);
        break;
    default:
        ret = -EINVAL;
    }

    return ret;
}

static int purin_rate2idx(int rat)
{
    int i;
    for (i=0; i<SAMPLE_RATE_SUPPORT; i++) {
        if (rat == sample_rate_table[i]) {
            return i;
        }
    }
    return -1;
}

static int spdif_hw_params(struct snd_pcm_substream *substream,
                struct snd_pcm_hw_params *params,
                struct snd_soc_dai *cpu_dai)
{
    struct purin_master *master;
    struct purin_device *device;
    int idx = 0, ret = 0;
    pr_debug("in function %s\n", __func__);

    master = snd_soc_dai_get_drvdata(cpu_dai);
    if (!master) {
        pr_debug("no master!!\n");
        ret = -ENODEV;
        goto out;
    }

    device = &master->device;
    device->bit_depth = snd_pcm_format_width(params_format(params));
    pr_debug("bit_depth=%d\n", device->bit_depth);
    idx = purin_rate2idx(params_rate(params));
    if (idx < 0 || spdif_rate_table[idx] < 0) {
        printk(KERN_ERR "Not support sample rate\n");
        ret = -EINVAL;
        goto out;
    }

    pr_debug("rate:%d, idx:%d\n", params_rate(params), idx);

#ifndef CONFIG_PANTHER_FPGA
    pmu_update_spdif_clk(spdif_rate_setting_table[idx].domain_sel,
                         spdif_rate_setting_table[idx].bypass_en,
                         spdif_rate_setting_table[idx].clk_div_sel,
                         spdif_rate_setting_table[idx].ndiv_sel);
#endif

out:
    return ret;
}

static int spdif_startup(struct snd_pcm_substream *substream,
                struct snd_soc_dai *dai)
{
    struct purin_master * master; 
    if(chip_revision < 2) {
        master = snd_soc_dai_get_drvdata(dai);
        master->ch_enabled = 0;
    }

    pr_debug("in function %s\n", __func__);
    return 0;
}

static void spdif_shutdown(struct snd_pcm_substream *substream,
                struct snd_soc_dai *dai)
{
    pr_debug("in function %s\n", __func__);
}

#ifdef CONFIG_PM
static int spdif_suspend(struct snd_soc_dai *cpu_dai)
{
    return 0;
}

static int spdif_resume(struct snd_soc_dai *cpu_dai)
{
    return 0;
}
#else
#define spdif_suspend NULL
#define spdif_resume NULL
#endif

static const struct snd_soc_dai_ops spdif_dai_ops = {
    .set_sysclk = spdif_set_sysclk,
    .prepare    = spdif_prepare,
    .trigger    = spdif_trigger,
    .hw_params  = spdif_hw_params,
    .startup    = spdif_startup,
    .shutdown   = spdif_shutdown,
};

#if 0
/*
 * let spdif support 16bit, 20bit & 24bit
 * but 16bit must filled data into 32bit as 0x00ffff00
 * we must alloc another memory to deal with it.
 */
#ifdef CONFIG_CPU_LITTLE_ENDIAN
    #define SPDIF_DAI_SUPPORT_FORMATS (SNDRV_PCM_FMTBIT_S16_LE | \
                                   SNDRV_PCM_FMTBIT_S20_3LE | SNDRV_PCM_FMTBIT_S24_LE)
#else
    #define SPDIF_DAI_SUPPORT_FORMATS (SNDRV_PCM_FMTBIT_S16_BE | \
                                   SNDRV_PCM_FMTBIT_S20_3BE | SNDRV_PCM_FMTBIT_S24_BE)
#endif

#else
/*
 * force alsa resample to 24bit-depth data
 */
#ifdef CONFIG_CPU_LITTLE_ENDIAN
    #define SPDIF_DAI_SUPPORT_FORMATS (SNDRV_PCM_FMTBIT_S24_LE)
#else
    #define SPDIF_DAI_SUPPORT_FORMATS (SNDRV_PCM_FMTBIT_S24_BE)
#endif

#endif

static struct snd_soc_dai_driver purin_spdif_dai = {
    .playback = {
        .stream_name = "S/PDIF Playback",
        .channels_min = 2,
        .channels_max = 2,
        .rates = STUB_RATES,
        .formats = SPDIF_DAI_SUPPORT_FORMATS, /* must be subset in purin_pcm_hardware settings */
    },
    .ops = &spdif_dai_ops,
    .suspend = spdif_suspend,
    .resume = spdif_resume,
};

static const struct snd_soc_component_driver purin_spdif_component = {
    .name       = "spdif-dai",
};

static int spdif_probe(struct platform_device *pdev)
{
    struct device *dev = &pdev->dev;
    int ret;
    struct resource *res;

    struct purin_master *master;
    master = kzalloc(sizeof(struct purin_master), GFP_KERNEL);

    if (unlikely(!master)) {
        dev_err(dev, "Could not allocate master\n");
        ret = -ENOMEM;
        goto err_irq;
    }

    spin_lock_init(&(master->reg_lock));

    res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    if (unlikely(res < 0)) {
        dev_err(dev, "no memory specified\n");
        ret = -ENOENT;
        goto err_irq;
    }

    ret = devm_snd_soc_register_component(dev, &purin_spdif_component, &purin_spdif_dai, 1);

    if (unlikely(ret != 0)) {
        dev_err(dev, "failed to register dai\n");
        goto err_irq;
    }
    master->res = res;
    master->pdev = pdev;
    dev_set_drvdata(dev, (void *)master);
    return ret;

err_irq:
    if (master) {
        kfree(master);
        master = NULL;
    }
    return ret;
}

static int spdif_remove(struct platform_device *pdev)
{
    struct device *dev;
    struct purin_master* master;

    dev=&pdev->dev;
    master = dev_get_drvdata(dev);

    if (likely(master)) {
        kfree(master);
        master = NULL;
    }
    snd_soc_unregister_component(&pdev->dev);
    return 0;
}

static struct platform_driver purin_spdif_driver = {
    .probe  = spdif_probe,
    .remove = spdif_remove,
    .driver = {
        .name   = "purin-spdif",
    },
};

//module_platform_driver(purin_spdif_driver);
static int __init purin_spdif_dai_init(void)
{
    return platform_driver_register(&purin_spdif_driver);
}

static void __exit purin_spdif_dai_exit(void)
{
    platform_driver_unregister(&purin_spdif_driver);
}

module_init(purin_spdif_dai_init);
module_exit(purin_spdif_dai_exit);

MODULE_AUTHOR("Edden Tsai");
MODULE_DESCRIPTION("Panther S/PDIF Controller Driver");
MODULE_ALIAS("platform:panther-spdif");
