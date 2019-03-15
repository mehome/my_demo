#!/bin/sh

var1=`grep -o "CI_BLKSZ=[^ ]*" /proc/cmdline`
echo $var1
CI_BLKSZ=`echo $var1 | cut -d "=" -f 2`
CI_LDADR=0x80041000

#panther_board_detect() {
#        name=`grep "^machine" /proc/cpuinfo | sed "s/machine.*: \(.*\)/\1/g" | sed "s/\(.*\) - .*/\1/g"`
#        model=`grep "^machine" /proc/cpuinfo | sed "s/machine.*: \(.*\)/\1/g" | sed "s/.* - \(.*\)/\1/g"`
#        [ -z "$name" ] && name="unknown"
#        [ -z "$model" ] && model="unknown"
#        [ -e "/tmp/sysinfo/" ] || mkdir -p "/tmp/sysinfo/"
#        echo $name > /tmp/sysinfo/board_name
#        echo $model > /tmp/sysinfo/model
#}
#
#panther_board_model() {
#        local model
#
#        [ -f /tmp/sysinfo/model ] && model=$(cat /tmp/sysinfo/model)
#        [ -z "$model" ] && model="unknown"
#
#        echo "$model"
#}

#PART_NAME=kernel

panther_board_name() {
	local name

	[ -f /tmp/sysinfo/board_name ] && name=$(cat /tmp/sysinfo/board_name)
	[ -z "$name" ] && name="unknown"

	echo "$name"
}

platform_find_partitions() {
	local first dev size erasesize name
	while read dev size erasesize name; do
		name=${name#'"'}; name=${name%'"'}
		case "$name" in
			kernel|rootfs|ubi)
				if [ -z "$first" ]; then
					first="$name"
				else
					echo "$erasesize:$first:$name"
					break
				fi
			;;
		esac
	done < /proc/mtd
}

platform_find_kernelpart() {
	local part
	for part in "${1%:*}" "${1#*:}"; do
		case "$part" in
			kernel)
				echo "$part"
				break
			;;
		esac
	done
}

platform_check_image() {
	[ "$#" -gt 1 ] && return 1
	local board=$(panther_board_name)

	# haven't added any board_name now, BTHOMEHUBV2B|BTHOMEHUBV3A|P2812HNUF* are used by lantiq
	case "$board" in
		BTHOMEHUBV2B|BTHOMEHUBV3A|P2812HNUF* )
			nand_do_platform_check $board $1
			return $?;
			;;
	esac

	# verify new firmware through msign
	msign -R -i "$1" -p /etc/key.pub 2>result
	if [ "$(cat result)" != "OK" ];
	then
		echo "Verify failed by msign."
		return 1
	fi

	# check image magic work
	case "$(get_magic_long "$1")" in
		# combined-image
		43494948)
		    echo "combined-image, magic_word: 43494948"
    		    # reference code: combined-image.sh, line 29
		    local md5_img=$(dd if="$1" bs=1 skip=32 count=32 2>/dev/null)
		    local md5_chk=$(dd if="$1" bs=$CI_BLKSZ skip=1 2>/dev/null | md5sum -); md5_chk="${md5_chk%% *}"

		    # to verify firmware integrity, need to check MD5 here
		    if [ -n "$md5_img" -a -n "$md5_chk" ] && [ "$md5_img" = "$md5_chk" ]; then
    			return 0
		    else
			echo "Invalid image. Contents do not match checksum (image:$md5_img calculated:$md5_chk)"
			return 1
		    fi
		;;
		*)
			echo "Invalid image. Use combined-image files on this platform"
			return 1
		;;
	esac
}

