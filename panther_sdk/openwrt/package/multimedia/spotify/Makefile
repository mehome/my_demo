#
# Copyright (C) 2016 Montage Inc.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=spotify

PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)

include $(INCLUDE_DIR)/package.mk

define Package/spotify
  SECTION:=multimedia
  CATEGORY:=Multimedia
#  DEPENDS:=+avahi-utils
  DEPENDS:=+libmpdclient +alsa-lib +monitor +wdk +libutils +libblobmsg-json +libubus +json-c +libcchip +librt
  TITLE:=Spotify connect
  VERSION:=1
endef

define Package/spotify/description
  Spotify connect
endef

define Package/spotify/config
  config SPOTIFY_IPC
        bool "spotify-ipc"
	depends on PACKAGE_spotify
        help
         To enable spotify ipc
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP)  ./src/include $(PKG_BUILD_DIR)/
	mkdir -p $(PKG_BUILD_DIR)/lib
ifeq ($(CONFIG_BIG_ENDIAN),y)
	$(CP)  ./src/lib/* $(PKG_BUILD_DIR)/lib
else
	$(CP)  ./src/lib_le/* $(PKG_BUILD_DIR)/lib
endif
	$(CP)  ./src/spotify $(PKG_BUILD_DIR)/
	$(call TSYNC,$(PKG_BUILD_DIR))
endef

TARGET_CFLAGS += $(FPIC) -DMONTAGE_CTRL_PLAYER #-DCERT_DEBUG
#TARGET_CFLAGS += -DRING_BUFFER_ENABLE #-DUNDER_FLOW_CHECK
ifeq ($(CONFIG_SPOTIFY_IPC),y)
TARGET_CFLAGS += -DSPOTIFY_IPC
endif
TARGET_CFLAGS += -D__PANTHER__

TARGET_CFLAGS += \
	 -I$(STAGING_DIR)/usr/include/libutils


LIBS:=-lasound -lmpdclient -lcdb -lpthread -ldl -lmonitor -lutils -lnl-tiny -lblobmsg_json -lubus -ljson-c

LOCAL_CFLAGS = -DPATH_TO_APP_KEY_FILE=\\\"/etc/spotify_appkey.key\\\"

#MAKE_FLAGS += \
#        CFLAGS="$(TARGET_CFLAGS) $(TARGET_CPPFLAGS) $(LOCAL_CFLAGS)" \
#        LDFLAGS="$(TARGET_LDFLAGS) $(LIBS)"

define Build/Compile
	$(MAKE) -C $(PKG_BUILD_DIR)/spotify \
		CC="$(TARGET_CC)" \
		CFLAGS="$(TARGET_CFLAGS) $(TARGET_CPPFLAGS) $(LOCAL_CFLAGS)" \
		LDFLAGS="$(TARGET_LDFLAGS) $(LIBS)"
endef

define Package/spotify/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_BUILD_DIR)/lib/libspotify_embedded_shared.so $(1)/usr/lib/libspotify_embedded_shared.so
	$(INSTALL_DIR) $(1)/etc
	$(CP) ./file/spotify_appkey.key $(1)/etc/spotify_appkey.key
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) ./file/spotify.init $(1)/etc/init.d/spotify
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/spotify/mspotify $(1)/usr/bin/mspotify
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/spotify/mspotify_cli $(1)/usr/bin/mspotify_cli
endef

$(eval $(call BuildPackage,spotify))
