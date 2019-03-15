#!/bin/sh
crond_process_process=""
crondfile="/etc/crontabs/root"
SYNTIMEFile="/tmp/synctime"

#loglevel='uci get "system.@system[0].cronloglevel"'

SEND_DATE()
{
	#time_date=`date`
	#数值： 秒，分，时，日，week，moon，year
    #例子：0x59, 0x46 ,0x11, 0x21 ,0x01 ,0x06 ,0x16
    #week的格式：bit0----------- 星期天，bit1------------bit6对应星期1-----------------星期6
	time_S=`date "+%S"` #秒
	time_M=`date "+%M"` #分
	time_H=`date "+%H"` #时
	time_d=`date "+%d"` #日
	time_w=`date "+%w"` #week
	time_m=`date "+%m"` #月
	time_Y=`date "+%Y"` #年

	echo "$time_S $time_M $time_H $time_d $time_w $time_m $time_Y" >> SYNTIMEFile
	echo "crondstart $time_S $time_M $time_H $time_d $time_w $time_m $time_Y"
	/usr/bin/uartdfifo.sh "settime $time_S $time_M $time_H $time_d $time_w $time_m $time_Y"
}

if [ "$1" = "syntime" ]; then
	if [ ! -f "$SYNTIMEFile" ]; then
		touch "$SYNTIMEFile"

		#发送同步时间指令到UART进程
		SEND_DATE

		#将此次同步时间的时间写到文件
		echo "`date +%s`" > SYNTIMEFile
	else
		#若和上次同步时间没超过5分钟 则此次不同步时间
		time_date=`date +%s`
		read old_time < SYNTIMEFile
		difference_time=`expr $time_date - $old_time`
		if [ $difference_time -ge "300" ];then
			echo "`date +%s`" > SYNTIMEFile
			SEND_DATE
		else
			echo " " > /dev/null
		fi
	fi
fi
if [ "$1" = "sendtime" ]; then
	#发送同步时间指令到UART进程
	SEND_DATE
fi
