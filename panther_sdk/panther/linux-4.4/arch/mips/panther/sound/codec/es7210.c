/*
 * ALSA SoC ES7210 adc driver
 *
 * Author:      David Yang, <yangxiaohua@everest-semi.com>
 * Copyright:   (C) 2018 Everest Semiconductor Co Ltd.,
 *
 * Based on sound/soc/codecs/es7210.c by David Yang
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Notes:
 *  ES7210 is a 4-ch ADC of Everest
 *
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/pm.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>
#include <sound/tlv.h>
#include <sound/initval.h>
#include <linux/regmap.h>
#include <asm/bootinfo.h>
#include "es7210.h"

//#define SUPPORT_2MIC
#if defined (CONFIG_INPUT_MIC_HW4_REF_HW1) || defined(CONFIG_INPUT_MIC_HW4_REF_HW2) ||defined( CONFIG_INPUT_MIC_HW3_REF_HW2)
#include "es7243.h"
#endif
#define MIC_CHN_16	16
#define MIC_CHN_14	14
#define MIC_CHN_12	12
#define MIC_CHN_10	10
#define MIC_CHN_8	8
#define MIC_CHN_6	6
#define MIC_CHN_4	4
#define MIC_CHN_2	2

#if defined (CONFIG_INPUT_MIC_HW4_REF_HW1) || defined(CONFIG_INPUT_MIC_HW4_REF_HW2) ||defined( CONFIG_INPUT_MIC_HW3_REF_HW2)
#define ES7210_CHANNELS_MAX	MIC_CHN_6
#else
#define ES7210_CHANNELS_MAX	MIC_CHN_4
#endif
#define SUPPORT_4096MHZ 1

#if ES7210_CHANNELS_MAX == MIC_CHN_2
#define ADC_DEV_MAXNUM	1
#endif
#if ES7210_CHANNELS_MAX == MIC_CHN_4
#define ADC_DEV_MAXNUM   1
#endif
#if ES7210_CHANNELS_MAX == MIC_CHN_6
#define ADC_DEV_MAXNUM   2
#endif

#define ES7210_TDM_1LRCK_DSPA                 0
#define ES7210_TDM_1LRCK_DSPB                 1
#define ES7210_TDM_1LRCK_I2S                  2
#define ES7210_TDM_1LRCK_LJ                   3
#define ES7210_TDM_NLRCK_DSPA                 4
#define ES7210_TDM_NLRCK_DSPB                 5
#define ES7210_TDM_NLRCK_I2S                  6
#define ES7210_TDM_NLRCK_LJ                   7

#define ES7210_WORK_MODE    ES7210_TDM_1LRCK_DSPA

#define ES7210_I2C_BUS_NUM 				0
#define ES7210_CODEC_RW_TEST_EN		0
#define ES7210_IDLE_RESET_EN			1	//reset ES7210 when in idle time
#define ES7210_MATCH_DTS_EN				0	//ES7210 match method select: 0: i2c_detect, 1:of_device_id

struct regmap *regmap_clt[ADC_DEV_MAXNUM];

/* codec private data */
struct es7210_priv {
	struct regmap *regmap;
	struct i2c_client *i2c;
	unsigned int dmic_enable;
	unsigned int sysclk;
	struct snd_pcm_hw_constraint_list *sysclk_constraints;
	unsigned int tdm_mode;
	struct delayed_work pcm_pop_work;
};

static int es7210_init_reg = 0;
static int es7243_init_reg = 0;

static const struct regmap_config es7210_regmap_config = {
	.reg_bits = 8,	//Number of bits in a register address
	.val_bits = 8,	//Number of bits in a register value
};
/*
* ES7210 register cache
*/
static const u8 es7210_reg[] = {
	0x32, 0x40, 0x02, 0x04, 0x01, 0x00, 0x00, 0x20,	/* 0 - 7 */
	0x10, 0x40, 0x40, 0x00, 0x00, 0x09, 0x00, 0x00,	/* 8 - F */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	/* 10 - 17 */
	0xf7, 0xf7, 0x00, 0xbf, 0xbf, 0xbf, 0xbf, 0x00,	/* 18 - 1f */
	0x26, 0x26, 0x06, 0x26, 0x00, 0x00, 0x00, 0x00,	/* 20 - 27 */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	/* 28 - 2f */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	/* 30 - 37 */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x72, 0x10, 0x00,	/* 38 - 3f */
	0x80, 0x71, 0x71, 0x00, 0x00, 0x00, 0x00, 0x00,	/* 40 - 47 */
	0x00, 0x00, 0x00, 0xff, 0xff,			/* 48 - 4c */
};

#define DEF_REF_GAIN_IDX 3

#if defined (CONFIG_INPUT_MIC_HW4_REF_HW1) || defined(CONFIG_INPUT_MIC_HW4_REF_HW2) ||defined( CONFIG_INPUT_MIC_HW3_REF_HW2)
static u8 gain_idx[4];
static const u8 es7243_gain_sel_table[] = {
	0x10, 0x12, 0x20, 0x21, 0x4, 0x40, 0x6, 0x43,
	/* 1dB, 3.5dB, 18dB, 20.5dB, 22.5dB, 24.5dB, 25dB, 27dB */
};
#endif


static const struct reg_default es7210_reg_defaults[] = {
	{ 0x00, 0x32 }, //0
	{ 0x01, 0x40 },
	{ 0x02, 0x02 },
	{ 0x03, 0x02 },
	{ 0x04, 0x01 },
	{ 0x05, 0x00 },
	{ 0x06, 0x00 },
	{ 0x07, 0x20 },
	{ 0x08, 0x20 },
	{ 0x09, 0x40 },
	{ 0x0a, 0x40 },
	{ 0x0b, 0x00 },
	{ 0x0c, 0x00 },
	{ 0x0d, 0x09 },
	{ 0x0e, 0x00 },
	{ 0x0f, 0x00 },
	{ 0x10, 0x00 },
	{ 0x11, 0x00 },
	{ 0x12, 0x00 },
	{ 0x13, 0x00 },
	{ 0x14, 0x00 },
	{ 0x15, 0x00 },
	{ 0x16, 0x00 },
	{ 0x17, 0x00 },
	{ 0x18, 0xf7 },
	{ 0x19, 0xf7 },
	{ 0x1a, 0x00 },
	{ 0x1b, 0xbf },
	{ 0x1c, 0xbf },
	{ 0x1d, 0xbf },
	{ 0x1e, 0xbf },
	{ 0x1f, 0x00 },
	{ 0x20, 0x26 },
	{ 0x21, 0x26 },
	{ 0x22, 0x06 },
	{ 0x23, 0x26 },
	{ 0x3d, 0x72 },
	{ 0x3e, 0x10 },
	{ 0x3f, 0x00 },
	{ 0x40, 0x80 },
	{ 0x41, 0x71 },
	{ 0x42, 0x71 },
	{ 0x43, 0x00 },
	{ 0x44, 0x00 },
	{ 0x45, 0x00 },
	{ 0x46, 0x00 },
	{ 0x47, 0x00 },
	{ 0x48, 0x00 },
	{ 0x49, 0x00 },
	{ 0x4a, 0x00 },
	{ 0x4b, 0xff },
	{ 0x4c, 0xff },
};
struct es7210_reg_config {
	unsigned char reg_addr;
	unsigned char reg_v;
};
static const struct es7210_reg_config es7210_tdm_reg_common_cfg1[] = {
	{ 0x00, 0xFF },
	{ 0x00, 0x32 },
	{ 0x09, 0x30 },
	{ 0x0A, 0x30 },
	{ 0x23, 0x26 },
	{ 0x22, 0x06 },
	{ 0x21, 0x26 },
	{ 0x20, 0x06 },
};
static const struct es7210_reg_config es7210_tdm_reg_fmt_cfg[] = {
	{ 0x11, 0x63 },
	{ 0x12, 0x01 },
};
static const struct es7210_reg_config es7210_tdm_reg_common_cfg2[] = {
	{ 0x40, 0xC3 },
	{ 0x41, 0x71 },
	{ 0x42, 0x71 },
	{ 0x43, 0x1d },
	{ 0x44, 0x1d },
#if defined (CONFIG_INPUT_MIC_HW4_REF_HW1) || defined(CONFIG_INPUT_MIC_HW4_REF_HW2) ||defined( CONFIG_INPUT_MIC_HW3_REF_HW2)
	{ 0x45, 0x1d },
	{ 0x46, 0x1d },
#else
	{ 0x45, 0x18 },
	{ 0x46, 0x18 },
#endif
	{ 0x47, 0x08 },
	{ 0x48, 0x08 },
	{ 0x49, 0x08 },
	{ 0x4A, 0x08 },
	{ 0x07, 0x20 },

};
static const struct es7210_reg_config es7210_tdm_reg_mclk_cfg[] = {
	{ 0x02, 0xC1 },
};
static const struct es7210_reg_config es7210_tdm_reg_common_cfg3[] = {
	{ 0x06, 0x04 },
	{ 0x4B, 0x0F },
	{ 0x4C, 0x0F },
	{ 0x00, 0x71 },
	{ 0x00, 0x41 },
};

