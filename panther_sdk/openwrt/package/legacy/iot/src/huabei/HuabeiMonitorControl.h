#ifndef __HUABEI_MONITOR_CONTROL_H__

#define __HUABEI_MONITOR_CONTROL_H__

#include "common.h"

int HuabeiPlayEvent();
int HuabeiPlayStopEvent();
int HuabeiPlayPlayEvent();
int HuabeiShutDownEvent();
int HuabeiGetStateEvent();
int HuabeiConnectServerEvent();


#define HUABEI_PLAY_STOP "HuabeiPlayStop"
#endif