#
# Copyright (C) 2014 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=cxdish
PKG_RELEASE:=1

include $(INCLUDE_DIR)/package.mk

define Package/cxdish
	SECTION:=utils
	CATEGORY:=Utilities
  TITLE:=cxdish
  DEPENDS:= 
endef

define Package/cxdish/description
  cxdish 
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef

TARGET_CFLAGS += \
	-I$(LINUX_DIR)/arch/mips/include \
	-I$(LINUX_DIR)/include \
	-I$(LINUX_DIR)/drivers/char

define Package/cxdish/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/cxdish $(1)/usr/bin/

endef

$(eval $(call BuildPackage,cxdish))
