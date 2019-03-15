TARGET			?= libcchip.so 
TARGET_T		?= test.excute
DIR_INC			+= -I.
CFLAGS			+= -fPIC -fno-builtin $(DIR_INC) -O2 \
					-Wno-unused-function \
					-Wno-unused-variable \
					-Wno-unused-but-set-variable  \
					-Wno-unused-label \
					-Wall 
#					-Werror
					
LDFLAGS_T		:= -L. $(LDFLAGS)
LDFLAGS			+= -shared
LIBS 			+= -lasound -lcdb -lpthread -lrt -ldl -lm

OBJECTS_ALL	:= $(patsubst %.c,%.o,$(shell find -name *.c))
OBJECTS_NON := $(patsubst %.c,%.o,$(shell find ./nonCompile -name *.c))
OBJECTS_T	:= $(patsubst %.c,%.o,$(shell find ./test_excute -name *.c))
OBJECTS 	:= $(filter-out $(OBJECTS_T),$(filter-out $(OBJECTS_NON),$(OBJECTS_ALL)))

all: $(TARGET) $(TARGET_T)

$(TARGET): $(OBJECTS)
	@echo "Building $@ ..."	
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

$(TARGET_T): $(OBJECTS_T)
	@echo "Building $@ ..."	
	$(CC) $(LDFLAGS_T) -o $@ $^ -lcchip $(LIBS)
	
%.o : %.c
	$(CC) -c $(CFLAGS) $< -o $@
	
clean:
	rm -rf  $(TARGET) $(OBJECTS_T)
	
install:
	mkdir -p ipkg-install/usr/lib
	cp *.so ipkg-install/usr/lib
	mkdir -p ipkg-install/usr/include
	cp `find  -name "*.h"` ipkg-install/usr/include --parents
#	cp $(TARGET) /home/samba/nfs/lz/duer/lib -rf
	
.PHONY:clean

ttt:
	@echo --$(OBJECTS_NON)--
