MAKEFLAGS += --no-print-directory

MKDIR ?= mkdir -p
INSTALL ?= install
CC ?= "gcc"

CFLAGS ?= -O2 -g
CFLAGS += -Wall -Wundef -Wstrict-prototypes -Wno-trigraphs -fno-strict-aliasing -fno-common -Werror-implicit-function-declaration
LIBS += -lmpdclient

LIBOBJS = lib/mon.o

LIBNAME = libmonitor.so

ifeq ($(V),1)
Q=
NQ=true
else
Q=@
NQ=echo
endif

all: ${LIBNAME}

VERSION_OBJS := $(filter-out version.o, $(OBJS))

$(LIBNAME): $(LIBOBJS)
	@echo "Building $< ..."
	$(CC) -shared -o $(shell pwd)/lib/$@ $^

clean:
	$(Q)rm -f *~ *.gz version.c *-stamp
