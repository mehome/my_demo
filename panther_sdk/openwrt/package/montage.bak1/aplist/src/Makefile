MAKEFLAGS += --no-print-directory

MKDIR ?= mkdir -p
INSTALL ?= install
CC ?= "gcc"

CFLAGS ?= -O2 -g
CFLAGS += -Wall -Wundef -Wstrict-prototypes -Wno-trigraphs -fno-strict-aliasing -fno-common -Werror-implicit-function-declaration

OBJS = aplist.o

ALL = aplist

ifeq ($(V),1)
Q=
NQ=true
else
Q=@
NQ=echo
endif

all: $(ALL)

VERSION_OBJS := $(filter-out version.o, $(OBJS))

%.o: %.c
	@$(NQ) ' CC  ' $@
	$(Q)$(CC) $(CFLAGS) -c -o $@ $<

aplist:	$(OBJS)
	@$(NQ) ' CC  ' aplist
	$(Q)$(CC) $(LDFLAGS) $(OBJS) $(LIBS) -o aplist -lcdb -lm -lutils -lnl-tiny

clean:
	$(Q)rm -f aplist *.o *~ *.gz version.c *-stamp
