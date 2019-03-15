#
# Copyright (C) 2007-2016 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=mpd
PKG_VERSION:=0.19.19
PKG_RELEASE:=1

#PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.xz
#PKG_SOURCE_URL:=http://www.musicpd.org/download/mpd/0.19/
#PKG_MD5SUM:=47e13f3f160bf94e7a897c5a48990f3d
#PKG_MAINTAINER:=Ted Hess <thess@kitschensync.net>

#PKG_LICENSE:=GPL-2.0
#PKG_LICENSE_FILES:=COPYING

PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)-$(BUILD_VARIANT)/$(PKG_NAME)-$(PKG_VERSION)
PKG_BUILD_PARALLEL:=1

PKG_CONFIG_DEPENDS:= \
	CONFIG_IPV6 \

PKG_INSTALL:=1

#USE_SOURCE_DIR:=$(CURDIR)/mpd-0.19.19

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/nls.mk

define Package/mpd/Default
  SECTION:=sound
  CATEGORY:=Sound
  TITLE:=Music Player Daemon
  URL:=http://www.musicpd.org/
#  DEPENDS:= 
endef

define Package/mpd/Default/description
 Music Player Daemon (MPD) is a flexible, powerful, server-side
 application for playing music. It is typically controlled over a
 network using one of it's many clients including mpc (console),
 gmpc (gnome), phpmp (php), etc...
endef

define Package/mpd-mini
$(call Package/mpd/Default)
  TITLE+= (mini)
  DEPENDS:= +libcurl +libpthread +libmpdclient +libstdcpp  $(ICONV_DEPENDS) \
	+AUDIO_SUPPORT:alsa-lib +libexpat +libvorbisidec \
	+libmms +libffmpeg-custom +libopensssl +libglib\
	+libbz2 +libcchip +librt \
	+wdk
  PROVIDES:=mpd
  VARIANT:=mini
endef

define Package/mpd-mini/description
$(call Package/mpd/Default/description)
 .
 This package contains a minimal Music Player Daemon, with support for
 only Flac, MP3 & OGG media types & only file: & http: protocols.
endef

define Package/mpd-mini/conffiles
/etc/mpd.conf
endef

define Package/mpd-avahi-service
$(call Package/mpd/Default)
  TITLE+= (Avahi service)
  DEPENDS+=+avahi-daemon
endef

define Package/mpd-avahi-service/description
$(call Package/mpd/Default/description)
 .
 This package contains the service definition for announcing the
 Music Player Daemon service via mDNS/DNS-SD.
endef

define Package/mpd-avahi-service/conffiles
/etc/avahi/services/mpd.service
endef

ifeq ($(BUILD_VARIANT),mini)

CONFIGURE_ARGS += \
	$(call autoconf_bool,CONFIG_IPV6,ipv6) \
	--disable-debug \
	--disable-documentation \
	--disable-test \
	--disable-werror \
	\
	--disable-ao \
	--disable-bzip2 \
	--disable-fluidsynth \
	--disable-gme \
	--disable-inotify \
	--disable-icu \
	--disable-nfs \
	--disable-smbclient \
	--disable-eventfd \
	--disable-iso9660 \
	--disable-jack \
	--disable-lame-encoder \
	--disable-libwrap \
	--disable-lsr \
	--disable-mikmod \
	--disable-modplug \
	--disable-mpc \
	--disable-mpg123 \
	--disable-adplug \
	--disable-openal \
	--disable-opus \
	--disable-dsd \
	--disable-pulse \
	--disable-mad \
	--disable-sidplay \
	--disable-solaris-output \
	--disable-sqlite \
	--disable-twolame-encoder \
	--disable-wave-encoder \
	--disable-wavpack \
	--disable-wildmidi \
	--without-boost \
	--disable-zzip \
	--with-zeroconf=no \
	--disable-soxr \
	\
	--enable-curl \
	--disable-flac \
	--disable-glib \
	--disable-httpd-output \
	$(call autoconf_bool,CONFIG_AUDIO_SUPPORT,alsa) \
	--enable-tcp \
	--enable-un \

CONFIGURE_VARS += \
	$(if $(CONFIG_BUILD_PATENTED),MMS_CFLAGS="-I$(STAGING_DIR)/usr/include") \
	$(if $(CONFIG_BUILD_PATENTED),MMS_LIBS="-L$(STAGING_DIR)/usr/lib -lmms") \

TARGET_CFLAGS += -ffunction-sections -fdata-sections -I$(STAGING_DIR)/usr/include \
        -I$(STAGING_DIR)/usr/include/wdk \
        -DMONTAGE_MPD_PATCH \
		    -DMONTAGE_ENABLE_DLNA_PAUSE \
		    -DMONTAGE_ENABLE_LINKIP \
		    -D__PANTHER__ \
#   -DMONTAGE_CTRL_PLAYER \
#		-DMONTAGE_MPD_CMD_TRIGGER

ifeq ($(CONFIG_PACKAGE_iot),y)
TARGET_CFLAGS += -DTURING_MPD_CHANGE 
endif
ifeq ($(CONFIG_PACKAGE_IOT_SOFTWARE_VOLUME),y)
TARGET_CFLAGS += -DUSE_SOFTWARE_VOLUME
endif
TARGET_LDFLAGS += -Wl,-gc-sections  \
	  -Wl,-rpath-link=$(STAGING_DIR)/usr/lib \
        $(if $(ICONV_FULL),-liconv) \
        -lcdb -lm -lcrypto -lssl -lvorbisidec -lfdk-aac -ldl -lcchip -lpthread -lrt

  # oggflac is not compatible with tremor
  CONFIGURE_ARGS += \
	--disable-upnp \
	--disable-aac \
	--disable-audiofile \
	--enable-fifo \
	--enable-ffmpeg \
	--disable-id3 \
	--enable-mms \
	--disable-oggflac \
	--disable-pipe-output \
	--disable-recorder-output \
	--disable-osx \
	--disable-shout \
	--disable-sndfile \
	--disable-vorbis \
	--disable-vorbis-encoder \
	--with-tremor=$(STAGING_DIR)/usr \

endif

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	#$(CP) ./src/* $(PKG_BUILD_DIR)/
	$(CP) ./mpd-0.19.19/* $(PKG_BUILD_DIR)/
	$(CP) ./files/boost $(STAGING_DIR)/usr/include
	$(call TSYNC,$(PKG_BUILD_DIR))
endef

define Package/mpd/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_INSTALL_DIR)/usr/bin/mpd $(1)/usr/bin/mpdaemon
	$(INSTALL_DIR) $(1)/etc
	$(INSTALL_CONF) ./files/mpd.conf $(1)/etc/mpd.conf
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) ./files/mpd.init $(1)/etc/init.d/mpd
	$(INSTALL_DIR) $(1)/etc/avahi/services
	$(INSTALL_DATA) ./files/mpd.service $(1)/etc/avahi/services/
#	$(CP) ./files/playlists $(1)/
	$(INSTALL_BIN) ./files/upmpdcliChangeName.sh $(1)/usr/bin/
endef

define Package/mpd-mini/install
$(call Package/mpd/install,$1)
endef

define Package/mpd-avahi-service/install
	$(INSTALL_DIR) $(1)/etc/avahi/services
	$(INSTALL_DATA) ./files/mpd.service $(1)/etc/avahi/services/
endef

$(eval $(call BuildPackage,mpd-mini))
#$(eval $(call BuildPackage,mpd-avahi-service))
