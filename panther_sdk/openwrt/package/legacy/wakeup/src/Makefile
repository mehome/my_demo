
TARGET_SO = libwakeup.so
TARGET_T = wakeup
TARGET_A = libwakeup.a
TARGET = $(TARGET_A)
#LDFLAGS += -luci -lubox -lpthread -lm -L./lib -lthf -lasound -lmpdclient -lcdb -ldl ./lib/libMNPCSQE.a -lswresample -lavutil -lm
LDFLAGS 	+= -lpthread -lrt -lm -lasound -lcdb -ldl -lcchip



OBJECTS_T	:= $(patsubst %.c,%.o,$(shell find ./wakeupTest -name "*.c"))
OBJECTS_NON := $(patsubst %.c,%.o,$(shell find ./nonCompile -name "*.c"))
OBJECTS_ALL := $(patsubst %.c,%.o,$(shell find -name "*.c"))
OBJECTS 	:= $(filter-out $(OBJECTS_NON),$(filter-out $(OBJECTS_T),$(OBJECTS_ALL)))
OBJECTS_A = `find -name "*.o";`


CFLAGS_T		+=""
CFLAGS 			+= -fPIC

LDFLAGS_T		:= $(LDFLAGS)
LDFLAGS			+= -shared 

all: $(TARGET) install $(TARGET_T)


$(TARGET_A):$(OBJECTS)
	@echo "Building $@ ..."
	$(foreach i,$(TARGET_LIBS_A),$(AR) -x $i;)
	$(AR) -r -o $@ $(OBJECTS_A)
	
$(TARGET_SO): $(OBJECTS)
	@echo "Building $@ ..."
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

$(TARGET_T): $(OBJECTS_T) $(OBJECTS)
	@echo "Building $@ ..."
	$(CC) -o $@ $^ $(LDFLAGS_T) $(LIBS)	
	
%.o : %.c
	$(CC) -c $(CFLAGS) $< -o $@
	
clean:
	rm -f *.o $(TARGET) $(TARGET_T)
	
install:
	mkdir -p ipkg-install/usr/lib
	cp `find -name "*.a"` ipkg-install/usr/lib
	mkdir -p ipkg-install/usr/include
	# cp `find  -name "*.h"` ipkg-install/usr/include --parents
	cp interface.h ipkg-install/usr/include

.PHONY:clean