static int regmap_get_i2caddr(struct regmap *map)
{
    if( map == regmap_clt[0] )
    	return 0x40;
#if defined (CONFIG_INPUT_MIC_HW4_REF_HW1) || defined(CONFIG_INPUT_MIC_HW4_REF_HW2) ||defined( CONFIG_INPUT_MIC_HW3_REF_HW2)

    else if( map == regmap_clt[1] )
    	return 0x13;
#endif
    return 0;
}

static int es7210_read(u8 reg, u8 *rt_value, struct regmap *regmap)
{
	 int val = 0, ret = 0;
	 ret = regmap_read(regmap, reg, &val );
	 if( ret < 0 ) {
		  pr_err("i2cread 0x%x error, reg = 0x%x, ret = %d\n", regmap_get_i2caddr(regmap), reg, ret );
	 }
	 *rt_value = val & 0xFF;

	 return ret;
}

static int es7210_write(u8 reg, u8 value, struct regmap *regmap)
{
		int ret = 0;

		ret = regmap_write(regmap, reg, value);
		if( ret < 0 ) {
		  pr_err("i2cwrite 0x%x error, reg = 0x%x, ret = %d\n", regmap_get_i2caddr(regmap), reg, ret );
		}
		return ret;
}

static int es7210_update_bits(u8 reg, u8 mask, u8 value, struct regmap *regmap)
{
	return regmap_update_bits(regmap, reg, mask, value);
}

static int es7210_multi_chips_write(u8 reg, unsigned char value)
{
	es7210_write(reg, value, regmap_clt[0]);

	return 0;
}

static int es7210_multi_chips_update_bits(u8 reg, u8 mask, u8 value)
{
	es7210_update_bits(reg, mask, value, regmap_clt[0]);

	return 0;
}



/* The set of rates we can generate from the above for each SYSCLK */
#ifdef SUPPORT_4096MHZ
static unsigned int rates_4096[] = {
	16000,
};

static struct snd_pcm_hw_constraint_list constraints_4096 = {
	.count  = ARRAY_SIZE(rates_4096),
	.list   = rates_4096,
};
#endif


static unsigned int rates_8192[] = {
	16000, 32000,
};

static struct snd_pcm_hw_constraint_list constraints_8192 = {
	.count  = ARRAY_SIZE(rates_8192),
	.list   = rates_8192,
};

static unsigned int rates_12288[] = {
	8000, 12000, 16000, 24000, 24000, 32000, 48000, 96000,
};

static struct snd_pcm_hw_constraint_list constraints_12288 = {
	.count  = ARRAY_SIZE(rates_12288),
	.list   = rates_12288,
};

static unsigned int rates_112896[] = {
	8000, 11025, 22050, 44100,
};

static struct snd_pcm_hw_constraint_list constraints_112896 = {
	.count  = ARRAY_SIZE(rates_112896),
	.list   = rates_112896,
};

static unsigned int rates_12[] = {
	8000, 11025, 12000, 16000, 22050, 24000, 32000, 44100, 48000,
	48000, 88235, 96000,
};

static struct snd_pcm_hw_constraint_list constraints_12 = {
	.count  = ARRAY_SIZE(rates_12),
	.list   = rates_12,
};

/*
* Note that this should be called from init rather than from hw_params.
*/
static int es7210_set_dai_sysclk(struct snd_soc_dai *codec_dai,
	int clk_id, unsigned int freq, int dir)
{
	struct snd_soc_codec *codec = codec_dai->codec;
	struct es7210_priv *es7210 = snd_soc_codec_get_drvdata(codec);
	pr_debug("freq = %d\n", freq );

	switch (freq) {
#ifdef SUPPORT_4096MHZ
  case 4096000:
	es7210->sysclk_constraints = &constraints_4096;
	es7210->sysclk = freq;
	return 0;
#endif
	case 8192000:
		es7210->sysclk_constraints = &constraints_8192;
		es7210->sysclk = freq;
		return 0;
	case 11289600:
	case 22579200:
		es7210->sysclk_constraints = &constraints_112896;
		es7210->sysclk = freq;
		return 0;

	case 12288000:
	case 24576000:
		es7210->sysclk_constraints = &constraints_12288;
		es7210->sysclk = freq;
		return 0;

	case 12000000:
	case 24000000:
		es7210->sysclk_constraints = &constraints_12;
		es7210->sysclk = freq;
		return 0;
	}
	return -EINVAL;
}


static int es7210_set_channels(u8 channel)
{
	 /*
		* Set the microphone numbers in array
		*/
	switch (channel) {
		case 2:
			es7210_multi_chips_write(ES7210_MODE_CFG_REG08, 0x18);
			break;
		case 4:
			es7210_multi_chips_write(ES7210_MODE_CFG_REG08, 0x28);
			break;
		case 6:
			es7210_multi_chips_write(ES7210_MODE_CFG_REG08, 0x38);
			break;
		case 8:
			es7210_multi_chips_write(ES7210_MODE_CFG_REG08, 0x48);
			break;
		case 10:
			es7210_multi_chips_write(ES7210_MODE_CFG_REG08, 0x58);
			break;
		case 12:
			es7210_multi_chips_write(ES7210_MODE_CFG_REG08, 0x68);
			break;
		case 14:
			es7210_multi_chips_write(ES7210_MODE_CFG_REG08, 0x78);
			break;
		case 16:
			es7210_multi_chips_write(ES7210_MODE_CFG_REG08, 0x88);
			break;
		default:
			break;
	}
	return 0;
}

