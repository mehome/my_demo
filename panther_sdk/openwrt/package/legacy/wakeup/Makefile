#
# Copyright (C) 2014 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

export WIFIAUDIO_MONTAGE=y

include $(TOPDIR)/rules.mk

PKG_NAME:=wakeup
PKG_RELEASE:=1

USE_SRC_DIR:=1

include $(INCLUDE_DIR)/package.mk

define Package/wakeup
  SECTION:=libs
  CATEGORY:=Libraries
  DEPENDS:=+alsa-lib +libpthread +wdk +libffmpeg-custom +libdl +libcchip
  TITLE:=wakeup 
endef

define Package/wakeup/config
	config INPUT_MIC_HW2_REF_HW2
		bool "INPUT: 2MIC + 2REF(HW)"
		depends on PACKAGE_wakeup
	config INPUT_MIC_HW3_REF_HW2
		bool "INPUT: 3MIC + 2REF(HW)"
		depends on PACKAGE_wakeup
	config INPUT_MIC_HW4_REF_HW2
		bool "INPUT: 4MIC + 2REF(HW)"
		depends on PACKAGE_wakeup
		default y
	config INPUT_MIC_HW1_REF_HW1
		bool "INPUT: 1MIC + 1REF(HW)"
		depends on PACKAGE_wakeup
	config INPUT_MIC_HW2_REF_SW2
		bool "INPUT: 2MIC + 2REF(SW)"
		depends on PACKAGE_wakeup
	config INPUT_MIC_HW3_REF_SW2
		bool "INPUT: 3MIC + 2REF(SW)"
		depends on PACKAGE_wakeup
	config INPUT_MIC_HW4_REF_SW2
		bool "INPUT: 4MIC + 2REF(SW)"
		depends on PACKAGE_wakeup
	config ASP_VEP
		bool "ASP: Alango VEP"
		depends on PACKAGE_wakeup
		default y
	config ASP_VEP_OPT
		bool "ASP: optimized Alango VEP"
		depends on PACKAGE_wakeup
	config ASP_VCP
		bool "ASP: Alango VCP"
		depends on PACKAGE_wakeup
	config ASP_SQE
		bool "ASP: SQE"
		depends on PACKAGE_wakeup
	config ASP_YSZ
		bool "ASP: YSZ"
		depends on PACKAGE_wakeup
	
	config CX_20921
		bool "CX_20921: CX_20921"
		depends on PACKAGE_wakeup
		
	config KWD_SENSORY
		bool "KWD: Sensory THF"
		depends on PACKAGE_wakeup
		default y
	config KWD_KITTAI
		bool "KWD: Baidu/Kitt.ai"
		depends on PACKAGE_wakeup
	config KWD_SKV
		bool "KWD: YSZ/skv"
		depends on PACKAGE_wakeup
endef


TARGET_CFLAGS = -D__PANTHER__ -I. -I./include
TARGET_LDFLAGS = -L./lib
TARGET_LIBS_A	= 

ifeq ($(CONFIG_PACKAGE_iot),y)
TARGET_CFLAGS += -DUSE_TURING_WECHAT
endif

export TARGET_LIBS_A

ifeq ($(CONFIG_ASP_VEP),y)
TARGET_CFLAGS += -DCONFIG_ASP_VEP 
TARGET_LDFLAGS += -lvep_O3
TARGET_LIBS_A	+= ./lib/libvep_O3.a
else ifeq ($(CONFIG_ASP_VEP_OPT),y)
TARGET_CFLAGS += -DCONFIG_ASP_VEP_OPT
TARGET_LDFLAGS += -lvep_opt
TARGET_LIBS_A	+= ./lib/libvep_opt.a
else ifeq ($(CONFIG_ASP_SQE),y)
TARGET_CFLAGS += -DCONFIG_ASP_SQE
TARGET_LDFLAGS += -lMNPCSQE
else ifeq ($(CONFIG_ASP_YSZ),y)
TARGET_CFLAGS += -DCONFIG_ASP_YSZ
TARGET_LDFLAGS += 
else ifeq ($(CONFIG_ASP_VCP),y)
TARGET_CFLAGS += -DCONFIG_ASP_VCP 
TARGET_LDFLAGS += -lvcp -lagc
TARGET_LIBS_A	+= ./lib/libvcp.a ./lib/libagc.a
else ifeq ($(CONFIG_CX_20921),y)
TARGET_CFLAGS += -DCONEXANT_20921
endif

