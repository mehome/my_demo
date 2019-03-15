#
# Copyright (C) 2014 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

export WIFIAUDIO_MONTAGE=y
#export WIFIAUDIO_MTK=y


include $(TOPDIR)/rules.mk

PKG_NAME:=myutils
PKG_RELEASE:=1

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/kernel.mk

define Package/myutils
	SECTION:=utils
	CATEGORY:=Utilities
  TITLE:=myutils
  DEPENDS:= +libjson-c 
endef

define Package/myutils/description
  configparser daemon
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef

TARGET_LDFLAGS += -Wl,-rpath-link=$(STAGING_DIR)/usr/lib -ljson-c
define Build/Compile
	CC="$(TARGET_CC)" \
	CFLAGS="$(TARGET_CFLAGS)" \
	LDFLAGS="$(TARGET_LDFLAGS)" \
	$(MAKE) -C $(PKG_BUILD_DIR) \
		all
endef	
define Build/InstallDev
	$(INSTALL_DIR) $(1)/usr/lib
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/libmyutils.so* $(STAGING_DIR)/usr/lib
	$(INSTALL_DIR) $(1)/usr/include
	$(CP) $(PKG_BUILD_DIR)/myutils $(STAGING_DIR)/usr/include/
endef

define Package/myutils/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/libmyutils.so* $(1)/usr/lib
endef

$(eval $(call BuildPackage,myutils))
