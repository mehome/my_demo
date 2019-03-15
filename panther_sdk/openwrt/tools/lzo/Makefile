#
# Copyright (C) 2006-2012 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=lzo
PKG_VERSION:=2.08
PKG_RELEASE:=1

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
PKG_SOURCE_URL:=http://www.oberhumer.com/opensource/lzo/download/
#PKG_MD5SUM:=fcec64c26a0f4f4901468f360029678f
DL_DIR:=${CURDIR}

HOST_BUILD_DIR:=$(BUILD_DIR_HOST)/$(PKG_NAME)-$(PKG_VERSION)

include $(INCLUDE_DIR)/host-build.mk

define Build/Configure
	$(call Build/Configure/Default, \
		--enable-shared \
		--enable-static \
	)
endef


#define Host/Compile
#	$(MAKE) -C $(UTIL_DIR) -f makefile.gcc
#	$(MAKE) -C $(ALONE_DIR) -f makefile.gcc
#endef

#define Host/Install
#	$(INSTALL_DIR) $(STAGING_DIR_HOST)/bin
#	$(INSTALL_BIN) $(HOST_BUILD_DIR)/CPP/7zip/Compress/LZMA_Alone/lzma_alone $(STAGING_DIR_HOST)/bin/lzma
#endef

define Host/Clean
endef

$(eval $(call HostBuild))
