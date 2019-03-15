/*
 *
 * ALSA SoC Audio Layer - Panther dac Controller driver
 *
 */

#include <linux/clk.h>
#include <linux/io.h>
#include <linux/module.h>

#include <sound/soc.h>
#include <sound/pcm_params.h>

#include <asm/mach-panther/panther.h>
#include <asm/mach-panther/pmu.h>

#include "codec/dac.h"
#include "purin-audio.h"
#include "purin-dma.h"

//#define PURIN_DEBUG
#ifdef PURIN_DEBUG
#undef pr_debug
#define purin_fmt(fmt) "[%s: %d]: " fmt, __func__, __LINE__
#define pr_debug(fmt, ...) \
    printk(KERN_EMERG purin_fmt(fmt), ##__VA_ARGS__)

#define dp(fmt, ...) \
    printk(KERN_EMERG pr_fmt(fmt), ##__VA_ARGS__)
#endif

#if !defined(CONFIG_PANTHER_SND_PCM0_PCMWB_44100_TX_ENABLE)
static int sample_rate_table[SAMPLE_RATE_SUPPORT] = {
    8000,  16000, 22050, 24000,  32000,
    44100, 48000, 88200, 96000, 128000, 176400, 192000
};

static int dac_rate_table[SAMPLE_RATE_SUPPORT] = {
    -1, -1, -1, -1, -1,
     2,  0, -1,  1, -1, -1, -1
};
#endif

#define ENABLE_INTERFACE()                                  \
    do {                                                    \
        if(!(AIREG_READ32(DAC_CONF) & BIT0))                \
            AIREG_UPDATE32(DAC_CONF, BIT0, BIT0);           \
    } while (0);

#define DISABLE_INTERFACE()                          \
    do { AIREG_UPDATE32(DAC_CONF, 0, BIT0); } while (0);


static int dac_prepare(struct snd_pcm_substream *substream,
                       struct snd_soc_dai *dai)
{
#if !defined(CONFIG_PANTHER_SND_PCM0_PCMWB_44100_TX_ENABLE)
    struct purin_master *master = snd_soc_dai_get_drvdata(dai);
    unsigned long irqflags;
    spin_lock_irqsave(&(master->reg_lock), irqflags);
    purin_desc_specific(substream);

    /* unmask Interrupt */
    AIREG_UPDATE32(INTR_MASK, 0, INTR_STATUS_TX);
    spin_unlock_irqrestore(&(master->reg_lock), irqflags);
#endif
    return 0;
}

static int dac_trigger(struct snd_pcm_substream *substream, int cmd,
                struct snd_soc_dai *dai)
{
    int ret = 0;
    struct purin_master *master = snd_soc_dai_get_drvdata(dai);
    unsigned long irqflags;

#if !defined(CONFIG_PANTHER_SND_PCM0_PCMWB_44100_TX_ENABLE)
    switch (cmd) {
    case SNDRV_PCM_TRIGGER_START:
    case SNDRV_PCM_TRIGGER_RESUME:
    case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
        spin_lock_irqsave(&(master->reg_lock), irqflags);
        /* dac is playback only */
        ENABLE_TXDMA();
        TXDMA_READY();
        if(chip_revision < 2) {
            ENABLE_DUAL_CH();
        }
        ENABLE_INTERFACE();
        spin_unlock_irqrestore(&(master->reg_lock), irqflags);
        break;
    case SNDRV_PCM_TRIGGER_STOP:
    case SNDRV_PCM_TRIGGER_SUSPEND:
    case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
        spin_lock_irqsave(&(master->reg_lock), irqflags);
        DISABLE_TXDMA();
        spin_unlock_irqrestore(&(master->reg_lock), irqflags);
        break;
    default:
        ret = -EINVAL;
    }

#else
    switch (cmd) {
    case SNDRV_PCM_TRIGGER_START:
    case SNDRV_PCM_TRIGGER_RESUME:
    case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
        spin_lock_irqsave(&(master->reg_lock), irqflags);
        TXDMA_READY();
        spin_unlock_irqrestore(&(master->reg_lock), irqflags);
        break;
    case SNDRV_PCM_TRIGGER_STOP:
    case SNDRV_PCM_TRIGGER_SUSPEND:
    case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
        break;
    default:
        ret = -EINVAL;
    }
#endif
    return ret;
}

#if !defined(CONFIG_PANTHER_SND_PCM0_PCMWB_44100_TX_ENABLE)
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
#endif

static int dac_hw_params(struct snd_pcm_substream *substream,
                struct snd_pcm_hw_params *params,
                struct snd_soc_dai *cpu_dai)
{
#if !defined(CONFIG_PANTHER_SND_PCM0_PCMWB_44100_TX_ENABLE)
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
    if (idx < 0 || dac_rate_table[idx] < 0) {
        printk(KERN_ERR "Not support sample rate\n");
        ret = -EINVAL;
        goto out;
    }

    pr_debug("rate:%d, idx:%d\n", params_rate(params), idx);

#ifndef CONFIG_PANTHER_FPGA
    pmu_update_dac_clk(dac_rate_table[idx]);
#endif

out:
    return ret;
#else
    return 0;
#endif
}

static int dac_startup(struct snd_pcm_substream *substream,
                struct snd_soc_dai *dai)
{
#if !defined(CONFIG_PANTHER_SND_PCM0_PCMWB_44100_TX_ENABLE)
    pr_debug("in function %s\n", __func__);
    if(chip_revision >= 2) {
        ENABLE_DUAL_CH();
    }
#endif
    return 0;
}

static void dac_shutdown(struct snd_pcm_substream *substream,
                struct snd_soc_dai *dai)
{
#if !defined(CONFIG_PANTHER_SND_PCM0_PCMWB_44100_TX_ENABLE)
    struct purin_master *master = snd_soc_dai_get_drvdata(dai);
    unsigned long irqflags;

    spin_lock_irqsave(&(master->reg_lock), irqflags);

    pr_debug("shutdown %d\n", substream->stream);
    /* mask interrupt */
    AIREG_UPDATE32(INTR_MASK, INTR_STATUS_TX, INTR_STATUS_TX);

    if(chip_revision < 2) {
        pr_debug("PCM_IS_CONFIGURED %d", PCM_IS_CONFIGURED);
        if (!PCM_IS_CONFIGURED) {
            DISABLE_DUAL_CH();
            PURIN_RESET_HW_PCM();
        }
    }
    DISABLE_INTERFACE();

    spin_unlock_irqrestore(&(master->reg_lock), irqflags);
#endif
}

#ifdef CONFIG_PM
static int dac_suspend(struct snd_soc_dai *cpu_dai)
{
    return 0;
}

static int dac_resume(struct snd_soc_dai *cpu_dai)
{
    return 0;
}
#else
#define dac_suspend NULL
#define dac_resume NULL
#endif

static const struct snd_soc_dai_ops dac_dai_ops = {
    .prepare    = dac_prepare,
    .trigger    = dac_trigger,
    .hw_params  = dac_hw_params,
    .startup    = dac_startup,
    .shutdown   = dac_shutdown,
};

#ifdef CONFIG_CPU_LITTLE_ENDIAN
#define DAC_DAI_SUPPORT_FORMATS (SNDRV_PCM_FMTBIT_S16_LE)
#else
#define DAC_DAI_SUPPORT_FORMATS (SNDRV_PCM_FMTBIT_S16_BE)
#endif

static struct snd_soc_dai_driver purin_dac_dai = {
    .playback = {
        .stream_name = "DAC Playback",
        .channels_min = 2,
        .channels_max = 2,
        .rates = DAC_RATES,
        .formats = DAC_DAI_SUPPORT_FORMATS, /* must be subset in purin_pcm_hardware settings */
    },
    .ops = &dac_dai_ops,
    .suspend = dac_suspend,
    .resume = dac_resume,
};

static const struct snd_soc_component_driver purin_dac_component = {
    .name       = "dac-dai",
};

static int dac_probe(struct platform_device *pdev)
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

    ret = devm_snd_soc_register_component(dev, &purin_dac_component, &purin_dac_dai, 1);

    if (unlikely(ret != 0)) {
        dev_err(dev, "failed to register dai\n");
        goto err_irq;
    }
    master->res = res;
    master->pdev = pdev;
    dev_set_drvdata(dev, (void *)master);
#ifndef CONFIG_PANTHER_FPGA
    pmu_initial_dac();
    if (chip_revision < 2)
    {
        pmu_internal_audio_enable(1);
    }
#endif
    return ret;

err_irq:
    if (master) {
        kfree(master);
        master = NULL;
    }
    return ret;
}

static int dac_remove(struct platform_device *pdev)
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

static struct platform_driver purin_dac_driver = {
    .probe  = dac_probe,
    .remove = dac_remove,
    .driver = {
        .name   = "purin-dac",
    },
};

static int __init purin_dac_dai_init(void)
{
    return platform_driver_register(&purin_dac_driver);
}

static void __exit purin_dac_dai_exit(void)
{
    platform_driver_unregister(&purin_dac_driver);
}

module_init(purin_dac_dai_init);
module_exit(purin_dac_dai_exit);

MODULE_AUTHOR("Edden Tsai");
MODULE_DESCRIPTION("Panther DAC Controller Driver");
MODULE_ALIAS("platform:panther-dac");
