#
# Copyright (C) 2006-2010 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=libplayer
PKG_VERSION:=2.0.1
PKG_RELEASE:=1

USE_SRC_DIR:=1

PKG_FIXUP:=no-autoreconf
PKG_INSTALL:=1

include $(INCLUDE_DIR)/package.mk

define Package/player-lib
  SECTION:=libs
  CATEGORY:=Libraries
  TITLE:=PLAYER (A multimedia A/V abstraction layer API) library
  URL:=http://libplayer.geexbox.org/
  DEPENDS:=+libpthread
endef

define Package/player-lib/description
libplayer provides a generic A/V API that relies on various multimedia player for Linux systems.
It currently supports MPlayer, xine VLC and GStreamer only.
endef

TARGET_CFLAGS += $(FPIC)

LIBPLAYER_CONFIGURE:= \
	./configure \
	--disable-static \
	--cross-compile \
	--enable-mplayer

define Build/Configure
	cd $(PKG_BUILD_DIR); $(LIBPLAYER_CONFIGURE)
endef

define Build/InstallDev
	$(INSTALL_DIR) $(1)/usr/include
	$(CP) $(PKG_INSTALL_DIR)/usr/local/include/player.h $(1)/usr/include/
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_INSTALL_DIR)/usr/local/lib/libplayer.so* $(1)/usr/lib/
	$(INSTALL_DIR) $(1)/usr/lib/pkgconfig
	$(CP) $(PKG_INSTALL_DIR)/usr/local/lib/pkgconfig/libplayer.pc $(1)/usr/lib/pkgconfig/
endef

define Package/player-lib/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_INSTALL_DIR)/usr/local/lib/libplayer.so.* $(1)/usr/lib/
	$(INSTALL_DIR) $(1)/usr/bin
	$(CP) $(PKG_INSTALL_DIR)/usr/local/bin/libplayer-test $(1)/usr/bin/
	$(INSTALL_DIR) $(1)/usr/bin
	$(CP) $(PKG_INSTALL_DIR)/usr/local/bin/mp_player_test $(1)/usr/bin/
endef

$(eval $(call BuildPackage,player-lib))
