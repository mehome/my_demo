export WIFIAUDIO_MONTAGE=y
#export WIFIAUDIO_MTK=y

include $(TOPDIR)/rules.mk

PKG_NAME:=creatPlayList
PKG_RELEASE:=1

PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/nls.mk

define Package/creatPlayList
	SECTION:=sound
	CATEGORY:=Sound
        TITLE:= creat http playlist
        VERSION:=1
      DEPENDS:= +libjson-c +libmpdclient +monitor +wdk
endef

define Package/creatPlayList/description
  creat http playlist
endef


TARGET_CFLAGS += \
	-I$(STAGING_DIR)/usr/include \

TARGET_LDFLAGS += -Wl,-gc-sections  \
  -Wl,-rpath-link=$(STAGING_DIR)/usr/lib -lcdb


#LDFLAGS="$(TARGET_LDFLAGS) -L$(STAGING_DIR)/usr/lib -Wl,-rpath-link=$(STAGING_DIR)/usr/lib" \
#TARGET_LDFLAGS += -Wl,-rpath-link=$(STAGING_DIR)/usr/lib  -ljson-c -ldl -lmpdclient -lmonitor -lcdb

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
	$(call TSYNC,$(PKG_BUILD_DIR))
endef

TARGET_LDFLAGS += -lm
#LIBS:= -ljson-c -ldl

define Package/creatPlayList/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_DIR) $(1)/usr/script/playlists
	$(INSTALL_DIR) $(1)/etc/conf
	$(CP) $(PKG_BUILD_DIR)/creatPlayList $(1)/usr/bin
	$(INSTALL_BIN) ./files/playlist.sh $(1)/usr/bin/
	$(CP) ./files/xzxwifiaudio.config $(1)/etc/conf/xzxwifiaudio
endef

$(eval $(call BuildPackage,creatPlayList))
