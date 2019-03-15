#ifndef __DEBUGPRINTF_H__
#define __DEBUGPRINTF_H__


//#define __DEBUGPRINTF__
//printf打印信息宏开关，不需要打印信息时，将上面整行屏蔽

#ifdef  __DEBUGPRINTF__
#define PRINTF printf
#else
#define PRINTF //
#endif

#endif

