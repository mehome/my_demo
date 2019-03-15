# Which compiler
#CC = $(CROSS_COMPILE)gcc
INCLUDES	+= -I. -I../../curl/src/include -I$(STAGING_DIR)/usr/include
#Options for development
CFLAGS= -Wall $(INCLUDES)
LDFLAGS += -lcurl -lnghttp2 -lssl -lcrypto -lutils -lpthread -lnl-tiny -lcdb -ldl  -lm -ljson-c
#LDFLAGS += -L$(ROOT)/utility/build_dir/target-mips-openwrt-linux/curl-7.51.0/ipkg-montage/libcurl/usr/lib
LIBS	+=

OBJECTS := $(patsubst %.c,%.o,$(wildcard *.c))
OBJECTS1 := $(patsubst %.c,%.o,$(wildcard httpapi/*.c))

TARGET=dueros_ota
all: $(TARGET)

$(TARGET): $(OBJECTS)
	@echo "Building $@ ..."
	$(CC) -o $@ $^ $(LDFLAGS) $(LIBS)
	
	
%.o : %.c
	$(CC) -c $(CFLAGS) $< -o $@
clean:
	rm -f *.o $(TARGET)

.PHONY:clean