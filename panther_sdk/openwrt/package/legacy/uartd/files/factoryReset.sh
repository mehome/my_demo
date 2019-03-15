#!/bin/sh
killall monitor 2> /dev/null
REMAIN_PROCESS="\(|PID|factoryReset|uartd|procd|login|ash|watchdog|\)"
ps | grep -E -v $REMAIN_PROCESS | cut -b -6 | xargs kill 2> /dev/null
#uartdfifo.sh duerSceneLedSet009
#myPlay /root/appresources/factory_date_reset.mp3
aplay /root/res/cn/factory_date_reset.wav -D common
#uartdfifo.sh duerSceneLedSet102
cdb reset
sleep 1
rm -rf /overlay/*
sleep 1
reboot