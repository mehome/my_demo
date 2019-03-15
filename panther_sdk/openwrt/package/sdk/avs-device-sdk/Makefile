#

include $(TOPDIR)/rules.mk

PKG_NAME:=avs-device-sdk
PKG_VERSION:=1.7.0
PKG_RELEASE:=1

#PKG_INSTALL:=1

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz

DL_DIR=$(CURDIR)

USE_SOURCE_DIR:=$(CURDIR)/src
#USE_SOURCE_DIR:=$(CURDIR)/src_public
SEPERATE_SOURCE_BUILD_DIR:=1
PRIVATE:=1
PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)-build
CMAKE_SOURCE_DIR:=$(BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/cmake.mk

MPDPLAYER:=1

define Package/avs-device-sdk
  SECTION:=net
  CATEGORY:=Network
  TITLE:=Alexa device SDK
ifeq ($(MPDPLAYER),0)
  DEPENDS:= +libpthread +libopenssl +libnghttp2 +libcurl +player-lib
  DEPENDS+= +libatomic +libstdcpp
  DEPENDS+= +libsqlite3
  DEPENDS+= +portaudio
  DEPENDS+= +libgstreamer1 +gstreamer1-libs +gstreamer1-plugins-base +gst1-libav +gstreamer1-plugins-good
  DEPENDS+= +gst1-mod-app +gst1-mod-autodetect
  #DEPENDS+= +gst1-mod-mpegpsdemux
else
  DEPENDS:= +libpthread +libopenssl +libnghttp2 +libcurl
  DEPENDS+= +libblobmsg-json +libubus +libubox +json-c +libmpdclient
  DEPENDS+= +libatomic +libstdcpp
  DEPENDS+= +libsqlite3
  DEPENDS+= +portaudio
endif
endef

define Package/avs-device-sdk/config
	config AVS_DEVSDK_NO_CACERT_CHECK
		depends on PACKAGE_avs-device-sdk
		bool "Disable peer SSL certificate check"
		default n
		help
			Enable this option if there is no CA certificates available in the root-filesystem
endef

define Package/avs-sample-app
  SECTION:=net
  CATEGORY:=Network
  TITLE:=Alexa sample APP
  DEPENDS:= +avs-device-sdk
endef

define Package/avs-sample-app/config
	config AVS_SAMPLE_APP_USE_THF_KWD
		depends on PACKAGE_avs-sample-app
		bool "Enable integration with TrulyHandsfree key word detector"
		default n
		help
			Enable this option if there is TrulyHandsfree library available
endef

TARGET_CFLAGS += -D_GLIBCXX_USE_C99 -pthread -lm

ifeq ($(MPDPLAYER),0)
TARGET_CFLAGS += -lplayer
else
CMAKE_OPTIONS += -DMPD_MEDIA_PLAYER=ON
endif
#TARGET_CFLAGS += -ggdb -D_GLIBCXX_USE_C99 -pthread -lm

TARGET_CFLAGS += -D__PANTHER__

ifeq ($(CONFIG_AVS_DEVSDK_NO_CACERT_CHECK),y)
TARGET_CFLAGS += -DSSL_NO_CACERT_CHECK
endif

#CMAKE_OPTIONS += -DCMAKE_THREAD_LIBS_INIT="-lpthread -ldl"
  
CMAKE_OPTIONS += -DCMAKE_BUILD_TYPE=DEBUG -DACSDK_EMIT_SENSITIVE_LOGS=ON
#CMAKE_OPTIONS += -DCMAKE_BUILD_TYPE=MINSIZEREL

ifeq ($(MPDPLAYER),0)
CMAKE_OPTIONS += -DGSTREAMER_MEDIA_PLAYER=ON
                 #-DCMAKE_PREFIX_PATH=<path-to-GStreamer-build>
endif

CMAKE_OPTIONS += -DPORTAUDIO=ON \
                 -DPORTAUDIO_LIB_PATH=$(STAGING_DIR)/usr/lib/libportaudio.a \
                 -DPORTAUDIO_INCLUDE_DIR=$(STAGING_DIR)/usr/include

ifeq ($(CONFIG_AVS_SAMPLE_APP_USE_THF_KWD),y)
CMAKE_OPTIONS += -DKWD=ON -DTHF_KEY_WORD_DETECTOR=ON \
                 -DTHF_KEY_WORD_DETECTOR_INCLUDE_DIR=$(TOPDIR)/package/sdk/avs-device-sdk/src/KWD/TrulyHandsfreewakeup/include
ifeq ($(CONFIG_BIG_ENDIAN),y)
CMAKE_OPTIONS += -DTHF_KEY_WORD_DETECTOR_LIB_PATH=$(TOPDIR)/package/sdk/avs-device-sdk/src/KWD/TrulyHandsfree/lib/libthf_be.a
else
CMAKE_OPTIONS += -DTHF_KEY_WORD_DETECTOR_LIB_PATH=$(TOPDIR)/package/sdk/avs-device-sdk/src/KWD/TrulyHandsfree/lib/libthf_le.a
endif
endif

define Package/avs-device-sdk/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/* $(1)/usr/lib	
endef

define Package/avs-sample-app/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_DIR) $(1)/etc/avs
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/SampleApp/src/SampleApp $(1)/usr/bin/
	$(INSTALL_DATA) ./files/AlexaClientSDKConfig.json $(1)/etc/avs/
	$(INSTALL_DATA) ./files/cblAuthDelegate.db $(1)/etc/avs/
	$(INSTALL_DATA) ./files/AlexaClientSDKConfig2.json $(1)/etc/avs/
	$(INSTALL_DATA) ./files/cblAuthDelegate2.db $(1)/etc/avs/
ifeq ($(CONFIG_AVS_SAMPLE_APP_USE_THF_KWD),y)
	$(INSTALL_DATA) ./files/kwd/thfft_alexa_enus_v3_250kb_am.raw $(1)/etc/avs/
	$(INSTALL_DATA) ./files/kwd/thfft_alexa_enus_v3_250kb_search_15.raw $(1)/etc/avs/
endif
endef

ifneq ($(BUILT_PREBUILT_IMAGE),0)
define Package/avs-device-sdk/PrebuildImage
	cd $(PKG_BUILD_DIR);tar zcf $(PREBUILT_DIR)/$(PKG_NAME)-$(PKG_VERSION).img ipkg-install/ \
	 SampleApp/src/SampleApp
endef
Hooks/Install/Post += Package/avs-device-sdk/PrebuildImage
endif

$(eval $(call BuildPackage,avs-device-sdk))
$(eval $(call BuildPackage,avs-sample-app))