/*
* to initialize es7210 for tdm mode
*/
static void es7210_tdm_init_codec(u8 mode)
{
	int i = 0, channel = 0;

	pr_debug("enter into %s\n", __func__);
	for (i = 0; i < sizeof(es7210_tdm_reg_common_cfg1) / sizeof(es7210_tdm_reg_common_cfg1[0]); i++) {
		es7210_multi_chips_write(es7210_tdm_reg_common_cfg1[i].reg_addr,
			es7210_tdm_reg_common_cfg1[i].reg_v);
	}
	switch (mode) {
	case ES7210_TDM_1LRCK_DSPA:
		/*
		* Here to set TDM format for DSP-A mode
		*/

		es7210_write(ES7210_SDP_CFG1_REG11, 0x63, regmap_clt[0]);
		es7210_write(ES7210_SDP_CFG2_REG12, 0x01, regmap_clt[0]);
		break;

	case ES7210_TDM_1LRCK_DSPB:
		/*
		* Here to set TDM format for DSP-B mode
		*/
		es7210_write(ES7210_SDP_CFG1_REG11, 0x73, regmap_clt[0]);
		es7210_write(ES7210_SDP_CFG2_REG12, 0x01, regmap_clt[0]);
		break;

	case ES7210_TDM_1LRCK_I2S:
		/*
		* Here to set TDM format for I2S mode
		*/
		es7210_write(ES7210_SDP_CFG1_REG11, 0x60, regmap_clt[0]);
		es7210_write(ES7210_SDP_CFG2_REG12, 0x02, regmap_clt[0]);
		break;

	case ES7210_TDM_1LRCK_LJ:
		/*
		* Here to set TDM format for Left Justified mode
		*/
		es7210_write(ES7210_SDP_CFG1_REG11, 0x61, regmap_clt[0]);
		es7210_write(ES7210_SDP_CFG2_REG12, 0x02, regmap_clt[0]);
		break;

	case ES7210_TDM_NLRCK_DSPA:
		/*
		 * Here to set TDM format for DSP-A with multiple LRCK TDM mode
		 */
		channel = ES7210_CHANNELS_MAX;
		/*
		* Set the microphone numbers in array
		*/
		es7210_set_channels(channel);
		/*
		* set format, dsp-a with multiple LRCK tdm mode
		*/
		es7210_write(ES7210_SDP_CFG1_REG11, 0x63, regmap_clt[0]);
		/*
		* set tdm flag in the interface chip
		*/
		es7210_write(ES7210_SDP_CFG2_REG12, 0x07, regmap_clt[0]);
		break;

	case ES7210_TDM_NLRCK_DSPB:
		/*
		* Here to set TDM format for DSP-B with multiple LRCK TDM mode
		*/
		channel = ES7210_CHANNELS_MAX;
		/*
		* Set the microphone numbers in array
		*/
		es7210_set_channels(channel);
		/*
		* set format, dsp-b with multiple LRCK tdm mode
		*/
		es7210_write(ES7210_SDP_CFG1_REG11, 0x73, regmap_clt[0]);
		/*
		* set tdm flag in the interface chip
		*/
		es7210_write(ES7210_SDP_CFG2_REG12, 0x07, regmap_clt[0]);
		break;

	case ES7210_TDM_NLRCK_I2S:
		/*
		* Here to set TDM format for I2S with multiple LRCK TDM mode
		*/
		channel = ES7210_CHANNELS_MAX;
		/*
		* Set the microphone numbers in array
		*/
		es7210_set_channels(channel);
		/*
		* set format, I2S with multiple LRCK tdm mode
		*/
		es7210_write(ES7210_SDP_CFG1_REG11, 0x60, regmap_clt[0]);
		/*
		* set tdm flag in the interface chip
		*/
		es7210_write(ES7210_SDP_CFG2_REG12, 0x07, regmap_clt[0]);
		break;

	case ES7210_TDM_NLRCK_LJ:
		/*
		* Here to set TDM format for left justified with multiple LRCK TDM mode
		*/
		channel = ES7210_CHANNELS_MAX;
		/*
		* Set the microphone numbers in array
		*/
		es7210_set_channels(channel);
		/*
		* set format, left justified with multiple LRCK tdm mode
		*/
		es7210_write(ES7210_SDP_CFG1_REG11, 0x61, regmap_clt[0]);
		/*
		* set tdm flag in the interface chip
		*/
		es7210_write(ES7210_SDP_CFG2_REG12, 0x07, regmap_clt[0]);

		break;
	default:
		/*
		* here to disable tdm and set i2s-16bit for normal mode
		*/
		es7210_multi_chips_write(ES7210_SDP_CFG1_REG11, 0x60); //i2s-16bits
		es7210_multi_chips_write(ES7210_SDP_CFG2_REG12, 0x00); //disable tdm
		break;
	}
	for (i = 0; i < sizeof(es7210_tdm_reg_common_cfg2) / sizeof(es7210_tdm_reg_common_cfg2[0]); i++) {
		es7210_multi_chips_write(es7210_tdm_reg_common_cfg2[i].reg_addr,
			es7210_tdm_reg_common_cfg2[i].reg_v);
	}
	switch (mode) {
	case ES7210_TDM_1LRCK_DSPA:
	case ES7210_TDM_1LRCK_DSPB:
	case ES7210_TDM_1LRCK_I2S:
	case ES7210_TDM_1LRCK_LJ:
		/*
		* to set internal mclk
		* here, we assume that cpu/soc always provides 256FS i2s clock to es7210.
		* dll bypassed, use clock doubler to get double frequency for internal modem which need
		* 512FS clock. the clk divider ratio is 1.
		* user must modify the setting of register0x02 according to FS ratio provided by CPU/SOC.
		*/
		es7210_multi_chips_write(ES7210_MCLK_CTL_REG02, 0xc1);
		break;
	case ES7210_TDM_NLRCK_DSPA:
	case ES7210_TDM_NLRCK_DSPB:
	case ES7210_TDM_NLRCK_I2S:
	case ES7210_TDM_NLRCK_LJ:
		/*
		* to set internal mclk
		* here, we assume that cpu/soc always provides 256FS i2s clock to es7210 and there is four
		* es7210 devices in tdm link. so the internal FS in es7210 is only FS/4;
		* dll bypassed, clock doubler bypassed. the clk divider ratio is 2. so the clock of internal
		* modem equals to (256FS / (FS/4) / 2) * FS = 512FS
		* user must modify the setting of register0x02 according to FS ratio provided by CPU/SOC.
		*/

		es7210_multi_chips_write(ES7210_MCLK_CTL_REG02, 0x82);
		break;
	default:
		/*
		* to set internal mclk for normal mode
		* here, we assume that cpu/soc always provides 256FS i2s clock to es7210.
		* dll bypassed, use clock doubler to get double frequency for internal modem which need
		* 512FS clock. the clk divider ratio is 1.
		* user must modify the setting of register0x02 according to FS ratio provided by CPU/SOC.
		*/
		es7210_multi_chips_write(ES7210_MCLK_CTL_REG02, 0xc1);

		break;
	}
	es7210_multi_chips_update_bits(ES7210_MODE_CFG_REG08, 0x08, 0x08);

	
	for (i = 0; i < sizeof(es7210_tdm_reg_common_cfg3) / sizeof(es7210_tdm_reg_common_cfg3[0]); i++) {
		es7210_multi_chips_write(es7210_tdm_reg_common_cfg3[i].reg_addr,
			es7210_tdm_reg_common_cfg3[i].reg_v);
	}
	/*
	* Mute All ADC
	*/
	es7210_multi_chips_update_bits(ES7210_ADC34_MUTE_REG14, 0x03, 0x03);
	es7210_multi_chips_update_bits(ES7210_ADC12_MUTE_REG15, 0x03, 0x03);

}

