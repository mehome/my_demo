#ifndef __DEBUGPRINTF_H__
#define __DEBUGPRINTF_H__


//#define __DEBUGPRINTF__
//printf��ӡ��Ϣ�꿪�أ�����Ҫ��ӡ��Ϣʱ����������������

#ifdef  __DEBUGPRINTF__
#define PRINTF printf
#else
#define PRINTF //
#endif

#endif

