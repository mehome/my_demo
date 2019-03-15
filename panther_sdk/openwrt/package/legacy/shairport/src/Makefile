ifeq ($(wildcard config.mk),)
$(warning config.mk does not exist, configuring.)
config.mk:
	sh ./configure
	$(MAKE) shairport
endif

-include config.mk

PREFIX ?= /usr/local

SRCS := shairport.c daemon.c rtsp.c mdns.c mdns_tinysvcmdns.c common.c rtp.c player.c alac.c audio.c audio_dummy.c audio_pipe.c tinysvcmdns.c 

AIR_CLI := air_cli.c

ifdef CONFIG_SNDIO
SRCS += audio_sndio.c
endif

ifdef CONFIG_AO
SRCS += audio_ao.c
endif

ifdef CONFIG_PULSE
SRCS += audio_pulse.c
endif

ifdef CONFIG_ALSA
SRCS += audio_alsa.c
endif

ifdef CONFIG_AVAHI
SRCS += mdns_avahi.c
endif

ifdef CONFIG_HAVE_DNS_SD_H
SRCS += mdns_dns_sd.c
endif

ifndef CONFIG_HAVE_GETOPT_H
SRCS += getopt_long.c
endif

# default target
all: shairport air_cli

install: shairport
	install -m 755 -d $(PREFIX)/bin
	install -m 755 shairport $(PREFIX)/bin/shairport

shairport: $(SRCS) config.h config.mk
	$(CC) $(CFLAGS) $(SRCS) $(LDFLAGS) -o shairport

air_cli: $(AIR_CLI)
	$(CC) $(CFLAGS) $(AIR_CLI) -o air_cli 

clean:
	rm -f shairport
