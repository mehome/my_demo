LIBRARYS += -lavsclient -lhttp_ret_json

DEVICEINFO_INC := Lenovo_Json.h Lenovo_Semaphore.h uuid.h
DEVICEINFO_SRC := Lenovo_deviceInfo.c Lenovo_Json.c Lenovo_Semaphore.c uuid.c
ifeq ($(WIFIAUDIO_DEBUG), y)
DEVICEINFO_TARGET := deviceInfo.cgi
else
DEVICEINFO_TARGET := deviceInfo
endif

COMPANIONINFO_INC := Lenovo_Json.h Lenovo_Semaphore.h
COMPANIONINFO_SRC := Lenovo_companionInfo.c Lenovo_Json.c Lenovo_Semaphore.c
ifeq ($(WIFIAUDIO_DEBUG), y)
COMPANIONINFO_TARGET := companionInfo.cgi
else
COMPANIONINFO_TARGET := companionInfo
endif


all:	$(DEVICEINFO_TARGET) $(COMPANIONINFO_TARGET) 
	
$(DEVICEINFO_TARGET):	$(DEVICEINFO_INC) $(DEVICEINFO_SRC)
	$(CC) $^ -o $@ $(CFLAGS) $(LDFLAGS) $(LIBRARYS)
	
$(COMPANIONINFO_TARGET):	$(COMPANIONINFO_INC) $(COMPANIONINFO_SRC)
	$(CC) $^ -o $@ $(CFLAGS) $(LDFLAGS) $(LIBRARYS)
	
.PHONY:	clean
clean:
	rm -rf $(DEVICEINFO_TARGET)
	rm -rf $(COMPANIONINFO_TARGET)
	
