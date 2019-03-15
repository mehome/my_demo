#
# Copyright (C) 2017 Montage Inc.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=bootvars
PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)

include $(INCLUDE_DIR)/package.mk

define Package/bootvars
  SECTION:=libs
  CATEGORY:=Administration
  SUBMENU:=Montage configuration items
  TITLE:= Montage Bootvars
  VERSION:=1
  DEPENDS:=+mtd +wdk +cdb
endef

define Package/bootvars/description
	This package contains Montage Bootvars, which read bootvars from flash
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
	$(call TSYNC,$(PKG_BUILD_DIR))
endef

define Build/Compile
	$(MAKE) -C $(PKG_BUILD_DIR) \
		$(TARGET_CONFIGURE_OPTS) \
		CFLAGS="$(TARGET_CFLAGS)" \
		all
	$(STRIP) $(PKG_BUILD_DIR)/bootvars
endef

define Package/bootvars/install
	$(INSTALL_DIR) $(1)/sbin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/bootvars $(1)/sbin
endef

$(eval $(call BuildPackage,bootvars))
