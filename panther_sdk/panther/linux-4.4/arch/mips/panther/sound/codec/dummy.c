/*
 * dummy.c -- DUMMY ALSA SoC audio driver
 *
 * Copyright 2005 Openedhand Ltd.
 *
 *
 * Based on WM8750.c
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/pm.h>
#include <linux/i2c.h>
#include <linux/spi/spi.h>
#include <linux/slab.h>
//#include <linux/of_device.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/initval.h>

//#include "dummy.h"

//#define dummy_debug
#ifdef dummy_debug
#undef pr_debug
#define pr_debug(fmt, ...) \
    printk(KERN_EMERG pr_fmt(fmt), ##__VA_ARGS__)
#endif


/*
 * dummy register cache
 * We can't read the DUMMY register space when we
 * are using 2 wire for device control, so we cache them instead.
 */
static const struct reg_default dummy_reg_defaults[] = {
    {  0, 0x0097 },
    {  1, 0x0097 },
    {  2, 0x0079 },
    {  3, 0x0079 },
    {  4, 0x0000 },
    {  5, 0x0008 },
    {  6, 0x0000 },
    {  7, 0x000a },
    {  8, 0x0000 },
    {  9, 0x0000 },
    { 10, 0x00ff },
    { 11, 0x00ff },
    { 12, 0x000f },
    { 13, 0x000f },
    { 14, 0x0000 },
    { 15, 0x0000 },
    { 16, 0x0000 },
    { 17, 0x007b },
    { 18, 0x0000 },
    { 19, 0x0032 },
    { 20, 0x0000 },
    { 21, 0x00c3 },
    { 22, 0x00c3 },
    { 23, 0x00c0 },
    { 24, 0x0000 },
    { 25, 0x0000 },
    { 26, 0x0000 },
    { 27, 0x0000 },
    { 28, 0x0000 },
    { 29, 0x0000 },
    { 30, 0x0000 },
    { 31, 0x0000 },
    { 32, 0x0000 },
    { 33, 0x0000 },
    { 34, 0x0050 },
    { 35, 0x0050 },
    { 36, 0x0050 },
    { 37, 0x0050 },
    { 38, 0x0050 },
    { 39, 0x0050 },
    { 40, 0x0079 },
    { 41, 0x0079 },
    { 42, 0x0079 },
};

/* codec private data */
struct dummy_priv {
    unsigned int sysclk;
};

#define dummy_reset(c)  snd_soc_write(c, DUMMY_RESET, 0)

static const struct snd_soc_dapm_widget dummy_dapm_widgets[] = {
    SND_SOC_DAPM_OUTPUT("dummy-out"),
};

static const struct snd_soc_dapm_route dummy_dapm_routes[] = {
    { "dummy-out", NULL, "Playback" },
};

struct _coeff_div {
    u32 mclk;
    u32 rate;
    u16 fs;
    u8 sr:5;
    u8 usb:1;
};

/* codec hifi mclk clock divider coefficients */
static const struct _coeff_div coeff_div[] = {
    {8192000, 8000, 1024, 0x8, 0x0},    // PCM always 8192000/1024=8000
    {2048000, 8000,  256, 0x0, 0x0},    // we set 2048000/256=8000 in I2S

    /* 16k */
    {8192000, 16000, 512, 0x1c, 0x0},   // PCM always 8192000/512=16000
    {4096000, 16000, 256,  0x0, 0x0},   // we set 4096000/256=16000 in I2S

    /* 22.05k */
    {5644800, 22050, 256, 0x10, 0x0},   // we set 5644800/256=22050 in I2S

    /* 32k */
    {8192000, 32000, 256, 0x0, 0x0},    // we set 8192000/256=32000 in PCM/I2S

    /* 44.1k */
    {11289600, 44100, 256, 0x10, 0x0},  // we set 11289600/256=44100 in I2S

    /* 48k */
    {12288000, 48000, 256, 0x0, 0x0},   // we set 12288000/256=48000 in I2S

    /* 96k */
    {24576000, 96000, 256, 0x0, 0x0},   // we set 24576000/256=96000 in I2S
};

static inline int get_coeff(int mclk, int rate)
{
    int i;

    for (i = 0; i < ARRAY_SIZE(coeff_div); i++) {
        if (coeff_div[i].rate == rate && coeff_div[i].mclk == mclk)
            return i;
    }

    printk(KERN_ERR "dummy: could not get coeff for mclk %d @ rate %d\n",
        mclk, rate);
    return -EINVAL;
}

static int dummy_set_dai_sysclk(struct snd_soc_dai *codec_dai,
        int clk_id, unsigned int freq, int dir)
{
    struct snd_soc_codec *codec = codec_dai->codec;
    struct dummy_priv *dummy = snd_soc_codec_get_drvdata(codec);
    pr_debug("in function %s\n", __func__);

    switch (freq) {
    case 2048000:
    case 4096000:
    case 5644800:
    case 8192000:
    case 11289600:
    case 12288000:
    case 24576000:
        dummy->sysclk = freq;
        return 0;
    }
    return -EINVAL;
}

