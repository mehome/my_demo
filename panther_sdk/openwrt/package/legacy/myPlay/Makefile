#
# Copyright (C) 2014 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=myPlay
PKG_RELEASE:=1

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/kernel.mk

USER_LIBCCHIP = n
export USER_LIBCCHIP

define Package/myPlay
	SECTION:=utils
	CATEGORY:=Utilities
  TITLE:= myPlay test
  DEPENDS:= +libstdcpp +libpthread +librt +libatomic \
	+alsa-lib +libcchip \
	+libffmpeg-custom
endef

define Package/myPlay/description
  myPlay test
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* 	$(PKG_BUILD_DIR)/
endef

TARGET_CFLAGS += 

define Package/myPlay/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/myPlay $(1)/usr/bin/
endef

$(eval $(call BuildPackage,myPlay))
