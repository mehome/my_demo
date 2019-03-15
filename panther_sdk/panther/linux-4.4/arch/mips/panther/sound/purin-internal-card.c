/*
 *
 * ALSA SoC Audio Layer - Panther S/PDIF soc card driver
 *
 */

#include <linux/clk.h>
#include <linux/module.h>

#include <sound/soc.h>
#include <asm/bootinfo.h>
#include <asm/mach-panther/pinmux.h>
#include "purin-dma.h"

struct internal_audio_data {
    struct snd_soc_card *card;
    struct platform_device *codec_device[3];
};

static struct snd_soc_dai_link purin_internal_dai[] = {
    {
    .name = "DAC",
    .stream_name = "DAC Playback",
    .platform_name = "purin-internal-dma", // declared in purin-pcm.c & platform.c
    .cpu_dai_name = "purin-dac",      // declared in platform.c
    .codec_dai_name = "dac-hifi",     // declared in codec/dac.c
    .codec_name = "panther-dac",      // declared in codec/dac.c & purin_dac_init (platform_device)
    },
#if !defined(CONFIG_PANTHER_SND_PCM0_PCMWB_44100_TX_ENABLE)
    {
    .name = "ADC",
    .stream_name = "ADC Capture",
    .platform_name = "purin-internal-dma", // declared in purin-pcm.c & platform.c
    .cpu_dai_name = "purin-adc",      // declared in platform.c
    .codec_dai_name = "adc-hifi",     // declared in codec/adc.c
    .codec_name = "panther-adc",      // declared in codec/adc.c & purin_adc_init (platform_device)
    },
    /*
     * Must Put it at last!! If we set spdif_en = 0 in bootcmd,
     * we can only change snd_soc_cheetah_0.num_links = num_links-1
     */
    {
    .name = "S/PDIF",
    .stream_name = "S/PDIF Playback",
    .platform_name = "purin-spdif-dma", // declared in purin-spdif-dai.c & platform.c
    .cpu_dai_name = "purin-spdif",      // declared in platform.c
    .codec_dai_name = "dit-hifi",       // declared in codec/spdif.c
    .codec_name = "spdif-dit",          // declared in codec/spdif.c & purin_spdif_init (platform_device)
    },
#endif
};

static struct snd_soc_card purin_internal_card = {
    .name = "internal",
    .owner = THIS_MODULE,
    .dai_link = purin_internal_dai,
    .num_links = ARRAY_SIZE(purin_internal_dai),
};

static int purin_internal_probe(struct platform_device *op)
{
    struct snd_soc_card *card = &purin_internal_card;
    struct internal_audio_data *pdata;
    int ret;

    pdata = devm_kzalloc(&op->dev, sizeof(struct internal_audio_data),
                         GFP_KERNEL);
    if (!pdata)
        return -ENOMEM;

    card->dev = &op->dev;
    pdata->card = card;
    pdata->codec_device[0] = NULL;
    pdata->codec_device[1] = NULL;
    pdata->codec_device[2] = NULL;

    /* dac */
    pdata->codec_device[0] = platform_device_alloc("panther-dac", -1);
    if (!pdata->codec_device[0]){
        dev_err(&op->dev, "platform_device_alloc() failed\n");
    } else {
        ret = platform_device_add(pdata->codec_device[0]);
        if (ret) {
            platform_device_put(pdata->codec_device[0]);
            pdata->codec_device[0] = NULL;
            dev_err(&op->dev, "platform_device_add() failed: %d\n", ret);
        }
    }

#if !defined(CONFIG_PANTHER_SND_PCM0_PCMWB_44100_TX_ENABLE)
    /* adc */
    pdata->codec_device[1] = platform_device_alloc("panther-adc", -1);
    if (!pdata->codec_device[1]){
        dev_err(&op->dev, "platform_device_alloc() failed\n");
    } else {
        ret = platform_device_add(pdata->codec_device[1]);
        if (ret) {
            platform_device_put(pdata->codec_device[1]);
            pdata->codec_device[1] = NULL;
            dev_err(&op->dev, "platform_device_add() failed: %d\n", ret);
        }
    }
    
    /* spdif */
    if(spdif_pinmux_selected() == 1)
    {
        pdata->codec_device[2] = platform_device_alloc("spdif-dit", -1);
        if (!pdata->codec_device[2]){
            dev_err(&op->dev, "platform_device_alloc() failed\n");
        } else {
            ret = platform_device_add(pdata->codec_device[2]);
            if (ret) {
                platform_device_put(pdata->codec_device[2]);
                pdata->codec_device[2] = NULL;
                dev_err(&op->dev, "platform_device_add() failed: %d\n", ret);
            }
        }
    }
    else
    {
        /* remove spdif */
        card->num_links -= 1;
    }
#endif
    
    ret = snd_soc_register_card(card);
    if (ret)
        dev_err(&op->dev, "snd_soc_register_card() failed: %d\n", ret);
    
    platform_set_drvdata(op, pdata);
    return ret;
}

static int purin_internal_remove(struct platform_device *op)
{
    struct internal_audio_data *pdata = platform_get_drvdata(op);
    int ret;

    ret = snd_soc_unregister_card(pdata->card);
    if (pdata->codec_device[0])
        platform_device_unregister(pdata->codec_device[0]);
    if (pdata->codec_device[1])
        platform_device_unregister(pdata->codec_device[1]);

    if(likely(pdata)) {
        kfree(pdata);
    }

    return ret;
}

static struct platform_driver purin_internal_driver = {
    .probe    = purin_internal_probe,
    .remove   = purin_internal_remove,
    .driver   = {
        .name    = "purin-internal",
        .owner   = THIS_MODULE,
    },
};

module_platform_driver(purin_internal_driver);

MODULE_AUTHOR("Edden Tsai");
MODULE_DESCRIPTION("ALSA SoC SPDIF");
