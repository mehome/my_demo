#common
THREAD_COUNT=`cat /proc/cpuinfo| grep "processor"| wc -l`
THIRD_PATH=$(STAGING_DIR)
BUILD_PATH=$(PKG_BUILD_DIR)/$(Platform)_buildout
OUTPUT_PATH=$(BUILD_PATH)/output
DUER_SDK_PATH==`pwd`/sdk

# compiler
DEBUG_SWITCH=
COMPILER_PATH=$(TOPDIR)/../panther/toolchains/toolchain-mipsel_interaptiv_gcc-4.8-linaro_uClibc-0.9.33.2
#SYSROOT_PATH=$(COMPILER_PATH)/mipsel-openwrt-linux
CMAKE_FPIC_FLAG=-fPIC
CMAKE_C_COMPILER=$(COMPILER_PATH)/bin/mipsel-openwrt-linux-uclibc-gcc
CMAKE_CXX_COMPILER=$(COMPILER_PATH)/bin/mipsel-openwrt-linux-uclibc-g++
CMAKE_C_FLAGS=$(DEBUG_SWITCH) $(CMAKE_FPIC_FLAG) -Os -pipe -mno-branch-likely -mips32r2 -mtune=34kc -fno-caller-saves -msoft-float
CMAKE_CXX_FLAGS=-std=c++11 -D_GLIBCXX_USE_C99 $(DEBUG_SWITCH) $(CMAKE_FPIC_FLAG) $(SYSROOT_PATH) -Os -pipe -mno-branch-likely -mips32r2 -mtune=34kc -fno-caller-saves -msoft-float
# -D_GLIBCXX_USE_C99
CMAKE_BUILD_TYPE=Release
STRIP_COMMAND=$(COMPILER_PATH)/bin/mipsel-openwrt-linux-uclibc-strip
CMAKE_CURRENT_LIST_DIR=$(STAGING_DIR_HOST)/bin
TOOLCHAIN_PATH=$(COMPILER_PATH)/bin
CROSS_PLATFORM=mipsel-openwrt-linux-uclibc
CROSS_TOOLS=mipsel-openwrt-linux-uclibc-

CPU_ARCH=mips
SUPPORT_HARD_FLOAT=no
DUER_USE_STATIC_ICONV=yes

# Third party: should built with script

# Platform
Platform=Cchip

# Function
KITTAI_KEY_WORD_DETECTOR=OFF

BUILD_TEST=OFF
BUILD_ONE_LIB=ON
BUILD_TTS_SDK=OFF
ENABLE_LOG_FILENAME=ON
BUILD_TV_LINK=ON
BUILD_TV_LIVE=ON
BUILD_VIDEO_PLAYER=ON
