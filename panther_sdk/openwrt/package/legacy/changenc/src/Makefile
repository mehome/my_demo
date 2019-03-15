#
# Copyright (C) 2016 Montage Inc.
#

CC = mips-openwrt-linux-gcc
CFLAGS = -g -O0

all: clean changenc

changenc: changenc.c
	$(CC) -o $@ ${CFLAGS} changenc.c ${LDFLAGS}
		
clean:
	rm -f changenc
