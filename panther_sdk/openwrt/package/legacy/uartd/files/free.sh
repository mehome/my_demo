#!/bin/sh
free_memory() {
    # stop processes to release memory and avoid unexcepted problem
    if [ "$2" = "all" ]; then
        PS=`ps | grep -v 'ash\|watchdog\|wdkupgrade'`
    else
    killall -9 mtdaemon mpdaemon upmpdcli mspotify shairport AlexaRequest uartd > /dev/null
    
 #       PS=`ps | grep -v 'uhttpd\|hostapd\|ash\|watchdog\|sysupgrade\|dhcpcd\|dnsmasq\|WIFIAudio_DownloadBurn\|telnetd\|$$\|ps'`
   fi
 #   if [ "${WDKUPGRADE_KEEP_ALIVE}" != "" ]; then
 #       PS=`echo "${PS}" | grep -v "${WDKUPGRADE_KEEP_ALIVE}"`
 #   fi
#    echo "${PS}" | awk '{if(NR > 2) print $1}' | tr '\n' ' ' | xargs kill -9 > /dev/null  2>&1
    echo "3" > /proc/sys/vm/drop_caches
    sync
}

case "$1" in
	"free")
		echo "free_memory..."
		free_memory $2
	;;
esac
