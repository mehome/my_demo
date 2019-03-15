#
# Copyright (C) 2014 Montage Inc.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=mhotplug

PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)

include $(INCLUDE_DIR)/package.mk

ifeq ($(CONFIG_PACKAGE_baiduasr),y)
	TARGET_CPPFLAGS+=-DBAIDU
endif

define Package/mhotplug
  CATEGORY:=Administration
  SUBMENU:=Montage configuration items
  TITLE:=Montage Hotplug
  VERSION:=1
  DEPENDS:=
  DEPENDS:=+wdk
endef

define Package/mhotplug/config
config PACKAGE_mhotplug_new
	bool "implement hotplug by C code"
	depends on PACKAGE_mhotplug
endef

TARGET_CFLAGS += \
	-I$(STAGING_DIR)/usr/include \
	-I$(STAGING_DIR)/usr/include/wdk

TARGET_LDFLAGS += \
	-Wl,-rpath-link=$(STAGING_DIR)/usr/lib -lcdb



define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef

define Build/Compile
	CC="$(TARGET_CC)" \
	CPPFLAGS="$(TARGET_CPPFLAGS)" \
	CFLAGS="$(TARGET_CFLAGS)" \
	LDFLAGS="$(TARGET_LDFLAGS)" \
	$(MAKE) -C $(PKG_BUILD_DIR) \
		all
endef


define Package/mhotplug/install
	$(INSTALL_DIR) $(1)/etc
	$(INSTALL_DIR) $(1)/sbin
#	$(CP) -a ./files/* $(1)/
	$(CP) -a ./files/etc/* $(1)/etc
#	$(INSTALL_BIN) $(PKG_BUILD_DIR)/hotplug-call $(1)/sbin/
endef

$(eval $(call BuildPackage,mhotplug))
