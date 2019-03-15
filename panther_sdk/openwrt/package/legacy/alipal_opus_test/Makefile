#
# Copyright (C) 2014 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

export WIFIAUDIO_MONTAGE=y
#export WIFIAUDIO_MTK=y
#澜起和MTK二选

include $(TOPDIR)/rules.mk

PKG_NAME:=alipal_opus_test
PKG_RELEASE:=1

include $(INCLUDE_DIR)/package.mk

define Package/alipal_opus_test
	SECTION:=utils
	CATEGORY:=Utilities
  TITLE:=alipal_opus_test
  DEPENDS:= +libopus
endef

define Package/alipal_opus_test/description
  alipal_opus_test task
endef

#########################modify by xdh_170904###############################################
define Build/Compile
	CC="$(TARGET_CC)" \
	CFLAGS="$(TARGET_CFLAGS) -I -I$(STAGING_DIR)/usr/include" \
	LDFLAGS="$(TARGET_LDFLAGS) -L$(STAGING_DIR)/usr/lib -Wl,-rpath-link=$(STAGING_DIR)/usr/lib" \
	$(MAKE) -C $(PKG_BUILD_DIR) \
		all
endef

#############################################################################################

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef

define Package/alipal_opus_test/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/alipal_opus_test $(1)/usr/bin/
	#$(INSTALL_BIN) ./files/powoff.sh $(1)/usr/bin/
endef

$(eval $(call BuildPackage,alipal_opus_test))
