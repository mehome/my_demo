DIR_INC = ./include
DIR_LIB = ../libs

TARGET	= Collection

CROSS_COMPILE = 
CFLAGS =  -Wunused-function -Wunused-but-set-variable -g  -I$(DIR_INC)  -I$(STAGING_DIR)/usr/include
LIBS = 
LDFLAGS += -lpthread -ljson-c -lcurl -ldl -lm

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
