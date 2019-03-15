#ifndef __MPD_CONTROL_H__
#define __MPD_CONTROL_H__

#include "DeviceStatus.h"


extern void ResumeMpdState();
extern void     SaveMpdState();


extern void MpdResume(AudioStatus *status);
extern AudioStatus *     MpdSuspend();
extern void SaveMpdStateBeforeShutDown();
extern void ResumeMpdStateStartingUp();
#ifdef ENABLE_HUABEI
extern int HuabeiPlayIndex(char * index);
extern int HuabeiPlayPrevNext(int arg);
#endif





#endif

