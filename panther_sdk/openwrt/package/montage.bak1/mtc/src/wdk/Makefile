
CFLAGS += -I./ -D_FILE_OFFSET_BITS=64 -I$(STAGING_DIR)/usr/include/libnl-tiny
LDFLAGS += 
INCLUDES += -I.

WDK_OBJS  = $(patsubst %.c,%.o,$(wildcard *.c))
WDK_OBJS += ../mtdm_client.o ../sys/mtdm_misc.o

LIBS = 

all: wdk-bin

wdk-bin: $(WDK_OBJS)
	@echo "Building $< ..."
	$(CC) -o $@ $^ $(LDFLAGS) $(LIBS)

%.o:%.c
	@echo "Compiling $< ..."
	$(CC) -c -Wall $(INCLUDES) $(CFLAGS) -o $@ $<

clean:
	rm -rf *.o $(image)


