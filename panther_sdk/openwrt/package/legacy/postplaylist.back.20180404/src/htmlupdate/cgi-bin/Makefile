LIBRARYS += -lhttp_json -lhttp_fifo -lhttp_ret_json -lhttp_sem -lhttp_uci

DEVICEUPDATE_TARGET = posthtmlupdate.cgi
#推送文件更新

all:	$(DEVICEUPDATE_TARGET)
	
$(DEVICEUPDATE_TARGET):	$(DEVICEUPDATE_TARGET:%.cgi=%.c) $(DEVICEUPDATE_LIB) $(DEVICEUPDATE_SRC)
	$(CC) $(@:%.cgi=%.c) -o $@ $(CFLAGS) $(LDFLAGS) $(LIBRARYS)
	
.PHONY:	clean
clean:
	rm -rf $(DEVICEUPDATE_TARGET) 

