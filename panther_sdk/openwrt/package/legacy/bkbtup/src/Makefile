# Which compiler
#CC = $(CROSS_COMPILE)gcc
INCLUDES	+= -I. -I$(STAGING_DIR)/usr/include
#Options for development
CFLAGS= -Wall $(INCLUDES)
LDFLAGS +=-lpthread -lcdb -ldl -lm
LIBS	+= 

OBJECTS := $(patsubst %.c,%.o,$(wildcard *.c))

TARGET=btup

all: $(TARGET)

$(TARGET): $(OBJECTS)
	@echo "Building $@ ..."
	$(CC) -o $@ $^ $(LDFLAGS) $(LIBS)

%.o : %.c
	$(CC) -c $(CFLAGS) $< -o $@
clean:
	rm -f *.o $(TARGET)

.PHONY:clean