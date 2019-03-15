include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

# Name and release number of this package
PKG_NAME:=libdlna
PKG_VERSION:=0.0.1
PKG_RELEASE:=1

PKG_BUILD_DIR := $(BUILD_DIR)/$(PKG_NAME)

include $(INCLUDE_DIR)/package.mk

# Specify package information for this program.
# The variables defined here should be self explanatory.
# If you are running Kamikaze, delete the DESCRIPTION
# variable below and uncomment the Kamikaze define
# directive for the description below
define Package/libdlna
	SECTION:=libs
	CATEGORY:=Libraries
	TITLE:=Third Party DLNA Support.
	VERSION:=1
	DEPENDS:=+libjson-c +libupnp
endef

define Package/libdlna/description
	For Third Party DLNA Compatible.
endef

TARGET_CFLAGS += $(FPIC) -std=c++11 -I${STAGING_DIR}/usr/include/ -ljson-c 

# Specify what needs to be done to prepare for building the package.
# In our case, we need to copy the source files to the build directory.
# This is NOT the default.  The default uses the PKG_SOURCE_URL and the
# PKG_SOURCE which is not defined here to download the source from the web.
# In order to just build a simple program that we have just written, it is
# much easier to do it this way.
define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
	$(call TSYNC,$(PKG_BUILD_DIR))
endef

define Build/InstallDev
	$(INSTALL_DIR) $(STAGING_DIR)/usr/include/upmpdcli
	$(INSTALL_DIR) $(STAGING_DIR)/usr/include/dlna
	$(CP) $(PKG_BUILD_DIR)/libupnpp/* $(STAGING_DIR)/usr/include/upmpdcli/
	$(CP) $(PKG_BUILD_DIR)/dlna/* $(STAGING_DIR)/usr/include/dlna/
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_BUILD_DIR)/libdlna.so $(1)/usr/lib/
endef

define Package/libdlna/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_BUILD_DIR)/libdlna.so $(1)/usr/lib/
endef

# This line executes the necessary commands to compile our program.
# The above define directives specify all the information needed, but this
# line calls BuildPackage which in turn actually uses this information to
# build a package.
$(eval $(call BuildPackage,libdlna))
