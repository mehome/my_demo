# 
# Copyright (C) 2006-2013 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=whitedb
PKG_VERSION:=0.7.3
PKG_RELEASE:=1

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
DL_DIR=$(CURDIR)

include $(INCLUDE_DIR)/package.mk

define Package/libwhitedb
  SECTION:=libs
  CATEGORY:=Libraries
  TITLE:=Library implementing the deflate compression method
  URL:=http://www.whitedb.net/
endef

TARGET_CFLAGS += $(FPIC)

#CONFIGURE_ARGS += --enable-locking=rpsin --disable-backlink --disable-checking
CONFIGURE_ARGS += --disable-backlink --disable-checking

define Build/InstallDev
	mkdir -p $(1)/usr/include/whitedb
	$(CP)	$(PKG_BUILD_DIR)/Db/dbapi.h $(1)/usr/include/whitedb/
	$(CP)   $(PKG_BUILD_DIR)/Db/indexapi.h $(1)/usr/include/whitedb/
	mkdir -p $(1)/usr/lib
	$(CP)	$(PKG_BUILD_DIR)/Main/.libs/libwgdb.a $(1)/usr/lib/
	#$(CP)	$(PKG_BUILD_DIR)/Main/.libs/libwgdb.{so*} $(1)/usr/lib/
endef

define Package/libwhitedb/install
	#$(INSTALL_DIR) $(1)/usr/lib
	#$(CP) $(PKG_INSTALL_DIR)/usr/lib/libz.so $(1)/usr/lib/
	#$(CP) $(PKG_INSTALL_DIR)/usr/lib/libz.so.* $(1)/usr/lib/
endef


$(eval $(call BuildPackage,libwhitedb))
