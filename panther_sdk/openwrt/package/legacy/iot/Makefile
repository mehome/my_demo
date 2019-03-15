#
# Copyright (C) 2014 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
CONFIG_PACKAGE_IOT_ENABLE_ULE=y
export CONFIG_PACKAGE_IOT_ENABLE_ULE
include $(TOPDIR)/rules.mk
CONFIG_IOT_ENABLE_OPUS=y

PKG_NAME:=iot
PKG_RELEASE:=1

include $(INCLUDE_DIR)/package.mk

define Package/iot
  SECTION:=sound
  CATEGORY:=Sound
  TITLE:=iot
  DEPENDS += +libjson-c +libcurl +libmosquitto +libopencore +libcares +libmpdclient  +libcchip +uci +myutils +alsa-lib +libmpg123 +libout123 +wakeup +libutils +libnl-tiny 
ifeq ($(CONFIG_PACKAGE_IOT_AMRWB),y)
	 DEPENDS += +amrwb
endif
ifeq ($(CONFIG_PACKAGE_IOT_MULTI),y)
	 DEPENDS += +libev
endif
ifeq ($(CONFIG_IOT_ENABLE_OPUS),y)
	 DEPENDS += +libopus
endif
endef

define Package/iot/description
  turing
endef
ifeq ($(CONFIG_PACKAGE_IOT_CLIENT),y)
export CONFIG_PACKAGE_IOT_CLIENT
endif
ifeq ($(CONFIG_PACKAGE_IOT_ENABLE_TTPOD),y)
export CONFIG_PACKAGE_IOT_ENABLE_TTPOD
endif
ifeq ($(CONFIG_PACKAGE_IOT_MULTI),y)
export CONFIG_PACKAGE_IOT_MULTI
endif
ifeq ($(CONFIG_PACKAGE_IOT_ENABLE_COMMSAT),y)
export CONFIG_PACKAGE_IOT_ENABLE_COMMSAT
endif
ifeq ($(CONFIG_PACKAGE_IOT_ENABLE_HUABEI),y)
export CONFIG_PACKAGE_IOT_ENABLE_HUABEI
endif
ifeq ($(CONFIG_PACKAGE_IOT_ENABLE_MIGU),y)
export CONFIG_PACKAGE_IOT_ENABLE_MIGU
endif
ifeq ($(CONFIG_IOT_ENABLE_OPUS),y)
export CONFIG_IOT_ENABLE_OPUS
endif
ifeq ($(CONFIG_PACKAGE_IOT_ENABLE_ULE),y)
export CONFIG_PACKAGE_IOT_ENABLE_ULE
endif
ifeq ($(CONFIG_PACKAGE_IOT_ENABLE_ULE),y)
export CONFIG_PACKAGE_IOT_ENABLE_ULE_IFLYTEK
endif
define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
ifeq ($(CONFIG_PACKAGE_IOT_ENABLE_ULE_IFLYTEK),y)
	$(CP) ./files/montage/*  $(STAGING_DIR)/usr/lib
endif
endef

TARGET_CFLAGS += \
	-I$(STAGING_DIR)/usr/include/ \
	-I$(STAGING_DIR)/usr/include/wdk \
	-I$(STAGING_DIR)/usr/include/wakeup \
	-I$(STAGING_DIR)/usr/include/opencore-amrnb \
	-DENABLE_LVMENG 
ifeq ($(CONFIG_PACKAGE_iot),y)
TARGET_CFLAGS += -DUSE_TURING_WECHAT
TARGET_CFLAGS += -DENABLE_CONTINUOUS_INTERACT
endif
	
ifeq ($(CONFIG_PACKAGE_IOT_AMRWB),y)	
TARGET_CFLAGS +=-I$(STAGING_DIR)/usr/include/opencore-amrwb 
endif

ifeq ($(CONFIG_PACKAGE_IOT_MSG),y)
TARGET_CFLAGS += -DUSE_IOT_MSG
endif

ifeq ($(CONFIG_PACKAGE_IOT_ENABLE_VOICE_DIARY),y)
TARGET_CFLAGS += -DENABLE_VOICE_DIARY
endif

ifeq ($(CONFIG_PACKAGE_IOT_ENABLE_TTPOD),y)
TARGET_CFLAGS += -DENABLE_TTPOD
endif

ifeq ($(CONFIG_PACKAGE_IOT_ENABLE_CODEC_ES8316),y)
TARGET_CFLAGS += -DENABLE_CODEC_ES8316
endif

ifeq ($(CONFIG_PACKAGE_IOT_BACKSTRACE),y)
TARGET_CFLAGS += -DENABLE_BACKSTRACE
endif 

#TARGET_LDFLAGS += -Wl,-rpath-link=$(STAGING_DIR)/usr/lib -lcurl -lmosquitto -lopencore-amrnb -lm  -Wl,-rpath-link=$(STAGING_DIR)/usr/lib -lavutil -lavformat -lavcodec  #-lvo-amrwbenc 
TARGET_LDFLAGS += -Wl,-rpath-link=$(STAGING_DIR)/usr/lib -lmyutils -lcurl -lmosquitto -lopencore-amrnb -lm -lrt -ldl  -lmpdclient -lcdb -lmpg123 -lout123  -lawaken -lvep_O3 -lthf  -lm -lcchip -lvoice -lutils -lnl-tiny #-lavutil  -lswresample
ifeq ($(CONFIG_PACKAGE_IOT_AMRWB),y)
TARGET_LDFLAGS +=  -L$(STAGING_DIR)/usr/lib  -lvo-amrwbenc
endif

ifeq ($(CONFIG_PACKAGE_IOT_BACKSTRACE),y)
TARGET_LDFLAGS +=  -L$(STAGING_DIR)/usr/lib   -lrt -ldl
endif 

ifeq ($(CONFIG_PACKAGE_IOT_MULTI),y)
TARGET_LDFLAGS += -L$(STAGING_DIR)/usr/lib -lev
endif

ifeq ($(CONFIG_IOT_ENABLE_OPUS),y)
TARGET_LDFLAGS +=  -L$(STAGING_DIR)/usr/lib  -Wl,-rpath-link=$(STAGING_DIR)/usr/lib -lopus
endif

ifeq ($(CONFIG_PACKAGE_IOT_ENABLE_ULE_IFLYTEK),y)
TARGET_LDFLAGS += -L$(STAGING_DIR)/usr/lib  -lmsc
endif

ifeq ($(CONFIG_PACKAGE_IOT_MULTI),y)
TARGET_CFLAGS += -DUSE_MULTI
endif

ifeq ($(CONFIG_PACKAGE_IOT_ENABLE_COMMSAT),y)
TARGET_CFLAGS += -DENABLE_COMMSAT
endif

ifeq ($(CONFIG_PACKAGE_IOT_ENABLE_YIYA),y)
TARGET_CFLAGS += -DENABLE_YIYA
endif

ifeq ($(CONFIG_PACKAGE_IOT_ENABLE_LVMENG),y)
TARGET_CFLAGS += -DENABLE_LVMENG
TARGET_CFLAGS += -DENABLE_CODEC_ES8316
endif

ifeq ($(CONFIG_PACKAGE_IOT_ENABLE_HUABEI),y)
TARGET_CFLAGS += -DENABLE_HUABEI
TARGET_CFLAGS += -DENABLE_CODEC_ES8316
endif

ifeq ($(CONFIG_PACKAGE_IOT_ENABLE_MIGU),y)
TARGET_CFLAGS += -DENABLE_MIGU
endif

ifeq ($(CONFIG_IOT_ENABLE_OPUS),y)
TARGET_CFLAGS += -DENABLE_OPUS
endif

ifeq ($(CONFIG_PACKAGE_IOT_ENABLE_ULE),y)
TARGET_CFLAGS += -DENABLE_ULE
ifeq ($(CONFIG_PACKAGE_IOT_ENABLE_ULE_IFLYTEK),y)
TARGET_CFLAGS += -DENABLE_ULE_IFLYTEK
endif
ifeq ($(CONFIG_PACKAGE_IOT_ENABLE_ULE_BAIDU),y)
TARGET_CFLAGS += -DENABLE_ULE_BAIDU
endif

endif

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
	$(CP) ./files/lib/*  $(STAGING_DIR)/usr/lib
endef

define Build/Compile
	CC="$(TARGET_CC)" \
	CFLAGS="$(TARGET_CFLAGS)" \
	LDFLAGS="$(TARGET_LDFLAGS)" \
	$(MAKE) -C $(PKG_BUILD_DIR) \
		all
endef

define Package/iot/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/turingIot $(1)/usr/bin/
	ln -sf /usr/bin/turingIot $(1)/usr/bin/micarray
ifeq ($(CONFIG_PACKAGE_IOT_CLIENT),y)
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/turingClient $(1)/usr/bin/
endif
	$(INSTALL_BIN) ./files/alertfifo.sh $(1)/usr/bin/
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) ./files/crond $(1)/etc/init.d/	
	$(INSTALL_DIR) $(1)/usr/lib
	$(INSTALL_DIR) $(1)/root/iot
	
ifeq ($(CONFIG_PROJECT_DOSS_1MIC_V1),y)
	$(INSTALL_BIN) ./files/root/doss_1mic_tone0/* $(1)/root/iot
else 
ifeq ($(CONFIG_PROJECT_BEDLAMP_V1),y)
	$(INSTALL_BIN) ./files/root/bedlamp_tone11/* $(1)/root/iot
else 
ifeq ($(CONFIG_PROJECT_K2_V1),y)
#	$(INSTALL_BIN) ./files/root/k2_tone0/* $(1)/root/iot	
	$(INSTALL_BIN) ./files/root/k2_flying_tone0/* $(1)/root/iot
else
	$(INSTALL_BIN) ./files/root/iot/* $(1)/root/iot	
endif
endif
endif	
	$(INSTALL_DIR) $(1)/etc/config
	
ifeq ($(CONFIG_PROJECT_K2_V1),y)
	$(INSTALL_BIN) ./files/k2_v1.json $(1)/etc/config/iot.json
#	$(INSTALL_BIN) ./files/k2_flying_v1.json $(1)/etc/config/iot.json	
else 
ifeq ($(CONFIG_PROJECT_BEDLAMP_V1),y)
	$(INSTALL_BIN) ./files/bedlamp_v1.json $(1)/etc/config/iot.json	
else
ifeq ($(CONFIG_PROJECT_DOSS_1MIC_V1),y)
	$(INSTALL_BIN) ./files/doss_1mic.json $(1)/etc/config/iot.json
else
	$(INSTALL_BIN) ./files/iot.json $(1)/etc/config/iot.json
endif
endif
endif

ifeq ($(CONFIG_PACKAGE_IOT_ENABLE_MIGU),y)
	$(INSTALL_DIR) $(1)/usr/lib
	$(INSTALL_BIN) ./files/lib/* $(1)/usr/lib
endif
ifeq ($(CONFIG_PACKAGE_IOT_ENABLE_ULE_IFLYTEK),y)
	$(INSTALL_DIR) $(1)/usr/lib
	$(INSTALL_BIN) ./files/montage/libmsc.so $(1)/usr/lib
endif
endef
$(eval $(call BuildPackage,iot))
