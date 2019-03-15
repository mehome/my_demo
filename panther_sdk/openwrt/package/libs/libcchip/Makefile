#
# Copyright (C) 2008 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=libcchip
PKG_VERSION:=1.0.1
PKG_RELEASE:=1

PKG_FIXUP:=libtool

PKG_INSTALL:=0

include $(INCLUDE_DIR)/package.mk

define Package/libcchip
  SECTION:=libs
  CATEGORY:=Libraries
  DEPENDS:=+alsa-lib +wdk +libpthread +librt
  TITLE:=libcchip 
  # URL:=http://xiph.org/ogg/
endef

define Package/libcchip/config
	config MICARRY_RECOED
		bool "MICARRY RECOED"
		depends on PACKAGE_libcchip
	config CX20921_RECORD
		bool "CX20921 RECORD"
		depends on PACKAGE_libcchip
endef

ifeq ($(CONFIG_MICARRY_RECOED),y)
TARGET_CFLAGS += -DCONFIG_MICARRY_RECOED
TARGET_CFLAGS += -DMICS=2	
TARGET_CFLAGS += -DMICS_CHANNEL=1
TARGET_CFLAGS += -DREFS=1		 
TARGET_CFLAGS += -DREF_CHANNEL=1 
else ifeq ($(CONFIG_CX20921_RECORD),y)
TARGET_CFLAGS += -DCONFIG_CX20921_RECORD
TARGET_CFLAGS += -DMICS=2
TARGET_CFLAGS += -DMICS_CHANNEL=2
TARGET_CFLAGS += -DREFS=2
TARGET_CFLAGS += -DREF_CHANNEL=2
endif


define Package/libcchip/description
# Ogg project codecs use the Ogg bitstream format to arrange the raw,
# compressed bitstream into a more robust, useful form.  For example,
# the Ogg bitstream makes seeking, time stamping and error recovery
# possible, as well as mixing several sepearate, concurrent media
# streams into a single physical bitstream.
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
	$(call TSYNC,$(PKG_BUILD_DIR))
endef



define Build/InstallDev
	$(INSTALL_DIR) $(1)/usr/include/libcchip/
	cp $(PKG_INSTALL_DIR)/usr/include/* $(1)/usr/include/libcchip/ -rf
	$(INSTALL_DIR) $(1)/usr/lib/
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/*.so* $(1)/usr/lib/
endef

define Package/libcchip/install
	$(INSTALL_DIR) $(1)/usr/lib/
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/*.so* $(1)/usr/lib/
endef

$(eval $(call BuildPackage,libcchip))
