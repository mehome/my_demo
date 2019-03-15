LIBNAME=libutils.so

CFLAGS += -I./
LDFLAGS += -lcdb
INCLUDES += -I.

OBJS = $(patsubst %.c,%.o,$(wildcard *.c))

all: $(LIBNAME)

$(LIBNAME): $(OBJS)
	@echo "Building $< ..."
	$(CC) -shared -o $@ $^

%.o:%.c
	@echo "Compiling $< ..."
	$(CC) -c -Wall $(INCLUDES) $(CFLAGS) -o $@ $<

clean:
	rm -rf *.o $(LIBNAME)


