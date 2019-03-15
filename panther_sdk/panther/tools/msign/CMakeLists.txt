cmake_minimum_required(VERSION 2.8)

PROJECT(msign C)
ADD_DEFINITIONS(-O2 -Wall -Werror --std=gnu99 -g3 -Wmissing-declarations)

SET(SOURCES ed25519.c edsign.c f25519.c fprime.c sha512.c main.c encrypt.c)

SET(LIBS)
IF(USE_LIBUBOX)
  ADD_DEFINITIONS(-DUSE_LIBUBOX)
  SET(LIBS ubox)
ELSE()
  SET(SOURCES ${SOURCES} base64.c)
ENDIF()

ADD_EXECUTABLE(msign ${SOURCES})
TARGET_LINK_LIBRARIES(msign ${LIBS})

INSTALL(TARGETS msign
	RUNTIME DESTINATION bin
)
