CXX ?= "g++"
CFG_INC += -I$(shell pwd)/

LIBOBJS = dlna.o
LIBNAME = libdlna.so

ifeq ($(V),1)
Q=
NQ=true
else
Q=@
NQ=echo
endif

all: ${LIBNAME}

VERSION_OBJS := $(filter-out version.o, $(OBJS))

$(LIBOBJS): dlna.cxx
	@echo "Building $< ..."
	$(Q)$(CXX) $(CFLAGS) -c -o $@ $(CFG_INC) $<

$(LIBNAME): $(LIBOBJS)
	@echo "Building Library $< ..."
	$(CC) -shared -o $(shell pwd)/$@ $^

clean:
	$(Q)rm -f version.c *-stamp
