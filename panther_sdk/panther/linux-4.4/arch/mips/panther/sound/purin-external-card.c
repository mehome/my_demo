#if !defined(CONFIG_PANTHER_SND_PCM0_NONE)
/*
 * SoC audio for Panther
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/timer.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/i2c-algo-bit.h>
#include <linux/sysctl.h>

#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>

#include <asm/bootinfo.h>
#include <asm/mach-panther/panther.h>
#include <asm/mach-panther/pinmux.h>
#include "codec/wm8750.h"
#include "codec/wm8988.h"
#include "codec/nau8822.h"
#include "codec/mp320.h"
#include "codec/wm8737.h"
#include "codec/ad83586.h"
#include "codec/es8316.h"
#include "codec/es7243.h"
#include "codec/es7210.h"
#include "codec/es8388.h"
#include "purin-dma.h"
#include "purin-audio.h"

//#define PURIN_DEBUG
#ifdef PURIN_DEBUG
#undef pr_debug
#define pr_debug(fmt, ...) \
    printk(KERN_EMERG pr_fmt(fmt), ##__VA_ARGS__)
#endif

#if defined(CONFIG_PANTHER_SND_PCM0_WM8750)
#define CODEC_SYSCLK WM8750_SYSCLK
#define WM8750_I2C_ADDR_0 0x1b
#define WM8750_I2C_ADDR_1 0x1a
#elif defined(CONFIG_PANTHER_SND_PCM0_WM8988)
#define CODEC_SYSCLK WM8988_SYSCLK
#define WM8988_I2C_ADDR_0 0x1b
#define WM8988_I2C_ADDR_1 0x1a
#elif defined(CONFIG_PANTHER_SND_PCM0_NAU8822)
#define CODEC_SYSCLK NAU8822_MCLK
#define NAU8822_I2C_ADDR_0 0x1b
#define NAU8822_I2C_ADDR_1 0x1a
#elif defined(CONFIG_PANTHER_SND_PCM0_TAS5711)
#define CODEC_SYSCLK 0
#define TAS5711_I2C_ADDR_0 0x1b
#define TAS5711_I2C_ADDR_1 0x1a
#elif defined(CONFIG_PANTHER_SND_PCM0_ES8316)
#define CODEC_SYSCLK 0
#define ES8316_I2C_ADDR_0 0x10
#define ES8316_I2C_ADDR_1 0x11
#elif defined(CONFIG_PANTHER_SND_PCM0_ES9023)
#define CODEC_SYSCLK 0
#define ES9023_I2C_ADDR_0 0x1b
#define ES9023_I2C_ADDR_1 0x1a
#elif defined(CONFIG_PANTHER_SND_PCM0_MP320)
#define CODEC_SYSCLK MP320_SYSCLK
#define MP320_I2C_ADDR_0 0x1b
#define MP320_I2C_ADDR_1 0x1a
#elif defined(CONFIG_PANTHER_SND_PCM0_DUMMY)
#define CODEC_SYSCLK 0
#define DUMMY_I2C_ADDR_0 0x1b
#define DUMMY_I2C_ADDR_1 0x1a
#elif defined(CONFIG_PANTHER_SND_PCM0_WM8737)
#define CODEC_SYSCLK 0
#define WM8737_I2C_ADDR_0 0x1a
#define WM8737_I2C_ADDR_1 0x1b
#elif defined(CONFIG_PANTHER_SND_PCM0_WM8737_AD83586)
#define CODEC_SYSCLK 0
#define WM8737_I2C_ADDR_0 0x1a
#define AD83586_I2C_ADDR_0 0x30
#elif defined(CONFIG_PANTHER_SND_PCM0_TLV320AIC3111)
#define CODEC_SYSCLK 0
#define TLV320AIC3111_I2C_ADDR_0 0x18
#elif defined(CONFIG_PANTHER_SND_PCM0_ESS9018Q2C)
#define CODEC_SYSCLK 0
#define ESS9018Q2C_I2C_ADDR_0 0x48
#elif defined(CONFIG_PANTHER_SND_PCM0_ES7243)
#define CODEC_SYSCLK 0
#elif defined(CONFIG_PANTHER_SND_PCM0_ES7210)
#define CODEC_SYSCLK 0
#elif defined(CONFIG_PANTHER_SND_PCM0_ES8388)
#define CODEC_SYSCLK 0
#else
#error "Error: Forget To Assign Codec!!"
#endif


#if defined(CONFIG_PANTHER_SND_PCM0)
static struct i2c_board_info purin_i2c_devs_0[] = {
    {
#if defined(CONFIG_PANTHER_SND_PCM0_WM8750)
        I2C_BOARD_INFO("wm8750", WM8750_I2C_ADDR_0),
#endif
#if defined(CONFIG_PANTHER_SND_PCM0_WM8988)
        I2C_BOARD_INFO("wm8988", WM8988_I2C_ADDR_0),
#endif
/* NAU8822 only accept 0x1a as its I2C addr */
#if defined(CONFIG_PANTHER_SND_PCM0_NAU8822)
        I2C_BOARD_INFO("nau8822", NAU8822_I2C_ADDR_1),
