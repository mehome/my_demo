#!/bin/sh
#################

sleep 10
status=`cat /tmp/BT_STATUS`
[ "$status" = "2" ] && echo 0 > /tmp/BT_STATUS
[ "$status" = "3" ] && echo 0 > /tmp/BT_STATUS
