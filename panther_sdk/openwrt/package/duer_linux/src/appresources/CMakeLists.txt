cmake_minimum_required(VERSION 3.1 FATAL_ERROR)
project(APPRESOURCES LANGUAGES CXX)

include(../build/BuildDefaults.cmake)

install(DIRECTORY 
    "${PROJECT_SOURCE_DIR}/common/"
    DESTINATION
    "${CMAKE_INSTALL_PREFIX}/appresources"
    FILES_MATCHING PATTERN "*.mp3")

install(DIRECTORY 
    "${PROJECT_SOURCE_DIR}/common/"
    DESTINATION
    "${CMAKE_INSTALL_PREFIX}/appresources"
    FILES_MATCHING PATTERN "*.wav")
	
install(DIRECTORY 
    "${PROJECT_SOURCE_DIR}/wakeUpSound/"
    DESTINATION
    "${CMAKE_INSTALL_PREFIX}/appresources"
    FILES_MATCHING PATTERN "*.mp3")
	
install(DIRECTORY 
    "${PROJECT_SOURCE_DIR}/common/"
    DESTINATION
    "${CMAKE_INSTALL_PREFIX}/appresources"
    FILES_MATCHING PATTERN "*.pcm")

if(${Platform} MATCHES "Hodor"
        OR ${Platform} MATCHES "MTK"
        OR ${Platform} MATCHES "Kuke"
        OR ${Platform} MATCHES "Raspiberry"
        OR ${Platform} MATCHES "Dot"
        OR ${Platform} MATCHES "Box86")
    install(DIRECTORY
            "${PROJECT_SOURCE_DIR}/${Platform}/"
            DESTINATION
            "${CMAKE_INSTALL_PREFIX}/appresources")
else()
    # install(DIRECTORY
            # "${PROJECT_SOURCE_DIR}/Ubuntu/"
            # DESTINATION
            # "${CMAKE_INSTALL_PREFIX}/appresources")
endif()