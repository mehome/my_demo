#
# Copyright (C) 2009-2012 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
include $(TOPDIR)/rules.mk

PKG_NAME:=squashfs4
PKG_VERSION:=4.3

PKG_SOURCE:=squashfs$(PKG_VERSION).tar.gz
PKG_SOURCE_URL:=@SF/squashfs
DL_DIR:=${CURDIR}
PKG_CAT:=zcat

HOST_BUILD_DIR:=$(BUILD_DIR_HOST)/squashfs$(PKG_VERSION)

include $(INCLUDE_DIR)/host-build.mk

define Host/Compile
	$(MAKE) -C $(HOST_BUILD_DIR)/squashfs-tools \
		CC="$(HOSTCC)" \
		XZ_SUPPORT=1 \
		LZO_SUPPORT=1 \
		LZMA_XZ_SUPPORT=1 \
		LZ4_SUPPORT=1 \
		ZSTD_SUPPORT=1 \
		XATTR_SUPPORT=1 \
		LZMA_LIB="$(STAGING_DIR_HOST)/lib/liblzma.a" \
		LZO_LIB="$(STAGING_DIR_HOST)/lib/liblzo2.a" \
		LZ4_LIB="$(STAGING_DIR_HOST)/lib/liblz4.a" \
		ZSTD_LIB="$(STAGING_DIR_HOST)/lib/libzstd.a" \
		EXTRA_CFLAGS="-I$(STAGING_DIR_HOST)/include" \
		mksquashfs unsquashfs
endef

define Host/Install
	$(INSTALL_BIN) $(HOST_BUILD_DIR)/squashfs-tools/mksquashfs $(STAGING_DIR_HOST)/bin/mksquashfs4
	$(INSTALL_BIN) $(HOST_BUILD_DIR)/squashfs-tools/unsquashfs $(STAGING_DIR_HOST)/bin/unsquashfs4
endef

define Host/Clean
	rm -f $(STAGING_DIR_HOST)/bin/mksquashfs4
	rm -f $(STAGING_DIR_HOST)/bin/unsquashfs4
endef

$(eval $(call HostBuild))