platform_do_upgrade() {
	v "platform_do_upgrade"
	sync
	disable_watchdog
	local partitions=$(platform_find_partitions)
	v "partitions = ${partitions}"
	local kernelpart=$(platform_find_kernelpart "${partitions#*:}")
	v "kernelpart = ${kernelpart}"
	local erase_size=$((0x${partitions%%:*})); partitions="${partitions#*:}"
	v "erase_size = ${erase_size}"
	local kern_length=0x$(dd if="$1" bs=2 skip=8 count=4 2>/dev/null)
	v "kern_length = ${kern_length}"
	local kern_blocks=$(($kern_length / $CI_BLKSZ))
	v "kern_blocks = ${kern_blocks}"
	local root_length=0x$(dd if="$1" bs=2 skip=12 count=4 2>/dev/null)
	v "root_length = ${root_length}"
	local root_blocks=$(($root_length / $CI_BLKSZ))
	v "root_blocks = ${root_blocks}"
	local total_length=$(($kern_length + $root_length))
	v "total_length = ${total_length}"
	local kern_length_d=$(($kern_length + 0))

	if [ -n "$partitions" ] && [ -n "$kernelpart" ] && \
	   [ ${kern_blocks:-0} -gt 0 ] && \
#          [ ${root_blocks:-0} -gt ${kern_blocks:-0} ] && \
	   [ ${erase_size:-0} -gt 0 ];
	then
		local append=""
		[ -f "$CONF_TAR" -a "$SAVE_CONFIG" -eq 1 ] && append="-j $CONF_TAR"

		# do flash.nor.enable_virtual_access if support it #
		if [ "$( sysctl -n flash.nor.support_virtual_access )" = "1" ]; then
			sysctl -w flash.nor.enable_virtual_access=1
		fi

        # do flash.nand.enable_virtual_access if support it #
        if [ "$( sysctl -n flash.nand.support_virtual_access )" = "1" ]; then
			sysctl -w flash.nand.enable_virtual_access=1
        fi

		/sbin/cdb set upgrade_progress_bar 30

		# clear combined image header partition for upgrade interrupt detection
		dd if=/dev/zero bs=$CI_BLKSZ count=1 2>/dev/null | mtd write - "combined_image_header"
		sync
		sync
		sync

		# erase rootfs/ubi partition before write to support firmware upgrade to different type images
		mtd erase "${partitions#*:}"

		/sbin/cdb set upgrade_progress_bar 45

		# download combined image to firmware partition instead of kernel and rootfs
		# write system image first
		export MTD_WRITE_TOTAL_SIZE=$total_length
		export MTD_WRITE_INIT_SIZE=0
#		dd if="$1" bs=$CI_BLKSZ skip=1 count=$kern_blocks 2>/dev/null | mtd write - "kernel"
		dd if="$1" bs=$CI_BLKSZ skip=1 count=$(($kern_blocks+$root_blocks)) 2>/dev/null | mtd write - "firmware"
		sync
		sync
		sync

#		/sbin/cdb set upgrade_progress_bar 60

		# no need to use -r flag, because after this function finished, common.sh will sleep for seconds then go to reboot
#		export MTD_WRITE_TOTAL_SIZE=$total_length
#		export MTD_WRITE_INIT_SIZE=$kern_length_d
#                dd if="$1" bs=$CI_BLKSZ skip=$((1+$kern_blocks)) count=$root_blocks 2>/dev/null | mtd $append write - "${partitions#*:}"
#		sync
#		sync
#		sync

		/sbin/cdb set upgrade_progress_bar 75

		# then, write the combined image header
		export MTD_WRITE_TOTAL_SIZE=$total_length
		export MTD_WRITE_INIT_SIZE=$total_length
		dd if="$1" bs=$CI_BLKSZ count=1 2>/dev/null | mtd write - "combined_image_header"
		sync
		sync
		sync

		/sbin/cdb set upgrade_progress_bar 90
#       	( dd if="$1" bs=$CI_BLKSZ skip=1 count=$kern_blocks 2>/dev/null; \
#       	  dd if="$1" bs=$CI_BLKSZ skip=$((1+$kern_blocks)) count=$root_blocks 2>/dev/null ) | \
#       		mtd -r $append -F $kernelpart:$kern_length:$CI_LDADR,rootfs write - $partitions
	fi

	# avoid dirty data
	sync

	# play finish music
	# if [ "$( sysctl -n flash.nor.support_virtual_access )" = "1" ]; then
	#	aplay /res/sysupgrade_completed.wav
	# fi

	# if [ "$( sysctl -n flash.nand.support_virtual_access )" = "1" ]; then
	#    aplay /res/sysupgrade_completed.wav
	# fi

	/sbin/cdb set upgrade_progress_bar 100
	sync

	#give 2 seconds for web update status
	sleep 2
}

#platform_copy_config() {
#        mount -t vfat -o rw,noatime /dev/mmcblk0p1 /mnt
#        cp -af "$CONF_TAR" /mnt/
#        sync
#        umount /mnt
#}

# use default for platform_do_upgrade
disable_watchdog() {
	killall watchdog
	( ps | grep -v 'grep' | grep '/dev/watchdog' ) && {
		echo 'Could not disable watchdog'
		return 1
	}
}
#append sysupgrade_pre_upgrade disable_watchdog
