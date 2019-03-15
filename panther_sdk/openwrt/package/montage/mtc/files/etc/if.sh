#!/bin/sh

FILE="/tmp/rule"
if [ -f ${FILE} ]; then
	. ${FILE}
fi

WANIF=${WAN%:*}
WANBIF=${WAN#*:}
WANBIF=${WANBIF:-${WAN}}
LANIF=${LAN}
PLUGIF=${IFPLUGD}
WANMODE=${MODE}
OPMODE=${OP}

