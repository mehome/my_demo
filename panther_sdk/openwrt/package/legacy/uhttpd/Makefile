#
# Copyright (C) 2010 Jo-Philipp Wich <xm@subsignal.org>
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=uhttpd
PKG_RELEASE:=7

PKG_BUILD_DIR := $(BUILD_DIR)/$(PKG_NAME)

include $(INCLUDE_DIR)/package.mk

define Package/uhttpd/default
  SECTION:=net
  CATEGORY:=Network
  TITLE:=uHTTPd - tiny, single threaded HTTP server
  DEPENDS:=+wdk
endef

define Package/uhttpd
  $(Package/uhttpd/default)
  MENU:=1
endef

define Package/uhttpd/description
 uHTTPd is a tiny single threaded HTTP server with TLS, CGI and Lua
 support. It is intended as a drop-in replacement for the Busybox
 HTTP daemon.
endef


define Package/uhttpd-mod-tls
  $(Package/uhttpd/default)
  TITLE+= (TLS plugin)
  DEPENDS:=uhttpd +libcyassl +px5g
endef

define Package/uhttpd-mod-tls/description
 The TLS plugin adds HTTPS support to uHTTPd.
endef


define Package/uhttpd-mod-lua
  $(Package/uhttpd/default)
  TITLE+= (Lua plugin)
  DEPENDS:=uhttpd +liblua
endef

define Package/uhttpd-mod-lua/description
 The Lua plugin adds a CGI-like Lua runtime interface to uHTTPd.
endef


# hack to use CyASSL headers
TARGET_CFLAGS += -I$(firstword $(wildcard $(BUILD_DIR)/cyassl-*/include))
# for file sharing access
TARGET_CFLAGS += -D_FILE_OFFSET_BITS=64
TARGET_LDFLAGS += -lm -lcdb
MAKE_VARS += FPIC="$(FPIC)"

# for omnicfg to fixed the user:pass at uhttp.c
ifeq ($(CONFIG_OMNI_CONFIGURATION),y)
TARGET_CFLAGS += -DCONFIG_OMNICFG
endif

#define Package/uhttpd/conffiles
#/etc/config/uhttpd
#endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
	$(call TSYNC,$(PKG_BUILD_DIR))
endef

define Package/uhttpd/install
	$(INSTALL_DIR) $(1)/etc/init.d
	#$(INSTALL_BIN) $(PKG_BUILD_DIR)/files/uhttpd.init $(1)/etc/init.d/uhttpd
	#$(INSTALL_BIN) ./files/uhttpd.init $(1)/etc/init.d/uhttpd
	#$(INSTALL_DIR) $(1)/etc/config
	#$(INSTALL_CONF) ./files/uhttpd.config $(1)/etc/config/uhttpd
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/uhttpd $(1)/usr/sbin/uhttpd
endef

define Package/uhttpd-mod-tls/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/uhttpd_tls.so $(1)/usr/lib/
endef

define Package/uhttpd-mod-lua/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/uhttpd_lua.so $(1)/usr/lib/
endef


$(eval $(call BuildPackage,uhttpd))
$(eval $(call BuildPackage,uhttpd-mod-tls))
#$(eval $(call BuildPackage,uhttpd-mod-lua))
