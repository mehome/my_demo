#ifndef __DEBUG_H__
#define __DEBUG_H__


#include <stdlib.h>
#include <stdio.h>
//#define USE_EN_US
#define ALERT_DEBUG  


#define true 1
#define false 0

#ifdef ALERT_DEBUG  
#define DEBUG_ERR(fmt, args...)    do { fprintf(stderr,"[%s:%d]"#fmt"\n", __func__, __LINE__, ##args); } while(0) 
#define DEBUG_INFO(fmt, args...)   do { printf("[%s:%d]"#fmt"\n", __func__, __LINE__, ##args); } while(0) 
#else
#define DEBUG_ERR(fmt, args...)   do {} while(0)  
#define DEBUG_INFO(fmt, args...)  do {} while(0)  
#endif

			

#endif