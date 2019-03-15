#
# Copyright (C) 2014 Montage Inc.
#
export WIFIAUDIO_MONTAGE=y
#export WIFIAUDIO_MTK=y

include $(TOPDIR)/rules.mk

PKG_NAME:=test
PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)

LDFLAGS="$(TARGET_LDFLAGS) -L$(STAGING_DIR)/usr/lib -Wl,-rpath-link=$(STAGING_DIR)/usr/lib"

include $(INCLUDE_DIR)/package.mk
define Package/test
  	SECTION:=utils
  	CATEGORY:=Utilities
    TITLE:= test
    VERSION:=1
    DEPENDS:= +libavsclient +libjson-c 
endef


TARGET_CFLAGS += 

TARGET_LDFLAGS += -Wl,-rpath-link=$(STAGING_DIR)/usr/lib

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
	$(call TSYNC,$(PKG_BUILD_DIR))
endef

define Build/Compile
	$(TARGET_CC) $(TARGET_CFLAGS) -Os -lpthread -ldl $(PKG_BUILD_DIR)/test.c -o $(PKG_BUILD_DIR)/$(PKG_NAME) -lavsclient
endef

define Package/test/install
	mkdir -p $(1)/usr/bin/
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/test $(1)/usr/bin/
endef

$(eval $(call BuildPackage,test))
