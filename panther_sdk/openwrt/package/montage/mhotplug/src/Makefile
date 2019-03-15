CC?=gcc
TARGET = hotplug-call

LDFLAGS +=

OBJS =$(patsubst %.c,%.o,$(wildcard *.c))

all: $(TARGET)

$(TARGET): $(OBJS)
	@echo "Building $@ ..."
	$(CC) -o $@ $^ $(LDFLAGS) $(LIB)

%.o:%.c
	@echo "Compiling $< ..."
	$(CC) -c -Wall $(CPPFLAGS) $(CFLAGS) -o $@ $<

clean:
	rm -rf *.o $(TARGET)

beautify:
	./call_Uncrustify.sh . c
	./call_Uncrustify.sh . h
