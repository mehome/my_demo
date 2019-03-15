CC=gcc
WFLAGS=-Wall
CFLAGS=-Os
INCLUDES=-Iinclude

OBJS += bootvars.o
OBJS += crc16.o
OBJS += otp.o

all: bootvars

%.o: %.c
	$(CC) $(WFLAGS) -c -o $@ $(INCLUDES) $(CFLAGS) $<

bootvars: $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -lcdb -lm -o $@

clean:
	rm -f  bootvars *.o *~

.PHONY: clean
