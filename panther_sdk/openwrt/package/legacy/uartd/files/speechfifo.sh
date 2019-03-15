#!/bin/sh
speech_fifo=/tmp/SpeechFIFO
if [[ ! -p $speech_fifo ]]; then
    #echo "Reader not running"
    exit 1
else
	#echo "Reader is runing.."
	echo "$*" > $speech_fifo
fi
