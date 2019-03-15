
# GNU Makefile for building with GCC on Linux and OS X.

SPOTIFY_INC = -I../include -I./
SPOTIFY_LIB = ../lib

CC = mips-openwrt-linux-gcc
CFLAGS = -g -O0
# Add the "lib" directory to the library search path, so that the example can
# find libspotify_embedded_shared.so at runtime. ("$ORIGIN" is for Linux and
# "@loader_path" is for OS X.)
# An alternative to passing these flags to the linker is to set the environment
# variable LD_LIBRARY_PATH to the "lib" directory when running the example.
LDFLAGS = -Wl,-rpath,\$$ORIGIN/${SPOTIFY_LIB} -Wl,-rpath,@loader_path/${SPOTIFY_LIB}

all: clean mspotify mspotify_cli quick_start

#mspotify: m_spotify.c sp_util.h sp_util.c ring_buffer.c ./ipc/ipc_message.c ./ipc/msg_handler.c
mspotify: m_spotify.c linux_alsa.c dummy.c appkey.c ring_buffer.c ./ipc/ipc_message.c ./ipc/msg_handler.c
#	$(CC) -o $@ ${CFLAGS} ${SPOTIFY_INC} m_spotify.c sp_util.c ring_buffer.c
	$(CC) -o $@ ${CFLAGS} ${SPOTIFY_INC} m_spotify.c linux_alsa.c appkey.c ring_buffer.c\
		./ipc/ipc_message.c ./ipc/msg_handler.c \
		${LDFLAGS} -L${SPOTIFY_LIB} -lspotify_embedded_shared -lm -lpthread -lrt -ldl -lcchip

mspotify_cli: ./ipc/m_spotify_cli.c
	$(CC) -o $@ ${CFLAGS} ${SPOTIFY_INC} ./ipc/m_spotify_cli.c \
		${LDFLAGS} -L${SPOTIFY_LIB} -lspotify_embedded_shared -lm

quick_start:  ./example/quick_start.c ./example/your_support_functions.h ./example/your_support_functions.c
	$(CC) -o $@ ${CFLAGS} ${SPOTIFY_INC} ./example/quick_start.c ./example/your_support_functions.c \
		${LDFLAGS} -L${SPOTIFY_LIB} -lspotify_embedded_shared -lm

clean:
	rm -f mspotify
	rm -f mspotify_cli
	rm -f quick_start