static int es7210_set_dai_fmt(struct snd_soc_dai *codec_dai,
	unsigned int fmt)
{

	pr_debug("es7210_set_dai_fmt\n");

	es7210_write(ES7210_SDP_CFG1_REG11, 0x63, regmap_clt[0]);
	es7210_write(ES7210_SDP_CFG2_REG12, 0x01, regmap_clt[0]);

	return 0;
}

static void es7210_unmute(void)
{
	pr_debug("enter into %s\n", __func__);
	es7210_multi_chips_update_bits(ES7210_ADC34_MUTE_REG14, 0x03, 0x00);
	es7210_multi_chips_update_bits(ES7210_ADC12_MUTE_REG15, 0x03, 0x00);

#if defined (CONFIG_INPUT_MIC_HW4_REF_HW1) || defined(CONFIG_INPUT_MIC_HW4_REF_HW2) ||defined( CONFIG_INPUT_MIC_HW3_REF_HW2)
	es7210_update_bits(ES7243_MUTECTL_REG05, 0x08, 0x00, regmap_clt[1]);
#endif
}

#if defined (CONFIG_INPUT_MIC_HW4_REF_HW1) || defined(CONFIG_INPUT_MIC_HW4_REF_HW2) ||defined( CONFIG_INPUT_MIC_HW3_REF_HW2)
static void es7243_init_work_events(void)
{
	pr_debug("enter into %s, initializating es7243\n", __func__);
	/*ES7243 AEC*/
	es7210_write(ES7243_MODECFG_REG00, 0x01, regmap_clt[1]);//enter into hardware mode
	es7210_write(ES7243_STATECTL_REG06, 0x18, regmap_clt[1]); //soft reset codec

	es7210_write(ES7243_ANACTL0_REG07, 0x20, regmap_clt[1]);
	es7210_write(ES7243_ANACTL2_REG09, 0x80, regmap_clt[1]);

	es7210_write(ES7243_SDPFMT_REG01, 0xCF, regmap_clt[1]); //dsp for tdm mode, DSP-A, 16BIT, bclk invert
#if 0
	es7210_write(ES7243_LRCDIV_REG02, 0x10, regmap_clt[1]);
	es7210_write(ES7243_BCKDIV_REG03, 0x04, regmap_clt[1]);
	es7210_write(ES7243_CLKDIV_REG04, 0x02, regmap_clt[1]);
#endif
	es7210_write(ES7243_MUTECTL_REG05, 0x1b, regmap_clt[1]);
	//printk(KERN_DEBUG "ref mic gain = 0x%x\n", es7243_gain_sel_table[gain_idx[2]] | 0x1  );
	//es7210_write(ES7243_ANACTL1_REG08, es7243_gain_sel_table[gain_idx[2]] | 0x1, regmap_clt[1]);
	es7210_write(ES7243_ANACTL1_REG08, 0x43 | 0x1, regmap_clt[1]);

	es7210_write(ES7243_ANACTL2_REG09, 0x3F, regmap_clt[1]);
	es7210_write(ES7243_ANACTL0_REG07, 0x80, regmap_clt[1]);

	es7210_write(ES7243_STATECTL_REG06, 0x00, regmap_clt[1]);
	es7210_write(ES7243_MUTECTL_REG05, 0x13, regmap_clt[1]);
}
#endif
static void pcm_pop_work_events(struct work_struct *work)
{
	pr_debug("enter into %s\n", __func__);
	es7210_unmute();
	es7210_init_reg = 1;
#if defined (CONFIG_INPUT_MIC_HW4_REF_HW1) || defined(CONFIG_INPUT_MIC_HW4_REF_HW2) ||defined( CONFIG_INPUT_MIC_HW3_REF_HW2)
	es7243_init_reg = 1;
#endif
}


static int es7210_mute(struct snd_soc_dai *dai, int mute)
{
	//struct snd_soc_codec *codec = dai->codec;
	u8 i;
	pr_debug("enter into %s, mute = %d\n", __func__, mute);
	for (i = 0; i < ADC_DEV_MAXNUM; i++){
		if (mute) {
			if (i == 1)
			{
#if defined (CONFIG_INPUT_MIC_HW4_REF_HW1) || defined(CONFIG_INPUT_MIC_HW4_REF_HW2) ||defined( CONFIG_INPUT_MIC_HW3_REF_HW2)
				es7210_update_bits(ES7243_MUTECTL_REG05, 0x08, 0x08, regmap_clt[i]);
#endif
			}
			else
			{
				es7210_update_bits(ES7210_ADC34_MUTE_REG14, 0x03, 0x03, regmap_clt[i]);
				es7210_update_bits(ES7210_ADC12_MUTE_REG15, 0x03, 0x03, regmap_clt[i]);
			}

			/*
				es7210_multi_chips_update_bits(ES7210_ADC34_MUTE_REG14, 0x03, 0x03);
				es7210_multi_chips_update_bits(ES7210_ADC12_MUTE_REG15, 0x03, 0x03);
			*/
		}
		else {
			if (i == 1)
			{
#if defined (CONFIG_INPUT_MIC_HW4_REF_HW1) || defined(CONFIG_INPUT_MIC_HW4_REF_HW2) ||defined( CONFIG_INPUT_MIC_HW3_REF_HW2)
				es7210_update_bits(ES7243_MUTECTL_REG05, 0x08, 0x00, regmap_clt[i]);
#endif
			}
			else
			{
				es7210_update_bits(ES7210_ADC34_MUTE_REG14, 0x03, 0x00, regmap_clt[i]);
				es7210_update_bits(ES7210_ADC12_MUTE_REG15, 0x03, 0x00, regmap_clt[i]);
			}

			/*
				es7210_multi_chips_update_bits(ES7210_ADC34_MUTE_REG14, 0x03, 0x00);
				es7210_multi_chips_update_bits(ES7210_ADC12_MUTE_REG15, 0x03, 0x00);
			*/
		}
	}
	return 0;
}

static int es7210_pcm_startup(struct snd_pcm_substream *substream,
struct snd_soc_dai *dai)
{

	struct snd_soc_codec *codec = dai->codec;
	struct es7210_priv *es7210 = snd_soc_codec_get_drvdata(codec);
	u8 val12 = 0, val34 = 0, val13 = 0;

	pr_debug("enter into %s\n", __func__);
	//es7210_read(ES7210_ADC34_MUTE_REG14, &val34, regmap_clt[0]);
	//es7210_read(ES7210_ADC12_MUTE_REG15, &val12, regmap_clt[0]);
	//es7210_read(ES7210_ADC_AUTOMUTE_REG13, &val13, regmap_clt[0]);

