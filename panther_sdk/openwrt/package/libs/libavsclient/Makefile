#
# Copyright (C) 2014 Montage Inc.
#


include $(TOPDIR)/rules.mk

PKG_NAME:=libavsclient

PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)
TARGET_LDFLAGS += -Wl,-rpath-link=$(STAGING_DIR)/usr/lib
include $(INCLUDE_DIR)/package.mk

define Package/libavsclient
  SECTION:=libs
  CATEGORY:=Libraries
  TITLE:=libavsclient with cchip
  ABI_VERSION:=$(PKG_VERSION)
  VERSION:=1
  DEPENDS:= +libopenssl +libcrypto +libjson-c
endef


define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef

define Build/Compile
	CC="$(TARGET_CC)" \
	CCFLAGS="$(TARGET_CCFLAGS)" \
	LDFLAGS="$(TARGET_LDFLAGS) -L$(STAGING_DIR)/usr/lib -fPIC -Wl,-rpath-link=$(STAGING_DIR)/usr/lib" \
	$(MAKE) -C $(PKG_BUILD_DIR)/ \
		all
		
endef

define Package/libavsclient/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) ./src/include/avsclient $(STAGING_DIR)/usr/include/
	$(CP) $(PKG_BUILD_DIR)/*.so* $(STAGING_DIR)/usr/lib/
	$(CP) $(PKG_BUILD_DIR)/*.so* $(1)/usr/lib/
endef

$(eval $(call BuildPackage,libavsclient))