#endif
/* TAS5711 only accept 0x1a as its I2C addr */
#if defined(CONFIG_PANTHER_SND_PCM0_TAS5711)
        I2C_BOARD_INFO("tas5711", TAS5711_I2C_ADDR_1),
#endif
#if defined(CONFIG_PANTHER_SND_PCM0_ES8316)
        I2C_BOARD_INFO("es8316", ES8316_I2C_ADDR_0),
#endif
#if defined(CONFIG_PANTHER_SND_PCM0_ES9023)
        I2C_BOARD_INFO("es9023", ES9023_I2C_ADDR_1),
#endif
#if defined(CONFIG_PANTHER_SND_PCM0_MP320)
        I2C_BOARD_INFO("mp320", MP320_I2C_ADDR_0),
#endif
#if defined(CONFIG_PANTHER_SND_PCM0_DUMMY)
        I2C_BOARD_INFO("panther-dummy", DUMMY_I2C_ADDR_0),
#endif
#if defined(CONFIG_PANTHER_SND_PCM0_WM8737)
        I2C_BOARD_INFO("wm8737", WM8737_I2C_ADDR_0),
#endif
#if defined(CONFIG_PANTHER_SND_PCM0_TLV320AIC3111)
        I2C_BOARD_INFO("tlv320aic3111", TLV320AIC3111_I2C_ADDR_0),
#endif
#if defined(CONFIG_PANTHER_SND_PCM0_ESS9018Q2C)
        I2C_BOARD_INFO("es9018k2m", ESS9018Q2C_I2C_ADDR_0),
#endif
#if defined(CONFIG_PANTHER_SND_PCM0_WM8737_AD83586)
        I2C_BOARD_INFO("wm8737", WM8737_I2C_ADDR_0),
    },
    {
        I2C_BOARD_INFO("ad83586", AD83586_I2C_ADDR_0),
#endif
#if defined(CONFIG_PANTHER_SND_PCM0_ES7243)
        I2C_BOARD_INFO("MicArray_0", 0x10),
#endif
#if defined(CONFIG_PANTHER_SND_PCM0_ES7210)
        I2C_BOARD_INFO("MicArray_0", 0x40),
#endif
#if defined(CONFIG_PANTHER_SND_PCM0_ES8388)
        I2C_BOARD_INFO("es8388", 0x11),
#endif
#if defined(CONFIG_PANTHER_SND_PCM1_ES8388)
    },
    {
        I2C_BOARD_INFO("es8388", 0x11),
#endif

    },
};
#endif