	pr_debug("MUTE_REG15 = %d, MUTE_REG14 = %d, AUTOMUTE_REG13 = %d\n", val12, val34, val13);
	if (es7210_init_reg == 0 || es7243_init_reg == 0) {
		schedule_delayed_work(&es7210->pcm_pop_work, msecs_to_jiffies(100));
	}
	else if( es7210_init_reg )
	{
		es7210_multi_chips_update_bits(ES7210_ADC34_MUTE_REG14, 0x03, 0x00);
		es7210_multi_chips_update_bits(ES7210_ADC12_MUTE_REG15, 0x03, 0x00);
#if defined (CONFIG_INPUT_MIC_HW4_REF_HW1) || defined(CONFIG_INPUT_MIC_HW4_REF_HW2) ||defined( CONFIG_INPUT_MIC_HW3_REF_HW2)		
		es7210_update_bits(ES7243_MUTECTL_REG05, 0x08, 0x00, regmap_clt[0]);
#endif
	}
	es7210_read(ES7210_ADC34_MUTE_REG14, &val34, regmap_clt[0]);
	es7210_read(ES7210_ADC12_MUTE_REG15, &val12, regmap_clt[0]);
	es7210_read(ES7210_ADC_AUTOMUTE_REG13, &val13, regmap_clt[0]);

	pr_debug("MUTE_REG15 = %d, MUTE_REG14 = %d, AUTOMUTE_REG13 = %d\n", val12, val34, val13);
	if (es7210_init_reg == 0 || es7243_init_reg == 0 || val12 == 0x03 || val34 == 0x03 ) {
		schedule_delayed_work(&es7210->pcm_pop_work, msecs_to_jiffies(100));
	}

	return 0;
}

static int es7210_pcm_hw_params(struct snd_pcm_substream *substream,
struct snd_pcm_hw_params *params,
struct snd_soc_dai *dai)
{
	u8 i = 0;

	for (i = 0; i < ADC_DEV_MAXNUM; i++){
		if (i == 1) {
#if defined (CONFIG_INPUT_MIC_HW4_REF_HW1) || defined(CONFIG_INPUT_MIC_HW4_REF_HW2) ||defined( CONFIG_INPUT_MIC_HW3_REF_HW2)			
			/* bit size */
			switch (params_format(params)) {
			case SNDRV_PCM_FORMAT_S16_LE:
				es7210_update_bits(ES7243_SDPFMT_REG01, 0x1c, 0x0c, regmap_clt[i]);
				break;
			case SNDRV_PCM_FORMAT_S20_3LE:
				es7210_update_bits(ES7243_SDPFMT_REG01, 0x1c, 0x04, regmap_clt[i]);
				break;
			case SNDRV_PCM_FORMAT_S24_LE:
				es7210_update_bits(ES7243_SDPFMT_REG01, 0x1c, 0x00, regmap_clt[i]);
				break;
			case SNDRV_PCM_FORMAT_S32_LE:
				es7210_update_bits(ES7243_SDPFMT_REG01, 0x1c, 0x10, regmap_clt[i]);
				break;
			}
#endif			
		}
		else
		{
			switch (params_format(params))
			{
			case SNDRV_PCM_FORMAT_S16_LE:
				es7210_update_bits(ES7210_SDP_CFG1_REG11, 0xE0, 0x60, regmap_clt[0]);
				break;
			case SNDRV_PCM_FORMAT_S20_3LE:
				es7210_update_bits(ES7210_SDP_CFG1_REG11, 0xE0, 0x20, regmap_clt[0]);
				break;
			case SNDRV_PCM_FORMAT_S24_LE:
				es7210_update_bits(ES7210_SDP_CFG1_REG11, 0xE0, 0x00, regmap_clt[0]);
				break;
			case SNDRV_PCM_FORMAT_S32_LE:
				es7210_update_bits(ES7210_SDP_CFG1_REG11, 0xE0, 0x80, regmap_clt[0]);
				break;
			}
		}
	}
	return 0;
}

#define es7210_RATES SNDRV_PCM_RATE_8000_96000

#define es7210_FORMATS (SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S20_3LE |\
	SNDRV_PCM_FMTBIT_S24_LE)

static struct snd_soc_dai_ops es7210_ops = {
	.startup = es7210_pcm_startup,
	.hw_params = es7210_pcm_hw_params,
	.set_fmt = es7210_set_dai_fmt,
	.set_sysclk = es7210_set_dai_sysclk,
	.digital_mute = es7210_mute,
};
#if defined (CONFIG_INPUT_MIC_HW4_REF_HW1) || defined(CONFIG_INPUT_MIC_HW4_REF_HW2) ||defined( CONFIG_INPUT_MIC_HW3_REF_HW2)

/* Aute mute thershold */
	static const char *threshold_txt[] = {
		"-96dB",
		"-84dB",
		"-72dB",
		"-60dB"
	};

	/* Speed Mode Selection */
	static const char *speed_txt[] = {
		"Single Speed Mode",
		"Double Speed Mode",
		"Quard Speed Mode"
	};


	static const struct soc_enum speed_mode =
		SOC_ENUM_SINGLE(ES7243_MODECFG_REG00, 2, 3, speed_txt);

	static const struct soc_enum automute_threshold =
	SOC_ENUM_SINGLE(ES7243_MUTECTL_REG05, 1, 4, threshold_txt);

	/* Analog Input gain */
	static const char *pga_txt[] = {"not allowed","0dB", "6dB"};
	static const struct soc_enum pga_gain =
	SOC_ENUM_SINGLE(ES7243_ANACTL1_REG08, 4, 3, pga_txt);

#endif

static struct snd_soc_dai_driver es7210_dai[] = {
#if ES7210_CHANNELS_MAX > 0
	{
		.name = "ES7210 HiFi ADC 0",
		.capture = {
			.stream_name = "Capture",
			.channels_min = 1,
			.channels_max = 8,
			.rates = es7210_RATES,
			.formats = es7210_FORMATS,
		},
		.ops = &es7210_ops,
		.symmetric_rates = 1,
	},
#endif
#if ES7210_CHANNELS_MAX > 4
	{
		.name = "ES7210 HiFi ADC 1",
		.capture = {
			.stream_name = "Capture",
			.channels_min = 1,
			.channels_max = 2,
			.rates = es7210_RATES,
			.formats = es7210_FORMATS,
		},
		.ops = &es7210_ops,
		.symmetric_rates = 1,
	},
#endif
};

static int es7210_suspend(struct snd_soc_codec *codec)
{
	return 0;
}

static int es7210_resume(struct snd_soc_codec *codec)
{
	snd_soc_cache_sync(codec);

	return 0;
}

static int es7210_probe(struct snd_soc_codec *codec)
{
	
	
	struct es7210_priv *es7210 = snd_soc_codec_get_drvdata(codec);
#if defined (CONFIG_INPUT_MIC_HW4_REF_HW1) || defined(CONFIG_INPUT_MIC_HW4_REF_HW2) ||defined( CONFIG_INPUT_MIC_HW3_REF_HW2)
	int i = 0;
	char *gain = NULL;

	if((gain = strstr(arcs_cmdline, "mic_gain_ctrl=")))
	{
		gain += 14;
		printk(KERN_DEBUG "es7243 set mic gain to %s\n", gain);
		for(i = 0; i < sizeof(gain_idx); i++)
		{
			gain_idx[i] = gain[i] - '0';
			if(gain_idx[i] > (sizeof(es7243_gain_sel_table) - 1))
				gain_idx[i] = DEF_REF_GAIN_IDX;
		}
	}
	else
	{
		printk(KERN_DEBUG "es7243 set mic gain to default\n");
		for(i = 0; i < sizeof(gain_idx); i++)
			gain_idx[i] = DEF_REF_GAIN_IDX;
	}
#endif
	INIT_DELAYED_WORK(&es7210->pcm_pop_work, pcm_pop_work_events);
	es7210_tdm_init_codec(es7210->tdm_mode);
#if defined (CONFIG_INPUT_MIC_HW4_REF_HW1) || defined(CONFIG_INPUT_MIC_HW4_REF_HW2) ||defined( CONFIG_INPUT_MIC_HW3_REF_HW2)
	es7243_init_work_events();
#endif
	return 0;
}

