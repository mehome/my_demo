/*=============================================================================+
|                                                                              |
| Copyright 2017                                                               |
| Montage Inc. All right reserved.                                           |
|                                                                              |
+=============================================================================*/
/*! 
*   \file dragonite_common.c
*   \brief  driver common data structure.
*   \author Montage
*/
#ifndef __DRAGONITE_COMMON_H__
#define __DRAGONITE_COMMON_H__

#define MAX_STA_NUM     32
#define MAX_DS_NUM      16
#define MAX_BSSIDS                  4

#define BUFFER_HEADER_POOL_SIZE                 1000
#define BEACON_Q_BUFFER_HEADER_POOL_SIZE        8 //( MAX_BSSIDS * 8 )

#define PROTECT_GUARD_COUNT 256

#define RX_DESCRIPTOR_COUNT         256 //48
#define BEACON_TX_DESCRIPTOR_COUNT  MAX_BSSIDS

#define DEF_BUF_SIZE        2000

#define CACHED_ADDR(x)     ( ((u32) (x) & 0x1fffffffUL) | 0x80000000UL )
#define UNCACHED_ADDR(x)   ( ((u32) (x) & 0x1fffffffUL) | 0xA0000000UL )
#define PHYSICAL_ADDR(x)   ((u32) (x) & 0x1fffffffUL)
#define VIRTUAL_ADDR(x)    ((u32) (x) | 0x80000000L)
#define UNCACHED_VIRTUAL_ADDR(x)    ((u32) (x) | 0xA0000000L)

#define ASSERT(cond,message)   \
if(!(cond)) { \
    panic(message);\
}

#define CIPHER_TYPE_NONE    0
#define CIPHER_TYPE_WEP40   1
#define CIPHER_TYPE_WEP104  2
#define CIPHER_TYPE_TKIP    3
#define CIPHER_TYPE_CCMP    4
#define CIPHER_TYPE_SMS4	5

#define WEP_DEF_TXKEY_ID	0
#define CIPHYER_TYPE_INVALID    CIPHER_TYPE_NONE

#define WEP_40_KEY_LEN		5
#define WEP_104_KEY_LEN		13
#define TKIP_KEY_LEN		16
#define TKIP_MICKEY_LEN		8
#define CCMP_KEY_LEN		16

typedef struct{
	u8 da[6];
	u8 sa[6];
	u16 type;
	u8 data[0];
} __attribute__((__packed__)) ehdr;

#define IS_MCAST_MAC(_da)	((_da[0]==0x01) && (_da[1]==0x00) && (_da[2]==0x5e))
#endif // __DRAGONITE_COMMON_H__

