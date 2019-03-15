#
# Copyright (C) 2013 Montage Inc.
#

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=webui
#PKG_VERSION:=0.1
#PKG_RELEASE:=$(shell LC_ALL=C svn info ${PKG_SOURCE_URL} | sed -ne's/^Last Changed Rev: //p')

PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)
MY_RM=rm

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/kernel.mk

define Package/webui
  SECTION:=admin
  CATEGORY:=Administration
  SUBMENU:=Montage configuration items
  DEPENDS:=+uhttpd +wdk +cdb
  TITLE:=WebUI for OpenWrt
  VERSION:=1
endef

define Package/webui/config
source "$(SOURCE)/config.in"
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR) 
	$(MY_RM) -rf ./src/www
	cp -rf ./src/www1 ./src/www 
	./build.sh ./src/www
	$(CP) ./src/* $(PKG_BUILD_DIR)/ 
endef

define Build/Compile
endef

define Package/webui/install
	$(CP) -a $(PKG_BUILD_DIR)/www $(1)/www
endef

$(eval $(call BuildPackage,webui))
