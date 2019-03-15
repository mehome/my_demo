#
# Copyright (C) 2014 Montage Inc.
#
export WIFIAUDIO_MONTAGE=y
#export WIFIAUDIO_MTK=y

include $(TOPDIR)/rules.mk

PKG_NAME:=duer_linux
PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)

#LDFLAGS="$(TARGET_LDFLAGS) -L$(STAGING_DIR)/usr/lib -Wl,-rpath-link=$(STAGING_DIR)/usr/lib"

include $(INCLUDE_DIR)/package.mk
define Package/duer_linux
    SECTION:=duer_linux
    CATEGORY:=duer_linux with cchip
    TITLE:= duer_linux
    VERSION:=1
    DEPENDS:= +libstdcpp +libpthread +librt +libatomic \
	+alsa-lib +libDeviceIo +libcchip +wdk +libcurl \
	+libffmpeg-custom +libsqlite3 +wakeup
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
	$(call TSYNC,$(PKG_BUILD_DIR))
endef

include build.mk

define Build/Compile
	cd $(PKG_BUILD_DIR) ;\
	$(CMAKE_CURRENT_LIST_DIR)/cmake ./ \
	-DCMAKE_INSTALL_PREFIX=$(OUTPUT_PATH) \
	-DCMAKE_BUILD_TYPE=$(CMAKE_BUILD_TYPE) \
	-DCMAKE_C_COMPILER=$(CMAKE_C_COMPILER) \
	-DCMAKE_CXX_COMPILER=$(CMAKE_CXX_COMPILER) \
	-DCMAKE_C_FLAGS="$(CMAKE_C_FLAGS)" \
	-DCMAKE_CXX_FLAGS="$(CMAKE_CXX_FLAGS)" \
	-DCMAKE_C_COMPILE_OBJECT=$(CMAKE_C_COMPILE_OBJECT) \
	\
	-DPlatform=$(Platform) \
	\
	-DOTHER_INCLUDE_PATH=$(DUER_SDK_PATH)/include \
	-DPORTAUDIO_LIB_PATH=$(THIRD_PATH)/usr/lib/libportaudio.so \
	-DPORTAUDIO_INCLUDE_DIR=$(THIRD_PATH)/usr/include/libportaudio \
	-DFFMPEG_LIB_PATH=$(THIRD_PATH)/usr/lib \
	-DFFMPEG_INCLUDE_DIR=$(THIRD_PATH)/usr/include \
	-DCURL_LIBRARY=$(THIRD_PATH)/usr/lib/libcurl.so \
	-DCURL_INCLUDE_DIR=$(THIRD_PATH)/usr/include \
	-DSQLITE_LDFLAGS=$(THIRD_PATH)/usr/lib/libsqlite3.so \
	-DSQLITE_INCLUDE_DIRS=$(THIRD_PATH)/usr/include \
	-DKITTAI_KEY_WORD_DETECTOR=$(KITTAI_KEY_WORD_DETECTOR) \
	-DBUILD_TTS_SDK=$(BUILD_TTS_SDK) \
	-DPORTAUDIO=ON \
	-DGSTREAMER_MEDIA_PLAYER=ON \
	-DBUILD_DOC=OFF \
	-DBUILD_TEST=OFF \
	-DBUILD_ONE_LIB=ON \
	-DOUTPUT_FOR_THIRD=ON \
	-DDEBUG_FLAG=$(DEBUG_FLAG) \
	-DDUERLINK_V2=$(DUERLINK_V2)\
	-DBUILD_CCHIP8198="for cchip8198"\
	-DBUILD_LOCALTTS="for user localtts player" \
	-DBUILD_USE_ALSA_RECORDER="tip:The variable must be defined before the semicolon.";\
	make -j$(THREAD_COUNT); \
	make install	
endef

define Package/duer_linux/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_BUILD_DIR)/sdk/lib/*.so* $(1)/usr/lib
#	$(CP) $(PKG_BUILD_DIR)/third/lib/*.so* $(1)/usr/lib
	$(CP) $(OUTPUT_PATH)/lib/*.so* $(1)/usr/lib
	
	$(INSTALL_DIR) $(1)/usr/bin
	$(CP) $(OUTPUT_PATH)/duer_linux $(1)/usr/bin
	ln -sf /usr/bin/duer_linux $(1)/usr/bin/micarray
	$(INSTALL_DIR) $(1)/root
	ln -sf /dev/null $(1)/root/duer_link.log
	$(CP) $(PKG_BUILD_DIR)/sdk/log.config $(1)/root
	$(CP) $(OUTPUT_PATH)/appresources  $(1)/root
	
	$(INSTALL_DIR) $(1)/root/resources
	$(CP) $(PKG_BUILD_DIR)/resources/universal_detect.res $(1)/root/resources
	$(CP) $(PKG_BUILD_DIR)/resources/hotword.umdl $(1)/root/resources	
	$(CP) $(PKG_BUILD_DIR)/resources/config.json $(1)/root/resources

	$(INSTALL_DIR) $(1)/etc/ssl
	$(CP) $(PKG_BUILD_DIR)/resources/certs $(1)/etc/ssl	
endef

$(eval $(call BuildPackage,duer_linux))
