#!/bin/sh

if [ -f .config ];
then
if grep -q "CONFIG_CPU_BIG_ENDIAN is not set" .config; then
echo Building little-endian kernel
else
echo Convert .config to build little-endian kernel
sed -i 's/CONFIG_CPU_BIG_ENDIAN=y/# CONFIG_CPU_BIG_ENDIAN is not set/g' .config
sed -i 's/# CONFIG_CPU_LITTLE_ENDIAN is not set/CONFIG_CPU_LITTLE_ENDIAN=y/g' .config
fi
fi