static int es7210_remove(struct snd_soc_codec *codec)
{
	return 0;
}

static const DECLARE_TLV_DB_SCALE(mic_boost_tlv, 0, 300, 0);

#if ES7210_CHANNELS_MAX > 0
static int es7210_micboost1_setting_set(struct snd_kcontrol *kcontrol,
struct snd_ctl_elem_value *ucontrol)
{
	es7210_update_bits(0x43, 0x0F, ucontrol->value.integer.value[0], regmap_clt[0]);
	return 0;
}

static int es7210_micboost1_setting_get(struct snd_kcontrol *kcontrol,
struct snd_ctl_elem_value *ucontrol)
{
	u8 val;
	es7210_read(0x43, &val, regmap_clt[0]);
	ucontrol->value.integer.value[0] = val;
	return 0;
}

static int es7210_micboost2_setting_set(struct snd_kcontrol *kcontrol,
struct snd_ctl_elem_value *ucontrol)
{
	es7210_update_bits(0x44, 0x0F, ucontrol->value.integer.value[0], regmap_clt[0]);
	return 0;
}

static int es7210_micboost2_setting_get(struct snd_kcontrol *kcontrol,
struct snd_ctl_elem_value *ucontrol)
{
	u8 val;
	es7210_read(0x44, &val, regmap_clt[0]);
	ucontrol->value.integer.value[0] = val;
	return 0;
}

static int es7210_micboost3_setting_set(struct snd_kcontrol *kcontrol,
struct snd_ctl_elem_value *ucontrol)
{
	es7210_update_bits(0x45, 0x0F, ucontrol->value.integer.value[0], regmap_clt[0]);
	return 0;
}

static int es7210_micboost3_setting_get(struct snd_kcontrol *kcontrol,
struct snd_ctl_elem_value *ucontrol)
{
	u8 val;
	es7210_read(0x45, &val, regmap_clt[0]);
	ucontrol->value.integer.value[0] = val;
	return 0;
}
static int es7210_micboost4_setting_set(struct snd_kcontrol *kcontrol,
struct snd_ctl_elem_value *ucontrol)
{
	es7210_update_bits(0x46, 0x0F, ucontrol->value.integer.value[0], regmap_clt[0]);
	return 0;
}

static int es7210_micboost4_setting_get(struct snd_kcontrol *kcontrol,
struct snd_ctl_elem_value *ucontrol)
{
	u8 val;
	es7210_read(0x46, &val, regmap_clt[0]);
	ucontrol->value.integer.value[0] = val;
	return 0;
}

static int es7210_adc1_mute_set(struct snd_kcontrol *kcontrol,
struct snd_ctl_elem_value *ucontrol)
{
	es7210_update_bits(ES7210_ADC12_MUTE_REG15, 0x01,
		ucontrol->value.integer.value[0], regmap_clt[0]);
	return 0;
}

static int es7210_adc1_mute_get(struct snd_kcontrol *kcontrol,
struct snd_ctl_elem_value *ucontrol)
{
	u8 val;
	es7210_read(ES7210_ADC12_MUTE_REG15, &val, regmap_clt[0]);
	ucontrol->value.integer.value[0] = val;
	return 0;
}
static int es7210_adc2_mute_set(struct snd_kcontrol *kcontrol,
struct snd_ctl_elem_value *ucontrol)
{
	es7210_update_bits(ES7210_ADC12_MUTE_REG15, 0x02,
		ucontrol->value.integer.value[0], regmap_clt[0]);
	return 0;
}

static int es7210_adc2_mute_get(struct snd_kcontrol *kcontrol,
struct snd_ctl_elem_value *ucontrol)
{
	u8 val;
	es7210_read(ES7210_ADC12_MUTE_REG15, &val, regmap_clt[0]);
	ucontrol->value.integer.value[0] = val;
	return 0;
}
static int es7210_adc3_mute_set(struct snd_kcontrol *kcontrol,
struct snd_ctl_elem_value *ucontrol)
{
	es7210_update_bits(ES7210_ADC34_MUTE_REG14, 0x01,
		ucontrol->value.integer.value[0], regmap_clt[0]);
	return 0;
}

static int es7210_adc3_mute_get(struct snd_kcontrol *kcontrol,
struct snd_ctl_elem_value *ucontrol)
{
	u8 val;
	es7210_read(ES7210_ADC34_MUTE_REG14, &val, regmap_clt[0]);
	ucontrol->value.integer.value[0] = val;
	return 0;
}
static int es7210_adc4_mute_set(struct snd_kcontrol *kcontrol,
struct snd_ctl_elem_value *ucontrol)
{
	es7210_update_bits(ES7210_ADC34_MUTE_REG14, 0x02,
		ucontrol->value.integer.value[0], regmap_clt[0]);
	return 0;
}

static int es7210_adc4_mute_get(struct snd_kcontrol *kcontrol,
struct snd_ctl_elem_value *ucontrol)
{
	u8 val;
	es7210_read(ES7210_ADC34_MUTE_REG14, &val, regmap_clt[0]);
	ucontrol->value.integer.value[0] = val;
	return 0;
}

#endif

#if ES7210_CHANNELS_MAX > 8
static int es7210_micboost9_setting_set(struct snd_kcontrol *kcontrol,
struct snd_ctl_elem_value *ucontrol)
{
	es7210_update_bits(0x43, 0x0F, ucontrol->value.integer.value[0], regmap_clt[2]);
	return 0;
}

static int es7210_micboost9_setting_get(struct snd_kcontrol *kcontrol,
struct snd_ctl_elem_value *ucontrol)
{
	u8 val;
	es7210_read(0x43, &val, regmap_clt[2]);
	ucontrol->value.integer.value[0] = val;
	return 0;
}
static int es7210_micboost10_setting_set(struct snd_kcontrol *kcontrol,
struct snd_ctl_elem_value *ucontrol)
{
	es7210_update_bits(0x44, 0x0F, ucontrol->value.integer.value[0], regmap_clt[2]);
	return 0;
}

static int es7210_micboost10_setting_get(struct snd_kcontrol *kcontrol,
struct snd_ctl_elem_value *ucontrol)
{
	u8 val;
	es7210_read(0x44, &val, regmap_clt[2]);
	ucontrol->value.integer.value[0] = val;
	return 0;
}
static int es7210_micboost11_setting_set(struct snd_kcontrol *kcontrol,
struct snd_ctl_elem_value *ucontrol)
{
	es7210_update_bits(0x45, 0x0F, ucontrol->value.integer.value[0], regmap_clt[2]);
	return 0;
}

static int es7210_micboost11_setting_get(struct snd_kcontrol *kcontrol,
struct snd_ctl_elem_value *ucontrol)
{
	u8 val;
	es7210_read(0x45, &val, regmap_clt[2]);
	ucontrol->value.integer.value[0] = val;
	return 0;
}
static int es7210_micboost12_setting_set(struct snd_kcontrol *kcontrol,
struct snd_ctl_elem_value *ucontrol)
{
	es7210_update_bits(0x46, 0x0F, ucontrol->value.integer.value[0], regmap_clt[2]);
	return 0;
}

