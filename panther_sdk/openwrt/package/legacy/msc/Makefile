# 
# Copyright (C) 2007 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=msc
PKG_RELEASE:=1

PKG_BUILD_DIR := $(BUILD_DIR)/$(PKG_NAME)
PKG_BUILD_DEPENDS := librt libusb-1.0

include $(INCLUDE_DIR)/package.mk

define Package/msc
  SECTION:=utils
  CATEGORY:=Utilities
  DEPENDS:=@DEVEL +librt +libusb-1.0
  TITLE:=USB Mass Storage Class Verification Tool
  URL:=http://gitorious.org/usb/usb-tools
endef

define Package/msc/description
	USB Mass Storage Class Verification Tool
endef

define Build/Prepare
	$(CP) ./src/* $(PKG_BUILD_DIR)/
	$(call TSYNC,$(PKG_BUILD_DIR))
endef

define Build/Compile
	$(TARGET_CC) $(TARGET_CFLAGS) -O2 -g -lrt -lpthread -ldl -D_GNU_SOURCE -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 $(PKG_BUILD_DIR)/msc.c -o $(PKG_BUILD_DIR)/$(PKG_NAME)
endef

define Package/msc/install
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/msc $(1)/usr/sbin/
	$(INSTALL_BIN) ./files/msc.sh $(1)/usr/sbin
endef

$(eval $(call BuildPackage,msc))
