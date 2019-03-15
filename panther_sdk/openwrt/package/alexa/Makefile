#
# Copyright (C) 2014 Montage Inc.
#
export WIFIAUDIO_MONTAGE=y
#export WIFIAUDIO_MTK=y

include $(TOPDIR)/rules.mk

PKG_NAME:=alexa
PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)

LDFLAGS="$(TARGET_LDFLAGS) -L$(STAGING_DIR)/usr/lib -Wl,-rpath-link=$(STAGING_DIR)/usr/lib"

include $(INCLUDE_DIR)/package.mk
define Package/alexa
    SECTION:=alexa
    CATEGORY:=alexa with cchip
    TITLE:= Alexa
    VERSION:=1
    DEPENDS:= +libavsclient +libnghttp2 +libopenssl +libcurl +libjson-c +alsa-lib +libuci +libmad +libmpdclient +monitor +wdk +librt +libcchip
endef

define Package/alexa/config
  source "$(SOURCE)/Config.in"
endef

TARGET_CFLAGS += \
    $(if $(CONFIG_ALEXA_SUPPORT_FF),-DALEXA_FF) \
    -I$(STAGING_DIR)/usr/include

TARGET_LDFLAGS += -Wl,-rpath-link=$(STAGING_DIR)/usr/lib

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
	$(call TSYNC,$(PKG_BUILD_DIR))
endef

define Build/Compile
	CC="$(TARGET_CC)" \
	CFLAGS="$(TARGET_CFLAGS)" \
	LDFLAGS="$(TARGET_LDFLAGS) -L$(STAGING_DIR)/usr/lib -Wl,-rpath-link=$(STAGING_DIR)/usr/lib" \
	$(MAKE) -C $(PKG_BUILD_DIR)/client \
		all
endef

define Package/alexa/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(INSTALL_DIR) $(1)/usr/bin
	$(if $(CONFIG_ALEXA_SUPPORT_FF),$(CP) ./files/*.raw $(1)/usr/bin/)
	$(CP) ./files/*.sh $(1)/usr/bin
	$(CP) $(PKG_BUILD_DIR)/client/AlexaRequest 
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) ./files/AlexaClient.init $(1)/etc/init.d/AlexaClient
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_DIR) $(1)/etc/config
	$(CP) ./files/config/AlexaClientConfig.json $(1)/etc/config
endef

$(eval $(call BuildPackage,alexa))
