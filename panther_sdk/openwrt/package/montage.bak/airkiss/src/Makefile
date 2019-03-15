# build airkiss executable when user executes "make"

AIR = airkiss
DIS = airkiss_discover
TARGET = $(AIR) $(DIS) 

image = airkiss airkissDis

CFG_INC += -I$(shell pwd)/inc
CFG_INC += -I$(CURDIR) -I$(CURDIR)/log 
LIBS += $(LDFLAGS)
#LIBS += -L$(STAGING_DIR)/usr/lib -lnl-tiny -lcdb
LIBS += -L$(STAGING_DIR)/usr/lib -lnl-tiny -lcdb -lm -ldl -ljson-c 

CFLAGS ?= $(CFG_INC) -g -o2
 
IMG_OBJS = $(patsubst %.c,%.o,$(wildcard *.c))
AIR_OBJS = airkiss.o 
AIR_DIS_OBJS += airkiss_discover.o  pidfile.o
AIRKISS_OBJS = $(filter-out $(AIR_DIS_OBJS) ,$(patsubst %.c,%.o,$(wildcard *.c)))
AIRKISS_DISCOVER_OBJS = $(filter-out $(AIR_OBJS) ,$(patsubst %.c,%.o,$(wildcard *.c)))

LOG_OBJS += $(patsubst %.c,%.o,$(wildcard log/*.c))
CONFIG_OBJS += $(patsubst %.c,%.o,$(wildcard config/*.c))
AIR_DIS_OBJS += $(LOG_OBJS)
AIR_DIS_OBJS += $(CONFIG_OBJS)

all: airkiss airkissDis

airkissDis: $(AIR_DIS_OBJS)
	@echo "Building $@ ..."
	$(CC) -o $@ $^ $(LIBS)
airkiss: $(AIR_OBJS)
	@echo "Building $@ ..."
	$(CC) -o $@ $^  $(LIBS)
	
%.o: %.c
	@echo "=== Compiling $< ..."
	$(CC) -c -Wall $(CFLAGS) -o $@ $<

clean:
	$(RM) *.d *.o $(image)
