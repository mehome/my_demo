#ifndef __IOT_H__

#define __IOT_H__

#include "myutils/debug.h"
#include "common.h"

extern void IotGetDataReport();
extern void  IotTopicReport();
extern void IotGetDataReport();

extern void   IotAudioStatusReport();
extern void IotSynchronizeLocalMusicReport(int operate,char *names);
extern void IotSynchronizeLocalStoryReport(int operate,char *names);

extern void IotGetAudio(int type);
extern void IotCollectSong(int id);

extern int ProcessIotJson(char *json,  int type);
extern void IotInit(void);







#endif
