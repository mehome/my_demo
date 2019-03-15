#
# Copyright (C) 2016 Montage Inc.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=mtc

PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)

include $(INCLUDE_DIR)/package.mk

define Package/mtc
  SECTION:=base
  CATEGORY:=Administration
  SUBMENU:=Montage configuration items
  TITLE:= Montage Controller
  VERSION:=1
  DEPENDS:=+libutils +wdk +libnl-tiny +alsa-lib +libavsclient +libcchip 
endef

define Package/mtc/description
  Montage Controller
endef

define Package/mtc/config
source "$(SOURCE)/Config.in"
endef

TARGET_CFLAGS += \
         -D__PANTHER__ \
	 -I$(STAGING_DIR)/usr/include/libnl-tiny \
	 -include $(STAGING_DIR)/usr/include/mon_config.h \
	 -include $(STAGING_DIR)/usr/include/mon_kconfig.h \
	 -I$(PKG_BUILD_DIR) \
	 -I$(STAGING_DIR)/usr/include \
	 -I$(STAGING_DIR)/usr/include/libutils \
	 -I$(STAGING_DIR)/usr/include/wdk \
#	 -Werror

ifeq ($(CONFIG_MTC_MONTAGE_STANDARD_BOARD),y)
TARGET_CFLAGS += -DMONTAGE_BOARD
endif
ifeq ($(CONFIG_MTC_POWER_DOWN_USB),y)
TARGET_CFLAGS += -DPOWER_DOWN_USB
endif
ifeq ($(CONFIG_MTC_POWER_DOWN_EPHY),y)
TARGET_CFLAGS += -DPOWER_DOWN_EPHY
endif
ifeq ($(CONFIG_MTC_POWER_DOWN_MADC),y)
TARGET_CFLAGS += -DPOWER_DOWN_MADC
endif

TARGET_CFLAGS += -DSPEEDUP_WLAN_CONF

TARGET_CFLAGS += -DOMNICFG_IPC_LOCK

TARGET_LDFLAGS += \
	-Wl,-rpath-link=$(STAGING_DIR)/usr/lib -ldl -lcdb -lutils -lnl-tiny \
	-lasound -lpthread -lrt -lm -lavsclient -ljson-c -lcchip

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
	$(call TSYNC,$(PKG_BUILD_DIR))
endef

define Build/Compile
	CC="$(TARGET_CC)" \
	CFLAGS="$(TARGET_CFLAGS)" \
	LDFLAGS="$(TARGET_LDFLAGS)" \
	$(MAKE) -C $(PKG_BUILD_DIR) \
		all
endef

define Package/mtc/install
	$(INSTALL_DIR) $(1)/etc
	# no need now, use package procd in linux ver.15
	#$(INSTALL_BIN) $(PKG_BUILD_DIR)/preinit $(1)/etc/

	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_DIR) $(1)/etc/config
	$(CP) ./files/etc/config/SystemConfiguration.json $(1)/etc/config
	$(INSTALL_DIR) $(1)/lib/wdk
	$(INSTALL_DIR) $(1)/www
	$(INSTALL_BIN) ./files/www/omnicfg.cgi $(1)/www/omnicfg.cgi

	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_DIR) $(1)/sbin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/mtdm $(1)/usr/bin/mtdaemon
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/mtc $(1)/usr/bin/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/mt_udhcpcs $(1)/usr/bin/
	
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/wdk/wdk-bin $(1)/lib/wdk/wdk-bin
	ln -sf wdk-bin $(1)/lib/wdk/sleep
	ln -sf wdk-bin $(1)/lib/wdk/ap
	ln -sf wdk-bin $(1)/lib/wdk/cdbreset
	ln -sf wdk-bin $(1)/lib/wdk/debug
	ln -sf wdk-bin $(1)/lib/wdk/dhcps
	ln -sf wdk-bin $(1)/lib/wdk/get_channel
	ln -sf wdk-bin $(1)/lib/wdk/logout
	ln -sf wdk-bin $(1)/lib/wdk/ping
	ln -sf wdk-bin $(1)/lib/wdk/reboot
	ln -sf wdk-bin $(1)/lib/wdk/route
	ln -sf wdk-bin $(1)/lib/wdk/wan
	ln -sf wdk-bin $(1)/lib/wdk/mangment
	ln -sf wdk-bin $(1)/lib/wdk/sta
	ln -sf wdk-bin $(1)/lib/wdk/stor
	ln -sf wdk-bin $(1)/lib/wdk/stapoll
	ln -sf wdk-bin $(1)/lib/wdk/status
	ln -sf wdk-bin $(1)/lib/wdk/system
	ln -sf wdk-bin $(1)/lib/wdk/http
	ln -sf wdk-bin $(1)/lib/wdk/sysupgrade
	ln -sf wdk-bin $(1)/lib/wdk/time
	ln -sf wdk-bin $(1)/lib/wdk/upnp
	ln -sf wdk-bin $(1)/lib/wdk/cdbupload
	ln -sf wdk-bin $(1)/lib/wdk/log
	ln -sf wdk-bin $(1)/lib/wdk/logconf
	ln -sf wdk-bin $(1)/lib/wdk/upload
	ln -sf wdk-bin $(1)/lib/wdk/wps
	ln -sf wdk-bin $(1)/lib/wdk/smbc
	ln -sf wdk-bin $(1)/lib/wdk/commit
	ln -sf wdk-bin $(1)/lib/wdk/save
	ln -sf wdk-bin $(1)/lib/wdk/rakey
	ln -sf wdk-bin $(1)/lib/wdk/mpc
	ln -sf wdk-bin $(1)/lib/wdk/pandora
	ln -sf wdk-bin $(1)/lib/wdk/omnicfg
	ln -sf wdk-bin $(1)/lib/wdk/omnicfg_ctrl
	ln -sf wdk-bin $(1)/lib/wdk/omnicfg_apply
	ln -sf /lib/wdk/wdk-bin $(1)/sbin/hotplug-call

endef

$(eval $(call BuildPackage,mtc))
