/*
 * Driver for the ESS SABRE9018Q2C
 *
 * Author: Satoru Kawase, Takahito Nishiara
 *      Copyright 2016
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 */


#include <linux/init.h>
#include <linux/module.h>
#include <linux/regmap.h>
#include <linux/i2c.h>
#include <sound/soc.h>
#include <sound/pcm_params.h>
#include <sound/tlv.h>

#include "es9018k2m.h"


/* SABRE9018Q2C Codec Private Data */
struct es9018k2m_priv {
    struct regmap *regmap;
    unsigned int fmt;
};


/* SABRE9018Q2C Default Register Value */
static const struct reg_default es9018k2m_reg_defaults[] = {
    { 0, 0x00 },
    { 1, 0x8c },
    { 4, 0x00 },
    { 5, 0x68 },
    { 6, 0x4a },
    { 7, 0x80 },
    { 8, 0x88 },
    { 10,0x02 },
    { 11,0x02 },
    { 12,0x5a },
    { 13,0x40 },
    { 14,0x8a },
    { 15,0x50 },
    { 16,0x50 },
    { 17,0xff },
    { 18,0xff },
    { 19,0xff },
    { 20,0x7f },
    { 21,0x00 },
    { 22,0x00 },
    { 23,0x00 },
    { 24,0x00 },
    { 25,0x00 },
    { 26,0x00 },
    { 27,0x00 },
    { 28,0x00 },
    { 29,0x00 },
    { 30,0x00 },
    { 34,0x00 },
    { 35,0x00 },
    { 36,0x00 },
    { 37,0x00 },
    { 38,0x00 },
    { 39,0x00 },
    { 40,0x00 },
    { 41,0x04 },
    { 42,0x20 },
    { 43,0x00 },
};

/* Volume Scale */
static const DECLARE_TLV_DB_SCALE(volume_tlv, -12750, 50, 1);

/* Control */
static const struct snd_kcontrol_new es9018k2m_controls[] = {
SOC_DOUBLE_R_TLV("Master Playback Volume", ES9018K2M_VOLUME1, ES9018K2M_VOLUME2,
         0, 255, 0, volume_tlv),
};

static int es9018k2m_hw_params(
    struct snd_pcm_substream *substream, struct snd_pcm_hw_params *params,
    struct snd_soc_dai *dai)
{
    struct snd_soc_codec *codec = dai->codec;

    snd_soc_write(codec, ES9018K2M_SOFT_START,                  0x0a);
    snd_soc_write(codec, ES9018K2M_HEADPHONE_AMPLIFIER_CTRL,    0x25);
    snd_soc_write(codec, ES9018K2M_INPUT_CONFIG,                0x8c);
    snd_soc_write(codec, ES9018K2M_HEADPHONE_AMPLIFIER_CTRL,    0x65);
    snd_soc_write(codec, ES9018K2M_SOFT_START,                  0x8a);

    return 0;
}

static int es9018k2m_set_dai_sysclk(struct snd_soc_dai *codec_dai,
        int clk_id, unsigned int freq, int dir)
{

    switch (freq) {
    case 2048000:
    case 4096000:
    case 5644800:
    case 8192000:
    case 11289600:
    case 12288000:
    case 24576000:
    case 45158400:
    case 49152000:
    case 98304000:
        return 0;
    }
    return -EINVAL;
}

static int es9018k2m_set_dai_fmt(struct snd_soc_dai *codec_dai,
        unsigned int fmt)
{
    return 0;
}

static int es9018k2m_check_chip_id(struct snd_soc_codec *codec)
{

    unsigned int value;

    value = snd_soc_read(codec, ES9018K2M_CHIP_STATUS);

    if (((value & 0x1C) >> 2) != 0) {
        return -EINVAL;
    }

    return 0;
}

static int es9018k2m_codec_probe(struct snd_soc_codec *codec)
{
    int ret;

    ret = es9018k2m_check_chip_id(codec);
    if (ret < 0)
    {
        printk(KERN_ERR "es9018: failed to check chip id\n");
        return ret;
    }

    /* soft_reset */
    snd_soc_write(codec, ES9018K2M_SYSTEM_SETTING,              0x01);
    /* initial sequence */
    snd_soc_write(codec, ES9018K2M_SOFT_START,                  0x0a);
    snd_soc_write(codec, ES9018K2M_HEADPHONE_AMPLIFIER_CTRL,    0x25);
    snd_soc_write(codec, ES9018K2M_INPUT_CONFIG,                0x8c);
    snd_soc_write(codec, ES9018K2M_MASTER_MODE_CONTROL,         0x02);
    snd_soc_write(codec, ES9018K2M_THD_COMPENSATION,            0x40);
    snd_soc_write(codec, ES9018K2M_VOLUME1,                     0x00);
    snd_soc_write(codec, ES9018K2M_VOLUME2,                     0x00);
    snd_soc_write(codec, ES9018K2M_HEADPHONE_AMPLIFIER_CTRL,    0x65);
    snd_soc_write(codec, ES9018K2M_SOFT_START,                  0x8a);

    return ret;
}

