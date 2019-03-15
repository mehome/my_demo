#
# Copyright (C) 2006-2013 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=ffmpeg
PKG_VERSION:=3.2.10
PKG_RELEASE:=2

#PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.bz2
#PKG_SOURCE_URL:=http://ffmpeg.org/releases/
#PKG_MD5SUM:=93c1908022567b321df74db7214da6ac

USE_SRC_DIR:=1
PRIVATE:=1

PKG_LICENSE:=LGPLv2.1+ GPLv2+ LGPLv3
PKG_LICENSE_FILES:=COPYING.GPLv2 COPYING.GPLv3 COPYING.LGPLv2.1 COPYING.LGPLv3

FFMPEG_CUSTOM_ENCODERS:= \

FFMPEG_CUSTOM_DECODERS:= \
	amrnb \
	amrwb \
	ape	\
	flac \
	wmav1 \
	wmav2 \
	wmapro \
	wmalossless \
	pcm_s16le \
	pcm_s16be \
	pcm_s24le \
	pcm_s24be \
	wav \
	flac \
	mp1 \
	mp2 \
	mp3 \

FFMPEG_CUSTOM_MUXERS:= \
	amr \

FFMPEG_CUSTOM_DEMUXERS:= \
	amr \
	aac \
	ape \
	asf \
	ffm \
	sdp \
	ac3 \
	flac \
	mp3 \
	mov \
	mpegts \
	mpegps \
	h264 \
	wav \
	xwma \
	ogg \
	pcm_s16le \
	pcm_s16be \
	pcm_s24le \
	pcm_s24be \
	hls \
	rtsp \

FFMPEG_CUSTOM_PARSERS:= \
	aac \
	flac \
	h264 \
	mpegaudio \
	mpegaudiodsp \
	vorbis \

FFMPEG_CUSTOM_PROTOCOLS:= \
	file \
	http \
	https \
	mmsh \
	mmst \
	hls \

FFMPEG_CUSTOM_FILTERS:= \

FFMPEG_CUSTOM_INDEVS:= \
	alsa

PKG_CONFIG_DEPENDS:= \
	$(patsubst %,CONFIG_FFMPEG_CUSTOM_ENCODER_%,$(FFMPEG_CUSTOM_ENCODERS)) \
	$(patsubst %,CONFIG_FFMPEG_CUSTOM_DECODER_%,$(FFMPEG_CUSTOM_DECODERS)) \
	$(patsubst %,CONFIG_FFMPEG_CUSTOM_MUXER_%,$(FFMPEG_CUSTOM_DEMUXERS)) \
	$(patsubst %,CONFIG_FFMPEG_CUSTOM_DEMUXER_%,$(FFMPEG_CUSTOM_DEMUXERS)) \
	$(patsubst %,CONFIG_FFMPEG_CUSTOM_PARSER_%,$(FFMPEG_CUSTOM_PARSERS)) \
	$(patsubst %,CONFIG_FFMPEG_CUSTOM_PROTOCOL_%,$(FFMPEG_CUSTOM_PROTOCOLS))

define Package/ffmpeg/Default
 TITLE:=FFmpeg
 URL:=http://ffmpeg.mplayerhq.hu/
endef

define Package/ffmpeg/Default/description
 FFmpeg is a a software package that can record, convert and stream digital 
 audio and video in numerous formats.
endef


define Package/ffmpeg
$(call Package/ffmpeg/Default)
 SECTION:=multimedia
 CATEGORY:=Multimedia
 TITLE+= program
 DEPENDS+= +libpthread +libopenssl
endef

define Package/ffmpeg/description
$(call Package/ffmpeg/Default/description)
 .
 This package contains the FFmpeg command line tool.
endef

TARGET_CFLAGS += -Dattribute_deprecated="" -ffast-math

TARGET_CFLAGS += -D__PANTHER__ -DCONFIG_PANTHER -DCONFIG_PANTHER_DSP -mdspr2
#TARGET_CFLAGS += -ggdb

ifeq ($(CONFIG_FFMPEG_CUSTOM_WIFISPK_SUPPORT),y)
  TARGET_CFLAGS += -DMONTAGE_AV_PATCH
