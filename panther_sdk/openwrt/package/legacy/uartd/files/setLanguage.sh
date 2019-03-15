#!/bin/sh
#################
case "$1" in
	"cn" | "CN")
	mkdir -p /tmp/res/
	umount /tmp/res/
	mount /root/res/cn/ /tmp/res/
	cdb set sys_language "cn" 
	;;
	"en" | "EN")
	mkdir -p /tmp/res/
	umount /tmp/res/
	mount /root/res/en/ /tmp/res/
	cdb set sys_language "en" 
	;;
	"init")
	LANGUAGE=`cdb get sys_language`
	[ "$LANGUAGE" = "" ] && LANGUAGE="cn"
	mkdir -p /tmp/res/
	umount /tmp/res/
	mount /root/res/$LANGUAGE/ /tmp/res/
	;;
	*)
	echo "not support this sys_language"
	;;
esac