static const struct snd_soc_dai_ops es9018k2m_dai_ops = {
    .hw_params    = es9018k2m_hw_params,
    .set_fmt      = es9018k2m_set_dai_fmt,
    .set_sysclk   = es9018k2m_set_dai_sysclk,
};


#define ES9018K2M_RATES (SNDRV_PCM_RATE_8000_44100 | SNDRV_PCM_RATE_64000 | SNDRV_PCM_RATE_48000 | SNDRV_PCM_RATE_88200 |SNDRV_PCM_RATE_96000|\
                             SNDRV_PCM_RATE_176400| SNDRV_PCM_RATE_192000 | SNDRV_PCM_RATE_384000)

#define ES9018K2M_FORMATS (SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S24_LE | \
            SNDRV_PCM_FMTBIT_S32_LE)

static struct snd_soc_dai_driver es9018k2m_dai = {
    .name = "es9018k2m-dai",
    .playback = {
        .stream_name  = "Playback",
        .channels_min = 1,
        .channels_max = 2,
        .rates        = ES9018K2M_RATES,
        .formats      = ES9018K2M_FORMATS,
    },
    .ops = &es9018k2m_dai_ops,
};

static struct snd_soc_codec_driver es9018k2m_codec_driver = {
    .probe            = es9018k2m_codec_probe,

    //.controls         = es9018k2m_controls,
    //.num_controls     = ARRAY_SIZE(es9018k2m_controls),
};


static const struct regmap_config es9018k2m_regmap = {
    .reg_bits         = 8,
    .val_bits         = 8,
    .max_register     = 74,

    .reg_defaults     = es9018k2m_reg_defaults,
    .num_reg_defaults = ARRAY_SIZE(es9018k2m_reg_defaults),
    .cache_type       = REGCACHE_RBTREE,
};

static void es9018k2m_remove(struct device *dev)
{
    snd_soc_unregister_codec(dev);
}

static int es9018k2m_i2c_probe(
        struct i2c_client *i2c, const struct i2c_device_id *id)
{
    struct regmap *regmap;
    struct es9018k2m_priv *es9018k2m;
    int ret;

    es9018k2m = devm_kzalloc(&i2c->dev, sizeof(struct es9018k2m_priv), GFP_KERNEL);
    if (!es9018k2m) {
        dev_err(&i2c->dev, "devm_kzalloc");
        return (-ENOMEM);
    }

    i2c_set_clientdata(i2c, es9018k2m);

    regmap = devm_regmap_init_i2c(i2c, &es9018k2m_regmap);
    if (IS_ERR(regmap)) {
        return PTR_ERR(regmap);
    }

    es9018k2m->regmap = regmap;
    ret = snd_soc_register_codec(&i2c->dev,
            &es9018k2m_codec_driver, &es9018k2m_dai, 1);
    if (ret != 0) {
        dev_err(&i2c->dev, "Failed to register CODEC: %d\n", ret);
        return ret;
    }

    return ret;
}

static int es9018k2m_i2c_remove(struct i2c_client *i2c)
{
    es9018k2m_remove(&i2c->dev);

    return 0;
}


static const struct i2c_device_id es9018k2m_i2c_id[] = {
    { "es9018k2m", },
    { }
};
MODULE_DEVICE_TABLE(i2c, es9018k2m_i2c_id);

static const struct of_device_id es9018k2m_of_match[] = {
    { .compatible = "ess,es9018k2m", },
    { }
};
MODULE_DEVICE_TABLE(of, es9018k2m_of_match);

static struct i2c_driver es9018k2m_i2c_driver = {
    .driver = {
        .name           = "es9018k2m-i2c",
        .owner          = THIS_MODULE,
        .of_match_table = of_match_ptr(es9018k2m_of_match),
    },
    .probe    = es9018k2m_i2c_probe,
    .remove   = es9018k2m_i2c_remove,
    .id_table = es9018k2m_i2c_id,
};
module_i2c_driver(es9018k2m_i2c_driver);


MODULE_DESCRIPTION("ASoC SABRE9018Q2C codec driver");
MODULE_AUTHOR("Satoru Kawase <satoru.kawase@gmail.com>");
MODULE_LICENSE("GPL");
