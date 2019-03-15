#!/bin/sh
MACHINE_TYPE=`uname -m`
if [ "$MACHINE_TYPE" = "x86_64" ]; then
echo x64 platform found ... nothing to do ...
else
echo rebuild msign
make -C panther/tools/msign
echo switch toolchain to x86 version
cd panther/toolchains
mv toolchain-mips_interaptiv_gcc-4.8-linaro_uClibc-0.9.33.2 toolchain-mips_interaptiv_gcc-4.8-linaro_uClibc-0.9.33.2-x64
mv toolchain-mipsel_interaptiv_gcc-4.8-linaro_uClibc-0.9.33.2 toolchain-mipsel_interaptiv_gcc-4.8-linaro_uClibc-0.9.33.2-x64
tar jxf toolchain-mips_interaptiv_gcc-4.8-linaro_uClibc-0.9.33.2-x86.tar.bz2 > /dev/null
tar jxf toolchain-mipsel_interaptiv_gcc-4.8-linaro_uClibc-0.9.33.2-x86.tar.bz2 > /dev/null
fi
echo done
