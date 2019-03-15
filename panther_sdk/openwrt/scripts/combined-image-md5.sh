#!/bin/sh


IMAGE=${1:-openwrt-combined.img}
image="${IMAGE}.image"
dd if="$1" skip=1 of="$image" bs="$2" conv=sync 2>/dev/null
md5=$(cat "$image" | md5sum -)

#dd if=$(md5) skip=18 of=${IMAGE} bs=1 count=32 conv=sync 2>/dev/null
( printf "%32s" \
	 "${md5%% *}" | \
	 dd conv=notrunc of=${IMAGE} bs=1 seek=32;
) 2>/dev/null

# Clean up.
rm -f "$image"
