include $(TOPDIR)/rules.mk

PKG_NAME:=reboot2
PKG_VERSION:=1.0.0

include $(INCLUDE_DIR)/package.mk

define Package/reboot2
	SECTION:=admin
	CATEGORY:=Administration
	SUBMENU:=Montage configuration items
	TITLE:=Reboot Immediately
	VERSION:=1
endef

TARGET_CFLAGS = -Wall -O3

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
	$(call TSYNC,$(PKG_BUILD_DIR))
endef

define Package/reboot2/install
	$(INSTALL_DIR) $(1)/sbin
	$(CP) $(PKG_BUILD_DIR)/reboot2 $(1)/sbin/
endef

$(eval $(call BuildPackage,reboot2))
