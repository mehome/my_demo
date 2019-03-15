CC = gcc
CFLAGS += -Os -Wall
#LDFLAGS +=
RM = rm -rf

all: cli.cgi

cli.cgi: cli_cgi.c
	$(CC) $(CFLAGS) $(LDFLAGS) cli_cgi.c -o $@

clean:
	$(RM) *~ cli.cgi

.PHONY: clean
