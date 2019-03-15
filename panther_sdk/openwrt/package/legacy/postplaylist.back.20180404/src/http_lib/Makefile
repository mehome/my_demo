ifeq ($(WIFIAUDIO_DEBUG), y)
CDB_LIB_SRC = cdb.c
CDB_LIB_TARGET = $(shell pwd)/openwrt_lib/libcdb.so
endif

JSON_LIB_SRC = WIFIAudio_RealTimeInterface.c WIFIAudio_RealTimeInterface.h
JSON_LIB_TARGET = libhttp_json.so

FIFO_LIB_SRC = WIFIAudio_FifoCommunication.c WIFIAudio_FifoCommunication.h filepathnamesuffix.h
FIFO_LIB_TARGET = libhttp_fifo.so

RET_LIB_SRC = WIFIAudio_RetJson.c WIFIAudio_RetJson.h
RET_LIB_TARGET = libhttp_ret_json.so

SEM_LIB_SRC = WIFIAudio_Semaphore.c WIFIAudio_Semaphore.h
SEM_LIB_TARGET = libhttp_sem.so

UCI_LIB_SRC = WIFIAudio_UciConfig.c WIFIAudio_UciConfig.h
UCI_LIB_TARGET = libhttp_uci.so

SO_LIB_TARGET = $(JSON_LIB_TARGET) $(FIFO_LIB_TARGET) $(RET_LIB_TARGET) $(SEM_LIB_TARGET) $(UCI_LIB_TARGET)
ifeq ($(WIFIAUDIO_DEBUG), y)
SO_LIB_TARGET += $(CDB_LIB_TARGET)
endif

all:	$(SO_LIB_TARGET)
ifeq ($(WIFIAUDIO_DEBUG), y)
$(CDB_LIB_TARGET):	$(CDB_LIB_SRC)
	$(CC) -shared -fPIC -Wl,-soname,$(CDB_LIB_TARGET) $^ -o $@ $(CFLAGS) $(LDFLAGS) $(LIBRARYS)
endif

$(JSON_LIB_TARGET):	$(JSON_LIB_SRC)
	$(CC) -shared -fPIC -Wl,-soname,$(JSON_LIB_TARGET) $^ -o $@ $(CFLAGS) $(LDFLAGS) $(LIBRARYS)
	
$(FIFO_LIB_TARGET):	$(FIFO_LIB_SRC)
	$(CC) -shared -fPIC -Wl,-soname,$(FIFO_LIB_TARGET) $^ -o $@ $(CFLAGS) $(LDFLAGS) $(LIBRARYS)
	
$(RET_LIB_TARGET):	$(RET_LIB_SRC)
	$(CC) -shared -fPIC -Wl,-soname,$(RET_LIB_TARGET) $^ -o $@ $(CFLAGS) $(LDFLAGS) $(LIBRARYS)
	
$(SEM_LIB_TARGET):	$(SEM_LIB_SRC)
	$(CC) -shared -fPIC -Wl,-soname,$(SEM_LIB_TARGET) $^ -o $@ $(CFLAGS) $(LDFLAGS) $(LIBRARYS)
	
$(UCI_LIB_TARGET):	$(UCI_LIB_SRC)
	$(CC) -shared -fPIC -Wl,-soname,$(UCI_LIB_TARGET) $^ -o $@ $(CFLAGS) $(LDFLAGS) $(LIBRARYS)

.PHONY:	clean
clean:
	rm -rf *.so

