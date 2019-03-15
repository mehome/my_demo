// Copyright 2017 Baidu Inc. All Rights Reserved.
// Author: chenguoguo (chenguoguo@baidu.com)
//         xingrentai (xingrentai@baidu.com)
//
// snowboy types


#ifndef DUER_SNOWBOY_SRC_UTILS_SNOWBOY_TYPES_H
#define DUER_SNOWBOY_SRC_UTILS_SNOWBOY_TYPES_H

#include <stdint.h>
#include <inttypes.h>

#ifdef DUER_PLATFORM_ESPRESSIF
//#include "esp_system.h"
#include <stdbool.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef uint8_t   uint8;
typedef uint16_t  uint16;
typedef uint32_t  uint32;
typedef uint64_t  uint64;
typedef int8_t    int8;
typedef int16_t   int16;
typedef int32_t   int32;
typedef int64_t   int64;

#ifndef DUER_PLATFORM_ESPRESSIF
typedef uint8_t   bool;
#endif

typedef int32     MatrixIndexT;

#define DUER_TRUE   (1)
#define DUER_FALSE  (0)

#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#define MIN(a, b) (((a) < (b)) ? (a) : (b))

typedef enum DuerDataType {
    DUER_DATA_TYPE_INT32,
    DUER_DATA_TYPE_INT16,
    DUER_DATA_TYPE_INT8,
    DUER_DATA_TYPE_INT8_QUANTIZED,
    DUER_DATA_TYPE_FLOAT,
    DUER_DATA_TYPE_BOOL,
    DUER_DATA_TYPE_STRING,
    DUER_DATA_TYPE_VECTOR,
    DUER_DATA_TYPE_VECTOR2,
    DUER_DATA_TYPE_VECTOR3,
    DUER_DATA_TYPE_VECTOR4,
    DUER_DATA_TYPE_MATRIX,

    DUER_DATA_TYPE_OTHER,

    DUER_DATA_TYPE_COUNT,
} DuerDataType;

typedef enum DuerFrameProperty {
    DUER_FRAME_PROPERTY_VOICE  = 1 << 0,   // 00000001
} DuerFrameProperty;

typedef enum DuerSnowboySignal {
    DUER_SIGNAL_SUCCESS                 = 1 << 0,   // 00000001
    DUER_SIGNAL_FAIL                    = 1 << 1,   // 00000010
    DUER_SIGNAL_SEGMENT_START           = 1 << 2,   // 00000100
    DUER_SIGNAL_SEGMENT_END             = 1 << 3,   // 00001000
    DUER_SIGNAL_STREAM_END              = 1 << 4,   // 00010000
    DUER_SIGNAL_STREAM_PAUSE            = 1 << 5,   // 00100000
    DUER_SIGNAL_STREAM_INIT_FAILURE     = 1 << 6,   // 01000000
    DUER_SIGNAL_STREAM_CONNECT_FAILURE  = 1 << 7,   // 10000000
    DUER_SIGNAL_STREAM_NO_DATA          = 1 << 8,

    DUER_SIGNAL_HOTWORD_TOO_LONG        = 1 << 9,
    DUER_SIGNAL_HOTWORD_TOO_SHORT       = 1 << 10,

    DUER_SIGNAL_BUFFER_OVERFLOW         = 1 << 11,

    DUER_SIGNAL_SNOWBOY_FAILURE = DUER_SIGNAL_FAIL | DUER_SIGNAL_STREAM_INIT_FAILURE | DUER_SIGNAL_STREAM_CONNECT_FAILURE,
} DuerSnowboySignal;

typedef struct DuerString {
    int32 size;
    int32 actual_size;
    char* data;
} DuerString;

typedef struct DuerFrameInfo {
    uint32 frame_id;
    DuerFrameProperty property;
} DuerFrameInfo;

typedef struct DuerMatrix {
    int32  actual_rows;
    int32  num_rows;
    int32  num_cols;
    int32  stride;
    float  max;
    float  min;
#ifdef APPLY_FIXED_POINT
    int32  range_scale;
    int32  range_min_rounded;
#else
    float  range_scale;
    float  range_min_rounded;
#endif
    int32  number_format;
    int32  elem_size;
    DuerDataType elem_type;
    void*  data;
} DuerMatrix;

typedef struct DuerVector {
    int32 size;
    int32 actual_size;
    float  max;
    float  min;
    float  range_scale;
    float  range_min_rounded;
    int32  number_format;
    int32 elem_size;
    DuerDataType elem_type;
    void* data;
} DuerVector;

typedef struct DuerVector2 {
    int32 size;
    int32 actual_size;
    float  max;
    float  min;
    float  range_scale;
    float  range_min_rounded;
    int32  number_format;
    int32 elem_size;
    DuerDataType elem_type;
    DuerVector* data;
} DuerVector2;

typedef struct DuerVector3 {
    int32 size;
    int32 actual_size;
    float  max;
    float  min;
    float  range_scale;
    float  range_min_rounded;
    int32  number_format;
    int32 elem_size;
    DuerDataType elem_type;
    DuerVector2* data;
} DuerVector3;

typedef struct DuerVector4 {
    int32 size;
    int32 actual_size;
    float  max;
    float  min;
    float  range_scale;
    float  range_min_rounded;
    int32  number_format;
    int32 elem_size;
    DuerDataType elem_type;
    DuerVector3* data;
} DuerVector4;

typedef struct DuerVectorString {
    int32        size;
    int32        actual_size;
    DuerString*  data;
} DuerVectorString;

typedef struct DuerVectorFrameInfo {
    int32          size;
    int32          actual_size;
    DuerFrameInfo* data;
} DuerVectorFrameInfo;

#ifdef __cplusplus
}
#endif

#endif