static int dummy_set_dai_fmt(struct snd_soc_dai *codec_dai,
        unsigned int fmt)
{
    return 0;
}

static int dummy_pcm_hw_params(struct snd_pcm_substream *substream,
                struct snd_pcm_hw_params *params,
                struct snd_soc_dai *dai)
{
    return 0;
}

static int dummy_mute(struct snd_soc_dai *dai, int mute)
{
    return 0;
}

static int dummy_set_bias_level(struct snd_soc_codec *codec,
                 enum snd_soc_bias_level level)
{
    return 0;
}

#define DUMMY_RATES (SNDRV_PCM_RATE_8000 | SNDRV_PCM_RATE_11025 |\
    SNDRV_PCM_RATE_16000 | SNDRV_PCM_RATE_22050 | SNDRV_PCM_RATE_44100 | SNDRV_PCM_RATE_32000 |\
    SNDRV_PCM_RATE_48000 | SNDRV_PCM_RATE_88200 | SNDRV_PCM_RATE_96000)

#define DUMMY_FORMATS (SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S24_LE)

static struct snd_soc_dai_ops dummy_dai_ops = {
    .hw_params  = dummy_pcm_hw_params,
    .digital_mute   = dummy_mute,
    .set_fmt    = dummy_set_dai_fmt,
    .set_sysclk = dummy_set_dai_sysclk,
};

static struct snd_soc_dai_driver dummy_dai = {
    .name = "dummy-hifi",
    .playback = {
        .stream_name = "Playback",
        .channels_min = 1,
        .channels_max = 2,
        .rates = DUMMY_RATES,
        .formats = DUMMY_FORMATS,},
    .ops = &dummy_dai_ops,
};

static int dummy_probe(struct snd_soc_codec *codec)
{
    return 0;
}

static struct snd_soc_codec_driver soc_codec_dev_dummy = {
    .probe =    dummy_probe,
    .set_bias_level = dummy_set_bias_level,
    .suspend_bias_off = true,

    .dapm_widgets = dummy_dapm_widgets,
    .num_dapm_widgets = ARRAY_SIZE(dummy_dapm_widgets),
    .dapm_routes = dummy_dapm_routes,
    .num_dapm_routes = ARRAY_SIZE(dummy_dapm_routes),
};

static const struct of_device_id dummy_of_match[] = {
    { .compatible = "wlf,panther-dummy", },
    { }
};
MODULE_DEVICE_TABLE(of, dummy_of_match);

static const struct regmap_config dummy_regmap = {
    .reg_bits = 7,
    .val_bits = 9,
    .max_register = 0xff,
    .reg_defaults = dummy_reg_defaults,
    .num_reg_defaults = ARRAY_SIZE(dummy_reg_defaults),
    .cache_type = REGCACHE_RBTREE,
};

static int dummy_i2c_probe(struct i2c_client *i2c,
                      const struct i2c_device_id *id)
{
    struct dummy_priv *dummy;
    struct regmap *regmap;
    int ret;

    dummy = devm_kzalloc(&i2c->dev, sizeof(struct dummy_priv),
                  GFP_KERNEL);
    if (dummy == NULL)
        return -ENOMEM;

    i2c_set_clientdata(i2c, dummy);

    regmap = devm_regmap_init_i2c(i2c, &dummy_regmap);
    if (IS_ERR(regmap))
        return PTR_ERR(regmap);

    ret =  snd_soc_register_codec(&i2c->dev,
            &soc_codec_dev_dummy, &dummy_dai, 1);
    return ret;
}

static int dummy_i2c_remove(struct i2c_client *client)
{
    snd_soc_unregister_codec(&client->dev);
    return 0;
}

static const struct i2c_device_id dummy_i2c_id[] = {
    { "panther-dummy", 0 },
    { }
};
MODULE_DEVICE_TABLE(i2c, dummy_i2c_id);

static struct i2c_driver dummy_i2c_driver = {
    .driver = {
        .name = "panther-dummy",
        .owner = THIS_MODULE,
//      .of_match_table = dummy_of_match,
    },
    .probe =    dummy_i2c_probe,
    .remove =   dummy_i2c_remove,
    .id_table = dummy_i2c_id,
};

static int __init dummy_modinit(void)
{
    int ret = 0;
    ret = i2c_add_driver(&dummy_i2c_driver);
    if (ret != 0) {
        printk(KERN_ERR "Failed to register dummy I2C driver: %d\n",
               ret);
    }
    return ret;
}
module_init(dummy_modinit);

static void __exit dummy_exit(void)
{
    i2c_del_driver(&dummy_i2c_driver);
}
module_exit(dummy_exit);

MODULE_DESCRIPTION("ASoC DUMMY driver");
MODULE_AUTHOR("Liam Girdwood");
MODULE_LICENSE("GPL");