int sysctl_ai_interface = IF_I2S;
#if defined(AUDIO_RECOVERY)
int sysctl_ai_recovery = 0;
extern void start_recovery_timer(void);
static int proc_start_recovery(struct ctl_table *table, int write,
                     void __user *buffer, size_t *lenp, loff_t *ppos)
{
    int ret = proc_dointvec(table, write, buffer, lenp, ppos);
    if(sysctl_ai_recovery == 1)
    {
        start_recovery_timer();
    }
    return ret;
}
#endif

static struct ctl_table ai_table[] = {
    {
        //.ctl_name   = CTL_UNNUMBERED,
        .procname   = "ai.interface",
        .data       = &sysctl_ai_interface,
        .maxlen     = sizeof(int),
        .mode       = 0644,
        .proc_handler   = proc_dointvec
    },
#if defined(AUDIO_RECOVERY)
    {
        //.ctl_name   = CTL_UNNUMBERED,
        .procname   = "ai.recovery",
        .data       = &sysctl_ai_recovery,
        .maxlen     = sizeof(int),
        .mode       = 0644,
        .proc_handler   = proc_start_recovery
    },
#endif

    { //.ctl_name = 0,
    }
};

static int purin_get_pcm_clk(unsigned int sample_rate)
{
#if defined(CONFIG_PANTHER_SND_PCM0_ES8388)
    return 6144000;
#endif
    if (sample_rate != 8000 && sample_rate != 16000 && sample_rate != 32000 && sample_rate != 48000)
    {
        return -EINVAL;
    }
#if defined(CONFIG_PANTHER_SND_PCM0_ES7243)
#if defined(CONFIG_PANTHER_SUPPORT_I2S_48K)
    return 6144000;
#else
    return 4096000;
#endif
#else
    return 8192000;
#endif
}

static int purin_get_i2s_clk(unsigned int sample_rate)
{
    int clk;
    switch (sample_rate)
    {
    case 8000:
        clk = 2048000;
        break;
    case 16000:
        clk = 4096000;
        break;
    case 22050:
        clk = 5644800;
        break;
    case 32000:
        clk = 8192000;
        break;
    case 44100:
        clk = 11289600;
        break;
    case 48000:
        clk = 12288000;
        break;
    case 96000:
        clk = 24576000;
        break;
    case 176400:
        clk = 45158400;
        break;
    case 192000:
        clk = 49152000;
        break;
    case 384000:
        clk = 98304000;
        break;
    default:
        return -EINVAL;
    }

    return clk;
}

static char *ai_name[] =
{
    "PCM", "I2S", "PCMWB"
};

static char *stream_name[] =
{
    "PLAYBACK", "CAPTURE"
};

static int purin_hw_params(struct snd_pcm_substream *substream,
    struct snd_pcm_hw_params *params)
{
    struct snd_soc_pcm_runtime *rtd = substream->private_data;
    struct snd_soc_dai *codec_dai = rtd->codec_dai;
    int clk = 0;
    int ret = 0;
    unsigned int sample_rate = params_rate(params);
    pr_debug("\n");
    if(IF_I2S != sysctl_ai_interface)
    {
        clk = purin_get_pcm_clk(sample_rate);
    } else
    {
        clk = purin_get_i2s_clk(sample_rate);
    }

    if (clk < 0)
    {
        dev_err(rtd->dev, "not support sample rate %d in %s\n", sample_rate, ai_name[sysctl_ai_interface]);
        return clk;
    }

    dev_info(rtd->dev, "%s %s sample rate %d\n", stream_name[substream->stream], 
             ai_name[sysctl_ai_interface], sample_rate);

    /* set the codec system clock for DAC and ADC */
    // it will call codec set_dai_sysclk
    ret = snd_soc_dai_set_sysclk(codec_dai, CODEC_SYSCLK, clk, SND_SOC_CLOCK_IN);
    if (ret < 0)
        return ret;

    return 0;
}

static struct snd_soc_ops purin_ops = {
    .hw_params = purin_hw_params,
};

