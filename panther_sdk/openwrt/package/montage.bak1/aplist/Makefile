#
# Copyright (C) 2010 Montage Inc.
#

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk
include $(INCLUDE_DIR)/kernel-defaults.mk

PKG_NAME:=aplist

PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)

include $(INCLUDE_DIR)/package.mk

define Package/aplist
  SECTION:=libs
  CATEGORY:=Administration
  SUBMENU:=Montage configuration items
  TITLE:= Montage ap list info 
  VERSION:=1
  DEPENDS:=+wdk +libutils
endef

define Package/aplist/description
 This package contains Montage AP List
endef

TARGET_CFLAGS += $(FPIC) -I${STAGING_DIR}/usr/include/

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
	$(call TSYNC,$(PKG_BUILD_DIR))
endef

define Build/Compile
	$(MAKE) -C $(PKG_BUILD_DIR) \
		$(TARGET_CONFIGURE_OPTS) \
		CFLAGS="$(TARGET_CFLAGS) " \
		LDFLAGS="$(TARGET_LDFLAGS) -lrt -lpthread -ldl" 	\
		all
	$(STRIP) $(PKG_BUILD_DIR)/aplist
endef

define Package/aplist/install
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/aplist $(1)/usr/sbin
endef

$(eval $(call BuildPackage,aplist))
