#
# Copyright (C) 2014 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

export WIFIAUDIO_MONTAGE=y
#export WIFIAUDIO_MTK=y


include $(TOPDIR)/rules.mk

PKG_NAME:=uartd
PKG_RELEASE:=1


include $(INCLUDE_DIR)/package.mk

define Package/uartd
	SECTION:=utils
	CATEGORY:=Utilities
  TITLE:=uartd
  DEPENDS:= +libpthread +librt +alsa-lib +libasound +libmpdclient +wdk +libavsclient +libcchip +myutils
endef

define Package/uartd/description
  uart daemon
endef


TARGET_CFLAGS += \
	-I$(STAGING_DIR)/usr/include \

TARGET_LDFLAGS += -Wl,-gc-sections  \
  -Wl,-rpath-link=$(STAGING_DIR)/usr/lib -lcdb -lmyutils


#LDFLAGS="$(TARGET_LDFLAGS) -L$(STAGING_DIR)/usr/lib -Wl,-rpath-link=$(STAGING_DIR)/usr/lib" \
#TARGET_LDFLAGS += -Wl,-rpath-link=$(STAGING_DIR)/usr/lib  -ljson-c -ldl -lmpdclient -lmonitor -lcdb

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
	$(call TSYNC,$(PKG_BUILD_DIR))
endef



ifeq ($(CONFIG_PACKAGE_iot),y)
TARGET_CFLAGS += -DUSE_TURING
endif
TARGET_LDFLAGS += -lm
#LIBS:= -lpthread -ldl -lasound -lmpdclient



define Package/uartd/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/uartd $(1)/usr/bin/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/send_uartd_cmd $(1)/usr/bin/
	$(INSTALL_BIN) ./files/uartdfifo.sh $(1)/usr/bin
	$(INSTALL_BIN) ./files/factoryReset.sh $(1)/usr/bin
	$(INSTALL_BIN) ./files/speechfifo.sh $(1)/usr/bin
	$(INSTALL_BIN) ./files/elian.sh $(1)/usr/bin/
	$(INSTALL_BIN) ./files/namestatus.sh $(1)/usr/bin/
	$(INSTALL_BIN) ./files/btstatus.sh $(1)/usr/bin/
	$(INSTALL_BIN) ./files/mpg123.sh $(1)/usr/bin/	
#	$(INSTALL_BIN) ./files/closeusbeth.sh $(1)/usr/bin/
	$(INSTALL_BIN) ./files/Alexa_iflytek.sh  $(1)/usr/bin/
	$(INSTALL_BIN) ./files/setLanguage.sh $(1)/usr/bin/
	$(INSTALL_BIN) ./files/upmpdfifo.sh $(1)/usr/bin/
	$(INSTALL_BIN) ./files/free.sh $(1)/usr/bin/
	$(INSTALL_BIN) ./files/root $(1)/usr/bin/
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) ./files/uartd.init $(1)/etc/init.d/uartd
	$(INSTALL_DIR) $(1)/usr/script
	$(INSTALL_BIN) ./files/crondStart.sh $(1)/usr/script/
	$(INSTALL_BIN) ./files/settimetask.sh $(1)/usr/script/
	$(INSTALL_BIN) ./files/timerplayer.sh $(1)/usr/script/
	$(INSTALL_BIN) ./files/play_tone.sh $(1)/usr/bin/
	$(INSTALL_DIR) $(1)/root/res/cn
	
	
ifeq ($(CONFIG_PACKAGE_iot),y)
ifeq ($(CONFIG_PROJECT_BEDLAMP_V1),y)
	$(INSTALL_BIN) ./files/res/bedlamp_tone11/* $(1)/root/res/cn
else 
ifeq ($(CONFIG_PROJECT_K2_V1),y)
#	$(INSTALL_BIN) ./files/res/K2_tone0/* $(1)/root/res/cn
	$(INSTALL_BIN) ./files/res/K2_flying_tone0/* $(1)/root/res/cn
else
ifeq ($(CONFIG_PROJECT_DOSS_1MIC_V1),y)
	$(INSTALL_BIN) ./files/res/doss_1mic_tone0/* $(1)/root/res/cn
else
	$(INSTALL_BIN) ./files/res/cn/* $(1)/root/res/cn
endif
endif
endif
endif

ifeq ($(CONFIG_PACKAGE_alexa),y)
	$(INSTALL_DIR) $(1)/root/res/en
	$(INSTALL_BIN) ./files/res/en/* $(1)/root/res/en
	$(INSTALL_DIR) $(1)/root/res/voices
	$(INSTALL_BIN) ./files/res/voices/* $(1)/root/res
	$(INSTALL_DIR) $(1)/www/music
endif

	
endef

$(eval $(call BuildPackage,uartd))
