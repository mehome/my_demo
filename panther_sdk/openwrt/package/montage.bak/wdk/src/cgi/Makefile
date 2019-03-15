CC = gcc
CFLAGS += -Os -Wall
#LDFLAGS +=
RM = rm -rf

all: upgrade.cgi check_login.cgi

upgrade.cgi: upgrade_cgi.c
	$(CC) $(CFLAGS) $(LDFLAGS) upgrade_cgi.c -o $@

check_login.cgi: md5.c check_login_cgi.c
	$(CC) $(CFLAGS) $(LDFLAGS) md5.c check_login_cgi.c -o $@

clean:
	$(RM) *~ cli.cgi upgrade.cgi check_login.cgi

.PHONY: clean
