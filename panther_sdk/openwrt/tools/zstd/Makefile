#
# Copyright (C) 2006-2012 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=zstd
PKG_VERSION:=1.3.2
PKG_RELEASE:=1

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.bz2
PKG_SOURCE_URL:=https://github.com/facebook/zstd.git
PKG_SOURCE_PROTO:=git
PKG_SOURCE_SUBDIR:=$(PKG_NAME)
PKG_SOURCE_VERSION:=ed6e36e80928ce657cc808712d9b0de85eb34aed

HOST_BUILD_DIR:=$(BUILD_DIR_HOST)/$(PKG_NAME)-$(PKG_VERSION)

DL_DIR:=${CURDIR}

HOST_BUILD_PARALLEL:=1

include $(INCLUDE_DIR)/host-build.mk

define Host/Install
	$(INSTALL_DIR) $(STAGING_DIR_HOST)/{bin,lib,include}
	#$(CP) $(HOST_BUILD_DIR)/zstd $(STAGING_DIR_HOST)/bin/
	$(CP) $(HOST_BUILD_DIR)/lib/zstd.h $(STAGING_DIR_HOST)/include/
	$(CP) $(HOST_BUILD_DIR)/lib/common/zstd_errors.h $(STAGING_DIR_HOST)/include/
	$(CP) $(HOST_BUILD_DIR)/lib/{libzstd.a,libzstd.so,libzstd.so.1,libzstd.so.1.3.2} $(STAGING_DIR_HOST)/lib/
endef

$(eval $(call HostBuild))
