/*
 *
 * ALSA SoC Audio Layer - Panther dac soc card driver
 *
 */

#include <linux/clk.h>
#include <linux/module.h>

#include <sound/soc.h>

static int purin_dac_hw_params(struct snd_pcm_substream *substream,
        struct snd_pcm_hw_params *params)
{
    return 0;
}

static struct snd_soc_ops purin_dac_ops = {
    .hw_params = purin_dac_hw_params,
};

static struct snd_soc_dai_link purin_dac_dai = {
    .name = "DAC",
    .stream_name = "DAC Playback",
    .platform_name = "purin-dac-dma", // declared in purin-dac-dai.c
    .cpu_dai_name = "purin-dac.1",    // declared in platform.c
    .codec_dai_name = "dac-hifi",     // declared in codec/dac.c
    .codec_name = "panther-dac",      // declared in codec/dac.c & purin_dac_init (platform_device)
    .ops = &purin_dac_ops,
};

static struct snd_soc_card purin_dac_card = {
    .name = "DAC",
    .owner = THIS_MODULE,
    .dai_link = &purin_dac_dai,
    .num_links = 1,
};

static struct platform_device *purin_snd_dac_device;
static struct platform_device *purin_snd_soc_dac_device;

static int __init purin_dac_init(void)
{
    int ret;

    purin_snd_dac_device = platform_device_alloc("panther-dac", -1);
    if (!purin_snd_dac_device)
        return -ENOMEM;

    ret = platform_device_add(purin_snd_dac_device);
    if (ret)
        goto err1;


    purin_snd_soc_dac_device = platform_device_alloc("soc-audio", 1);
    if (!purin_snd_soc_dac_device) {
        ret = -ENOMEM;
        goto err2;
    }

    platform_set_drvdata(purin_snd_soc_dac_device, &purin_dac_card);

    ret = platform_device_add(purin_snd_soc_dac_device);
    if (ret)
        goto err3;

    return 0;

err3:
    platform_device_put(purin_snd_soc_dac_device);
err2:
    platform_device_del(purin_snd_dac_device);
err1:
    platform_device_put(purin_snd_dac_device);
    return ret;
}

static void __exit purin_dac_exit(void)
{
    platform_device_unregister(purin_snd_dac_device);
    platform_device_unregister(purin_snd_soc_dac_device);
}

module_init(purin_dac_init);
module_exit(purin_dac_exit);

MODULE_AUTHOR("Edden Tsai");
MODULE_DESCRIPTION("ALSA SoC SPDIF");
