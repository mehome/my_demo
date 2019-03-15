/*
 *
 * ALSA SoC Audio Layer - Panther dac codec driver
 *
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/slab.h>
#include <sound/soc.h>
#include <sound/pcm.h>
#include <sound/initval.h>
#include <linux/of.h>

#include "dac.h"
#define DRV_NAME "panther-dac"

static const struct snd_soc_dapm_widget dac_widgets[] = {
    SND_SOC_DAPM_OUTPUT("dac-out"),
};

static const struct snd_soc_dapm_route dac_routes[] = {
    { "dac-out", NULL, "Playback" },
};

static int dac_set_dai_sysclk(struct snd_soc_dai *cpu_dai,
                int clk_id, unsigned int freq, int dir)
{
    return 0;
}

static int dac_hw_params(struct snd_pcm_substream *substream,
                struct snd_pcm_hw_params *params,
                struct snd_soc_dai *socdai)
{
    return 0;
}

static int dac_set_dai_fmt(struct snd_soc_dai *dai, unsigned int fmt)
{
    return 0;
}

static struct snd_soc_dai_ops dac_dai_ops = {
    .hw_params  = dac_hw_params,
    .set_fmt    = dac_set_dai_fmt,
    .set_sysclk = dac_set_dai_sysclk,
};

static struct snd_soc_codec_driver soc_codec_dac = {
    .dapm_widgets = dac_widgets,
    .num_dapm_widgets = ARRAY_SIZE(dac_widgets),
    .dapm_routes = dac_routes,
    .num_dapm_routes = ARRAY_SIZE(dac_routes),
};

static struct snd_soc_dai_driver dac_dai = {
    .name       = "dac-hifi",
    .playback   = {
        .stream_name    = "Playback",
        .channels_min   = 2,
        .channels_max   = 2,
        .rates      = DAC_RATES,
        .formats    = DAC_FORMATS,
    },
    .ops = &dac_dai_ops,
};

static int dac_probe(struct platform_device *pdev)
{
    int ret = 0;
    ret = snd_soc_register_codec(&pdev->dev, &soc_codec_dac,
            &dac_dai, 1);
    return ret;
}

static int dac_remove(struct platform_device *pdev)
{
    snd_soc_unregister_codec(&pdev->dev);
    return 0;
}

static struct platform_driver dac_driver = {
    .probe      = dac_probe,
    .remove     = dac_remove,
    .driver     = {
        .name   = DRV_NAME,
        .owner  = THIS_MODULE,
    },
};

module_platform_driver(dac_driver);

MODULE_AUTHOR("Edden Tsai");
MODULE_DESCRIPTION("Panther DAC codec driver");
MODULE_ALIAS("platform:" DRV_NAME);
