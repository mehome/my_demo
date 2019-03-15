#!/bin/sh
if [ $# -eq 0 ];then
	echo build image without second image.
else
	SND_SIZE=$(stat -c%s "$1")
	if [ $SND_SIZE -le 0 ];then
		echo build image with second image but image size is $SND_SIZE
	else
		SND_SIZE_NOR=$(((($SND_SIZE+(0x10000-1)+0x10000)/0x10000)*0x10000)) #0x10000 is for CDB size
		SND_1K_C_NOR=$(($SND_SIZE_NOR/1024))
		CI_START_NOR=$(($SND_1K_C_NOR+640))
		SND_SIZE_NAND=$(((($SND_SIZE+(0x20000-1)+0x20000)/0x20000)*0x20000)) #0x20000 is for CDB size
		SND_1K_C_NAND=$(($SND_SIZE_NAND/1024))
		CI_START_NAND=$(($SND_1K_C_NAND+640))
		echo build image with second image and image path is $1 \($SND_SIZE\)
	fi
fi

# mark-out the line below to build big-endian image
LE=1

NOR_IMG_NAME=img/NOR_64K.img
NAND_128K_IMG_NAME=img/NAND_128K.img
SNOR_IMG_NAME=img/SECURE_NOR_64K.img
SNAND_128K_IMG_NAME=img/SECURE_NAND_128K.img
CI_IMG=openwrt/bin/panther/combined-uzImage-ubi-overlay
SCI_IMG=openwrt/bin/panther/secure-combined-uzImage-ubi-overlay

if [ ! -z "$LE" ]; then
	if [ $LE -eq 1 ]; then
		ENDIAN_SEL="LE=1"
		echo "\n* Generating little-endian flash images"
	else
		echo "\n* Generating big-endian flash images"
	fi
else
	echo "\n* Generating big-endian flash images"
fi

echo "* Cleanup..."
rm -f img/*

echo "* Collecting binaries..."
mkdir -p img

if [ $# -eq 0 ];then #build without second image
	make ${ENDIAN_SEL} MPCFG=1 -C panther/boot/boot/ clean
	make ${ENDIAN_SEL} MPCFG=1 BOOT3_SUBMODE=1 -C panther/boot/boot/
	cp -f panther/boot/boot/boot.img img/
	cp -f panther/boot/boot/sboot.img img/
	echo "* Boot code done\n"

	make ${ENDIAN_SEL} MPCFG=1 -C panther/boot/boot/ clean
	make ${ENDIAN_SEL} MPCFG=1 WIFI=1 MASS_PRODUCTION_TEST=1 -C panther/boot/boot/ boot3.img
	cp -f panther/boot/boot/boot3.img img/recovery.img
	cp -f panther/boot/boot/sboot3_nor.img img/srecovery_nor.img
	cp -f panther/boot/boot/sboot3_nand_128k.img img/srecovery_nand_128k.img
	echo "* Recovery code done\n"

	echo "* Creating NOR 64K image..."
	tr "\000" "\377" < /dev/zero | dd of=${NOR_IMG_NAME} ibs=1 count=640K
	dd if=img/boot.img of=${NOR_IMG_NAME} conv=notrunc
	dd if=img/recovery.img of=${NOR_IMG_NAME} obs=1024 seek=256 conv=notrunc
	dd if=${CI_IMG} of=${NOR_IMG_NAME} obs=1024 seek=640 conv=notrunc
	echo "Done\n"

	echo "* Creating NAND 128K image..."
	tr "\000" "\377" < /dev/zero | dd of=${NAND_128K_IMG_NAME} ibs=1 count=640K
	dd if=img/boot.img of=${NAND_128K_IMG_NAME} conv=notrunc
	dd if=img/recovery.img of=${NAND_128K_IMG_NAME} obs=1024 seek=256 conv=notrunc
	dd if=${CI_IMG} of=${NAND_128K_IMG_NAME} obs=1024 seek=640 conv=notrunc
	echo "Done\n"

	echo "* Creating SECURE NOR 64K image..."
	tr "\000" "\377" < /dev/zero | dd of=${SNOR_IMG_NAME} ibs=1 count=640K
	dd if=img/sboot.img of=${SNOR_IMG_NAME} conv=notrunc
	dd if=img/srecovery_nor.img of=${SNOR_IMG_NAME} obs=1024 seek=256 conv=notrunc
	dd if=${SCI_IMG} of=${SNOR_IMG_NAME} obs=1024 seek=640 conv=notrunc
	echo "Done\n"

	echo "* Creating SECURE NAND 128K image..."
	tr "\000" "\377" < /dev/zero | dd of=${SNAND_128K_IMG_NAME} ibs=1 count=640K
	dd if=img/sboot.img of=${SNAND_128K_IMG_NAME} conv=notrunc
	dd if=img/srecovery_nand_128k.img of=${SNAND_128K_IMG_NAME} obs=1024 seek=256 conv=notrunc
	dd if=${SCI_IMG} of=${SNAND_128K_IMG_NAME} obs=1024 seek=640 conv=notrunc
	echo "Done\n"

else #build with second image
	make ${ENDIAN_SEL} MPCFG=1 -C panther/boot/boot/ clean
	make ${ENDIAN_SEL} MPCFG=1 BOOT3_SUBMODE=1 SND_SIZE=${SND_SIZE_NOR} -C panther/boot/boot/
	cp -f panther/boot/boot/boot.img img/boot_nor.img
	cp -f panther/boot/boot/sboot.img img/sboot_nor.img
	echo "* Boot code nor done\n"

	make ${ENDIAN_SEL} MPCFG=1 -C panther/boot/boot/ clean
	make ${ENDIAN_SEL} MPCFG=1 BOOT3_SUBMODE=1 SND_SIZE=${SND_SIZE_NAND} -C panther/boot/boot/
	cp -f panther/boot/boot/boot.img img/boot_nand.img
	cp -f panther/boot/boot/sboot.img img/sboot_nand.img
	echo "* Boot code nand done\n"

	make ${ENDIAN_SEL} MPCFG=1 -C panther/boot/boot/ clean
	make ${ENDIAN_SEL} MPCFG=1 WIFI=1 MASS_PRODUCTION_TEST=1 SND_SIZE=${SND_SIZE_NOR} -C panther/boot/boot/ boot3.img
	cp -f panther/boot/boot/boot3.img img/recovery_nor.img
	cp -f panther/boot/boot/sboot3_nor.img img/srecovery_nor.img
	echo "* Recovery nor code done\n"

	make ${ENDIAN_SEL} MPCFG=1 -C panther/boot/boot/ clean
	make ${ENDIAN_SEL} MPCFG=1 WIFI=1 MASS_PRODUCTION_TEST=1 SND_SIZE=${SND_SIZE_NAND} -C panther/boot/boot/ boot3.img
	cp -f panther/boot/boot/boot3.img img/recovery_nand.img
	cp -f panther/boot/boot/sboot3_nand_128k.img img/srecovery_nand_128k.img
	echo "* Recovery nand code done\n"

	echo "* Creating NOR 64K image..."
	tr "\000" "\377" < /dev/zero | dd of=${NOR_IMG_NAME} ibs=1 count=640K
	dd if=img/boot_nor.img of=${NOR_IMG_NAME} conv=notrunc
	dd if=img/recovery_nor.img of=${NOR_IMG_NAME} obs=1024 seek=256 conv=notrunc
	dd if=$1 of=${NOR_IMG_NAME} obs=1024 seek=640 conv=notrunc
	dd if=${CI_IMG} of=${NOR_IMG_NAME} obs=1024 seek=${CI_START_NOR} conv=notrunc
	echo "Done\n"

	echo "* Creating NAND 128K image..."
	tr "\000" "\377" < /dev/zero | dd of=${NAND_128K_IMG_NAME} ibs=1 count=640K
	dd if=img/boot_nand.img of=${NAND_128K_IMG_NAME} conv=notrunc
	dd if=img/recovery_nand.img of=${NAND_128K_IMG_NAME} obs=1024 seek=256 conv=notrunc
	dd if=$1 of=${NAND_128K_IMG_NAME} obs=1024 seek=640 conv=notrunc
	dd if=${CI_IMG} of=${NAND_128K_IMG_NAME} obs=1024 seek=${CI_START_NAND} conv=notrunc
	echo "Done\n"

	echo "* Creating SECURE NOR 64K image..."
	tr "\000" "\377" < /dev/zero | dd of=${SNOR_IMG_NAME} ibs=1 count=640K
	dd if=img/sboot_nor.img of=${SNOR_IMG_NAME} conv=notrunc
	dd if=img/srecovery_nor.img of=${SNOR_IMG_NAME} obs=1024 seek=256 conv=notrunc
	dd if=$1 of=${SNOR_IMG_NAME} obs=1024 seek=640 conv=notrunc
	dd if=${SCI_IMG} of=${SNOR_IMG_NAME} obs=1024 seek=${CI_START_NOR} conv=notrunc
	echo "Done\n"

	echo "* Creating SECURE NAND 128K image..."
	tr "\000" "\377" < /dev/zero | dd of=${SNAND_128K_IMG_NAME} ibs=1 count=640K
	dd if=img/sboot_nand.img of=${SNAND_128K_IMG_NAME} conv=notrunc
	dd if=img/srecovery_nand_128k.img of=${SNAND_128K_IMG_NAME} obs=1024 seek=256 conv=notrunc
	dd if=$1 of=${SNAND_128K_IMG_NAME} obs=1024 seek=640 conv=notrunc
	dd if=${SCI_IMG} of=${SNAND_128K_IMG_NAME} obs=1024 seek=${CI_START_NAND} conv=notrunc
	echo "Done\n"
fi
rm -rf `find openwrt/bin/panther/ -name "FW_panther_*_all.img"`
TARGET_NAME=`find openwrt/bin/panther/ -name "FW_panther_*.img" | cut -b -45`_all.img
cp -f $NOR_IMG_NAME $TARGET_NAME
echo Created:$TARGET_NAME size:`stat -c%s $TARGET_NAME`
