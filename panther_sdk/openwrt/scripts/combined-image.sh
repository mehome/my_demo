#!/bin/sh

[ -f "$1" -a -f "$2" ] || {
	echo "Usage: $0 <kernel image> <rootfs image> <output file> <block size>"
	exit 1
}

IMAGE=${3:-openwrt-combined.img}

# Make sure provided images are 64k aligned.
kern="${IMAGE}.kernel"
root="${IMAGE}.rootfs"
ci="${IMAGE}.ci"
cih="${IMAGE}.cih"

#header="${IMAGE}.header"
dd if="$1" of="$kern" bs="$4" conv=sync 2>/dev/null
dd if="$2" of="$root" bs="$4" conv=sync 2>/dev/null
dd if=/dev/zero of="$cih" bs="$4" count=1 conv=sync 2>/dev/null

# Calculate md5sum over combined kernel and rootfs image.
md5_all=$(cat "$kern" "$root" | md5sum | awk '{print $1}')
md5_kernel=$(cat "$kern" | md5sum |  awk '{print $1}')
md5_rootfs=$(cat "$root" | md5sum |  awk '{print $1}')

#echo $md5_all
#echo $md5_kernel
#echo $md5_rootfs

( printf "CIIH000100000000%08x%08x%s%s%s" \
  $(stat -c "%s" "$kern") $(stat -c "%s" "$root") "${md5_all}${md5_kernel}${md5_rootfs}"  \
)  > ${ci} 2>/dev/null

dd if="$ci" of="$cih" bs=1 seek=0 conv=notrunc 2>/dev/null
dd if="$1" of="$cih" bs=1 seek=8 count=4 conv=notrunc 2>/dev/null
dd if="$2" of="$cih" bs=1 seek=12 count=4 conv=notrunc 2>/dev/null

cat "$cih" "$kern" "$root" > ${IMAGE} 2>/dev/null

# Clean up.
rm -f "$kern" "$root" "$ci" "$cih"
