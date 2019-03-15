#!/bin/sh
#kill applications about mtd
killall mtdaemon
killall hostapd
killall wpa_supplicant
killall uhttpd
killall mpdaemon
killall upmpdcli
killall omnicfg_bc
killall dnsmasq
killall ntpclient
#prepare before beginning
touch /tmp/ipconfig.txt
ifconfig sta0 down
ifconfig sta0 hw ether 0a:1b:33:5f:46:77
ifconfig sta0 up
ifconfig br0 down
ifconfig br1 down
brctl delbr br0
brctl delbr br1
ifconfig eth0.3 192.168.250.32 netmask 255.255.0.0
sleep 1

#start test application
./wfa_dut lo 8000&
sleep 1
export WFA_ENV_AGENT_IPADDR=127.0.0.1
export WFA_ENV_AGENT_PORT=8000
./wfa_ca eth0.3 9000 &

sleep 1
wpa_supplicant -Dnl80211 -ista0 -c./wpa.conf&
