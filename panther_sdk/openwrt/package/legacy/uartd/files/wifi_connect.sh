#!/bin/sh
. /etc/cdb.sh

set_security() {
	echo ${1}
	echo ${2}
	for i in `seq 1 5`
	do
		ifconfig sta0 up
		rule=`iw --ssid "${1}" dev sta0 scan`
		[ -n "${rule}" ] && break
	done
	eval $rule
	config_setraw wl_bss_ssid2 "${1}"

	[ -z "${sec}" ] && sec=0

	sec=$(($sec&0x7))
	pass_len=`echo "${2}" | wc -c`
	if [ ${sec} = 0 ]; then
		if [ -z "${2}" ]; then
			config_set wl_bss_sec_type2 0
			config_set wl_bss_cipher2 0
			config_set wl_bss_wep_1key2 0
			config_set wl_bss_wep_2key2 0
			config_set wl_bss_wep_3key2 0
			config_set wl_bss_wep_4key2 0
			config_set wl_bss_wpa_psk2 0
		fi
	fi

	if [ ${sec} -gt 1 ]; then
		if [ $pcipher = 8 ]; then
			config_set wl_bss_cipher2 4
		elif [ $pcipher = 16 ]; then
			config_set wl_bss_cipher2 8
		elif [ $pcipher = 24 ]; then
			config_set wl_bss_cipher2 12
		fi
		echo passlen $pass_len
		if [ $pass_len -ge 9 -a $pass_len -le 64 ]; then
			config_setraw wl_bss_wpa_psk2 "${2}"
			echo set password
		fi
	elif [ ${sec} = 1 ]; then
		if [ $pass_len == 6 -o $pass_len == 14 -o  $pass_len == 11 -o $pass_len == 27 ]; then
			config_setraw wl_bss_wep_1key2 "$2"
			config_setraw wl_bss_wep_2key2 "$2"
			config_setraw wl_bss_wep_3key2 "$2"
			config_setraw wl_bss_wep_4key2 "$2"
		fi
	fi

	if [ ${sec} = 1 ]; then
		config_set wl_bss_sec_type2 4
	elif [ ${sec} = 2 ]; then
		config_set wl_bss_sec_type2 8
	elif [ ${sec} = 4 ]; then
		config_set wl_bss_sec_type2 16
	elif [ ${sec} = 6 ]; then
		config_set wl_bss_sec_type2 24
	fi
	return 0
}

apply() {
	mode=3
	set_security  $* #ssid=$1;pass=$2
	echo "set_security return $?"
	config_set op_work_mode "${mode}"
	/lib/wdk/commit
	if [ "${mode}" = "3" ]; then
		sleep 8
		/etc/init.d/lan restart
	fi
	return 0
}

omnicfg_cdb_get()
{
	cdb_data=""
	[ $# -lt 2 ] && echo usage: omnicfg_cdb_get output_variable default_value && exit 1

	config_get cdb_data "$1"
	cdb_data=$( config_chk "$cdb_data" )
	[ -z "$cdb_data" ] && cdb_data="$2"

	echo $cdb_data
}

restore() {
	PID=`ps -w | grep upmpdcli | grep br1 | awk '{print $1}'`
	[ "$PID" != "" ] && kill $PID
	mpc stop
	#/etc/init.d/shairport stop
	#/etc/init.d/spotify stop
	
	#wifi
	#pass=$( omnicfg_cdb_get wl_bss_wpa_psk2 "DLNA" )
	#ssid=$( omnicfg_cdb_get wl_bss_ssid2 "12345678" )
	pass="`cdb get wl_bss_wpa_psk2`"
	ssid="`cdb get wl_bss_ssid2`"
	echo $ssid
	echo $pass
	
	apply  $ssid $pass

	echo "wifi  restore success"
}

disconnet() {
	echo "disconnet"
}

#action->mode->ssid->pass
action=$1
#mode=$2
#shift 2
shift 1
case $action in
	"connect")
	mpc stop
	/etc/init.d/shairport restart
	aplay /tmp/res/connecting.wav
	apply "$*"
	;; #need ssid,password
	"restore") 		restore		;; #重连，自动连接上一次的wifi
	"disconnet") 	disconnet ;;
	*)
		echo "$0 usage:"
		echo "  $0 connect   save/apply < ssid password>"
		echo "  $0 restore   read elian config from flash and apply"
	;;
esac
