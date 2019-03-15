#include "HuabeiUartData.h"


#include "HuabeiUartKey.h"


#include "HuabeiUartProc.h"

static int DataRes0Proc(void *arg)
{
	info("....");
	HuabeiIotMsg *msg = (HuabeiIotMsg *)arg;
	char buf[128] = {0};
	int len ;
	len = msg->len;
	if(len <= 0)
		return -1;
	memcpy(buf, msg->data, len);
	buf[len] = 0;
	info("buf:%s", buf);
	SetHuabeiDataDirName(buf);
	SetHuabeiDataRes(0);
	return HuabeiEventSelectSubject(buf);
}
static int DataRes1Proc(void *arg) 
{
	info("....");
	HuabeiIotMsg *msg = (HuabeiIotMsg *)arg;
	char buf[128] = {0};
	int len ,ret ;
	len = msg->len;
	if(len <= 0)
		return -1;
	memcpy(buf, msg->data, len);
	buf[len] = 0;
	info("buf:%s", buf);
	SetHuabeiDataRes(1);	
	if(strcmp(buf, "OK") == 0) {
		ret = HuabeiConfirm();
	} else {
		ret =HuabeiPlayMc(buf);
	}
	return ret;
}
static int DataRes2Proc(void *arg)
{
	info("....");
	HuabeiIotMsg *msg = (HuabeiIotMsg *)arg;
	int tf =0, ret, len = 0;
	tf = cdb_get_int("$tf_status", 0);
	char buf[128] = {0};
	len = msg->len;
	if(len <= 0)
		return -1;
	memcpy(buf, msg->data, len);
	buf[len] = 0;
	info("buf:%s", buf);
	SetHuabeiDataDirName(buf);
	SetHuabeiDataRes(2);
	if(tf == 1) {
		ret = HuabeiSendDataEvent(1);
	} else {
		ret = HuabeiEventSelectSubject(buf);
	}
	
	return ret;
}


static HuabeiProc keyProcTable[] = {
	{0x00, DataRes0Proc},
	{0x01, DataRes1Proc},
	{0x02, DataRes2Proc},
};


int DataProcExec(void *arg)
{
	//int table_len = ARRAY_SIZE(proc_table);
	int ret = -1;
	int i = 0;
	HuabeiIotMsg *packet = (HuabeiIotMsg *)arg;
	int res ;

	res = packet->res0;

		
	int tbl_len = sizeof(keyProcTable) / sizeof(keyProcTable[0]);

	for(i = 0; i < tbl_len; i++) 
	{
		if( keyProcTable[i].operate == res)
		{
			if(keyProcTable[i].exec != NULL) {
				ret = keyProcTable[i].exec(arg);
				
			}
			break;
			
		}
	}

	return ret;
}






