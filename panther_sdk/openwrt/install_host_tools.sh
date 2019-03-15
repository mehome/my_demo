#!/bin/sh

if [ -d staging_dir/host ];
then
echo host tools already exist, re-installing
rm -rf staging_dir/host
fi

mkdir -p staging_dir
MACHINE_TYPE=`uname -m`
if [ "$MACHINE_TYPE" = "x86_64" ]; then
echo extracting prebuilt host tools for Ubuntu 14.04 x64 ...
tar jxf tools/ubuntu-14.04-host-tools.tar.bz2
mv -f ubuntu-14.04-host-tools staging_dir/host
else
echo extracting prebuilt host tools for Ubuntu 14.04 x86 ...
tar jxf tools/ubuntu-14.04-x86-host-tools.tar.bz2
mv -f ubuntu-14.04-x86-host-tools staging_dir/host
fi
mkdir -p staging_dir/target-mips-openwrt-linux/stamp/
mkdir -p staging_dir/target-mipsel-openwrt-linux/stamp/
touch staging_dir/target-mips-openwrt-linux/stamp/.tools_install_yynyynynynyyyyynnyyynyyyyyyynnynyyyyynnyyynnyynnnyyy
touch staging_dir/target-mipsel-openwrt-linux/stamp/.tools_install_yynyynynynyyyyynnyyynyyyyyyynnynyyyyynnyyynnyynnnyyy
echo done

