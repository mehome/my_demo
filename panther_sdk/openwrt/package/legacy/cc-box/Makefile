#
# Copyright (C) 2014 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=cc-box
PKG_RELEASE:=1

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/kernel.mk

define Package/cc-box
	SECTION:=utils
	CATEGORY:=Utilities
  TITLE:= paly wav file 
  DEPENDS:= +alsa-lib +libcchip +libpthread +librt
endef

define Package/cc-box/description
  wav player
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef

TARGET_CFLAGS += 

define Package/cc-box/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/cc-box $(1)/usr/bin/
	ln -sf /usr/bin/cc-box $(1)/usr/bin/wavplayer
	# ln -sf /usr/bin/cc-box $(1)/usr/bin/wavRecorder
	# ln -sf /usr/bin/cc-box $(1)/usr/bin/mp3player
	# ln -sf /usr/bin/cc-box $(1)/usr/bin/nameCmdline
	# ln -sf /usr/bin/cc-box $(1)/usr/bin/pidCmdline
endef

$(eval $(call BuildPackage,cc-box))
