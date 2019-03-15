#!/bin/sh
pipe=/tmp/UpmpdFIFO
if [[ ! -p $pipe ]]; then
    #echo "Reader not running"
    exit 1
else
	#echo "Reader is runing.."
	echo "$*" > $pipe
fi

