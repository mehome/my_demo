CC=gcc
WFLAGS=-Wall
CFLAGS=-Os
INCLUDES=-Iinclude

LIBCDB=libcdb.so
LIBCDB_STATIC=libcdb.a

#all: $(LIBCDB) $(LIBCDB_STATIC) cdb
all: $(LIBCDB) cdb

%.o: %.c
	$(CC) $(WFLAGS) -c -o $@ $(INCLUDES) $(CFLAGS) $<

LIBCDB_OBJ=libcdb.o

$(LIBCDB): $(LIBCDB_OBJ)
	#$(CC) -shared -o $@ $^ -Wl,--whole-archive -lwgdb
	$(CC) -shared -o $@ $^ -lwgdb

$(LIBCDB_STATIC): $(LIBCDB_OBJ)
	$(AR) crv $@ $<

cdb: cdb_cli.c
	$(CC) $(CFLAGS) $(LDFLAGS) cdb_cli.c $(LIBCDB) -lm -o $@

clean:
	rm -f $(LIBCDB) cdb *.o *~
	