static int es7210_micboost12_setting_get(struct snd_kcontrol *kcontrol,
struct snd_ctl_elem_value *ucontrol)
{
	u8 val;
	es7210_read(0x46, &val, regmap_clt[2]);
	ucontrol->value.integer.value[0] = val;
	return 0;
}

static int es7210_adc9_mute_set(struct snd_kcontrol *kcontrol,
struct snd_ctl_elem_value *ucontrol)
{
	es7210_update_bits(ES7210_ADC12_MUTE_REG15, 0x01,
		ucontrol->value.integer.value[0], regmap_clt[2]);
	return 0;
}

static int es7210_adc9_mute_get(struct snd_kcontrol *kcontrol,
struct snd_ctl_elem_value *ucontrol)
{
	u8 val;
	es7210_read(ES7210_ADC12_MUTE_REG15, &val, regmap_clt[2]);
	ucontrol->value.integer.value[0] = val;
	return 0;
}
static int es7210_adc10_mute_set(struct snd_kcontrol *kcontrol,
struct snd_ctl_elem_value *ucontrol)
{
	es7210_update_bits(ES7210_ADC12_MUTE_REG15, 0x02,
		ucontrol->value.integer.value[0], regmap_clt[2]);
	return 0;
}

static int es7210_adc10_mute_get(struct snd_kcontrol *kcontrol,
struct snd_ctl_elem_value *ucontrol)
{
	u8 val;
	es7210_read(ES7210_ADC12_MUTE_REG15, &val, regmap_clt[2]);
	ucontrol->value.integer.value[0] = val;
	return 0;
}
static int es7210_adc11_mute_set(struct snd_kcontrol *kcontrol,
struct snd_ctl_elem_value *ucontrol)
{
	es7210_update_bits(ES7210_ADC34_MUTE_REG14, 0x01,
		ucontrol->value.integer.value[0], regmap_clt[2]);
	return 0;
}

static int es7210_adc11_mute_get(struct snd_kcontrol *kcontrol,
struct snd_ctl_elem_value *ucontrol)
{
	u8 val;
	es7210_read(ES7210_ADC34_MUTE_REG14, &val, regmap_clt[2]);
	ucontrol->value.integer.value[0] = val;
	return 0;
}
static int es7210_adc12_mute_set(struct snd_kcontrol *kcontrol,
struct snd_ctl_elem_value *ucontrol)
{
	es7210_update_bits(ES7210_ADC34_MUTE_REG14, 0x02,
		ucontrol->value.integer.value[0], regmap_clt[2]);
	return 0;
}

static int es7210_adc12_mute_get(struct snd_kcontrol *kcontrol,
struct snd_ctl_elem_value *ucontrol)
{
	u8 val;
	es7210_read(ES7210_ADC34_MUTE_REG14, &val, regmap_clt[2]);
	ucontrol->value.integer.value[0] = val;
	return 0;
}
#endif

static const struct snd_kcontrol_new es7210_snd_controls[] = {
#if ES7210_CHANNELS_MAX > 0
	SOC_SINGLE_EXT_TLV("PGA1_setting",
	0x43, 0, 0x0F, 0,
	es7210_micboost1_setting_get, es7210_micboost1_setting_set,
	mic_boost_tlv),
	SOC_SINGLE_EXT_TLV("PGA2_setting",
	0x44, 0, 0x0F, 0,
	es7210_micboost2_setting_get, es7210_micboost2_setting_set,
	mic_boost_tlv),
	SOC_SINGLE_EXT_TLV("PGA3_setting",
	0x45, 0, 0x0F, 0,
	es7210_micboost3_setting_get, es7210_micboost3_setting_set,
	mic_boost_tlv),
	SOC_SINGLE_EXT_TLV("PGA4_setting",
	0x46, 0, 0x0F, 0,
	es7210_micboost4_setting_get, es7210_micboost4_setting_set,
	mic_boost_tlv),
	SOC_SINGLE_EXT("ADC1_MUTE", ES7210_ADC12_MUTE_REG15, 0, 1, 0,
	es7210_adc1_mute_get, es7210_adc1_mute_set),
	SOC_SINGLE_EXT("ADC2_MUTE", ES7210_ADC12_MUTE_REG15, 1, 1, 0,
	es7210_adc2_mute_get, es7210_adc2_mute_set),
	SOC_SINGLE_EXT("ADC3_MUTE", ES7210_ADC34_MUTE_REG14, 0, 1, 0,
	es7210_adc3_mute_get, es7210_adc3_mute_set),
	SOC_SINGLE_EXT("ADC4_MUTE", ES7210_ADC34_MUTE_REG14, 1, 1, 0,
	es7210_adc4_mute_get, es7210_adc4_mute_set),
#endif
#if ES7210_CHANNELS_MAX > 4

	SOC_ENUM("Input PGA", pga_gain),
	SOC_SINGLE("ADC Mute", ES7243_MUTECTL_REG05, 3, 1, 0),
	SOC_SINGLE("AutoMute Enable", ES7243_MUTECTL_REG05, 0, 1, 1),
	SOC_ENUM("AutoMute Threshold", automute_threshold),
	SOC_SINGLE("TRI State Output", ES7243_STATECTL_REG06, 7, 1, 0),
	SOC_SINGLE("MCLK Disable", ES7243_STATECTL_REG06, 6, 1, 0),
	SOC_SINGLE("Reset ADC Digital", ES7243_STATECTL_REG06, 3, 1, 0),
	SOC_SINGLE("Reset All Digital", ES7243_STATECTL_REG06, 4, 1, 0),
	SOC_SINGLE("Master Mode", ES7243_MODECFG_REG00, 1, 1, 0),
	SOC_SINGLE("Software Mode", ES7243_MODECFG_REG00, 0, 1, 0),
	SOC_ENUM("Speed Mode", speed_mode),
	SOC_SINGLE("High Pass Filter Disable", ES7243_MODECFG_REG00, 4, 1, 0),
	SOC_SINGLE("TDM Mode", ES7243_SDPFMT_REG01, 7, 1, 0),
	SOC_SINGLE("BCLK Invertor", ES7243_SDPFMT_REG01, 6, 1, 0),
	SOC_SINGLE("LRCK Polarity Set", ES7243_SDPFMT_REG01, 5, 1, 0),
	SOC_SINGLE("Analog Power down", ES7243_ANACTL2_REG09, 7, 1, 0),
#endif
};

static unsigned int es7210_codec_read(struct snd_soc_codec *codec, unsigned int reg)
{
	u8 val_r;
	struct es7210_priv *es7210 = dev_get_drvdata(codec->dev);

	pr_debug("es7210_codec_read, reg = %d\n", reg);
	es7210_read(reg, &val_r, es7210->regmap);
	return val_r;
}

static int es7210_codec_write(struct snd_soc_codec *codec, unsigned int reg, unsigned int value)
{
	struct es7210_priv *es7210 = dev_get_drvdata(codec->dev);

	pr_debug("es7210_codec_write, reg = %d\n", reg);
	es7210_write(reg, value, es7210->regmap);
	return 0;
}

static struct snd_soc_codec_driver soc_codec_dev_es7210 = {
	.probe = es7210_probe,
	.remove = es7210_remove,
	.suspend = es7210_suspend,
	.resume = es7210_resume,

	.read = es7210_codec_read,
	.write = es7210_codec_write,

	.reg_word_size = sizeof(u8),
	.reg_cache_default = es7210_reg_defaults,
	.reg_cache_size = ARRAY_SIZE(es7210_reg_defaults),
	.controls = es7210_snd_controls,
	.num_controls = ARRAY_SIZE(es7210_snd_controls),
};

