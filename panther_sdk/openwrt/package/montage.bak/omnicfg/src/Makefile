#
# Copyright (C) 2016 Montage Inc.
#

CC = mips-openwrt-linux-gcc
CFLAGS = -g -O0

all: clean omnicfg_broadcast

omnicfg_broadcast: omnicfg_broadcast.c
	$(CC) -o $@ ${CFLAGS} omnicfg_broadcast.c ${LDFLAGS}
		
clean:
	rm -f omnicfg_broadcast