ifeq ($(CONFIG_INPUT_MIC_HW2_REF_HW2),y)
TARGET_CFLAGS += -DCONFIG_INPUT_MIC_HW2_REF_HW2
else ifeq ($(CONFIG_INPUT_MIC_HW3_REF_HW2),y)
TARGET_CFLAGS += -DCONFIG_INPUT_MIC_HW3_REF_HW2
else ifeq ($(CONFIG_INPUT_MIC_HW4_REF_HW2),y)
TARGET_CFLAGS += -DCONFIG_INPUT_MIC_HW4_REF_HW2
else ifeq ($(CONFIG_INPUT_MIC_HW1_REF_HW1),y)
TARGET_CFLAGS += -DCONFIG_INPUT_MIC_HW1_REF_HW1
else ifeq ($(CONFIG_INPUT_MIC_HW2_REF_SW2),y)
TARGET_CFLAGS += -DCONFIG_INPUT_MIC_HW2_REF_SW2
else ifeq ($(CONFIG_INPUT_MIC_HW3_REF_SW2),y)
TARGET_CFLAGS += -DCONFIG_INPUT_MIC_HW3_REF_SW2
else ifeq ($(CONFIG_INPUT_MIC_HW4_REF_SW2),y)
TARGET_CFLAGS += -DCONFIG_INPUT_MIC_HW4_REF_SW2
endif

ifeq ($(CONFIG_KWD_SENSORY),y)
TARGET_CFLAGS += -DCONFIG_KWD_SENSORY
TARGET_LDFLAGS += -lthf
TARGET_LIBS_A	+= ./lib/libthf.a
endif

ifeq ($(CONFIG_KWD_KITTAI),y)
TARGET_CFLAGS += -DCONFIG_KWD_KITTAI
TARGET_LDFLAGS += -lsnowboy_opt
TARGET_LIBS_A	+= ./lib/libsnowboy_opt.a
endif

ifeq ($(CONFIG_KWD_SKV),y)
TARGET_CFLAGS += -DCONFIG_KWD_SKV
TARGET_LDFLAGS += -lawaken
TARGET_LDFLAGS += -Wl,-rpath-link=$(STAGING_DIR)/root-panther/lib
TARGET_LIBS_A	+= ./lib/libawaken.a
endif

ifeq ($(CONFIG_INPUT_MIC_HW2_REF_SW2),y)
TARGET_LDFLAGS += -lswresample -lavutil -lm
else ifeq ($(CONFIG_INPUT_MIC_HW4_REF_SW2),y)
TARGET_LDFLAGS += -lswresample -lavutil -lm
else ifeq ($(CONFIG_INPUT_MIC_HW1_REF_SW1),y)
TARGET_LDFLAGS += -lswresample -lavutil -lm
endif

define Build/InstallDev
	$(INSTALL_DIR) $(1)/usr/include/wakeup/
	cp $(PKG_INSTALL_DIR)/usr/include/* $(1)/usr/include/wakeup/ -rf
	$(INSTALL_DIR) $(1)/usr/lib/
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/*.a $(1)/usr/lib/
endef

define Package/wakeup/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(INSTALL_DIR) $(1)/etc
ifeq ($(CONFIG_INPUT_MIC_HW2_REF_HW2),y)
	$(CP) ./files/alango/profile_2mic-3outs-dist66-SAEC-AGC-2018-06-14.bix $(1)/etc/
endif
ifeq ($(CONFIG_INPUT_MIC_HW3_REF_HW2),y)
	$(CP) ./files/alango/profile_3mic-7outs-circular-radius35-SAEC-2018-05-29.bix $(1)/etc/
endif
ifeq ($(CONFIG_INPUT_MIC_HW4_REF_HW2),y)
	$(CP) ./files/alango/profile_4mic-9outs-circular-r33-MAEC-AGC-2018-06-13-36.bix $(1)/etc/profile_4mic-9outs-circular-r33-AEC-AGC.bix
endif
ifeq ($(CONFIG_ASP_SQE),y)
	$(CP) $(PKG_BUILD_DIR)/lib/libMNPCSQE.so $(1)/usr/lib/libMNPCSQE.so
endif
	# $(INSTALL_DIR) $(1)/usr/lib
	# $(INSTALL_BIN) $(PKG_BUILD_DIR)/libwakeup.a $(1)/usr/lib/
	#$(INSTALL_DIR) $(1)/etc/init.d
	#$(INSTALL_BIN) ./files/wakeup.init $(1)/etc/init.d/wakeup
ifeq ($(CONFIG_KWD_SENSORY),y)
	$(CP) ./files/sensory/*.raw $(1)/etc
endif
ifeq ($(CONFIG_KWD_KITTAI),y)
	$(CP) ./files/kitt.ai/* $(1)/etc
endif
ifeq ($(CONFIG_ASP_YSZ),y)
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/libzmath7.so $(1)/usr/lib/
endif
endef

$(eval $(call BuildPackage,wakeup))
