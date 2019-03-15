#
# Copyright (C) 2010 Montage Inc.
#

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=cdb
PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)
ifeq ($(PROFILE),Default)
CDBPROFILE:=
else
CDBPROFILE:=-$(PROFILE)
endif

include $(INCLUDE_DIR)/package.mk

define Package/cdb
  SECTION:=libs
  CATEGORY:=Administration
  SUBMENU:=Montage configuration items
  TITLE:= Montage CDB
  VERSION:=1
endef

define Package/cdb/description
	This package contains Montage CDB
endef

define Package/cdb/config
	source "$(SOURCE)/Config.in"
endef

define Build/Prepare
endef

define Build/Compile
endef

define Package/cdb/install
	mkdir -p $(1)/etc/config
#	@if [ -f $(PLATFORM_SUBDIR)/cdbs/default.cdb$(CDBPROFILE) ]; then \
#		$(CP) -a $(PLATFORM_SUBDIR)/cdbs/default.cdb$(CDBPROFILE) $(1)/etc/config/default.cdb ; \
#		if [ ! -z $(CONFIG_CDB_SSID_NAME) ]; then \
#			$(SED) "s/wl_bss_ssid1=.*/wl_bss_ssid1='$(CONFIG_CDB_SSID_NAME)_%${CONFIG_CDB_SSID_SUFFIX_MACNUM}x'/" -e 's/\"//g' $(1)/etc/config/default.cdb ; \
#		fi \
#	else \
#		$(CP) -a $(PLATFORM_SUBDIR)/cdbs/default.cdb $(1)/etc/config/default.cdb ; \
#		echo "Assigned default.cdb$(CDBPROFILE) is nonexistent, change to default!!" ; \
#		if [ ! -z $(CONFIG_CDB_SSID_NAME) ]; then \
#			$(SED) "s/wl_bss_ssid1=.*/wl_bss_ssid1='$(CONFIG_CDB_SSID_NAME)_%${CONFIG_CDB_SSID_SUFFIX_MACNUM}x'/" -e 's/\"//g' $(1)/etc/config/default.cdb ; \
#		fi \
#	fi
	@if [ -f $(PLATFORM_SUBDIR)/cdbs/cdb.define$(CDBPROFILE) ]; then \
		$(CP) -a $(PLATFORM_SUBDIR)/cdbs/cdb.define$(CDBPROFILE) $(1)/etc/config/cdb.define ; \
	else \
		$(CP) -a $(PLATFORM_SUBDIR)/cdbs/cdb.define $(1)/etc/config/cdb.define ; \
		echo "Assigned cdb.define$(CDBPROFILE) is nonexistent, change to default!!" ; \
	fi
	@if [ -f $(PLATFORM_SUBDIR)/cdbs/rdb.define$(CDBPROFILE) ]; then \
		$(CP) -a $(PLATFORM_SUBDIR)/cdbs/rdb.define$(CDBPROFILE) $(1)/etc/config/rdb.define ; \
	else \
		$(CP) -a $(PLATFORM_SUBDIR)/cdbs/rdb.define $(1)/etc/config/rdb.define ; \
		echo "Assigned rdb.define$(CDBPROFILE) is nonexistent, change to default!!" ; \
	fi
endef

$(eval $(call BuildPackage,cdb))
