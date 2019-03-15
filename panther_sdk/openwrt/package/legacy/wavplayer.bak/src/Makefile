#common makefile header
#CC=mips-openwrt-linux-gcc
#LD=mips-openwrt-linux-ld
DIR_INC = ./include
DIR_LIB = ../libs

TARGET	=   wavplayer 

CROSS_COMPILE = 
CFLAGS =	-g  -I$(DIR_INC)  -I$(STAGING_DIR)/usr/include -O2 \
			-Wunused-function -Wunused-but-set-variable
LIBS = 
LDFLAGS += -lasound -lcchip -lpthread -ldl -lm -lrt


OBJECTS := $(patsubst %.c,%.o,$(wildcard *.c))

all: $(TARGET)


$(TARGET): $(OBJECTS)
	@echo "Building $@ ..."
	$(CC) -o $@ $^ $(LDFLAGS) $(LIBS)

%.o : %.c
	$(CC) -c $(CFLAGS) $< -o $@
clean:
	rm -f *.o $(TARGET)

.PHONY:clean
