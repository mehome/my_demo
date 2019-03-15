#include "HuabeiUartProc.h"

#include <assert.h>

#include "HuabeiUartKey.h"

#include "HuabeiUartData.h"
#include "HuabeiUartPlay.h"
#include "HuabeiUartState.h"

HuabeiIotMsg *dupHuabeiIotMsg(void *arg)
{
	HuabeiIotMsg *dup = NULL;
	HuabeiIotMsg *msg = (HuabeiIotMsg *)arg;
	assert(msg != NULL);

	dup = calloc(1, sizeof(HuabeiIotMsg));
	if(dup) {
		memcpy(dup, msg, sizeof(HuabeiIotMsg));
	}
	return dup;
}



static HuabeiProc huabeiTypeTable[] = {
	{0x43, NULL}, 				   //'C'
	{0x4F, NULL},                  //'O'
	{0x4B, KeyProcExec},   //'K'
	{0x50, PlayProcExec},  //'P'
	{0x52, NULL},  //'R'
	{0x53, StateProcExec}, //'S'
	{0x44, DataProcExec}, //'D'
	{0x4c, DataProcExec}, //'L'
};



int HuabeiProcExec(void *arg)
{
	
	int table_len = ARRAY_SIZE(huabeiTypeTable);
	int ret = -1;
	int i = 0;
	HuabeiIotMsg *msg = (HuabeiIotMsg *)arg;
	debug("table_len:%d",table_len);
	debug("opearte->type:0x%02x",msg->type);
	for(i = 0; i < table_len; i++) 
	{
		if(huabeiTypeTable[i].operate == msg->type) 
		{
			debug("i:%d",i);
			if(huabeiTypeTable[i].exec != NULL) {
				ret = huabeiTypeTable[i].exec(msg);
			}
				
			break;
		}
	}
	return ret;
}

#ifdef USE_IOT_MSG
int HuabeiMonitorControl(char *cmd) 
{
	int ret;
	char buf[128];	
	HuabeiIotMsg *msg = NULL;
	memset(buf, 0, 128);
	msg = (HuabeiIotMsg *)cmd;
	info("msg->type:0x%02x" ,msg->type);
	info("msg->operate:0x%02x" ,msg->operate);
	info("msg->res0:0x%02x" ,msg->res0);
	info("msg->len:0x%02x" ,msg->len);
	info("msg->arg0:0x%02x" ,msg->arg0);
	info("msg->arg1:0x%02x" ,msg->arg1);
	if(msg->len > 0) {
		memcpy(buf, msg->data, msg->len);
		buf[msg->len] = '\0';
		info("buf:%s" ,buf);
	}
	return HuabeiProcExec(msg);

}
#endif


