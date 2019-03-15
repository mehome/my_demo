#!/bin/sh

#if [ -f .config ];
#then
#if grep -q mipsel .config; then
#echo .config: already converted
#else
#echo convert .config
#sed -i 's/\"mips/\"mipsel/g' .config
#sed -i 's/toolchain-mips/toolchain-mipsel/g' .config
#sed -i 's/CONFIG_mips/CONFIG_mipsel/g' .config
#sed -i 's/CONFIG_TARGET_panther_be=y/# CONFIG_TARGET_panther_be is not set/g' .config
#sed -i 's/# CONFIG_TARGET_panther_le is not set/CONFIG_TARGET_panther_le=y/g' .config
#sed -i 's/CONFIG_TARGET_panther_be_Default=y/CONFIG_TARGET_panther_le_Default=y/g' .config
#fi
#fi

if [ -f gdb_scripts/load.gdb ];
then
if grep -q openwrt-panther-be-vmlinux-initramfs.elf gdb_scripts/load.gdb; then
echo convert load.gdb
sed -i 's/panther-be-vmlinux/panther-le-vmlinux/g' gdb_scripts/load.gdb
else 
echo gdb_scripts/load.gdb: already converted
fi
fi

if [ -f gdb_scripts/run.gdb ];
then
if grep -q openwrt-panther-be-vmlinux-initramfs.elf gdb_scripts/run.gdb; then
echo convert run.gdb
sed -i 's/panther-be-vmlinux/panther-le-vmlinux/g' gdb_scripts/run.gdb
else 
echo gdb_scripts/run.gdb: already converted
fi
fi
