#include "HuabeiUartData.h"
#include "HuabeiUartKey.h"
#include "HuabeiUartProc.h"
#include "mpdcli.h"
#include "common.h"


int HuabeiPlayMusic(char *buf)
{	
	int ret, mpdState;
	int tfStatus =0;

	mpdState = MpdGetPlayState();
	fatal("mpdState:%d",mpdState);
	if(mpdState != MPDS_STOP) 
		return -1;
	return HuabeiPlayHttp(buf);
}
static int PlayServerMusicProc(void *arg)
{	
	unsigned char arg0;
	unsigned char arg1;
	int index ,ret, mpdState;
	int tfStatus =0;
	char buf[4] = {0};
	HuabeiIotMsg *msg = NULL;
	
	msg = (HuabeiIotMsg *)arg;
	arg0 = msg->arg0;
	arg1 = msg->arg1;
	warning("arg0:%d",arg0);
	warning("arg1:%d",arg1);
	index = arg0*10+arg1;
	warning("index:%d",index);

	
	snprintf(buf, 4, "%02d", index);
    info("buf:%s",buf);
	SetHuabeiSendStop(1);
	cdb_set_int("$turing_mpd_play", 3);
	HuabeiPlayHttp(buf);
	
	return ret;
}

int PlayBootMusicProc(void *arg)
{	
	unsigned char arg0;
	unsigned char arg1;
	int index ,ret;
	int tfStatus =0;
	char value[16]={0};
	char cmd[256] = {0};
	SetHuabeiSendStop(0);
	cdb_set_int("$turing_mpd_play", 0);
	tfStatus = cdb_get_int("$tf_status", 0);
	if(tfStatus == 1) 
	{
		char file[128] = {0};
		cdb_get_str("$tf_mount_point",value,16,"");
		snprintf(file , 128, "%s/poweron.mp3", value);
//#ifdef USE_MPDCLI
#if 0
		MpdVolume(200);
		ret = MpdClear();
		ret = MpdRepeat(0);
		ret = MpdSingle(1);
		ret = MpdAdd(file);
		ret = MpdPlay(-1);
#else
		exec_cmd("mpc volume 200");
		exec_cmd("mpc update");
		exec_cmd("mpc clear");
		exec_cmd("mpc single on");
		exec_cmd("mpc repeat off");
		eval("mpc", "add", file);
		exec_cmd("mpc play");
#endif

	} 
	else 
	{
		char *url= "http://api.drawbo.com/static/upload/poweron/poweron.mp3";
		int ret = 0;
//#ifdef USE_MPDCLI
#if 0
		ret = MpdClear();
		warning("MpdClear:%d",ret);
		ret = MpdSingle(1);
		ret = MpdRepeat(0);
		warning("MpdRepeat:%d",ret);
		ret = MpdAdd(url);
		warning("MpdAdd:%d",ret);
		ret = MpdPlay(-1);
		warning("MpdPlay:%d",ret);
#else
		exec_cmd("mpc clear");	
		exec_cmd("mpc single on");
		exec_cmd("mpc repeat off");
		eval("mpc", "add", url);
		exec_cmd("mpc play");
#endif
	}
		
	return ret;
}

static HuabeiProc playProcTable[] = {
	{0x01, PlayServerMusicProc},
	{0x03, NULL},
};



int PlayProcExec(void *arg)
{
	int ret = -1;
	int i = 0;
	HuabeiIotMsg *packet = (HuabeiIotMsg *)arg;
	int res ;

	res = packet->operate;

		
	int tbl_len = sizeof(playProcTable) / sizeof(playProcTable[0]);

	for(i = 0; i < tbl_len; i++) 
	{
		if( playProcTable[i].operate == res)
		{
			if(playProcTable[i].exec != NULL) {
				ret = playProcTable[i].exec(arg);
				
			}
			break;
			
		}
	}

	return ret;
}