endif
ifeq ($(CONFIG_FFMPEG_CUSTOM_MINIDLNA_SUPPORT),y)
  TARGET_CFLAGS += -DMONTAGE_AV_PATCH
endif

define Package/ffprobe
$(call Package/ffmpeg/Default)
 SECTION:=multimedia
 CATEGORY:=Multimedia
 TITLE+= CLI media identifier
 DEPENDS+= +libffmpeg-full
 VARIANT:=full
endef

define Package/ffprobe/description
$(call Package/ffmpeg/Default/description)
 .
 This package contains the FFprobe command line tool.
endef

define Package/ffserver
$(call Package/ffserver/Default)
 SECTION:=multimedia
 CATEGORY:=Multimedia
 TITLE+= streaming server
 DEPENDS+= +libpthread 
endef

define Package/ffserver/description
$(call Package/ffmpeg/Default/description)
 .
 This package contains the FFmpeg streaming server.
endef

define Package/libffmpeg/Default
$(call Package/ffmpeg/Default)
 SECTION:=libs
 CATEGORY:=Libraries
 TITLE+= libraries
 DEPENDS+= +libpthread +zlib +libfdk-aac
 PROVIDES:= libffmpeg
endef

define Package/libffmpeg-custom
$(call Package/libffmpeg/Default)
 TITLE+= (custom)
 DEPENDS+= @DEVEL +libopenssl
 VARIANT:=custom
 MENU:=1
endef

define Package/libffmpeg-custom/config
source "$(SOURCE)/Config.in"
endef

define Package/libffmpeg-custom/description
$(call Package/ffmpeg/Default/description)
 .
 This package contains customized FFmpeg shared libraries.
endef

define Package/libffmpeg-full
$(call Package/libffmpeg/Default)
 TITLE+= (full)
 DEPENDS+= @BUILD_PATENTED +alsa-lib +libopenssl
 VARIANT:=full
endef

define Package/libffmpeg-full/description
$(call Package/ffmpeg/Default/description)
 .
 This package contains full-featured FFmpeg shared libraries.
endef

TARGET_CFLAGS +=-I$(STAGING_DIR)/usr/include -ffunction-sections -fdata-sections
TARGET_LDFLAGS += -Wl,-gc-sections -Wl,-rpath-link=$(STAGING_DIR)/usr/lib -lfdk-aac

FFMPEG_CONFIGURE:= \
	CFLAGS="$(TARGET_CFLAGS) $(TARGET_CPPFLAGS) $(FPIC)" \
	LDFLAGS="$(TARGET_LDFLAGS)" \
	./configure \
	--extra-libs="-ldl" \
	--enable-cross-compile \
	--cross-prefix="$(TARGET_CROSS)" \
	--arch="$(ARCH)" \
	--target-os=linux \
	--prefix="/usr" \
	--enable-static \
	--pkg-config="pkg-config" \
	--disable-optimizations \
	\
	--enable-gpl \
	--enable-version3 \
	\
	--disable-asm \
	--disable-debug  \
	--disable-doc \
	--disable-dxva2 \
	--enable-pthreads \
	--enable-small \
	--enable-zlib \
	--disable-runtime-cpudetect \
	--disable-altivec \
	--disable-amd3dnow \
	--disable-amd3dnowext \
	--disable-mmx \
	--disable-mmxext \
	--disable-sse \
	--disable-sse2 \
	--disable-sse3 \
	--disable-ssse3 \
	--disable-sse4 \
	--disable-sse42 \
	--disable-avx \
	--disable-xop \
	--disable-fma3 \
	--disable-fma4 \
	--disable-avx2 \
	--disable-vfp \
	--disable-neon \
	--disable-inline-asm \
	--disable-yasm \
	--disable-lzma \
	--disable-vaapi \
	--disable-vda \
	--disable-vdpau \
	--disable-outdevs

