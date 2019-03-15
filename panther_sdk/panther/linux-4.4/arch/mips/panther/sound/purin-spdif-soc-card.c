/*
 *
 * ALSA SoC Audio Layer - Panther S/PDIF soc card driver
 *
 */

#include <linux/clk.h>
#include <linux/module.h>

#include <sound/soc.h>
#include <asm/bootinfo.h>

static int purin_spdif_hw_params(struct snd_pcm_substream *substream,
        struct snd_pcm_hw_params *params)
{
    return 0;
}

static struct snd_soc_ops purin_spdif_ops = {
    .hw_params = purin_spdif_hw_params,
};

static struct snd_soc_dai_link purin_spdif_dai = {
    .name = "S/PDIF",
    .stream_name = "S/PDIF Playback",
    .platform_name = "purin-spdif-dma", // declared in purin-spdif-dai.c
    .cpu_dai_name = "purin-spdif.2",    // declared in platform.c
    .codec_dai_name = "dit-hifi",       // declared in codec/spdif.c
    .codec_name = "spdif-dit",          // declared in codec/spdif.c & purin_spdif_init (platform_device)
    .ops = &purin_spdif_ops,
};

static struct snd_soc_card purin_spdif_card = {
    .name = "S/PDIF",
    .owner = THIS_MODULE,
    .dai_link = &purin_spdif_dai,
    .num_links = 1,
};

static struct platform_device *purin_snd_spdif_dit_device;
static struct platform_device *purin_snd_spdif_device;

static int __init purin_spdif_init(void)
{
    int ret;
    int spdif_en = 1;
    char *init_info = strstr(arcs_cmdline, "spdif_en=");
    if(init_info != NULL) {
        sscanf(&init_info[9], "%d", &spdif_en);
    }

    if (0 == spdif_en) {
        return 0;
    }

    purin_snd_spdif_dit_device = platform_device_alloc("spdif-dit", -1);
    if (!purin_snd_spdif_dit_device)
        return -ENOMEM;

    ret = platform_device_add(purin_snd_spdif_dit_device);
    if (ret)
        goto err1;


    purin_snd_spdif_device = platform_device_alloc("soc-audio", 2);
    if (!purin_snd_spdif_device) {
        ret = -ENOMEM;
        goto err2;
    }

    platform_set_drvdata(purin_snd_spdif_device, &purin_spdif_card);

    ret = platform_device_add(purin_snd_spdif_device);
    if (ret)
        goto err3;

    return 0;

err3:
    platform_device_put(purin_snd_spdif_device);
err2:
    platform_device_del(purin_snd_spdif_dit_device);
err1:
    platform_device_put(purin_snd_spdif_dit_device);
    return ret;
}

static void __exit purin_spdif_exit(void)
{
    platform_device_unregister(purin_snd_spdif_dit_device);
    platform_device_unregister(purin_snd_spdif_device);
}

module_init(purin_spdif_init);
module_exit(purin_spdif_exit);

MODULE_AUTHOR("Edden Tsai");
MODULE_DESCRIPTION("ALSA SoC SPDIF");
