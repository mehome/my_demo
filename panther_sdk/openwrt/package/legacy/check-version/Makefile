#
# Copyright (C) 2014 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=check-version
PKG_RELEASE:=1

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/kernel.mk

define Package/check-version
	SECTION:=utils
	CATEGORY:=Utilities
  TITLE:=check-version
  DEPENDS:= +libcurl +libnghttp2 +libssl +libcrypto +libutils +libpthread +libnl-tiny +wdk +libairkiss +libestimate +libcchip
endef

define Package/check-version/description
  check-version 
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef

TARGET_CFLAGS += \
	-I$(LINUX_DIR)/arch/mips/include \
	-I$(LINUX_DIR)/include \
	-I$(LINUX_DIR)/drivers/char

define Package/check-version/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/check-version $(1)/usr/bin/

endef

$(eval $(call BuildPackage,check-version))
