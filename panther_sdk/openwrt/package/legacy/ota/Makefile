#
# Copyright (C) 2014 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=ota
PKG_RELEASE:=1

include $(INCLUDE_DIR)/package.mk

define Package/ota
	SECTION:=utils
	CATEGORY:=Utilities
  TITLE:=ota
  DEPENDS:= +libcurl +libnghttp2 +libssl +libcrypto +libutils +libpthread +libnl-tiny +wdk +libcchip
endef

define Package/ota/description
  ota 
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef

TARGET_CFLAGS += \
	-I$(LINUX_DIR)/arch/mips/include \
	-I$(LINUX_DIR)/include \
	-I$(LINUX_DIR)/drivers/char

define Package/ota/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/ota $(1)/usr/bin/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/httpapi.cgi $(1)/usr/bin/

endef

$(eval $(call BuildPackage,ota))
