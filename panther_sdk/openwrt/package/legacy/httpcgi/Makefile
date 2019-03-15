#
# Copyright (C) 2014 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
export WIFIAUDIO_MONTAGE=y
#export WIFIAUDIO_MTK=y

include $(TOPDIR)/rules.mk

PKG_NAME:=httpcgi
PKG_RELEASE:=1

export LIBRARYS := -lavsclient -ljson-c -lcdb -ldl -lasound -lpthread -lrt -lmpdclient -lcchip

include $(INCLUDE_DIR)/package.mk

define Package/httpcgi
  SECTION:=sound
  CATEGORY:=Sound
  TITLE:=httpcgi
  DEPENDS:= +libjson-c +wdk +libavsclient +libasound +libpthread +librt +libmpdclient +libcchip
endef

define Package/httpcgi/description
  post music playlist
endef

TARGET_CFLAGS += \
	-I$(STAGING_DIR)/usr/include \

TARGET_LDFLAGS += -Wl,-gc-sections  \
  -Wl,-rpath-link=$(STAGING_DIR)/usr/lib -lcdb

TARGET_LDFLAGS += -lm

define Build/Compile
	CC="$(TARGET_CC)" \
	CFLAGS="$(TARGET_CFLAGS) -I$(STAGING_DIR)/usr/include/json-c -I$(STAGING_DIR)/usr/include" \
	LDFLAGS="$(TARGET_LDFLAGS) -L$(STAGING_DIR)/usr/lib -Wl,-rpath-link=$(STAGING_DIR)/usr/lib" \
	$(MAKE) -C $(PKG_BUILD_DIR) \
		all
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef

#define Build/InstallDev
#	$(INSTALL_DIR) $(1)/usr/lib
#	$(CP) $(PKG_BUILD_DIR)/http_lib/libcurl.so.4 $(1)/usr/lib/
#	$(INSTALL_DIR) $(1)/usr/lib
#	$(CP) $(PKG_BUILD_DIR)/http_lib/libjson.so.0 $(1)/usr/lib/
#	$(INSTALL_DIR) $(1)/usr/lib/pkgconfig
#	$(CP) $(PKG_BUILD_DIR)/http_lib/libubox.so $(1)/usr/lib/
#endef

define Package/httpcgi/install
#	$(INSTALL_DIR) $(1)/usr/bin
#	$(INSTALL_BIN) $(PKG_BUILD_DIR)/bin/* $(1)/usr/bin
	
#	$(INSTALL_DIR) $(1)/usr/lib
#	$(CP) $(PKG_BUILD_DIR)/lib/*.so $(1)/usr/lib

	$(INSTALL_DIR) $(1)/www/
	$(INSTALL_DIR) $(1)/www/cgi-bin
	$(CP) $(PKG_BUILD_DIR)/amazon/*.cgi $(1)/www/cgi-bin
	$(CP) $(PKG_BUILD_DIR)/command/*.cgi $(1)/www/cgi-bin

	$(CP) $(PKG_BUILD_DIR)/html/css $(1)/www/
	$(CP) $(PKG_BUILD_DIR)/html/js $(1)/www/
	$(CP) $(PKG_BUILD_DIR)/html/xzx.html $(1)/www/
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) ./files/httpcgi.init $(1)/etc/init.d/httpcgi
	
endef

$(eval $(call BuildPackage,httpcgi))
