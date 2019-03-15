#ifndef __DEBUG_H__
#define __DEBUG_H__

#include "c_types.h"

#define DEBUG_ON         1
#define DEBUG_OFF	      0

extern UINT8 g_byDebugFlag;

#define DEBUG_PRINTF(fmt, args...) do { \
	if (DEBUG_ON == g_byDebugFlag)\
	{printf("[%s:%d]"#fmt"\r\n", __func__, __LINE__, ##args); }} while(0) 
	
#define DEBUG_ERROR(fmt, args...) do { printf("\033[1;31m""[%s:%d]"#fmt"\r\n""\033[0m", __func__, __LINE__, ##args); } while(0) 


#endif /* __DEBUG_H__ */
