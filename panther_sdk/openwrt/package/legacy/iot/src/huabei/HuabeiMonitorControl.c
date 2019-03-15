#include "HuabeiMonitorControl.h"
#include <pthread.h>
#include "HuabeiUart.h"
#include "mpdcli.h"
#include "debug.h"
#include "huabei.h"
#include "HuabeiStruct.h"

static char huabeiDir[512] = {0};

static int huabeiSendStop = 0;
static pthread_mutex_t huabeiSendStopMtx = PTHREAD_MUTEX_INITIALIZER;  

void SetHuabeiSendStop(int state)
{	
	pthread_mutex_lock(&huabeiSendStopMtx);  
	huabeiSendStop = state;
	pthread_mutex_unlock(&huabeiSendStopMtx);  
}

int GetHuabeiSendStop()
{	
	int ret ;
	pthread_mutex_lock(&huabeiSendStopMtx);  
	ret  = huabeiSendStop ;
	pthread_mutex_unlock(&huabeiSendStopMtx);  
	return ret;
}

int HuabeiPlayEvent()
{	
	int state, operate;
	HuabeiUartMsg msg;
	int playmode;
	
	state = MpdGetPlayState();

	warning("state:%d",state);
	
	if(state == MPDS_PAUSE) 
	{
		cdb_set_int("$turing_stop_time",1);
		operate = HUABEI_COMMAND_PAUSE;
	} 
	else if(state == MPDS_PLAY)
	{
		cdb_set_int("$turing_stop_time",0);
		operate = HUABEI_COMMAND_PLAY;
	}
	
	 return HuabeiUartSendCommand(operate);

}



int HuabeiPlayStopEvent()
{
	return HuabeiUartSendCommand(HUABEI_COMMAND_EXIT);
}
int HuabeiPlayDoneEvent()
{

	return HuabeiUartSendPlay(HUABEI_PLAY_DONE);
}

int HuabeiPlayPlayEvent()
{

	return HuabeiUartSendPlay(HUABEI_PLAY_PLAY);
}


int HuabeiShutDownEvent()
{
	return HuabeiUartSendCommand(HUABEI_COMMAND_POWER_OFF);
}

int HuabeiGetStateEvent()
{
	return HuabeiUartSendCommand(HUABEI_COMMAND_GET_STATE);
}

int HuabeiConnectServerEvent()
{
	return HuabeiUartSendCommand(HUABEI_COMMAND_CONNECT_SERVER);
}


int HuabeiSendDataEvent(int operate)
{
	char *dirName = NULL;
	dirName = GetHuabeiDataDirName();
	if(dirName == NULL) {
		return -1;
	}
	return HuabeiUartSendData(dirName, operate);

}

int HuabeiGetSendDataEvent()
{
	char *url = NULL;
	char *dirName = NULL;
	int ret, operate; 
	url = GetHuabeiDataUrl();
	if(url == NULL)
		return -1;
	dirName = GetHuabeiDataDirName();
	if(dirName == NULL) {
		return -1;
	}
	ret = HuabeiDownLoadFile(url, dirName, &operate);
	info("ret:%d",ret);
	if(ret != 0)
		return ret;
	return HuabeiUartSendData(dirName, operate);


}


int HuabeiSelectSubject(char *cmd)
{
	int operate, len;
	HuabeiSendMsg *msg;
	char buf[512] = {0};
	sscanf(cmd, "HuabeiSelectSubject-%s", buf);
	info("buf:%s",buf);
	info("len:%d",strlen(buf));
	len = strlen(buf);
	if(len <= 0)
		return -1;
	return HuabeiEventSelectSubject(buf);
}

int HuabeiConfirm()
{
	int tfStatus =0, ret;
	tfStatus = cdb_get_int("$tf_status", 0);
	char *dirName = GetHuabeiDataDirName();
	if(dirName == NULL)
		return -1;
	info("dirName:%s",dirName);
	if(tfStatus == 1) {
		ret = HuabeiSendDataEvent(1);
	}
	else 
	{
		int res = GetHuabeiDataRes();
		info("res:%d",res);
		if(res == 1) {
			HuabeiGetSendDataEvent();
			SetHuabeiDataRes(-1);
		} else if (res == -1) {
			HuabeiGetSendDataEvent();
		}		
		//ret = HuabeiEventSelectSubject(dirName);
		//ret =  HuabeiGetSendDataEvent();
	}

	return ret;
}

int HuabeiPlayLocalMc()
{
#if 1
	char *name = GetHuabeiDataDirName();
	char *dir  = strdup(name);
	HuabeiSetLocalUrl(dir);
	return HuabeiPlayHttp("mc");
#else
	 HuabeiCreatPlaylist(name);
	 return HuabeiPlaylistPlayMc();
#endif
}

int HuabeiPlayMc(char *name)
{ 
	int tfStatus = 0 , ret;
	tfStatus = cdb_get_int("$tf_status", 0);
	info("dirName:%s",name);
	SetHuabeiDataDirName(name);
	if(tfStatus == 1) {
		ret =  HuabeiPlayLocalMc();	
	} else {
		ret = HuabeiEventSelectSubject(name);
	}
	return ret;
}


