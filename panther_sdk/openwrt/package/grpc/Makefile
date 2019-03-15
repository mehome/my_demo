#
# Copyright (C) 2007-2015 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=grpc
PKG_VERSION:=1.7.2

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
DL_DIR=$(CURDIR)

PKG_BUILD_DEPENDS:=grpc/host protobuf zlib libcares libopenssl
HOST_BUILD_DEPENDS:=protobuf/host zlib/host libcares/host libopenssl/host

PKG_INSTALL:=1

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/host-build.mk

define Package/grpc
  SECTION:=libs
  CATEGORY:=Libraries
  TITLE:=A General RPC library
  #URL:=https://github.com/grpc/grpc
  #DEPENDS:=+libpthread +protobuf +libcares +zlib +libopenssl
endef

#define Package/grpc/host
#  $(call Package/grpc)
#  DEPENDS:=+libpthread/host +protobuf/host +libcares/host +zlib/host +libopenssl/host
#endef


define Package/grpc/description
  Remote Procedure Calls (RPCs) provide a useful abstraction for building distributed applications and services. The libraries in this repository provide a concrete implementation of the gRPC protocol, layered over HTTP/2. These libraries enable communication between clients and servers using any combination of the supported languages.
endef

define Host/Compile
	CC=$(HOSTCC) \
	LD=$(HOSTCC) \
	prefix=$(STAGING_DIR_HOST) \
	$(MAKE) -C $(HOST_BUILD_DIR) all
endef

define Host/Install
	CC=$(HOSTCC) \
	LD=$(HOSTCC) \
	prefix=$(STAGING_DIR_HOST) \
	STAGING_PREFIX=$(STAGING_DIR_HOST) \
	PKG_CONFIG_PATH=$(STAGING_DIR_HOST)/lib/pkgconfig \
	$(MAKE) -C $(HOST_BUILD_DIR) install
endef

#CONFIGURE_ARGS += --with-protoc=$(STAGING_DIR_HOST)/bin/protoc
TARGET_CFLAGS += -D_GLIBCXX_USE_C99

define Build/Compile
	$(CONFIGURE_VARS) \
	LD=$(TARGET_CC) \
	STRIP=echo \
	protoc=$(STAGING_DIR_HOST)/bin/protoc \
	PROTOC_PLUGINS_DIR=$(STAGING_DIR_HOST)/bin \
	GRPC_CROSS_COMPILE=true \
	GRPC_CROSS_AROPTS=rcs \
	prefix=$(STAGING_DIR)/usr \
	$(MAKE) -C $(PKG_BUILD_DIR) all
endef

#define Build/Install
#	$(INSTALL_DIR) $(DESTDIR)
#	$(CONFIGURE_VARS) \
	LD=$(TARGET_CC) \
	STRIP=echo \
	protoc=$(STAGING_DIR_HOST)/bin/protoc \
	PROTOC_PLUGINS_DIR=$(STAGING_DIR_HOST)/bin \
	GRPC_CROSS_COMPILE=true \
	GRPC_CROSS_AROPTS=rcs \
	prefix=$(STAGING_DIR)/usr \
	MAKEFLAGS= \
	$(MAKE) -C $(PKG_BUILD_DIR) install
#endef

define Build/Install
endef

define Build/InstallDev
	$(INSTALL_DIR) \
		$(1)/usr/include/grpc

	$(CP) \
		$(PKG_BUILD_DIR)/include/grpc/* \
		$(1)/usr/include/grpc/

	$(INSTALL_DIR) \
		$(1)/usr/include/grpc++

	$(CP) \
		$(PKG_BUILD_DIR)/include/grpc++/* \
		$(1)/usr/include/grpc++/

	$(INSTALL_DIR) \
		$(1)/usr/lib

	$(CP) \
		$(PKG_BUILD_DIR)/libs/opt/*.a \
		$(1)/usr/lib/

	$(INSTALL_DIR) \
		$(1)/usr/lib/pkgconfig

	$(CP) \
		$(PKG_BUILD_DIR)/libs/opt/pkgconfig/*.pc \
		$(1)/usr/lib/pkgconfig/
endef

define Package/grpc/install
	$(INSTALL_DIR) $(1)/etc/grpc
	$(CP) $(PKG_BUILD_DIR)/etc/roots.pem $(1)/etc/grpc/
endef

$(eval $(call HostBuild))
$(eval $(call BuildPackage,grpc))
