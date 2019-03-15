#!/bin/sh
if [ "0" ="$1" ]; then
	musicurl0=`cdb get timer1_url`
	mpc clear
	mpc add "$musicurl0"
	mpc repeat off
	mpc play 1
elif [ "1" ="$1" ]; then
	musicurl1=`cdb get timer2_url`
	mpc clear
	mpc add "$musicurl1"
	mpc repeat off
	mpc play 1
else
	echo "param error: $1"
fi