ifeq ($(BUILD_VARIANT),custom)
	ifeq ($(CONFIG_FFMPEG_CUSTOM_IPCAM_SUPPORT),y)
		FFMPEG_CONFIGURE+= \
			--enable-mipsdsp \
			--enable-mipsdspr2 \
			--disable-mipsfpu \
			--disable-shared \
			--disable-ffplay \
			--disable-ffprobe \
			--disable-outdevs \
			--disable-encoders \
			--disable-decoders \
			--disable-filters \
			--disable-muxers \
			--disable-demuxers \
			--disable-parsers \
			--disable-indevs \
			--disable-protocols \
			--disable-bsfs
	endif
	ifeq ($(CONFIG_FFMPEG_CUSTOM_WIFISPK_SUPPORT),y)
		FFMPEG_CONFIGURE+= \
			--disable-everything \
			--enable-mipsdsp \
			--enable-mipsdspr2 \
			--disable-mipsfpu \
			--enable-shared \
			--enable-static \
			--disable-ffmpeg \
			--disable-ffplay \
			--disable-ffprobe \
			--disable-encoders \
			--disable-ffserver \
			--enable-openssl \
			--disable-avresample \
			--disable-avdevice \
			--enable-swresample \
			--disable-swscale \
			--disable-gnutls	\
			--disable-postproc \
			--disable-pixelutils \
			--disable-avfilter \
			--disable-faan \
			--disable-error-resilience \
			--disable-gcrypt \
			--enable-protocol=file,mmsh,mmst  \
			--enable-nonfree \
			--enable-libfdk-aac \
			--enable-decoder=libfdk_aac \
			--disable-libvorbis \
			--disable-libopencore-amrnb \
			--disable-libopencore-amrwb
	endif
	ifeq ($(CONFIG_FFMPEG_CUSTOM_MINIDLNA_SUPPORT),y)
		FFMPEG_CONFIGURE+= \
			--enable-shared \
			--disable-ffmpeg \
			--disable-ffplay \
			--disable-ffprobe \
			--disable-ffserver \
			--disable-avfilter \
			--disable-postproc \
			--enable-swresample \
			--disable-swscale \
			--disable-everything 
	endif

  FFMPEG_ENABLE= \
	$(foreach c, $(2), \
		$(if $($(3)_$(c)),--enable-$(1)="$(c)") \
	)

  FFMPEG_CONFIGURE+= \
	$(call FFMPEG_ENABLE,encoder,$(FFMPEG_CUSTOM_ENCODERS),CONFIG_FFMPEG_CUSTOM_ENCODER) \
	$(call FFMPEG_ENABLE,decoder,$(FFMPEG_CUSTOM_DECODERS),CONFIG_FFMPEG_CUSTOM_DECODER) \
	$(call FFMPEG_ENABLE,muxer,$(FFMPEG_CUSTOM_MUXERS),CONFIG_FFMPEG_CUSTOM_MUXER) \
	$(call FFMPEG_ENABLE,demuxer,$(FFMPEG_CUSTOM_DEMUXERS),CONFIG_FFMPEG_CUSTOM_DEMUXER) \
	$(call FFMPEG_ENABLE,parser,$(FFMPEG_CUSTOM_PARSERS),CONFIG_FFMPEG_CUSTOM_PARSER) \
	$(call FFMPEG_ENABLE,protocol,$(FFMPEG_CUSTOM_PROTOCOLS),CONFIG_FFMPEG_CUSTOM_PROTOCOL) \
	$(call FFMPEG_ENABLE,filter,$(FFMPEG_CUSTOM_FILTERS),CONFIG_FFMPEG_CUSTOM_FILTER) \
	$(call FFMPEG_ENABLE,indev,$(FFMPEG_CUSTOM_INDEVS),CONFIG_FFMPEG_CUSTOM_INDEV) \

endif

#PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)-$(BUILD_VARIANT)/$(PKG_NAME)-$(PKG_VERSION)
include $(INCLUDE_DIR)/package.mk

ifneq ($(CONFIG_TARGET_x86),)
  TARGET_CFLAGS += -fomit-frame-pointer
endif

#define Build/Prepare
#	mkdir -p $(PKG_BUILD_DIR)
#	$(CP) ./ffmpeg-$(PKG_VERSION)/* $(PKG_BUILD_DIR)/
#	$(call TSYNC,$(PKG_BUILD_DIR))
#endef