#if defined(CONFIG_PANTHER_SND_PCM0_TAS5711)
#define MCLKFS_RATIO 256
static int purin_tas5711_init(struct snd_soc_dapm_context *dapm)
{
    struct snd_soc_codec *codec = dapm->codec;
    snd_soc_codec_set_sysclk(codec, 1, 0, 48000 * MCLKFS_RATIO, 0);
    return 0;
}
#endif

/*
 * Logic for a codec as connected on a Panther Device
 */

struct external_audio_data {
    struct snd_soc_card *card;
    /* array size of codec_i2c "2" for CONFIG_PANTHER_SND_PCM0_WM8737_AD83586 */
    struct i2c_client *codec_i2c[2];
};

#if defined(CONFIG_PANTHER_SND_PCM0)
static struct snd_soc_dai_link purin_codec_dai []= {
#if defined(CONFIG_PANTHER_SND_PCM0_WM8750)
    {
    .name = "wm8750",
    .stream_name = "WM8750",
    .cpu_dai_name = "purin-audio",
    .codec_dai_name = "wm8750-hifi",
    .platform_name = "purin-external-dma",
    .codec_name = "wm8750.0-001b",
    .ops = &purin_ops,
    },
#endif
#if defined(CONFIG_PANTHER_SND_PCM0_WM8988)
    {
    .name = "wm8988",
    .stream_name = "WM8988",
    .cpu_dai_name = "purin-audio",
    .codec_dai_name = "wm8988-hifi",
    .platform_name = "purin-external-dma",
    .codec_name = "wm8988.0-001b",
    .ops = &purin_ops,
    },
#endif
#if defined(CONFIG_PANTHER_SND_PCM0_NAU8822)
    {
    .name = "nau8822",
    .stream_name = "NAU8822",
    .cpu_dai_name = "purin-audio",
    .codec_dai_name = "nau8822-hifi",
    .platform_name = "purin-external-dma",
    .codec_name = "nau8822.0-001a",
    .ops = &purin_ops,
    },
#endif
#if defined(CONFIG_PANTHER_SND_PCM0_TAS5711)
    {
    .name = "tas5711",
    .stream_name = "TAS5711",
    .cpu_dai_name = "purin-audio",
    .codec_dai_name = "tas5711",
    .platform_name = "purin-external-dma",
    .codec_name = "tas5711.0-001a",
    .ops = &purin_ops,
    },
#endif
#if defined(CONFIG_PANTHER_SND_PCM0_ES8316)
    {
    .name = "es8316",
    .stream_name = "ES8316",
    .cpu_dai_name = "purin-audio",
    .codec_dai_name = "es8316-hifi",
    .platform_name = "purin-external-dma",
    .codec_name = "es8316.0-0010",
    .ops = &purin_ops,
    },
#endif
#if defined(CONFIG_PANTHER_SND_PCM0_ES9023)
    {
    .name = "es9023",
    .stream_name = "Hifi DAC ES9023",
    .cpu_dai_name = "purin-audio",
    .codec_dai_name = "es9023",
    .platform_name = "purin-external-dma",
    .codec_name = "es9023.0-001a",
    .ops = &purin_ops,
    },
#endif
#if defined(CONFIG_PANTHER_SND_PCM0_MP320)
    {
    .name = "mp320",
    .stream_name = "MP320",
    .cpu_dai_name = "purin-audio",
    .codec_dai_name = "mp320-hifi",
    .platform_name = "purin-external-dma",
    .codec_name = "mp320.0-001b",
    .ops = &purin_ops,
    },
#endif
#if defined(CONFIG_PANTHER_SND_PCM0_DUMMY)
    {
    .name = "DUMMY",
    .stream_name = "DUMMY PLAYBACK",
    .cpu_dai_name = "purin-audio",
    .codec_dai_name = "dummy-hifi",
    .platform_name = "purin-external-dma",
    .codec_name = "panther-dummy.0-001b",
    .ops = &purin_ops,
    },
#endif
#if defined(CONFIG_PANTHER_SND_PCM0_TLV320AIC3111)
    {
    .name = "tlv320aic3111",
    .stream_name = "TLV320AIC3111",
    .cpu_dai_name = "purin-audio",
    .codec_dai_name = "tlv320aic31xx-hifi",
    .platform_name = "purin-external-dma",
    .codec_name = "tlv320aic31xx-codec.0-0018",
    .ops = &purin_ops,
    },
#endif
#if defined(CONFIG_PANTHER_SND_PCM0_ESS9018Q2C)
    {
    .name = "es9018k2m",
    .stream_name = "ESS9018Q2C",
    .cpu_dai_name = "purin-audio",
    .codec_dai_name = "es9018k2m-dai",
    .platform_name = "purin-external-dma",
    .codec_name = "es9018k2m-i2c.0-0048",
    .ops = &purin_ops,
    },
#endif
#if defined(CONFIG_PANTHER_SND_PCM0_WM8737)
    {
    .name = "wm8737",
    .stream_name = "WM8737",
    .cpu_dai_name = "purin-audio",
    .codec_dai_name = "wm8737",
    .platform_name = "purin-external-dma",
    .codec_name = "wm8737.0-001a",
    .ops = &purin_ops,
    },
#endif
#if defined(CONFIG_PANTHER_SND_PCM0_WM8737_AD83586)
    {
    .name = "wm8737",
    .stream_name = "WM8737",
    .cpu_dai_name = "purin-audio",
    .codec_dai_name = "wm8737",
    .platform_name = "purin-external-dma",
    .codec_name = "wm8737.0-001a",
    .ops = &purin_ops,
    },
    {
    .name = "ad83586",
    .stream_name = "AD83586",
    .cpu_dai_name = "purin-audio",
    .codec_dai_name = "ad83586-hifi",
    .platform_name = "purin-external-dma",
    .codec_name = "ad83586.0-0030",
    .ops = &purin_ops,
    },
#endif
#if defined(CONFIG_PANTHER_SND_PCM0_ES7243)
    {
    .name = "MicArray_0",
    .stream_name = "MicArray_0",
    .cpu_dai_name = "purin-audio",
    .codec_dai_name = "ES7243 HiFi 0",
    .platform_name = "purin-external-dma",
    .codec_name = "es7243-audio-adc.0-0010",
    .ops = &purin_ops,
    },
#endif
#if defined(CONFIG_PANTHER_SND_PCM0_ES7210)
    {
    .name = "MicArray_0",
    .stream_name = "MicArray_0",
    .cpu_dai_name = "purin-audio",
    .codec_dai_name = "ES7210 HiFi ADC 0",
    .platform_name = "purin-external-dma",
    .codec_name = "es7210-audio-adc.0-0040",
    .ops = &purin_ops,
    },
#endif
#if defined(CONFIG_PANTHER_SND_PCM0_ES8388) || defined(CONFIG_PANTHER_SND_PCM1_ES8388)
    {
    .name = "es8388",
    .stream_name = "ES8388",
    .cpu_dai_name = "purin-audio",
    .codec_dai_name = "es8388-hifi",
    .platform_name = "purin-external-dma",
    .codec_name = "es8388.0-0011",
    .ops = &purin_ops,
    },
#endif
};
#endif

