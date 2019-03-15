#!/bin/sh
#CFG=./upmpdcli.conf
#CFG=/etc/upmpdcli.conf
#K="friendlyname"
NAME=""
#BR0PID="/var/run/upmpdcli.br0"
#BR1PID="/var/run/upmpdcli.br1"

. /etc/cdb.sh
omnicfg_cdb_get()
{
	cdb_data=""
	[ $# -lt 2 ] && echo usage: omnicfg_cdb_get output_variable default_value && exit 1

	config_get cdb_data "$1"
	cdb_data=$( config_chk "$cdb_data" )
	[ -z "$cdb_data" ] && cdb_data="$2"

	echo $cdb_data
}

if [ ! -n "$*" ] ;then
    echo "you have not input a name!"
else
	NAME="$*"
	#echo "the name you input is $NAME"
	#change name to upmpdcli.conf
	#sed -i "/^$K/c $K = $NAME" $CFG
	cdb set ra_name "$NAME"
	
	#[ -f ${BR0PID} ] && {
	#	pid=`cat ${BR0PID}`
	#	kill -9 $pid
	#	rm ${BR0PID}
	#}
	#[ -f ${BR1PID} ] && {
	#	pid=`cat ${BR1PID}`
	#	kill -9 $pid
	#	rm ${BR1PID}
	#}
	cdb commit
	
	uci set xzxwifiaudio.config.audioname="$NAME"
	uci commit
	
	killall -9 upmpdcli

	[ -x /usr/bin/upmpdcli ] && {
		#config_get op_work_mode op_work_mode
		op_work_mode=$( omnicfg_cdb_get op_work_mode '0' )
		if [ ${op_work_mode} = 9 ]; then
			echo "op_work_mode = 9"
			/usr/bin/upmpdcli -h 127.0.0.1 -f "$NAME" -i br1 -D
		elif [ ${op_work_mode} = 3 ]; then
			echo "op_work_mode = 3"
			/usr/bin/upmpdcli -h 127.0.0.1 -f "$NAME" -i br0 -D 
			/usr/bin/upmpdcli -h 127.0.0.1 -f "$NAME" -i br1 -D
		else
			echo "op_work_mode else"
			/usr/bin/upmpdcli -h 127.0.0.1 -f "$NAME" -i br0 -D 
		fi
	}
	
	#killall -9 shairport
	#/usr/sbin/shairport -B mpc clear -a $NAME -b 256
	/etc/init.d/shairport stop
	/etc/init.d/shairport start
fi
