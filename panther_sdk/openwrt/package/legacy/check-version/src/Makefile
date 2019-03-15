# Which compiler
#CC = $(CROSS_COMPILE)gcc
INCLUDES	+= -I. -I../../curl/src/include -I$(STAGING_DIR)/usr/include
#Options for development
CFLAGS= -Wall $(INCLUDES)
LDFLAGS += -lcurl -lnghttp2 -lssl -lcrypto -lutils -lpthread -lnl-tiny -lcdb -lm -ldl -lcchip -lrt
LIBS	+= 

OBJECTS := $(patsubst %.c,%.o,$(wildcard *.c))

TARGET=check-version

all: $(TARGET)

$(TARGET): $(OBJECTS)
	@echo "Building $@ ..."
	$(CC) -o $@ $^ $(LDFLAGS) $(LIBS)

%.o : %.c
	$(CC) -c $(CFLAGS) $< -o $@
clean:
	rm -f *.o $(TARGET)

.PHONY:clean