/*
 *
 * ALSA SoC Audio Layer - Panther adc codec driver
 *
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/slab.h>
#include <sound/soc.h>
#include <sound/pcm.h>
#include <sound/initval.h>
#include <linux/of.h>

#include "adc.h"
#define DRV_NAME "panther-adc"

static const struct snd_soc_dapm_widget adc_widgets[] = {
    SND_SOC_DAPM_OUTPUT("adc-in"),
};

static const struct snd_soc_dapm_route adc_routes[] = {
    { "adc-in", NULL, "Capture" },
};

static int adc_set_dai_sysclk(struct snd_soc_dai *cpu_dai,
                int clk_id, unsigned int freq, int dir)
{
    return 0;
}

static int adc_hw_params(struct snd_pcm_substream *substream,
                struct snd_pcm_hw_params *params,
                struct snd_soc_dai *socdai)
{
    return 0;
}

static int adc_set_dai_fmt(struct snd_soc_dai *dai, unsigned int fmt)
{
    return 0;
}

static struct snd_soc_dai_ops adc_dai_ops = {
    .hw_params  = adc_hw_params,
    .set_fmt    = adc_set_dai_fmt,
    .set_sysclk = adc_set_dai_sysclk,
};

static struct snd_soc_codec_driver soc_codec_adc = {
    .dapm_widgets = adc_widgets,
    .num_dapm_widgets = ARRAY_SIZE(adc_widgets),
    .dapm_routes = adc_routes,
    .num_dapm_routes = ARRAY_SIZE(adc_routes),
};

static struct snd_soc_dai_driver adc_dai = {
    .name = "adc-hifi",
    .capture = {
        .stream_name  = "Capture",
        .channels_min = 2,
        .channels_max = 2,
        .rates        = ADC_RATES,
        .formats      = ADC_FORMATS,
    },
    .ops = &adc_dai_ops,
};

static int adc_probe(struct platform_device *pdev)
{
    int ret = 0;
    ret = snd_soc_register_codec(&pdev->dev, &soc_codec_adc,
            &adc_dai, 1);
    return ret;
}

static int adc_remove(struct platform_device *pdev)
{
    snd_soc_unregister_codec(&pdev->dev);
    return 0;
}

static struct platform_driver adc_driver = {
    .probe      = adc_probe,
    .remove     = adc_remove,
    .driver     = {
        .name   = DRV_NAME,
        .owner  = THIS_MODULE,
    },
};

module_platform_driver(adc_driver);

MODULE_AUTHOR("Edden Tsai");
MODULE_DESCRIPTION("Panther ADC codec driver");
MODULE_ALIAS("platform:" DRV_NAME);
