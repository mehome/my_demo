#
# Copyright (C) 2016 Montage Inc.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=changenc

PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)

include $(INCLUDE_DIR)/package.mk

define Package/changenc
  SECTION:=net
  CATEGORY:=Network
  TITLE:=Publish Broadcast Json
  VERSION:=1
endef

define Package/changenc/description
  Publish Json TXT at Broadcast address
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP)  ./src/* $(PKG_BUILD_DIR)/
	$(call TSYNC,$(PKG_BUILD_DIR))
endef

TARGET_CFLAGS += $(FPIC)

define Build/Compile
	$(MAKE) -C $(PKG_BUILD_DIR) \
		CC="$(TARGET_CC)" \
		CFLAGS="$(TARGET_CFLAGS) $(TARGET_CPPFLAGS)" \
		LDFLAGS="$(TARGET_LDFLAGS)"
endef

define Package/changenc/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/changenc $(1)/usr/bin/changenc
endef

$(eval $(call BuildPackage,changenc))
