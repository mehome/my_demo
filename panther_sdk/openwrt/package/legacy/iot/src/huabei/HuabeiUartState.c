#include "HuabeiUartData.h"


#include "HuabeiUartKey.h"

#include "HuabeiUartProc.h"

static int sendFlag = 1;//Heartbeat Packet;

void SetSendFlag(int flag)
{
	sendFlag = flag;
}
int GetSendFlag()
{
	return sendFlag;
}

int StateProc(void *arg)
{
	HuabeiIotMsg *msg = NULL;
	int operate;
	msg = (HuabeiIotMsg *)arg;
	operate = msg->operate;

}

static HuabeiProc stateProcTable[] = {
		{0x7F, StateProc},
		{0x80, StateProc},
		{0x81, StateProc},
		{0x82, StateProc},
		{0x83, StateProc},
		{0x84, StateProc},
		{0x85, StateProc},
		{0x86, StateProc},
};


int StateProcExec(void *arg)
{
	//int table_len = ARRAY_SIZE(proc_table);
	int ret = -1;
	int i = 0;
	HuabeiIotMsg *packet = (HuabeiIotMsg *)arg;
	int operate ;

	operate = packet->operate;

#if 0	
	int tbl_len = sizeof(stateProcTable) / sizeof(stateProcTable[0]);

	for(i = 0; i < tbl_len; i++) 
	{
		if( stateProcTable[i].operate == res)
		{
			if(stateProcTable[i].exec != NULL) {
				ret = stateProcTable[i].exec(arg);
			}
			break;
			
		}
	}
#else
	info("state:0x%x",operate);
	if(operate == 0x80 && sendFlag  == 0)
		return 0;	
	ret = HuabeiEventNotifyStatus(operate);
#endif
	return ret;
}