define Build/Configure
	( cd $(PKG_BUILD_DIR); $(FFMPEG_CONFIGURE) )
endef

ifneq ($(BUILT_PREBUILT_IMAGE),0)
define Build/Compile
	$(MAKE) -C $(PKG_BUILD_DIR) \
		DESTDIR="$(PKG_INSTALL_DIR)" \
		all install
endef
endif

define Build/InstallDev/custom
	$(INSTALL_DIR) $(1)/usr/include
	$(CP) $(PKG_INSTALL_DIR)/usr/include/lib{avcodec,avformat,avutil,swresample} $(1)/usr/include/
	$(INSTALL_DIR) $(1)/usr/lib
ifeq ($(CONFIG_FFMPEG_CUSTOM_IPCAM_SUPPORT),y)
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/lib{avcodec,avformat,avutil,swresample}.a $(1)/usr/lib/
else
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/lib{avcodec,avformat,avutil,swresample}.{a,so*} $(1)/usr/lib/
#	$(CP) $(PKG_INSTALL_DIR)/usr/lib/lib{avcodec,avformat,avutil,swresample}.a $(1)/usr/lib/	
endif		
	$(INSTALL_DIR) $(1)/usr/lib/pkgconfig
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/pkgconfig/lib{avcodec,avformat,avutil,swresample}.pc $(1)/usr/lib/pkgconfig/
endef

define Build/InstallDev/full
	$(INSTALL_DIR) $(1)/usr/include
	$(CP) $(PKG_INSTALL_DIR)/usr/include/lib{avcodec,avdevice,avfilter,avformat,avutil,postproc,swresample,swscale} $(1)/usr/include/
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/lib{avcodec,avdevice,avfilter,avformat,avutil,postproc,swresample,swscale}.{a,so*} $(1)/usr/lib/
	$(INSTALL_DIR) $(1)/usr/lib/pkgconfig
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/pkgconfig/lib{avcodec,avdevice,avfilter,avformat,avutil,postproc,swresample,swscale}.pc $(1)/usr/lib/pkgconfig/
endef


# XXX: attempt at installing "best" dev files available
ifeq ($(BUILD_VARIANT),custom)
  # XXX: only install "custom" dev files if -full & -mini are not selected
  ifeq ($(CONFIG_PACKAGE_libffmpeg-full),)
    Build/InstallDev = $(Build/InstallDev/custom)
  endif
endif
ifeq ($(BUILD_VARIANT),full)
  # XXX: always install "full" dev files if -full is selected
  Build/InstallDev = $(Build/InstallDev/full)
endif

define Package/ffmpeg/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(CP) $(PKG_INSTALL_DIR)/usr/bin/ffmpeg $(1)/usr/bin/
endef

define Package/ffprobe/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(CP) $(PKG_INSTALL_DIR)/usr/bin/ffprobe $(1)/usr/bin/
endef

define Package/ffserver/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(CP) $(PKG_INSTALL_DIR)/usr/bin/ffserver $(1)/usr/bin/
endef

define Package/libffmpeg-custom/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(INSTALL_DIR) $(1)/usr/bin
#	$(CP) $(PKG_INSTALL_DIR)/usr/bin/ffplay $(1)/usr/bin/
ifneq ($(CONFIG_FFMPEG_CUSTOM_IPCAM_SUPPORT),y)
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/lib{avcodec,avformat,avutil,swresample}.so.* $(1)/usr/lib/
endif
endef


define Package/libffmpeg-full/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/lib{avcodec,avdevice,avfilter,avformat,avutil,postproc,swresample,swscale}.so.* $(1)/usr/lib/
endef

define Package/ffmpeg/PrebuildImage
	cd $(PKG_BUILD_DIR);tar zcf $(PREBUILT_DIR)/$(PKG_NAME)-$(PKG_VERSION).img ipkg-install
endef
Hooks/Install/Post += Package/ffmpeg/PrebuildImage

$(eval $(call BuildPackage,ffmpeg))
$(eval $(call BuildPackage,ffprobe))
$(eval $(call BuildPackage,ffserver))
$(eval $(call BuildPackage,libffmpeg-custom))
$(eval $(call BuildPackage,libffmpeg-full))
