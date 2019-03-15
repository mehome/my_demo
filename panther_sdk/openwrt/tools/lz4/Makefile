#
# Copyright (C) 2006-2012 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=lz4
PKG_VERSION:=1.8.0
PKG_RELEASE:=1

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.bz2
PKG_SOURCE_URL:=https://github.com/lz4/lz4.git
PKG_SOURCE_PROTO:=git
PKG_SOURCE_SUBDIR:=$(PKG_NAME)
PKG_SOURCE_VERSION:=c10863b98e1503af90616ae99725ecd120265dfb
HOST_BUILD_DIR:=$(BUILD_DIR_HOST)/$(PKG_NAME)-$(PKG_VERSION)

DL_DIR:=${CURDIR}

HOST_BUILD_PARALLEL:=1

include $(INCLUDE_DIR)/host-build.mk

define Host/Install
	$(INSTALL_DIR) $(STAGING_DIR_HOST)/{bin,lib,include}
	#$(CP) $(HOST_BUILD_DIR)/lz4 $(STAGING_DIR_HOST)/bin/
	$(CP) $(HOST_BUILD_DIR)/lib/{lz4.h,lz4hc.h} $(STAGING_DIR_HOST)/include/
	$(CP) $(HOST_BUILD_DIR)/lib/{liblz4.a,liblz4.so,liblz4.so.1,liblz4.so.1.8.0} $(STAGING_DIR_HOST)/lib/
endef

$(eval $(call HostBuild))
