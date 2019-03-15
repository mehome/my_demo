#
# Copyright (C) 2014 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=cpp-box
PKG_RELEASE:=1

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/kernel.mk

define Package/cpp-box
	SECTION:=utils
	CATEGORY:=Utilities
    TITLE:= cpp-box test
  	DEPENDS:= +libstdcpp +libpthread +librt +libatomic \
	+alsa-lib +libcchip \
	+libffmpeg-custom	
endef

define Package/cpp-box/description
  cpp-box test
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* 	$(PKG_BUILD_DIR)/
	$(CP) ./files/* $(PKG_BUILD_DIR)/
endef

TARGET_CFLAGS += 

define Package/cpp-box/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/cpp-box $(1)/usr/bin/
	ln -sf /usr/bin/cpp-box $(1)/usr/bin/tcp_server
	ln -sf /usr/bin/cpp-box $(1)/usr/bin/myPlay
	#cp $(1)/usr/bin/cpp-box /home/samba/nfs/`whoami`/duer/
endef

$(eval $(call BuildPackage,cpp-box))
