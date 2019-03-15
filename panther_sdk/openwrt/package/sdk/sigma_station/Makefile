include $(TOPDIR)/rules.mk

PKG_NAME:=sigma
PKG_VERSION:=9.0.0
PKG_RELEASE:=1

PKG_BUILD_DIR := $(BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)

include $(INCLUDE_DIR)/package.mk

define Package/sigma
  SECTION:=utils
  CATEGORY:=Utilities
  DEPENDS:=+libpthread +iputils-ping
  TITLE:=Sigma (Wi-fi alliance)
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
	$(call TSYNC,$(PKG_BUILD_DIR))
endef

define Package/sigma/install
	$(INSTALL_DIR) $(1)/sigma/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/ca/wfa_ca $(1)/sigma/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/dut/wfa_dut $(1)/sigma/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/run_ca.sh $(1)/sigma/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/run_dut.sh $(1)/sigma/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/run_wpa.sh $(1)/sigma/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/wpa.conf $(1)/sigma/
	$(INSTALL_BIN) -d $(1)/sigma/scripts
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/scripts/* -t $(1)/sigma/scripts
	
endef

$(eval $(call BuildPackage,sigma))
