DIR_INC = ./include
DIR_LIB = ../libs

TARGET = bb

CROSS_COMPILE =
CFLAGS = -Os -ggdb -Wunused-function -Wunused-but-set-variable -I./ -I$(DIR_INC)  -I$(STAGING_DIR)/usr/include
LIBS =
LDFLAGS += -L./lib -ldl -lcdb -lutils -lnl-tiny

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

