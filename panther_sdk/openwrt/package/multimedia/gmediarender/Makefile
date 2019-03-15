#
# Copyright (C) 2013 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=gmediarender
PKG_VERSION:=2017-10-01
PKG_RELEASE:=1
PKG_REV:=fa3baff2ae7a877d1a6f00900169dbce41729765

PKG_SOURCE_PROTO:=git
PKG_SOURCE_URL:=https://github.com/hzeller/gmrender-resurrect.git
PKG_SOURCE_SUBDIR:=$(PKG_NAME)-$(PKG_VERSION)
PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz

USE_SRC_DIR:=1

PKG_FIXUP:=autoreconf
PKG_INSTALL=1
PKG_REMOVE_FILES:=autogen.sh aclocal.m4

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/nls.mk

define Package/gmediarender
  SECTION:=multimedia
  CATEGORY:=Multimedia
  DEPENDS:= +libpthread +libgstreamer1 +glib2 +libupnp
  TITLE:=A Headless UPnP Renderer
endef

define Package/gmediarender/description
  gmediarender implements the server component that provides UPnP
  controllers a means to render media content (audio, video and images)
  from a UPnP media server.
endef

CONFIGURE_ARGS+= \
	CFLAGS="-std=c99" \
	LIBS="-lpthread -ldl" \
	--with-build-cc="$(HOSTCC)" \
	--prefix="\usr"

define Package/gmediarender/install
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_BIN) $(PKG_INSTALL_DIR)/usr/bin/$(PKG_NAME) $(1)/usr/sbin/
	$(INSTALL_DIR) $(1)/etc/init.d
	$(CP) 		   $(PKG_BUILD_DIR)/scripts/init.d/$(PKG_NAME)er $(1)/etc/init.d/$(PKG_NAME)
endef

$(eval $(call BuildPackage,gmediarender))
