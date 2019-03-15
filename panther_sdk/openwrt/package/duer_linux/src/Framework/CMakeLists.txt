cmake_minimum_required(VERSION 3.1 FATAL_ERROR)
project(Framework LANGUAGES CXX)

include_directories("${LoggerUtils_SOURCE_DIR}/include")
add_definitions("-DACSDK_LOG_MODULE=frameware")

add_subdirectory("DeviceIo")

if(${Platform} MATCHES "Hodor"
        OR ${Platform} MATCHES "Kuke" 
        OR ${Platform} MATCHES "MTK" 
        OR ${Platform} MATCHES "Dot"
        OR ${Platform} MATCHES "Box86")
    add_subdirectory("Timer")
endif()
