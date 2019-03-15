#
# Copyright (C) 2010 Montage Inc.
#

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk
include $(INCLUDE_DIR)/kernel-defaults.mk

PKG_NAME:=monitor

PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)

include $(INCLUDE_DIR)/package.mk

define Package/monitor
  SECTION:=libs
  CATEGORY:=Administration
  SUBMENU:=Montage configuration items
  TITLE:= Montage Signal handler
  VERSION:=1
  DEPENDS:=+libmpdclient
endef

define Package/monitor/description
 This package contains Montage Signal
endef

define Package/monitor/config
    source "$(SOURCE)/Config.in"
endef

TARGET_CFLAGS += $(FPIC) -I${STAGING_DIR}/usr/include/

ifeq ($(CONFIG_MONITOR_GPIO_INPUTDEV),y)
	TARGET_CFLAGS += -DMONITOR_INPUTDEV -DMONITOR_GPIO_INPUTDEV
endif

ifeq ($(CONFIG_MONITOR_MADC_INPUTDEV),y)
	TARGET_CFLAGS += -DMONITOR_INPUTDEV -DMONITOR_MADC_INPUTDEV
endif

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
	$(call TSYNC,$(PKG_BUILD_DIR))
endef

define Build/Compile
	$(MAKE) -C $(PKG_BUILD_DIR) \
		$(TARGET_CONFIGURE_OPTS) \
		CFLAGS="$(TARGET_CFLAGS) " \
		LDFLAGS="$(TARGET_LDFLAGS) -lrt" 	\
		all
endef

define Build/InstallDev
	$(CP) $(PKG_BUILD_DIR)/lib/*.h $(STAGING_DIR)/usr/include/
	$(CP) $(PKG_BUILD_DIR)/lib/libmonitor.so $(STAGING_DIR)/usr/lib/
endef

define Package/monitor/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/lib/libmonitor.so $(1)/usr/lib/
endef

$(eval $(call BuildPackage,monitor))