struct snd_soc_aux_dev purin_aux_dev[] = {
#if defined(CONFIG_PANTHER_SND_PCM0_TAS5711)
    {
        .name = "tas5711",
        .codec_name = "tas5711.0-001a",
        .init = purin_tas5711_init,
    },
#endif
};

struct snd_soc_codec_conf purin_codec_conf[] = {
#if defined(CONFIG_PANTHER_SND_PCM0_TAS5711)
    {
        .dev_name = "tas5711.0-001a",
        .name_prefix = "AMP",
    },
#endif
};

/* Panther audio machine driver */
#if defined(CONFIG_PANTHER_SND_PCM0)
static struct snd_soc_card purin_external_card = {
      .name = "external",
      .owner = THIS_MODULE,
      .dai_link = purin_codec_dai,
      .num_links = ARRAY_SIZE(purin_codec_dai),
};

#endif

#ifdef CONFIG_SYSCTL
static struct ctl_table_header *ai_table_header;
#endif

static int purin_external_probe(struct platform_device *op)
{
    struct snd_soc_card *card = &purin_external_card;
    struct external_audio_data *pdata;
    struct i2c_adapter *adapter;
    struct i2c_client *client;
    int ret;

    if((i2sclk_pinmux_selected() == 0) || (i2sdat_pinmux_selected() == 0))
    {
        dev_err(&op->dev, "there is no setting about i2s pinmux.\n");
        return -ENODEV;
    }

    pdata = devm_kzalloc(&op->dev, sizeof(struct external_audio_data),
                 GFP_KERNEL);
    if (!pdata)
        return -ENOMEM;

    card->dev = &op->dev;
    pdata->card = card;
    pdata->codec_i2c[0] = NULL;
    pdata->codec_i2c[1] = NULL;

    adapter = i2c_get_adapter(0);
    if (!adapter)
        return -ENODEV;

    client = i2c_new_device(adapter, purin_i2c_devs_0);
    if (!client) {
        i2c_put_adapter(adapter);
    } else {
        client->adapter = adapter;
        pdata->codec_i2c[0] = client;
    }
    purin_external_card.dai_link->dai_fmt = (SND_SOC_DAIFMT_NB_NF | SND_SOC_DAIFMT_CBS_CFS);
    if (IF_I2S == sysctl_ai_interface) {
        purin_external_card.dai_link->dai_fmt |= SND_SOC_DAIFMT_I2S;
    } else {
        purin_external_card.dai_link->dai_fmt |= SND_SOC_DAIFMT_DSP_A;
    }
#if defined(CONFIG_PANTHER_SND_PCM0_WM8737_AD83586) || defined(CONFIG_PANTHER_SND_PCM1_ES8388)
    client = i2c_new_device(adapter, &purin_i2c_devs_0[1]);
    if (!client) {
        i2c_put_adapter(adapter);
    } else {
        client->adapter = adapter;
        pdata->codec_i2c[1] = client;
    }
#endif

    ret = snd_soc_register_card(card);
    if (ret)
        dev_err(&op->dev, "snd_soc_register_card() failed: %d\n", ret);
    
    platform_set_drvdata(op, pdata);
    strcpy(card->snd_card->mixername, card->dai_link->name);

#ifdef CONFIG_SYSCTL
    ai_table_header = register_sysctl_table(ai_table);
    if (ai_table_header == NULL)
        return -ENOMEM;
#endif
    return ret;
}

static int purin_external_remove(struct platform_device *op)
{
    struct external_audio_data *pdata = platform_get_drvdata(op);
    int ret;

    ret = snd_soc_unregister_card(pdata->card);
    if (pdata->codec_i2c[0]) {
        i2c_put_adapter(pdata->codec_i2c[0]->adapter);
        i2c_unregister_device(pdata->codec_i2c[0]);
    }
    if (pdata->codec_i2c[1]) {
        i2c_put_adapter(pdata->codec_i2c[1]->adapter);
        i2c_unregister_device(pdata->codec_i2c[1]);
    }

#ifdef CONFIG_SYSCTL
    unregister_sysctl_table(ai_table_header);
    ai_table_header = NULL;
#endif

    return ret;
}

static struct platform_driver purin_external_driver = {
    .probe      = purin_external_probe,
    .remove     = purin_external_remove,
    .driver     = {
        .name   = "purin-external",
        .owner = THIS_MODULE,
    },
};

module_platform_driver(purin_external_driver);

MODULE_AUTHOR("Edden Tsai");
MODULE_DESCRIPTION("ALSA SoC Panther");
#endif //!defined(CONFIG_PANTHER_SND_PCM0_NONE)
