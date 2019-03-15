# Cheetah Platform Support

BACKPORT_PWD := $(shell pwd)/net/compat-wireless-2013-06-27
NOSTDINC_FLAGS := \
	-I$(BACKPORT_PWD)/include/ \
	-I$(BACKPORT_PWD)/include/uapi \
	-I$(BACKPORT_PWD)/include/drm \
	$(CFLAGS)

snd-aloop-objs := purin-aloop.o
#obj-$(CONFIG_PANTHER_SND) += cta-ai.o cta-i2s.o cta-pcm.o
obj-$(CONFIG_PANTHER_SND) += purin-asp-aloop.o
obj-$(CONFIG_PANTHER_SND) += purin-audio-dai.o purin-dma.o
obj-$(CONFIG_PANTHER_SND) += purin-spdif-dai.o codec/spdif.o
obj-$(CONFIG_PANTHER_SND) += purin-dac-dai.o codec/dac.o
obj-$(CONFIG_PANTHER_SND) += purin-adc-dai.o codec/adc.o
obj-$(CONFIG_PANTHER_SND_PCM0_DUMMY) += codec/dummy.o
obj-$(CONFIG_PANTHER_SND_PCM0_NAU8822) += codec/nau8822.o
obj-$(CONFIG_PANTHER_SND_PCM0_WM8988) += codec/wm8988.o
obj-$(CONFIG_PANTHER_SND_PCM0_WM8750) += codec/wm8750.o
obj-$(CONFIG_PANTHER_SND_PCM0_TAS5711) += codec/tas5711.o
obj-$(CONFIG_PANTHER_SND_PCM0_ES8316) += codec/es8316.o
obj-$(CONFIG_PANTHER_SND_PCM0_ES9023) += codec/es9023_cchip.o
obj-$(CONFIG_PANTHER_SND_PCM0_MP320) += codec/mp320.o
obj-$(CONFIG_PANTHER_SND_PCM0_WM8737) += codec/wm8737.o
obj-$(CONFIG_PANTHER_SND_PCM0_WM8737_AD83586) += codec/wm8737.o codec/ad83586.o
obj-$(CONFIG_PANTHER_SND_PCM0_TLV320AIC3111) += codec/tlv320aic31xx.o
obj-$(CONFIG_PANTHER_SND_PCM0_ESS9018Q2C) += codec/es9018k2m.o
obj-$(CONFIG_PANTHER_SND_PCM0_ES7243) += codec/es7243.o
obj-$(CONFIG_PANTHER_SND_PCM0_ES7210) += codec/es7210.o
obj-$(CONFIG_PANTHER_SND) += purin-internal-card.o purin-external-card.o
obj-$(CONFIG_SND_ALOOP) += snd-aloop.o
