/*
 * Copyright 2005 Openedhand Ltd.
 *
 * Author: Richard Purdie <richard@openedhand.com>
 *
 * Based on WM8753.h
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#ifndef _ES9023_H
#define _ES9023_H

#include <linux/platform_device.h>
#include <linux/types.h>
#include <linux/notifier.h>
#include <linux/workqueue.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/regmap.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/control.h>
#include <sound/ac97_codec.h>


#define AT_VOL "AXX+VOL+"

/* ES9023 register space */

#define ES9023_LINVOL    0x00
#define ES9023_RINVOL    0x01
#define ES9023_LOUT1V    0x02
#define ES9023_ROUT1V    0x03
#define ES9023_ADCDAC    0x05
#define ES9023_IFACE     0x07
#define ES9023_SRATE     0x08
#define ES9023_LDAC      0x0a
#define ES9023_RDAC      0x0b
#define ES9023_BASS      0x0c
#define ES9023_TREBLE    0x0d
#define ES9023_RESET     0x0f
#define ES9023_3D        0x10
#define ES9023_ALC1      0x11
#define ES9023_ALC2      0x12
#define ES9023_ALC3      0x13
#define ES9023_NGATE     0x14
#define ES9023_LADC      0x15
#define ES9023_RADC      0x16
#define ES9023_ADCTL1    0x17
#define ES9023_ADCTL2    0x18
#define ES9023_PWR1      0x19
#define ES9023_PWR2      0x1a
#define ES9023_ADCTL3    0x1b
#define ES9023_ADCIN     0x1f
#define ES9023_LADCIN    0x20
#define ES9023_RADCIN    0x21
#define ES9023_LOUTM1    0x22
#define ES9023_LOUTM2    0x23
#define ES9023_ROUTM1    0x24
#define ES9023_ROUTM2    0x25
#define ES9023_MOUTM1    0x26
#define ES9023_MOUTM2    0x27
#define ES9023_LOUT2V    0x28
#define ES9023_ROUT2V    0x29
#define ES9023_MOUTV     0x2a

#define ES9023_CACHE_REGNUM 0x2a

#define ES9023_SYSCLK	0


#define ES9023_SOC_DOUBLE_R(xname, reg_left, reg_right, xshift, xmax, xinvert) \
{	.iface = SNDRV_CTL_ELEM_IFACE_MIXER, .name = (xname), \
	.info = es9023_snd_soc_info_volsw, \
	.get = es9023_snd_soc_get_volsw, .put = es9023_snd_soc_put_volsw, \
	.private_value = SOC_DOUBLE_R_VALUE(reg_left, reg_right, xshift, \
					    xmax, xinvert) }




#define ES9023_SOC_ENUM(xname, xenum) \
{	.iface = SNDRV_CTL_ELEM_IFACE_MIXER, .name = xname, \
	.info = es9023_snd_soc_info_adcin, .get = es9023_snd_soc_get_adcin,\
	.put = es9023_snd_soc_put_adcin, \
	.private_value = (unsigned long)&xenum  }

#endif
