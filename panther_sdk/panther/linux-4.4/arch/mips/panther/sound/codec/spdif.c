/*
 *
 * ALSA SoC Audio Layer - Panther SPDIF codec driver
 *
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/slab.h>
#include <sound/soc.h>
#include <sound/pcm.h>
#include <sound/initval.h>
#include <linux/of.h>

#include "spdif.h"
#define DRV_NAME "spdif-dit"

static const struct snd_soc_dapm_widget dit_widgets[] = {
    SND_SOC_DAPM_OUTPUT("spdif-out"),
};

static const struct snd_soc_dapm_route dit_routes[] = {
    { "spdif-out", NULL, "Playback" },
};

static int dit_set_dai_sysclk(struct snd_soc_dai *cpu_dai,
                int clk_id, unsigned int freq, int dir)
{
    return 0;
}

static int dit_hw_params(struct snd_pcm_substream *substream,
                struct snd_pcm_hw_params *params,
                struct snd_soc_dai *socdai)
{
    return 0;
}

static int dit_set_dai_fmt(struct snd_soc_dai *dai, unsigned int fmt)
{
    return 0;
}

static struct snd_soc_dai_ops dit_dai_ops = {
    .hw_params  = dit_hw_params,
    .set_fmt    = dit_set_dai_fmt,
    .set_sysclk = dit_set_dai_sysclk,
};

static struct snd_soc_codec_driver soc_codec_spdif_dit = {
    .dapm_widgets = dit_widgets,
    .num_dapm_widgets = ARRAY_SIZE(dit_widgets),
    .dapm_routes = dit_routes,
    .num_dapm_routes = ARRAY_SIZE(dit_routes),
};

static struct snd_soc_dai_driver dit_stub_dai = {
    .name       = "dit-hifi",
    .playback   = {
        .stream_name    = "Playback",
        .channels_min   = 2,
        .channels_max   = 2,
        .rates      = STUB_RATES,
        .formats    = STUB_FORMATS,
    },
    .ops = &dit_dai_ops,
};

static int spdif_dit_probe(struct platform_device *pdev)
{
    int ret = 0;
    ret = snd_soc_register_codec(&pdev->dev, &soc_codec_spdif_dit,
            &dit_stub_dai, 1);
    return ret;
}

static int spdif_dit_remove(struct platform_device *pdev)
{
    snd_soc_unregister_codec(&pdev->dev);
    return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id spdif_dit_dt_ids[] = {
    { .compatible = "linux,spdif-dit", },
    { }
};
MODULE_DEVICE_TABLE(of, spdif_dit_dt_ids);
#endif

static struct platform_driver spdif_dit_driver = {
    .probe      = spdif_dit_probe,
    .remove     = spdif_dit_remove,
    .driver     = {
        .name   = DRV_NAME,
        .owner  = THIS_MODULE,
    },
};

module_platform_driver(spdif_dit_driver);

MODULE_AUTHOR("Edden Tsai");
MODULE_DESCRIPTION("SPDIF codec driver");
MODULE_ALIAS("platform:" DRV_NAME);
