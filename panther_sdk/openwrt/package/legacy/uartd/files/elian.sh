#!/bin/sh
alexa_playlist="/usr/script/playlists/alexaPlayList.m3u"
apply() {
	cdb set wanif_state 0
	wavplayer /tmp/res/connecting.wav &
  /lib/wdk/omnicfg_apply 9 "$1" "$2"
	local loop=30
	cdb set setup_network 1
	while true
	do
		wanif_state=`cdb get wanif_state`
		if [ $wanif_state = 2 ]; then

			return 0
		fi
		if [ $loop = 0 ]; then
			return 1
		fi
		sleep 1
		loop=`expr $loop "-" 1`
	done

	return 1
}


mapply() {
	cdb set wanif_state "0"	
	/lib/wdk/omnicfg_apply 9 "$1" "$2"
	local loop=30
	cdb set wificonnectstatus '2'
	while true
	do #query
		wanif_state=`cdb get wanif_state '0'`
		if [ $wanif_state = 2 ]; then
        cdb set ip=`ifconfig  br0 | grep "inet" | awk -F ' ' '{print $2}' | sed 's/.*://g'`
			cdb set wificonnectstatus '3'

   
			return 0
		fi
		if [ $loop = 0 ]; then
		  cdb set op_work_mode 1          
      cdb set ip='000.000.000.000'
			cdb set wificonnectstatus '4'
			return 1
		fi
		sleep 1
		loop=`expr $loop "-" 1`
	done

	return 1
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
	pass="`cdb get wl_bss_wpa_psk2`"
	ssid="`cdb get wl_bss_ssid2`"
	echo $ssid
	echo $pass
	
	apply  $ssid $pass

	echo "wifi  restore success"
}
disconnect(){
	echo "disconnect"
	mpc pause > /dev/null
	mspotify_cli t > /dev/null
	air_cli d playpause > /dev/null
	cdb set op_work_mode 1 > /dev/null
	/lib/wdk/commit > /dev/null
    cdb set ip '000.000.000.000'	
    aplay /tmp/res/speaker_not_connected.wav
}

check() {
	ssid1=`cdb get wl_bss_ssid1`
	ssid2=`cdb get  wl_bss_ssid2 '0'`
	if [ $ssid2 = "" ] || [ $ssid2 = " " ]; then
		cdb set connect_failed 1
	  cdb set wificonnectstatus=4
    cdb set ip '000.000.000.000'
  	/usr/bin/uartdfifo.sh failed
		return 1
	fi
	local loop=20 #20 seconds
	while true
	do #query
		wanif_state=`cdb get  wanif_state '0'`
		if [ $wanif_state = 2 ]; then
		cdb set ip `ifconfig  br1 | grep "inet" | awk -F ' ' '{print $2}' | sed 's/.*://g'`		
		cdb set wificonnectstatus '3'
		/usr/bin/uartdfifo.sh succeed
			return 0
		fi
		if [ $loop = 0 ]; then
			cdb set ip '000.000.000.000' 
		    cdb set wificonnectstatus '4'
			sleep 1
			return 1
		fi
		sleep 1
		loop=`expr $loop "-" 1`
	done
}

config () {
	cdb set wificonnectstatus '1'
	echo "config"
	cdb set wl_bss_ssid2 ""
	cdb set wl_bss_wpa_psk2 ""
	/lib/wdk/omnicfg_ctrl trigger
	local loop=80 #1 minute
	local state=0
	while true
	do #query
		omi_result=`cdb get omi_result '0'`
		if [ $omi_result = 0 ] && [ $state != 1 ]; then
			state=1
		fi
		if [ $omi_result = 1 ] && [ $state != 2 ]; then
			state=2
			cdb set wificonnectstatus '2' 
		fi
		if [ $omi_result = 2 ] && [ $state = 1  -o $state = 2 ]; then
			wanif_state=`cdb get wanif_state '0'`
			if [ $wanif_state = 2 ]; then
			cdb set ip `ifconfig  br1 | grep "inet" | awk -F ' ' '{print $2}' | sed 's/.*://g'`
		    cdb set audiodetectstatus '2'
				state=3
				cdb set wificonnectstatus '3'
				return 0
			fi
		fi
		if [ $loop = 0 ] || [ $omi_result -gt 2 ]; then
        cdb set ip '000.000.000.000'
		cdb set wificonnectstatus '4'
			return 1
		fi
		sleep 1
		loop=`expr $loop "-" 1`
	done
}

air_conf () {
	echo "air_conf is running"
	cdb set airkiss_state 0
	#local airkiss_is_running=0
	killall -9 airkiss 

	echo "air_conf"

	/usr/bin/airkiss start &
	#fi
	cdb set wl_bss_ssid2 ""
	cdb set wl_bss_wpa_psk2 ""
	local loop=65
	local air_conf_state=0
	while true
	do
		airkiss_state=`cdb get airkiss_state`	
		if [ $airkiss_state = 1 ]; then
			if [ $air_conf_state = 0 ]; then
				air_conf_state=1
			fi
		fi
		if [ $airkiss_state = 3 ]; then
			wanif_state=`cdb get wanif_state`
			if [ $wanif_state = 2 ]; then
				 echo "air_conf succeed!..."
				 cdb set airkiss_start 0
				 return 0;
			fi
		fi
		if [ $loop = 0 ] || [ $airkiss_state = 4 ]; then
			echo "air_conf airkiss_timeout++++++"
			killall -9 aplay
		  aplay -D common /tmp/res/invalid_password.wav &
			cdb set airkiss_start 0
			return 1
		fi
		sleep 1
		loop=`expr $loop "-" 1`
	done
}
action=$1
shift 1
case $action in
	"connect")
	  mpc pause > /dev/null
	  #mspotify_cli t > /dev/null
	  #air_cli d playpause > /dev/null
		apply "$@"
	;; #need ssid,password
	"mconnect")
	  mpc stop > /dev/null
	  mspotify_cli t > /dev/null
	  air_cli d playpause > /dev/null

		mapply "$@"
	;; #need ssid,password
	"restore")
		mpc pause > /dev/null
	  mspotify_cli t > /dev/null
	  air_cli d playpause > /dev/null
		restore
	;; #重连，自动连接上一次的wifi
	"disconnect") 	disconnect ;;
	"check") 	check ;;
	"config")
	   #     cdb set audionamestatus '1'
		mpc pause > /dev/null
	  mspotify_cli t > /dev/null
	  air_cli d playpause > /dev/null
	#	cdb set wl_bss_ssid_hidden1 1
		config
	;;
	"air_conf")
		mpc pause > /dev/null
	  #uci set xzxwifiaudio.config.audionamestatus=1 && uci commit
		killall -9 wavplayer > /dev/null
	  air_conf
	;;	
	*)
		echo "$0 usage:"
		echo "  $0 connect   save/apply < ssid password>"
		echo "  $0 restore   read elian config from flash and apply"
		echo "  $0 check   	 check wifi connect status"
	;;
esac
