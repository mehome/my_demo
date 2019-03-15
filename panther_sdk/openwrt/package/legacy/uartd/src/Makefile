#common makefile header
#CC=mips-openwrt-linux-gcc
#LD=mips-openwrt-linux-ld
DIR_INC = ./include
DIR_LIB = ../libs

TARGET	=   uartd 
TEST = send_uartd_cmd

TEST_FILE= demo/send_uartd_cmd.c
CROSS_COMPILE = 
CFLAGS = -I$(DIR_INC)  -I$(STAGING_DIR)/usr/include -O2
LIBS = 
LDFLAGS += -lpthread -lrt -lasound -lcdb -ldl -lavsclient -lcchip 


OBJECTS := $(patsubst %.c,%.o,$(wildcard *.c))

all: $(TARGET) $(TEST)

$(TEST):$(TEST_FILE)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LIBS)

$(TARGET): $(OBJECTS)
	@echo "Building $@ ..."
	$(CC) -o $@ $^ $(LDFLAGS) $(LIBS)

%.o : %.c
	$(CC) -c $(CFLAGS) $< -o $@
clean:
	rm -f *.o $(TARGET)

.PHONY:clean
