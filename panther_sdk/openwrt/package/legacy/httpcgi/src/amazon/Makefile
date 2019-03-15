LIBRARYS += -lavsclient

BASE_SRC := amazon_Json.c amazon_Semaphore.c WIFIAudio_RetJson.c
BASE_INC := amazon_Json.h amazon_Semaphore.h WIFIAudio_RetJson.h

DEVICEINFO_SRC := amazon_deviceInfo.c ${BASE_SRC} ${BASE_INC}
DEVICEINFO_TARGET := deviceInfo.cgi

COMPANIONINFO_SRC := amazon_companionInfo.c ${BASE_SRC} ${BASE_INC}
COMPANIONINFO_TARGET := companionInfo.cgi

TARGETS := ${DEVICEINFO_TARGET} ${COMPANIONINFO_TARGET}

.PHONY: all install clean
all:	${TARGETS}

${DEVICEINFO_TARGET}:	${DEVICEINFO_SRC}
	$(CC) $^ -o $@ ${CFLAGS} ${LDFLAGS} ${LIBRARYS}
	
${COMPANIONINFO_TARGET}:	${COMPANIONINFO_SRC}
	$(CC) $^ -o $@ ${CFLAGS} ${LDFLAGS} ${LIBRARYS}

install:
	mkdir -p ${INSTALL_DIR}/include/
	mkdir -p ${INSTALL_DIR}/www/cgi-bin/
	cp -rf ${BASE_INC} ${INSTALL_DIR}/include/
	cp -rf ${TARGETS} ${INSTALL_DIR}/www/cgi-bin/

clean:
	rm -rf ${TARGETS}
	
