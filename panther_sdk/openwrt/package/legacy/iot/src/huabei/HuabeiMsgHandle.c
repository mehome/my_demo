#include "HuabeiMsgHandle.h"

#include <stdio.h>
#include <string.h>

#include <json-c/json.h>

#include "HuabeiMsgVoiceIntercomHandle.h"
#include "HuabeiMsgMusicStoryHandle.h"
#include "HuabeiMsgDataFileHandle.h"
#include "HuabeiMsgOperationControlHandle.h"
#include "HuabeiMsgUserInterfaceControlHandle.h"
#include "HuabeiMsgStableDiseaseControlHandle.h"



#include "debug.h"

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
#endif


static HuabeiMsgHanlder handlers[] = {
	{0, VoiceIntercomHandler},
	{1, MusicStoryHanlder},
	{2, DataFileHanlder},
	{3, OperationControlHandler},
	{4, UserInterfaceControlHandler},
	{5, StableDiseaseControlHandler},
};

int HuabeiMsgHanlde(int type ,void *arg)
{
	
	int table_len = ARRAY_SIZE(handlers);
	int ret = -1;
	int i = 0;;
	info("type:%d, table_len:%d",type, table_len);
	for(i = 0; i < table_len; i++) 
	{
		info("handlers[%d].type:%d",i, handlers[i].type );
		if(handlers[i].type == type) 
		{
			
			if(handlers[i].handle != NULL) {
				ret = handlers[i].handle(arg);
				break;
			}	
		}
	}
	return ret;
}


