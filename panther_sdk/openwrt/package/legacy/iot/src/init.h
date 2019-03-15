#ifndef __INIT_H__
#define __INIT_H__
typedef struct GLOBALPRARM
{
	int m_iVadThreshold;
	char vadFlag;
	char m_CaptureFlag; 	  // 0、空闲 1、开始录音 2、正在录音 3、录音结束
	char identify[64];
}GLOBALPRARM_STRU;

extern void init(int argc , char **argv);

extern void deinit(void);

extern GLOBALPRARM_STRU g;

#endif