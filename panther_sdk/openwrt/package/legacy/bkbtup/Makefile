#
# Copyright (C) 2014 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=bkbtup
PKG_RELEASE:=1

include $(INCLUDE_DIR)/package.mk


define Package/btup
	SECTION:=utils
	CATEGORY:=Utilities
  TITLE:=btup
  DEPENDS:= +libpthread +wdk
endef

define Package/btup/description
  btup 
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef

TARGET_CFLAGS += \
	-I$(LINUX_DIR)/arch/mips/include \
	-I$(LINUX_DIR)/include \
	-I$(LINUX_DIR)/drivers/char

define Package/btup/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/btup $(1)/usr/bin/

endef

$(eval $(call BuildPackage,btup))
