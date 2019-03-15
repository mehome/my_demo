include $(TOPDIR)/rules.mk

PKG_NAME:=wt
PKG_VERSION:=1.0
PKG_RELEASE:=1

PKG_BUILD_DIR := $(BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)

include $(INCLUDE_DIR)/package.mk

define Package/wt
	SECTION:=utils
	CATEGORY:=Utilities
	TITLE:=Wifi test tool (Dragonite Driver Only)
	URL:=http://www.montage-tech.com/
	VERSION:=1
	DEPENDS:=+libutils +wdk +libnl-tiny +bb
endef

TARGET_LDFLAGS += -L$(STAGING_DIR)/usr/lib \
		-I$(STAGING_DIR)/usr/include/libnl-tiny \
		-I$(STAGING_DIR)/usr/include/libutils \
		-I$(STAGING_DIR)/usr/include/wdk \
		-Werror

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
	$(call TSYNC,$(PKG_BUILD_DIR))
endef

define Package/wt/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/wt $(1)/usr/bin
endef

$(eval $(call BuildPackage,wt))
