#
# Copyright (C) 2007-2016 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=glib-networking
PKG_VERSION:=2.54.0
PKG_RELEASE:=1

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.xz
#PKG_BUILD_DIR:=$(BUILD_DIR)/glib-$(PKG_VERSION)
#PKG_SOURCE_URL:=@GNOME/glib/2.53
#PKG_HASH:=028efbf6df4d88d7725314456524b923169f4d4f8503347194b2baf2222f3e33
DL_DIR=$(CURDIR)

PKG_BUILD_PARALLEL:=1
PKG_USE_MIPS16:=0

PKG_FIXUP:=autoreconf

include $(INCLUDE_DIR)/package.mk

define Package/glib-networking
  SECTION:=libs
  CATEGORY:=Libraries
  DEPENDS:=+glib2 +libgnutls
  TITLE:=glib-networking 2.0
  MAINTAINER:=Peter Wagner <tripolar@gmx.at>
  URL:=http://www.gtk.org/
endef

define Package/glib-networking/description
  The GLib-networking library
endef

TARGET_CFLAGS+=-I$(PKG_BUILD_DIR)

CONFIGURE_ARGS += \
	--enable-shared \
	--disable-static \
	--disable-nls \
	--with-gnutls

define Build/InstallDev
	$(INSTALL_DIR) $(1)/usr/lib/gio/modules
	$(CP) $(PKG_BUILD_DIR)/tls/gnutls/.libs/libgiognutls.so $(1)/usr/lib/gio/modules
endef

define Package/glib-networking/install
	$(INSTALL_DIR) $(1)/usr/lib/gio/modules
	$(CP) $(PKG_BUILD_DIR)/tls/gnutls/.libs/libgiognutls.so $(1)/usr/lib/gio/modules
endef

$(eval $(call BuildPackage,glib-networking))
