#
# Copyright (C) 2009-2010 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=oprofile
PKG_VERSION:=1.2.0
PKG_RELEASE:=1

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
PKG_SOURCE_URL:=@SF/oprofile/
PKG_MD5SUM:=00aec1287da2dfffda17a9b1c0a01868
DL_DIR=$(CURDIR)

PKG_MAINTAINER:=Felix Fietkau <nbd@openwrt.org>

PKG_BUILD_DEPENDS:=binutils
PKG_LICENSE:=GPL-2.0+
PKG_FIXUP:=autoreconf
PKG_INSTALL:=1
PKG_BUILD_PARALLEL:=1
PKG_USE_MIPS16:=0

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/kernel.mk

define Package/oprofile1
  SECTION:=devel
  CATEGORY:=Development
  TITLE:=New OProfile System Profiler
  URL:=http://oprofile.sourceforge.net
  #DEPENDS:=+libpopt +kmod-oprofile +libstdcpp +objdump +librt @(!TARGET_uml||BROKEN)
  DEPENDS:=+libpopt +libstdcpp +objdump +librt @(!TARGET_uml||BROKEN)
endef

define Package/oprofile1/description
 A transparent low-overhead system-wide profiler.
endef

define Package/oprofile1-utils
  SECTION:=devel
  CATEGORY:=Development
  TITLE:=New OProfile System Profiler (extra utilities)
  URL:=http://oprofile.sourceforge.net
  DEPENDS:=oprofile1
endef

define Build/Configure
	$(call Build/Configure/Default, \
		--with-kernel-support \
		--without-x \
	)
endef

TARGET_CXXFLAGS += -fpermissive
TARGET_LDFLAGS := -L$(STAGING_DIR)/usr/lib $(TARGET_LDFLAGS) 

define Package/oprofile1-utils/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) \
		$(PKG_INSTALL_DIR)/usr/bin/opannotate \
		$(PKG_INSTALL_DIR)/usr/bin/oparchive \
		$(PKG_INSTALL_DIR)/usr/bin/opgprof \
		$(1)/usr/bin
endef

define Package/oprofile1/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) \
		$(PKG_INSTALL_DIR)/usr/bin/* \
		$(1)/usr/bin

	rm -f \
		$(1)/usr/bin/opannotate \
		$(1)/usr/bin/oparchive \
		$(1)/usr/bin/opgprof

	$(INSTALL_DIR) $(1)/usr/lib/oprofile
	$(CP) \
		$(PKG_INSTALL_DIR)/usr/lib/oprofile/*.so* \
		$(1)/usr/lib/oprofile/

	$(INSTALL_DIR) $(1)/usr/share/oprofile
	$(CP) \
		$(PKG_INSTALL_DIR)/usr/share/oprofile/stl.pat \
		$(PKG_INSTALL_DIR)/usr/share/oprofile/$(patsubst x86,i386,$(LINUX_KARCH)) \
		$(1)/usr/share/oprofile/
endef

$(eval $(call BuildPackage,oprofile1))
$(eval $(call BuildPackage,oprofile1-utils))
