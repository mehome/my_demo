
include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

# Name and release number of this package
PKG_NAME:=airkiss
PKG_VERSION:=0.0.1
PKG_RELEASE:=1


# This specifies the directory where we're going to build the program.
# The root build directory, $(BUILD_DIR), is by default the build_mipsel
# directory in your OpenWrt SDK directory
PKG_BUILD_DIR := $(BUILD_DIR)/$(PKG_NAME)


include $(INCLUDE_DIR)/package.mk

# Specify package information for this program.
# The variables defined here should be self explanatory.
# If you are running Kamikaze, delete the DESCRIPTION
# variable below and uncomment the Kamikaze define
# directive for the description below
define Package/airkiss
	SECTION:=utils
	CATEGORY:=Utilities
	TITLE:=airkiss -- scan apply or tencent configure network
	URL:=http://www.montage-tech.com/
	VERSION:=1
	DEPENDS:=+libtapi +libpcap +libpthread +wdk +libnl-tiny +libutils +libmpdclient  +myutils +libopenssl
endef

# Uncomment portion below for Kamikaze and delete DESCRIPTION variable above
define Package/airkiss/description
	airkiss, to support scan and apply in cdb or
	tencent configure network function,
endef

define Package/airkiss/config
    source "$(SOURCE)/Config.in"
endef

TARGET_CFLAGS += $(FPIC) -I${STAGING_DIR}/usr/include/ -I$(STAGING_DIR)/usr/include/libnl-tiny
ifeq ($(CONFIG_BIG_ENDIAN),y)
TARGET_LDFLAGS += -L$(PKG_BUILD_DIR)/lib -lairkiss_log
else
TARGET_LDFLAGS += -L$(PKG_BUILD_DIR)/lib_le -lairkiss_log
endif
#TARGET_LDFLAGS += -L$(STAGING_DIR)/usr/lib -lpcap -ltapi -lpthread -lutils -lnl-tiny -lcrypto
TARGET_LDFLAGS += -L$(STAGING_DIR)/usr/lib -lpcap -ltapi -lpthread -lutils -lnl-tiny -lcrypto
TARGET_LDFLAGS += -lmyutils
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

define Build/Compile
	CC="$(TARGET_CC)" \
	CFLAGS="$(TARGET_CFLAGS)" \
	LDFLAGS="$(TARGET_LDFLAGS)" \
	$(MAKE) -C $(PKG_BUILD_DIR) \
		all
endef

# We do not need to define Build/Configure or Build/Compile directives
# The defaults are appropriate for compiling a simple program such as this one


# Specify where and how to install the program. Since we only have one file,
# the himalaya executable, install it by copying it to the /bin directory on
# the router. The $(1) variable represents the root directory on the router running
# OpenWrt. The $(INSTALL_DIR) variable contains a command to prepare the install
# directory if it does not already exist.  Likewise $(INSTALL_BIN) contains the
# command to copy the binary file from its current location (in our case the build
# directory) to the install directory.
define Package/airkiss/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/airkiss $(1)/usr/bin/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/airkissDis $(1)/usr/bin/
endef


# This line executes the necessary commands to compile our program.
# The above define directives specify all the information needed, but this
# line calls BuildPackage which in turn actually uses this information to
# build a package.
$(eval $(call BuildPackage,airkiss))
