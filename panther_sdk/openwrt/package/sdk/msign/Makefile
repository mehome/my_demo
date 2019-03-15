include $(TOPDIR)/rules.mk

PKG_NAME:=msign
PKG_VERSION:=1.0
#PKG_RELEASE=$(PKG_SOURCE_VERSION)

#PKG_SOURCE_PROTO:=git
#PKG_SOURCE_URL:=git://git.openwrt.org/project/usign.git
#PKG_SOURCE_SUBDIR:=$(PKG_NAME)-$(PKG_VERSION)
#PKG_SOURCE_VERSION:=cf8dcdb8a4e874c77f3e9a8e9b643e8c17b19131
PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
CMAKE_INSTALL:=1
PKG_CHECK_FORMAT_SECURITY:=1
PKG_USE_MIPS16:=0

PKG_LICENSE:=ISC
PKG_LICENSE_FILES:=

DL_DIR:=${CURDIR}
USE_SRC_DIR:=1

PKG_BUILD_DEPENDS:=msign/host

PKG_MAINTAINER:=Felix Fietkau <nbd@openwrt.org>

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/host-build.mk
include $(INCLUDE_DIR)/cmake.mk

define Package/msign
  SECTION:=base
  CATEGORY:=Base system
  #DEPENDS:=+libubox
  TITLE:=Montage signature verification utility
endef

#CMAKE_OPTIONS += \
#	-DUSE_LIBUBOX=on

define Package/msign/install
	$(INSTALL_DIR) $(1)/usr/bin

	$(INSTALL_BIN) $(PKG_INSTALL_DIR)/usr/bin/msign $(1)/usr/bin
	#ln -s msign $(1)/usr/bin/signify
endef

$(eval $(call BuildPackage,msign))
$(eval $(call HostBuild))
