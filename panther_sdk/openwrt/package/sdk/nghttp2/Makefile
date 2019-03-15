#
# Copyright (C) 2014 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=nghttp2
PKG_VERSION:=1.24.0
PKG_RELEASE:=1
PKG_INSTALL:=1

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
#PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.bz2

DL_DIR=$(CURDIR)

include $(INCLUDE_DIR)/package.mk

define Package/libnghttp2
  SECTION:=libs
  CATEGORY:=Libraries
  #TITLE:=libnghttp2
  TITLE:=HTTP2 Library
  DEPENDS:= +libopenssl
endef

TARGET_CFLAGS += $(FPIC)

define Build/InstallDev
	$(INSTALL_DIR) $(1)/usr/include
	$(CP) $(PKG_INSTALL_DIR)/usr/include/nghttp2 $(1)/usr/include/
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/libnghttp2*.{a,so*} $(1)/usr/lib/
	$(INSTALL_DIR) $(1)/usr/lib/pkgconfig
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/pkgconfig/libnghttp2.pc $(1)/usr/lib/pkgconfig/
endef

define Package/libnghttp2/install
	$(INSTALL_DIR) $(1)/usr/lib
	#$(INSTALL_DATA) $(PKG_BUILD_DIR)/lib/.libs/libnghttp2.{a,so*} $(1)/usr/lib/
	#$(INSTALL_DATA) $(PKG_BUILD_DIR)/lib/.libs/libnghttp2.so* $(1)/usr/lib/
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/libnghttp2.so* $(1)/usr/lib/
endef

$(eval $(call BuildPackage,libnghttp2))
