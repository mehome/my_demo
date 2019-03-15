TARGET	= libavsclient_test 
DIR_INC = ./include/avsclient
CROSS_COMPILE = 
CCFLAGS += -Wunused-function -Wunused-but-set-variable -g -I$(DIR_INC) -I$(STAGING_DIR)/usr/include
CCFLAGS +=  -fPIC
LIBS = 
#   
LDFLAGS += -lssl -lcrypto -ljson-c -ldl
LDFLAGS += -shared
 
OBJECTS := $(patsubst %.c,%.o,$(wildcard *.c))

all: $(TARGET)

$(TARGET): $(OBJECTS)
	@echo "Building $@ ..."
	$(CC) -o libavsclient.so $^ $(LDFLAGS) $(LIBS)
	#$(CC) -o $@ $^ $(LDFLAGS) $(LIBS)

%.o : %.c
	$(CC) -c $(CCFLAGS) $< -o $@
clean:
	rm -f *.o *.so $(TARGET)

.PHONY:clean