int HuabeiPlayLocal(char *cmd)
{
	int operate, len;
	char buf[512] = {0};
	char *speechNoTf = "请先插入tf卡";
	int tfStatus =0, ret;
	tfStatus = cdb_get_int("$tf_status", 0);
	
	warning("tf_status=%d", tfStatus);
	sscanf(cmd, "HuabeiLocal-%s", buf);
	info("buf:%s",buf);
	info("len:%d",strlen(buf));
	len = strlen(buf);
	if(len <= 0)
		return -1;
	buf[len] = 0;
	if(strcmp(buf, "OK") == 0) {
		ret = HuabeiConfirm();
	} else {
		SetHuabeiDataDirName(buf);
		ret =HuabeiPlayMc(buf);
	}


	return ret;
}

int HuabeiDataRes0(char *cmd)
{
	int operate, len;
	char buf[128] = {0};
	int  ret;
	sscanf(cmd, "HuabeiDataRes0-%s", buf);
	info("buf:%s",buf);
	info("len:%d",strlen(buf));
	len = strlen(buf);
	if(len <= 0)
		return -1;
	buf[len] = 0;
	SetHuabeiDataDirName(buf);
	SetHuabeiDataRes(0);
	return HuabeiEventSelectSubject(buf);
}
int HuabeiDataRes1(char *cmd)
{
	int operate, len;
	char buf[128] = {0};
	int ret;
	
	sscanf(cmd, "HuabeiDataRes1-%s", buf);
	info("buf:%s",buf);
	info("len:%d",strlen(buf));
	len = strlen(buf);
	if(len <= 0)
		return -1;
	buf[len] = 0;
	SetHuabeiDataRes(1);	
	if(strcmp(buf, "OK") == 0) {
		ret = HuabeiConfirm();
	} else {
		ret =HuabeiPlayMc(buf);
	}
	return ret;
}
int HuabeiDataRes2(char *cmd)
{
	int operate, len;
	char buf[128] = {0};
	int tfStatus =0, ret;
	tfStatus = cdb_get_int("$tf_status", 0);
	
	sscanf(cmd, "HuabeiDataRes2-%s", buf);
	info("buf:%s",buf);
	info("len:%d",strlen(buf));
	len = strlen(buf);
	if(len <= 0)
		return -1;
	buf[len] = 0;
	SetHuabeiDataDirName(buf);
	SetHuabeiDataRes(2);
	if(tfStatus == 1) {
		ret = HuabeiSendDataEvent(1);
	} else {
		ret = HuabeiEventSelectSubject(buf);
	}
	
	return ret;
}

int HuabeiPlay(char *cmd)
{
	int len, index;
	char buf[4] = {0};
	sscanf(cmd, "HuabeiPlay-%s", buf);

	int tfStatus =0, ret;
	tfStatus = cdb_get_int("$tf_status", 0);
	len = strlen(buf);
	if(len <= 0)
		return -1;
	buf[len] = 0;	
	info("buf:%s",buf);
#if 0
	index = atoi(buf);
	info("index:%d",index);
	if(index > 99)
		return -1;
#endif
	SetHuabeiSendStop(1);
	cdb_set_int("$turing_mpd_play", 3);
	if(tfStatus == 1) {
		HuabeiPlayIndex(buf);
	} else {
		HuabeiPlayHttp(buf);
	}
	
}

int HuabeiPrevNext(int arg)
{
	int tfStatus =0, ret;
	tfStatus = cdb_get_int("$tf_status", 0);
	info("tfStatus:%d",tfStatus);
	if(tfStatus == 1) {
		//ret = HuabeiLocalPrevNext(arg);
		ret = HuabeiPlaylistPrevNext(arg);
	} else {
		ret = HuabeiHttpPrevNext(arg);
	}
	return ret;
}

int HuabeiRequestData(char* mediaid)
{
	HuabeiSendMsg *msg;
	msg = newHuabeiSendMsg();
	if(msg) {
		msg->type = HUABEI_MESSAGE_SEND_SELECT_SUBJECT;
		msg->mediaid = mediaid;
		HuabeiEventSendMessage(msg);
		freeHuabeiSendMsg(&msg);
	}
	return 0;
}

int HuabeiStatusReport(char *cmd)
{
	int operate;
	sscanf(cmd, "HuabeiStatusReport_%02x", &operate);
	info("operate:0x%02x",operate);
	return HuabeiEventNotifyStatus(operate);
}

int HuabeiTest(char *cmd)
{
	int operate;
	HuabeiUartMsg msg;
	sscanf(cmd, "HuabeiTest_%02x", &operate);
	info("operate:0x%02x",operate);
	if(operate < 6) {
		memset(&msg, 0, sizeof(HuabeiUartMsg));

		msg.type = HUABEI_TYPE_OPERATE_COMMAND;
		msg.len = 0;
		msg.operate = operate;
	} else {
		memset(&msg, 0, sizeof(HuabeiUartMsg));
		char *file = "uartd";
		memcpy(&msg.data, file, strlen(file));
		msg.type = HUABEI_TYPE_DATA;
		msg.len = strlen(file);
		msg.operate = 1;
	}
	info("msg.type:0x%02x",msg.type);
	return NotifyToHuabei(&msg);
}

