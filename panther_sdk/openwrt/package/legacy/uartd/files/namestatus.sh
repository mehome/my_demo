#!/bin/sh
#################

loop=100
while true
do
	if [ $loop = 0 ]; then
#		uci set xzxwifiaudio.config.audionamestatus=1
        cdb set audionamestatus '1'
		exit 0
	fi
	sleep 1
	loop=`expr $loop "-" 1`
done
