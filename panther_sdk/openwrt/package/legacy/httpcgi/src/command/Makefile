CFLAGS := -I${shell pwd}/ ${CFLAGS}
CFLAGS := -Wall
LDFLAGS := -L${shell pwd}/ ${LDFLAGS}

BASE_SRC := WIFIAudio_GetCommand.c WIFIAudio_RealTimeInterface.c WIFIAudio_RetJson.c WIFIAudio_SystemCommand.c
BASE_INC := WIFIAudio_GetCommand.h WIFIAudio_RealTimeInterface.h WIFIAudio_RetJson.h WIFIAudio_SystemCommand.h

HTTPAPI_SRC := WIFIAudio_httpapi.c ${BASE_SRC} ${BASE_INC}
HTTPAPI_TARGET := httpapi.cgi

POSTHTMLUPDATE_SRC := posthtmlupdate.c ${BASE_SRC} ${BASE_INC}
POSTHTMLUPDATE_TARGET := posthtmlupdate.cgi

POSTBLUETOOTHUPDATE_SRC := postbluetoothupdate.c ${BASE_SRC} ${BASE_INC}
POSTBLUETOOTHUPDATE_TARGET := postbluetoothupdate.cgi

POSTCONEXANTBIN_SRC := PostConexantBin.c ${BASE_SRC} ${BASE_INC}
POSTCONEXANTBIN_TARGET := PostConexantBin.cgi

POSTCONEXANTSYS_SRC := PostConexantSfs.c ${BASE_SRC} ${BASE_INC}
POSTCONEXANTSYS_TARGET := PostConexantSfs.cgi

TARGET := ${HTTPAPI_TARGET} ${POSTHTMLUPDATE_TARGET} ${POSTBLUETOOTHUPDATE_TARGET} ${POSTCONEXANTBIN_TARGET} ${POSTCONEXANTSYS_TARGET}

.PHONY: all install clean
all:	${TARGET}

${HTTPAPI_TARGET}:	${HTTPAPI_SRC}
	${CC} -o $@ $^ ${CFLAGS} ${LDFLAGS} ${LIBRARYS}
	
${POSTHTMLUPDATE_TARGET}:	${POSTHTMLUPDATE_SRC}
	${CC} -o $@ $^ ${CFLAGS} ${LDFLAGS} ${LIBRARYS}
	
${POSTBLUETOOTHUPDATE_TARGET}:	${POSTBLUETOOTHUPDATE_SRC}
	${CC} -o $@ $^ ${CFLAGS} ${LDFLAGS} ${LIBRARYS}
	
${POSTCONEXANTBIN_TARGET}:	${POSTCONEXANTBIN_SRC}
	${CC} -o $@ $^ ${CFLAGS} ${LDFLAGS} ${LIBRARYS}
	
${POSTCONEXANTSYS_TARGET}:	${POSTCONEXANTSYS_SRC}
	${CC} -o $@ $^ ${CFLAGS} ${LDFLAGS} ${LIBRARYS}
	
install:
	mkdir -p ${INSTALL_DIR}/include/
	mkdir -p ${INSTALL_DIR}/www/cgi-bin/
	cp -rf ${BASE_INC} ${INSTALL_DIR}/include/
	cp -rf ${TARGET} ${INSTALL_DIR}/www/cgi-bin/
	
clean:
	-${RM} -r ${TARGET}
	
