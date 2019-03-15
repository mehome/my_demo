#
# Copyright (C) 2013 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
# porting source from: https://github.com/abrasive/shairport
# file name: shairport-master.zip

include $(TOPDIR)/rules.mk

PKG_NAME:=shairport

PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)

include $(INCLUDE_DIR)/package.mk

define Package/shairport
  SECTION:=multimedia
  CATEGORY:=Multimedia
  DEPENDS:=+libpthread +libopenssl +libmpdclient +monitor +wdk +alsa-lib +libblobmsg-json +libubus +json-c +libcchip +librt
  TITLE:=ShairPort AirPort Express emulator
  VERSION:=1
endef

define Package/shairport/description
  This program emulates an AirPort Express for the purpose of streaming
  music from iTunes and compatible iPods. It implements a server for the
  Apple RAOP protocol.
  ShairPort does not support AirPlay v2 (video and photo streaming).

  It supports multiple simultaneous streams, if your audio output chain
  (as detected by libao) does so.
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
	$(call TSYNC,$(PKG_BUILD_DIR))
endef

TARGET_CFLAGS += $(FPIC) -DMONTAGE_CTRL_PLAYER

LIBS:=-lm -lcrypto -lpthread -ldl -lasound -lcdb -lmpdclient -lmonitor -lblobmsg_json -lubus -ljson-c -lcchip -lrt

MAKE_FLAGS += \
	CFLAGS="$(TARGET_CFLAGS) $(TARGET_CPPFLAGS)" \
	LDFLAGS="$(TARGET_LDFLAGS) $(LIBS)"

define Package/shairport/install
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/shairport $(1)/usr/sbin/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/air_cli $(1)/usr/sbin/
	$(INSTALL_DIR) $(1)/etc/init.d/
#	$(INSTALL_BIN) files/shairport.init $(1)/etc/init.d/shairport
#ifneq ($(CONFIG_PACKAGE_zrootfs),y)
	#$(INSTALL_BIN) files/shairport $(1)/etc/init.d/shairport
#endif
	$(INSTALL_DIR) $(1)/etc/config
	$(INSTALL_DATA) files/shairport.config $(1)/etc/config/shairport
endef

$(eval $(call BuildPackage,shairport))
