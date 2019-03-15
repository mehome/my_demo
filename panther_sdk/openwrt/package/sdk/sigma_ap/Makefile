include $(TOPDIR)/rules.mk

PKG_NAME:=sigma_ap
PKG_VERSION:=1.0
PKG_RELEASE:=1

PKG_BUILD_DIR := $(BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)

#pull it from https://github.com/qca/sigma-dut

include $(INCLUDE_DIR)/package.mk

define Package/sigma_ap
  SECTION:=utils
  CATEGORY:=Utilities
  DEPENDS:=+libpthread
  TITLE:=Sigma test for ap mode (Wi-fi alliance)
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
	$(call TSYNC,$(PKG_BUILD_DIR))
endef

define Package/sigma_ap/install
	$(INSTALL_DIR) $(1)/sigma_ap/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/sigma_dut $(1)/sigma_ap/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/*.py -t $(1)/sigma_ap/
	$(INSTALL_BIN) -d $(1)/sigma_ap/scripts
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/scripts/* -t $(1)/sigma_ap/scripts
	
endef

$(eval $(call BuildPackage,sigma_ap))
