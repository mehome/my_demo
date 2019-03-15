#
# Copyright (C) 2010 Montage Inc.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=wdk

PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)

include $(INCLUDE_DIR)/package.mk

define Package/wdk
  SECTION:=libs
  CATEGORY:=Administration
  SUBMENU:=Montage configuration items
  TITLE:= Montage WDK
  VERSION:=1
  DEPENDS:=+libwhitedb +reboot2
endef

define Package/wdk/description
 This package contains Montage WDK
endef

#TARGET_CFLAGS += $(FPIC)
TARGET_CFLAGS += -D__PANTHER__
TARGET_CFLAGS_B += $(FPIC)

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
	$(call TSYNC,$(PKG_BUILD_DIR))
endef

define Build/Compile
	$(MAKE) -C $(PKG_BUILD_DIR) \
		$(TARGET_CONFIGURE_OPTS) \
		CFLAGS="$(TARGET_CFLAGS) $(TARGET_CFLAGS_B)" \
		all
	$(STRIP) $(PKG_BUILD_DIR)/cdb

	CC="$(TARGET_CC)" \
		$(MAKE) -C $(PKG_BUILD_DIR)/cgi \
		$(TARGET_CONFIGURE_OPTS) \
		CFLAGS="$(TARGET_CFLAGS) -I$(STAGING_DIR)/usr/include $(TARGET_CFLAGS_EXTRA)" \
		LDFLAGS="$(TARGET_LDFLAGS)"
	$(STRIP) $(PKG_BUILD_DIR)/cgi/upgrade.cgi
	$(STRIP) $(PKG_BUILD_DIR)/cgi/check_login.cgi
endef

define Build/InstallDev
	$(INSTALL_DIR) $(1)/usr/lib/pkgconfig $(1)/usr/include/wdk
	$(CP) $(PKG_BUILD_DIR)/include/* $(1)/usr/include/wdk
	$(CP) $(PKG_BUILD_DIR)/libcdb.so $(1)/usr/lib/
	#$(CP) $(PKG_BUILD_DIR)/libcdb.a $(1)/usr/lib/
endef

define Package/wdk/install
	$(CP) -a $(PKG_BUILD_DIR)/files/* $(1)/
ifeq ($(CONFIG_PACKAGE_zmtc),y)
	rm -fr $(1)/lib/wdk
endif
	$(INSTALL_DIR) $(1)/lib/wdk
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_BUILD_DIR)/libcdb.so $(1)/usr/lib/
	$(INSTALL_DIR) $(1)/sbin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/cdb $(1)/sbin
	$(INSTALL_DIR) $(1)/www
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/cgi/upgrade.cgi $(1)/www/upgrade.cgi
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/cgi/check_login.cgi $(1)/www/check_login.cgi
	export KCONF=$(LINUX_DIR)/.config; export ACONF=$(TOPDIR)/.config; ./strip.sh $(1)/lib/wdk
	ln -sf /usr/bin/avahi-publish-service $(1)/lib/wdk/omnicfg-publish-service
endef

$(eval $(call BuildPackage,wdk))
