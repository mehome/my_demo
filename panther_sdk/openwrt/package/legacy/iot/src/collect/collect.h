#ifndef __COLLECT_H__
#define __COLLECT_H__


#include "common.h"
#include "myutils/debug.h"


#define COLLECT_DIR		      "收藏"

enum {
	COLLECT_NONE= 0,
	COLLECT_STARTED,
	COLLECT_ONGING,
	COLLECT_DONE,
	COLLECT_CLOSE,
	COLLECT_CANCLE,
};


int IsCollectCancled();
void SetCollectState(int state);
int IsCollectFinshed();
int IsCollectDone();
void   CreateCollectthread(void);
int   DestoryCollectPthread(void);


#endif

