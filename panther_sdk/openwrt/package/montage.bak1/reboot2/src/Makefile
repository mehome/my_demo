PROG = reboot2
OBJS = reboot2.o
SRCS = reboot2.c

all: $(PROG)

$(PROG): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(PROG) $(LIBS)

clean:
	rm -f reboot2 *.o *~
