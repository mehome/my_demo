# 
# OpenWrt
#
# This is free software, licensed under the GNU General Public License v2.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=MPlayer
PKG_VERSION:=1.3.0
PKG_RELEASE:=1

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
#PKG_SOURCE_URL:=http://www.mplayerhq.hu/MPlayer/releases/

#DL_DIR=$(CURDIR)
USE_SRC_DIR:=1
PRIVATE:=1

include $(INCLUDE_DIR)/package.mk
#include $(INCLUDE_DIR)/nls.mk

PKG_BUILD_PARALLEL:=1

define Package/mplayer
  SECTION:=multimedia
  CATEGORY:=Multimedia
  TITLE:=MPlayer, the movie player
  URL:=http://www.mplayerhq.hu
  DEPENDS:=+alsa-lib +libpthread +librt +zlib +libfdk-aac +libvorbisidec +libopencore +libopenssl +libogg
endef

define Package/mplayer/description
  MPlayer is a movie player which runs on many systems.
endef

-include $(TOPDIR)/.config
ifeq ($(CONFIG_BIG_ENDIAN),y)
CONFIGURE_ENDIAN:= --enable-big-endian
else
CONFIGURE_ENDIAN:= --disable-big-endian
endif

CONFIGURE_OPTIONS:= \
		--charset=UTF-8 \
		--enable-alsa \
		--disable-librtmp \
		--disable-gui \
		--disable-ivtv \
		--disable-sdl \
		--disable-freetype \
		--disable-fontconfig \
		--disable-enca \
		--disable-fribidi \
		--disable-joystick \
		--disable-apple-ir \
		--disable-iconv \
		--disable-termcap \
		--disable-termios \
		--disable-smb \
		--disable-vcd \
		--disable-dvdnav \
		--disable-dvdread \
		--disable-cdparanoia \
		--disable-cddb \
		--disable-vm \
		--disable-bluray \
		--disable-libcdio \
		--disable-mencoder \
		--disable-sortsub \
		--disable-ass \
		--enable-ass-internal \
		--enable-tremor-internal \
		--disable-tremor-low \
		--disable-tremor \
		--disable-libvorbis \
		--disable-gnutls \
		--disable-ossaudio \
		--disable-esd \
		--disable-pulse \
		--disable-jack \
		--disable-sgiaudio \
		--disable-mmx \
		--disable-radio-v4l2 \
		--disable-tv-v4l2 \
		--disable-md5sum \
		--disable-yuv4mpeg \
		--disable-tga \
		--disable-menu \
		--disable-fribidi \
		--disable-unrarexec \
		--disable-pnm \
		--disable-inet6 \
		--disable-pvr \
		--disable-tv \
		--disable-tv-v4l1 \
		--disable-tv-v4l2 \
		--disable-tv-bsdbt848 \
		--disable-fastmemcpy \
		--disable-ftp \
		--disable-gif \
		--disable-png \
		--disable-mng \
		--disable-jpeg \
		--disable-libcdio \
		--disable-liblzo \
		--disable-qtx \
		--disable-real \
		--disable-xanim \
		--disable-win32dll \
		--disable-xvid \
		--disable-xvid-lavc \
		--disable-x264 \
		--disable-x264-lavc \
		--disable-corevideo \
		--disable-libdirac-lavc \
		--disable-libschroedinger-lavc \
		--disable-libvpx-lavc \
		--disable-libnut \
		--disable-postproc \
		--disable-vf-lavfi  \
		--disable-speex \
		--disable-libgsm \
		--disable-theora \
		--disable-faad \
		--disable-x11 \
		--disable-xshape \
		--disable-direct3d \
		--disable-faac \
		--enable-fdk-aac \
		--enable-fdk-aac-lavc \
		--disable-ladspa \
		--disable-libbs2b \
		--disable-libdv \
		--disable-libilbc \
		--disable-libopus \
		--disable-mpg123 \
		--disable-mad \
		--disable-toolame \
		--disable-twolame \
		--disable-libdca \
		--disable-liba52 \
		--enable-libopencore_amrnb \
		--enable-libopencore_amrwb \
		--disable-libopenjpeg \
		--disable-vidix \
		--disable-gl \
		--disable-caca \
		--disable-matrixview \
		--disable-dga2 \
		--disable-dga1 \
		--disable-svga \
		--disable-decoder=aac \
		--disable-decoder=amrnb \
		--disable-decoder=amrwb \
		--disable-decoder=a64multi \
		--disable-decoder=a64multi5 \
		--disable-decoder=aasc \
		--disable-decoder=aic \
		--disable-decoder=alias_pix \
		--disable-decoder=amv \
		--disable-decoder=anm \
		--disable-decoder=ansi \
		--disable-decoder=apng \
		--disable-decoder=asv1 \
		--disable-decoder=asv2 \
		--disable-decoder=aura \
		--disable-decoder=aura2 \
		--disable-decoder=avrp \
		--disable-decoder=avrn \
		--disable-decoder=avs \
		--disable-decoder=avui \
		--disable-decoder=ayuv \
		--disable-decoder=bethsoftvid \
		--disable-decoder=bfi \
		--disable-decoder=bink \
		--disable-decoder=bmp \
		--disable-decoder=bmv_video \
		--disable-decoder=brender_pix \
		--disable-decoder=c93 \
		--disable-decoder=cavs \
		--disable-decoder=cdgraphics \
		--disable-decoder=cdxl \
		--disable-decoder=cfhd \
		--disable-decoder=cinepak \
		--disable-decoder=cljr \
		--disable-decoder=cllc \
		--disable-decoder=comfortnoise \
		--disable-decoder=cpia \
		--disable-decoder=cscd \
		--disable-decoder=cyuv \
		--disable-decoder=dds \
		--disable-decoder=dfa \
		--disable-decoder=dirac \
		--disable-decoder=dnxhd \
		--disable-decoder=dpx \
		--disable-decoder=dsicinvideo \
		--disable-decoder=dvvideo \
		--disable-decoder=dxa \
		--disable-decoder=dxtory \
		--disable-decoder=dxv \
		--disable-decoder=eacmv \
		--disable-decoder=eamad \
		--disable-decoder=eatgq \
		--disable-decoder=eatgv \
		--disable-decoder=eatqi \
		--disable-decoder=eightbps \
		--disable-decoder=eightsvx_exp \
		--disable-decoder=eightsvx_fib \
		--disable-decoder=escape124 \
		--disable-decoder=escape130 \
		--disable-decoder=exr \
		--disable-decoder=ffv1 \
		--disable-decoder=ffvhuff \
		--disable-decoder=fic \
		--disable-decoder=flashsv \
		--disable-decoder=flashsv2 \
		--disable-decoder=flic \
		--disable-decoder=flv \
		--disable-decoder=fourxm \
		--disable-decoder=fraps \
		--disable-decoder=frwu \
		--disable-decoder=g2m \
		--disable-decoder=gif \
		--disable-decoder=h261 \
		--disable-decoder=h263 \
		--disable-decoder=h263i \
		--disable-decoder=h263p \
		--disable-decoder=h264 \
		--disable-decoder=hap \
		--disable-decoder=hevc \
		--disable-decoder=hnm4_video \
		--disable-decoder=hq_hqa \
		--disable-decoder=hqx \
		--disable-decoder=huffyuv \
		--disable-decoder=idcin \
		--disable-decoder=iff_ilbm \
		--disable-decoder=indeo2 \
		--disable-decoder=indeo3 \
		--disable-decoder=indeo4 \
		--disable-decoder=indeo5 \
		--disable-decoder=interplay_video \
		--disable-decoder=jpeg2000 \
		--disable-decoder=jpegls \
		--disable-decoder=jv \
		--disable-decoder=kgv1 \
		--disable-decoder=kmvc \
		--disable-decoder=lagarith \
		--disable-decoder=ljpeg \
		--disable-decoder=loco \
		--disable-decoder=mdec \
		--disable-decoder=mimic\
		--disable-decoder=mjpeg \
		--disable-decoder=mjpegb \
		--disable-decoder=mmvideo \
		--disable-decoder=motionpixels \
		--disable-decoder=mpeg1video \
		--disable-decoder=mpeg2video \
		--disable-decoder=mpegvideo \
		--disable-decoder=msmpeg4v1 \
		--disable-decoder=msmpeg4v2 \
		--disable-decoder=msmpeg4v3 \
		--disable-decoder=msrle \
		--disable-decoder=mss1 \
		--disable-decoder=mss2 \
		--disable-decoder=msvideo1 \
		--disable-decoder=mszh \
		--disable-decoder=mts2 \
		--disable-decoder=mvc1 \
		--disable-decoder=mvc2 \
		--disable-decoder=mxpeg \
		--disable-decoder=nuv \
		--disable-decoder=paf_video \
		--disable-decoder=pam \
		--disable-decoder=pbm \
		--disable-decoder=pcx \
		--disable-decoder=pgm \
		--disable-decoder=pgmyuv \
		--disable-decoder=pictor \
		--disable-decoder=png \
		--disable-decoder=ppm \
		--disable-decoder=prores \
		--disable-decoder=prores_lgpl \
		--disable-decoder=ptx \
		--disable-decoder=qdraw \
		--disable-decoder=qpeg \
		--disable-decoder=qtrle \
		--disable-decoder=r10k \
		--disable-decoder=r210 \
		--disable-decoder=rawvideo \
		--disable-decoder=rl2 \
		--disable-decoder=roq \
		--disable-decoder=rpza \
		--disable-decoder=rscc \
		--disable-decoder=rv10 \
		--disable-decoder=rv20 \
		--disable-decoder=rv30 \
		--disable-decoder=rv40 \
		--disable-decoder=s302m \
		--disable-decoder=sanm \
		--disable-decoder=screenpresso \
		--disable-decoder=sdx2_dpcm \
		--disable-decoder=sgi \
		--disable-decoder=sgirle \
		--disable-decoder=smacker \
		--disable-decoder=smc \
		--disable-decoder=smvjpeg \
		--disable-decoder=snow \
		--disable-decoder=sp5x \
		--disable-decoder=sunrast \
		--disable-decoder=svq1 \
		--disable-decoder=svq3 \
		--disable-decoder=targa \
		--disable-decoder=targa_y216 \
		--disable-decoder=tdsc \
		--disable-decoder=theora \
		--disable-decoder=thp \
		--disable-decoder=tiertexseqvideo \
		--disable-decoder=tiff \
		--disable-decoder=tmv \
		--disable-decoder=truemotion1 \
		--disable-decoder=truemotion2 \
		--disable-decoder=tscc \
		--disable-decoder=tscc2 \
		--disable-decoder=txd \
		--disable-decoder=ulti \
		--disable-decoder=utvideo \
		--disable-decoder=v210 \
		--disable-decoder=v210x \
		--disable-decoder=v308 \
		--disable-decoder=v408 \
		--disable-decoder=v410 \
		--disable-decoder=vb \
		--disable-decoder=vble \
		--disable-decoder=vc1 \
		--disable-decoder=vc1image \
		--disable-decoder=vcr1 \
		--disable-decoder=vmdvideo \
		--disable-decoder=vmnc \
		--disable-decoder=vp3 \
		--disable-decoder=vp5 \
		--disable-decoder=vp6 \
		--disable-decoder=vp6a \
		--disable-decoder=vp6f \
		--disable-decoder=vp7 \
		--disable-decoder=vp8 \
		--disable-decoder=vp9 \
		--disable-decoder=vqa \
		--disable-decoder=webp \
		--disable-decoder=wmv1 \
		--disable-decoder=wmv2 \
		--disable-decoder=wmv3 \
		--disable-decoder=wmv3image \
		--disable-decoder=wnv1 \
		--disable-decoder=xan_wc3 \
		--disable-decoder=xan_wc4 \
		--disable-decoder=xbm \
		--disable-decoder=xface \
		--disable-decoder=xl \
		--disable-decoder=xwd \
		--disable-decoder=y41p \
		--disable-decoder=yop \
		--disable-decoder=yuv4 \
		--disable-decoder=zero12v \
		--disable-decoder=zerocodec \
		--disable-decoder=zlib \
		--disable-decoder=zmbv

