#common makefile header
#CC=mips-openwrt-linux-gcc
#LD=mips-openwrt-linux-ld
DIR=$(shell pwd)
DIR_INC += -I$(CURDIR)
DIR_INC += -I$(CURDIR)/myutils
TARGET := libmyutils.so
CFLAGS +=  -Wunused-function -Wunused-but-set-variable -g  -I$(STAGING_DIR)/usr/include -O2
CFLAGS += $(DIR_INC) 
OBJECTS += $(patsubst %.c,%.o,$(wildcard ./*.c))

all: $(TARGET)
$(TARGET): $(OBJECTS)
	@echo "Building $@ ..."
	$(CC)  -o $@ $^ -shared  $(LDFLAGS) $(LIBS)
%.o : %.c
	$(CC) -fPIC -c $(CFLAGS) $< -o $@
	
clean:
	rm -f *.o $(TARGET)

.PHONY:clean
