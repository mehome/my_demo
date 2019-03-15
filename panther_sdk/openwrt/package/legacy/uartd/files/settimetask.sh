#!/bin/sh

if [ "$1" = "setalarm" ];then
#设置闹钟信息 /usr/script/settimetask.sh setalarm '闹钟编号' '闹钟信息' '歌曲'
#timernumber=$2 timerinfo=$3 timermusic=$4
	if [ "0" = "$2" ];then
		cdb set timer0 "$3"
		cdb set timer0_url "$4"
		/usr/bin/uartdfifo.sh setalarm "$2 $3"
	elif [ "1" = "$2" ];then
		cdb set timer1 "$3"
		cdb set timer1_url "$4"
		/usr/bin/uartdfifo.sh setalarm "$2 $3"
	else
		echo "timernumber error: $2"
	fi
	
elif [ "$1" = "deletealarm" ];then
	if [ "0" = "$2" ];then
		cdb set timer0 " "
		cdb set timer0_url " "
		/usr/bin/uartdfifo.sh deletealarm "$2"
	elif [ "1" = "$2" ];then
		cdb set timer1 " "
		cdb set timer1_url " "
		/usr/bin/uartdfifo.sh deletealarm "$2"
	else
		echo "timernumber error: $2"
	fi

elif [ "$1" = "settime" ];then
	echo "set time sync.."
	time=$2
	varlength=`echo $time | awk '{print length ($0)}'`
	echo "varlength:$varlength"

	if [ $varlength = 14 ]; then
		echo "varlength ok!"
		YYYY=${time:0:4}
		echo "$YYYY"
		if [ "$YYYY" -ge "1970" ]; then
			flag=`echo $time | grep ^[0-9][0-9][0-9]*$`
			if [ -n "$flag" ];then
					MM=${time:4:2}
					DD=${time:6:2}
					hh=${time:8:2}
					mm=${time:10:2}
					ss=${time:12:2}
					echo "YYYY:$YYYY,MM:$MM,DD:$DD,hh:$hh,mm:$mm,ss:$ss"
					date -s $YYYY.$MM.$DD-$hh:$mm:$ss > /dev/null
					/usr/script/crondStart.sh syntime
					echo "ok"
					/etc/init.d/sysntpd restart
			else
					echo "error"
			fi
		fi
	fi

else
	echo "param error: $1"
fi