static ssize_t es7210_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	int val = 0, flag = 0;
	u8 i = 0, reg, num, value_w, value_r;
	struct es7210_priv *es7210 = dev_get_drvdata(dev);

	pr_debug("enter into %s\n", __func__);
	val = simple_strtol(buf, NULL, 16);
	flag = (val >> 16) & 0xFF;

	if (flag) {
		reg = (val >> 8) & 0xFF;
		value_w = val & 0xFF;
		pr_debug("Write: start REG:0x%02x,val:0x%02x,count:0x%02x\n", reg, value_w, flag);
		while (flag--) {
			es7210_write(reg, value_w, es7210->regmap);
			pr_debug("Write 0x%02x to REG:0x%02x\n", value_w, reg);
			reg++;
		}
	}
	else {
		reg = (val >> 8) & 0xFF;
		num = val & 0xff;
		pr_debug("Read: start REG:0x%02x,count:0x%02x\n", reg, num);
		do {
			value_r = 0;
			es7210_read(reg, &value_r, es7210->regmap);
			pr_debug("REG[0x%02x]: 0x%02x;  ", reg, value_r);
			reg++;
			i++;
			if ((i == num) || (i % 4 == 0))
				pr_debug("\n");
		} while (i < num);
	}

	return count;
}

static ssize_t es7210_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	pr_debug("echo flag|reg|val > es7210\n");
	pr_debug("eg read star addres=0x06,count 0x10:echo 0610 >es7210\n");
	pr_debug("eg write star addres=0x90,value=0x3c,count=4:echo 4903c >es7210\n");
	//pr_debug("eg write value:0xfe to address:0x06 :echo 106fe > es7210\n");
	return 0;
}

static DEVICE_ATTR(es7210, 0644, es7210_show, es7210_store);

static struct attribute *es7210_debug_attrs[] = {
	&dev_attr_es7210.attr,
	NULL,
};

static struct attribute_group es7210_debug_attr_group = {
	.name = "es7210_debug",
	.attrs = es7210_debug_attrs,
};

/*
 * If the i2c layer weren't so broken, we could pass this kind of data
 * around
 */
extern int micarray_channels;
static int es7210_i2c_probe(struct i2c_client *i2c,
	const struct i2c_device_id *i2c_id)
{
	struct es7210_priv *es7210 = NULL;
	int ret;

	pr_debug("begin->>>>>>>>>>%s!\n", __func__);
	micarray_channels = ES7210_CHANNELS_MAX;

	es7210 = devm_kzalloc(&i2c->dev, sizeof(struct es7210_priv), GFP_KERNEL);
	if (es7210 == NULL) {
		dev_err(&i2c->dev, "failed devm_kzalloc");
		return -ENOMEM;
	}
	es7210->i2c = i2c;
	es7210->tdm_mode = ES7210_WORK_MODE;  //to set tdm mode or normal mode
	dev_set_drvdata(&i2c->dev, es7210);
	if (i2c_id->driver_data < ADC_DEV_MAXNUM) {
		pr_err("i2c_id->driver_data %ld, addr = 0x%x\n", i2c_id->driver_data, i2c->addr);
		es7210->regmap = devm_regmap_init_i2c(i2c, &es7210_regmap_config);
		regmap_clt[i2c_id->driver_data] = es7210->regmap;
		if (IS_ERR(es7210->regmap)) {
			ret = PTR_ERR(es7210->regmap);
			dev_err(&i2c->dev, "Failed to allocate register map: %d\n",
				ret);
			return ret;
		}
		ret = snd_soc_register_codec(&i2c->dev, &soc_codec_dev_es7210,
			&es7210_dai[i2c_id->driver_data], 1);
		if (ret < 0) {
			pr_debug("snd_soc_register_codec failed ret = %d\n", ret);
			kfree(es7210);
			return ret;
		}
	}
	ret = sysfs_create_group(&i2c->dev.kobj, &es7210_debug_attr_group);
	if (ret) {
		pr_err("failed to create attr group\n");
	}
	return ret;
}
static int __exit es7210_i2c_remove(struct i2c_client *i2c)
{
	snd_soc_unregister_codec(&i2c->dev);
	kfree(i2c_get_clientdata(i2c));
	return 0;
}

static int es7210_i2c_detect(struct i2c_client *client, struct i2c_board_info *info)
{
	struct i2c_adapter *adapter = client->adapter;

	if (adapter->nr == ES7210_I2C_BUS_NUM) {

		if (client->addr == 0x40) {
			strlcpy(info->type, "MicArray_0", I2C_NAME_SIZE);
			return 0;
		}
		else if (client->addr == 0x13) {
			strlcpy(info->type, "MicArray_1", I2C_NAME_SIZE);
			return 0;
		}
		else if (client->addr == 0x42) {
			strlcpy(info->type, "MicArray_2", I2C_NAME_SIZE);
			return 0;
		}
		else if (client->addr == 0x41) {
			strlcpy(info->type, "MicArray_3", I2C_NAME_SIZE);
			return 0;
		}
	}

	return -ENODEV;
}

static const unsigned short es7210_i2c_addr[] = {
/*
#if ES7210_CHANNELS_MAX > 0
	0x40,
#endif
*/

#if ES7210_CHANNELS_MAX > 4
	0x13,
#endif

	I2C_CLIENT_END,
};


static const struct i2c_device_id es7210_i2c_id[] = {
#if ES7210_CHANNELS_MAX > 0
	{ "MicArray_0", 0 },//es7210_0
#endif

#if ES7210_CHANNELS_MAX > 4
	{ "MicArray_1", 1 },//es7210_1
#endif

	{}
};
MODULE_DEVICE_TABLE(i2c, es7210_i2c_id);

static const struct of_device_id es7210_dt_ids[] = {
#if ES7210_CHANNELS_MAX > 0
	{ .compatible = "MicArray_0", },//es7210_0
#endif

#if ES7210_CHANNELS_MAX > 4
	{ .compatible = "MicArray_1", },//es7210_1
#endif

};
MODULE_DEVICE_TABLE(of, es7210_dt_ids);

static struct i2c_driver es7210_i2c_driver = {
	.driver = {
		.name = "es7210-audio-adc",
		.owner = THIS_MODULE,
		.of_match_table = es7210_dt_ids,

	},
	.probe = es7210_i2c_probe,
	.remove = __exit_p(es7210_i2c_remove),
	.class = I2C_CLASS_HWMON,
	.id_table = es7210_i2c_id,
#if !ES7210_MATCH_DTS_EN
	.address_list = es7210_i2c_addr,
	.detect = es7210_i2c_detect,
#endif
};


static int __init es7210_modinit(void)
{
	int ret;

	pr_debug("%s enter es7210\n", __func__);

	ret = i2c_add_driver(&es7210_i2c_driver);
	if (ret != 0)
		pr_err("Failed to register es7210 i2c driver : %d \n", ret);
	return ret;
}
module_init(es7210_modinit);

static void __exit es7210_exit(void)
{
	i2c_del_driver(&es7210_i2c_driver);
}

module_exit(es7210_exit);
MODULE_DESCRIPTION("ASoC ES7210 audio adc driver");
MODULE_AUTHOR("David Yang <yangxiaohua@everest-semi.com> / info@everest-semi.com");
MODULE_LICENSE("GPL v2");

