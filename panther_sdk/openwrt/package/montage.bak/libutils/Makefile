#
# Copyright (C) 2006-2009 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=libutils
PKG_VERSION:=1.0
PKG_RELEASE:=1

PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)

include $(INCLUDE_DIR)/package.mk

define Package/libutils
  SECTION:=libs
  CATEGORY:=Administration
  SUBMENU:=Montage configuration items
  TITLE:=Montange library
  DEPENDS:= +wdk +libnl-tiny
endef

define Package/libutils/description
 This package contains a lib to implement montage application easily
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
	$(call TSYNC,$(PKG_BUILD_DIR))
endef

TARGET_CFLAGS += $(FPIC) \
         -D__PANTHER__ \
         -I${STAGING_DIR}/usr/include/ \
	 -I${STAGING_DIR}/usr/include/libnl-tiny/ \
	 -Werror \
	 -D_FILE_OFFSET_BITS=64

TARGET_CFLAGS += -DOMNICFG_IPC_LOCK

MAKE_FLAGS += \
	CFLAGS="$(TARGET_CFLAGS) $(TARGET_CPPFLAGS)" \
	LDFLAGS="$(TARGET_LDFLAGS) $(LIBS)"

define Build/Compile
	$(MAKE) -C $(PKG_BUILD_DIR) $(TARGET_CONFIGURE_OPTS) CFLAGS="$(TARGET_CFLAGS)"
	$(INSTALL_DIR) $(STAGING_DIR)/usr/include
	#$(CP) $(TOPDIR)/.config $(STAGING_DIR)/usr/include/mon_config.h
	#$(SED) '/^#/d' -e 's/=.*/ 1/g' -e 's/-/_/g' -e 's/\./_/g' -e '/^$$$$/d' $(STAGING_DIR)/usr/include/mon_config.h 
	#$(SED) 's/^/#define /g' $(STAGING_DIR)/usr/include/mon_config.h
	#
	#$(CP) $(LINUX_DIR)/.config $(STAGING_DIR)/usr/include/mon_kconfig.h
	#$(SED) '/^#/d' -e 's/=.*/ 1/g' -e 's/-/_/g' -e 's/\./_/g' -e '/^$$$$/d' $(STAGING_DIR)/usr/include/mon_kconfig.h 
	#$(SED) 's/^/#define /g' $(STAGING_DIR)/usr/include/mon_kconfig.h
endef

define Build/InstallDev
	$(INSTALL_DIR) $(1)/usr/lib/pkgconfig $(1)/usr/include/libutils
	$(CP) $(PKG_BUILD_DIR)/*.h $(1)/usr/include/libutils
	$(CP) $(PKG_BUILD_DIR)/libutils.so $(1)/usr/lib/
	$(CP) ./files/libutils.pc $(1)/usr/lib/pkgconfig
endef

define Package/libutils/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_BUILD_DIR)/libutils.so $(1)/usr/lib/
endef

$(eval $(call BuildPackage,libutils))
