# build airkiss executable when user executes "make"

image = airkiss

CFG_INC ?= -I$(shell pwd)/inc
LIBS += $(LDFLAGS)
#LIBS += -L$(STAGING_DIR)/usr/lib -lnl-tiny -lcdb
LIBS += -L$(STAGING_DIR)/usr/lib -lnl-tiny -lcdb -lm -ldl

CFLAGS ?= $(CFG_INC) -g -o2
 
IMG_OBJS = $(patsubst %.c,%.o,$(wildcard *.c))

all: $(image)

%.o: %.c
	@echo "=== Compiling $< ..."
	$(CC) -c -Wall $(CFLAGS) -o $@ $<

$(image): $(IMG_OBJS)
	@echo "=== link $< ..."
	$(CC) -o $@ $^ $(CFG_INC) $(LIBS)

clean:
	$(RM) *.d *.o $(image)
