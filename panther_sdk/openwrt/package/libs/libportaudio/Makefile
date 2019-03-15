#
# Copyright (C) 2008 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=libportaudio
PKG_VERSION:=v190600_20161030
PKG_RELEASE:=1

PKG_FIXUP:=libtool

PKG_INSTALL:=0

include $(INCLUDE_DIR)/package.mk

define Package/libportaudio
  SECTION:=libs
  CATEGORY:=Libraries
  DEPENDS:=+alsa-lib +libpthread +librt +libdl +libm
  TITLE:=libportaudio 
endef

define Package/libportaudio/description
#	libportaudio
endef



TARGET_CFLAGS += $(FPIC)

CONFIGURE_ARGS+= \
	LIBS="-lpthread -ldl -lm" \
	--without-jack \
	--without-oss \
	--with-alsa
		
define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
	$(call TSYNC,$(PKG_BUILD_DIR))
endef

define Build/InstallDev
	$(INSTALL_DIR) $(1)/usr/include/libportaudio/
	cp $(PKG_INSTALL_DIR)/usr/include/* $(1)/usr/include/libportaudio/ -rf
	$(INSTALL_DIR) $(1)/usr/lib/
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/*.so* $(1)/usr/lib/
endef

define Package/libportaudio/install
	$(INSTALL_DIR) $(1)/usr/lib/
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/*.so* $(1)/usr/lib/
endef

$(eval $(call BuildPackage,libportaudio))
