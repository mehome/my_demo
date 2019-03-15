/*
 *
 * ALSA SoC Audio Layer - Panther adc Controller driver
 *
 */

#include <linux/clk.h>
#include <linux/io.h>
#include <linux/module.h>

#include <sound/soc.h>
#include <sound/pcm_params.h>

#include <asm/mach-panther/panther.h>
#include <asm/mach-panther/pmu.h>

#include "codec/adc.h"
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

#define ADC_IS_ENABLED (AIREG_READ32(ADC_CONF) & BIT0)

#define ENABLE_INTERFACE()                          \
    do { AIREG_UPDATE32(ADC_CONF, BIT0, BIT0); } while (0);

#define DISABLE_INTERFACE()                          \
    do { AIREG_UPDATE32(ADC_CONF, 0, BIT0); } while (0);

static int adc_prepare(struct snd_pcm_substream *substream,
                       struct snd_soc_dai *dai)
{
    struct purin_master *master = snd_soc_dai_get_drvdata(dai);
    struct purin_runtime_data *purin_rtd = substream->runtime->private_data;
    unsigned long irqflags;
    spin_lock_irqsave(&(master->reg_lock), irqflags);

    purin_rtd->is_adc = 1;
    purin_desc_specific(substream);

    /* unmask Interrupt */
    AIREG_UPDATE32(INTR_MASK, 0, INTR_STATUS_ADC);
    spin_unlock_irqrestore(&(master->reg_lock), irqflags);
    return 0;
}

static int adc_trigger(struct snd_pcm_substream *substream, int cmd,
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
        /* adc is record only */
        RXDMA_READY();
        if (!ADC_IS_ENABLED)
        {
            if(chip_revision < 2) {
                ENABLE_DUAL_CH();
            }
            ENABLE_INTERFACE();
#if !defined(CONFIG_PANTHER_FPGA)
            if(chip_revision < 2) {
                pmu_internal_adc_clock_en(1);
            }
#endif
        }
        spin_unlock_irqrestore(&(master->reg_lock), irqflags);
        break;
    case SNDRV_PCM_TRIGGER_STOP:
    case SNDRV_PCM_TRIGGER_SUSPEND:
    case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
#if !defined(CONFIG_PANTHER_FPGA)
        if(chip_revision < 2) {
            pmu_internal_adc_clock_en(0);
        }
#endif
        break;
    default:
        ret = -EINVAL;
    }

    return ret;
}

static int adc_hw_params(struct snd_pcm_substream *substream,
                struct snd_pcm_hw_params *params,
                struct snd_soc_dai *cpu_dai)
{
    struct purin_master *master;
    struct purin_device *device;
    int ret = 0;
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
    if (params_rate(params) != 64000) { // only support 64k in ADC
        ret = -EINVAL;
        goto out;
    }
#ifndef CONFIG_PANTHER_FPGA
    pmu_update_adc_clk(AADC_FS_16k);
#endif

out:
    return ret;
}

static int adc_startup(struct snd_pcm_substream *substream,
                struct snd_soc_dai *dai)
{

    pr_debug("in function %s\n", __func__);
    if(chip_revision >= 2) {
        ENABLE_DUAL_CH();
        if(POWERCTL_DYNAMIC == pmu_get_audio_codec_powercfg()) {
            pmu_internal_adc_enable(1);
        }
    }
    return 0;
}

static void adc_shutdown(struct snd_pcm_substream *substream,
                struct snd_soc_dai *dai)
{
    struct purin_master *master = snd_soc_dai_get_drvdata(dai);
    unsigned long irqflags;

    spin_lock_irqsave(&(master->reg_lock), irqflags);

    pr_debug("shutdown %d\n", substream->stream);
    /* mask interrupt */
    AIREG_UPDATE32(INTR_MASK, INTR_STATUS_ADC, INTR_STATUS_ADC);
    if (chip_revision < 2) {
        pr_debug("PCM_IS_CONFIGURED %d", PCM_IS_CONFIGURED);
        if (!PCM_IS_CONFIGURED) {
            DISABLE_DUAL_CH();
            PURIN_RESET_HW_PCM();
        }
    } else {
        if(POWERCTL_DYNAMIC == pmu_get_audio_codec_powercfg()) {
            pmu_internal_adc_enable(0);
        }
    }

    DISABLE_INTERFACE();
    spin_unlock_irqrestore(&(master->reg_lock), irqflags);
}

#ifdef CONFIG_PM
static int adc_suspend(struct snd_soc_dai *cpu_dai)
{
    return 0;
}

static int adc_resume(struct snd_soc_dai *cpu_dai)
{
    return 0;
}
#else
#define adc_suspend NULL
#define adc_resume NULL
#endif

static const struct snd_soc_dai_ops adc_dai_ops = {
    .prepare    = adc_prepare,
    .trigger    = adc_trigger,
    .hw_params  = adc_hw_params,
    .startup    = adc_startup,
    .shutdown   = adc_shutdown,
};

#ifdef CONFIG_CPU_LITTLE_ENDIAN
#define ADC_DAI_SUPPORT_FORMATS (SNDRV_PCM_FMTBIT_S16_LE)
#else
#define ADC_DAI_SUPPORT_FORMATS (SNDRV_PCM_FMTBIT_S16_BE)
#endif

static struct snd_soc_dai_driver purin_adc_dai = {
    .capture = {
        .stream_name = "ADC Capture",
        .channels_min = 2,
        .channels_max = 2,
        .rates = ADC_RATES,
        .formats = ADC_DAI_SUPPORT_FORMATS, /* must be subset in purin_pcm_hardware settings */
    },
    .ops = &adc_dai_ops,
    .suspend = adc_suspend,
    .resume = adc_resume,
};

static const struct snd_soc_component_driver purin_adc_component = {
    .name       = "adc-dai",
};

static int adc_probe(struct platform_device *pdev)
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

    ret = devm_snd_soc_register_component(dev, &purin_adc_component, &purin_adc_dai, 1);

    if (unlikely(ret != 0)) {
        dev_err(dev, "failed to register dai\n");
        goto err_irq;
    }
    master->res = res;
    master->pdev = pdev;
    dev_set_drvdata(dev, (void *)master);
#ifndef CONFIG_PANTHER_FPGA
    pmu_internal_adc_clock_en(0);
    if (chip_revision < 2)
    {
        pmu_internal_audio_enable(1);
    }
    else
    {
        if(POWERCTL_DYNAMIC == pmu_get_audio_codec_powercfg()) {
            pmu_internal_adc_enable(0);
        }
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

static int adc_remove(struct platform_device *pdev)
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

static struct platform_driver purin_adc_driver = {
    .probe  = adc_probe,
    .remove = adc_remove,
    .driver = {
        .name   = "purin-adc",
    },
};

static int __init purin_adc_dai_init(void)
{
    return platform_driver_register(&purin_adc_driver);
}

static void __exit purin_adc_dai_exit(void)
{
    platform_driver_unregister(&purin_adc_driver);
}

module_init(purin_adc_dai_init);
module_exit(purin_adc_dai_exit);

MODULE_AUTHOR("Edden Tsai");
MODULE_DESCRIPTION("Panther ADC Controller Driver");
MODULE_ALIAS("platform:panther-adc");