ifdef CONFIG_USE_MUSL
CONFIGURE_OPTIONS+= --enable-pthreads
endif

define Build/Configure
	(cd $(PKG_BUILD_DIR); \
		./configure \
		$(CONFIGURE_ENDIAN) \
		--enable-cross-compile \
		--prefix=/usr \
		--confdir=/etc \
		--target=$(TARGET_CROSS) \
		--host-cc=gcc \
		--cc="$(TARGET_CC)" \
		--as="$(TARGET_AS)" \
		--ar="$(TARGET_AR)" \
		$(CONFIGURE_OPTIONS) \
		--extra-cflags="-DCONFIG_PANTHER -DCONFIG_PANTHER_DSP" \
		--extra-ldflags="$(TARGET_LDFLAGS)" \
	);
endef

define Build/RunMake
	CFLAGS="$(TARGET_CPPFLAGS) $(TARGET_CFLAGS)" \
	$(MAKE) $(PKG_JOBS) -C $(PKG_BUILD_DIR)/$(1) \
		$(TARGET_CONFIGURE_OPTS) \
		$(DRIVER_MAKEOPTS) \
		LIBS="$(TARGET_LDFLAGS)" \
		LIBS_c="$(TARGET_LDFLAGS_C)" \
		BCHECK= \
		$(2)
endef

define Package/mplayer/install
	$(INSTALL_DIR) \
		$(1)/usr/bin \
		$(1)/etc

	$(INSTALL_BIN) \
		$(PKG_BUILD_DIR)/mplayer \
		$(1)/usr/bin/mplayer
	#$(INSTALL_DATA) \
	#	$(FILES_DIR)/mplayer.conf \
	#	$(1)/etc/

endef

define Package/mplayer/PrebuildImage
	cd $(PKG_BUILD_DIR);tar zcf $(PREBUILT_DIR)/$(PKG_NAME)-$(PKG_VERSION).img mplayer
endef
Hooks/Install/Post += Package/mplayer/PrebuildImage

$(eval $(call BuildPackage,mplayer))
