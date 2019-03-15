#include "HuabeiUartKey.h"


#include "HuabeiUartProc.h"
#include "HuabeiPlaylist.h"

#include "huabei.h"


static int KeyPrevProc(void *arg)
{
	info("....");
	return HuabeiPrevNext(PLAYLIST_MODE_PREV);
}
static int KeyNextProc(void *arg) 
{
	return HuabeiPrevNext(PLAYLIST_MODE_NEXT);
}
static int KeyConfirmProc(void *arg)
{
	info("....");
	SetHuabeiDataRes(-1);
	SetHuabeiSendStop(0);
	cdb_set_int("$turing_mpd_play", 0);
	return HuabeiConfirm();
}
static int KeyPauseProc(void *arg)
{
	info("....");
	return 0;
}
static int KeyResumeProc(void *arg)
{
	info("....");

	return 0;
}

static int KeyExitProc(void *arg)
{
	info("....");	

}
static int KeyBootProc(void *arg)
{

	info("....");
	return 0;

}
static int KeyShutProc(void *arg)
{
	info("....");
	return 0;
}

static int KeyVolUpProc(void *arg)
{
	info("....");
	return 0;
}


static int KeyVolDownProc(void *arg)
{
	info("....");
	return 0;
}
static int KeyAiStartProc(void *arg)
{
	info("....");
	return TuringSpeechStart(0);

}

static int KeyAiEndProc(void *arg)
{
	info("....");
	return TuringCaptureEnd();
}
static int KeyWechatStartProc(void *arg)
{
	return TuringChatStart();
}


static int KeyWechatEndProc(void *arg)
{
	info("....");

	return TuringCaptureEnd();
}
static int KeyWechatPlayProc(void *arg)
{
	info("....");
	

	return 0;
}

static int KeyWpsProc(void *arg)
{
	info("....");
	return 0;

}
static int KeyUnbindProc(void *arg)
{
	info("....");
	int ret;

	return 0;
}

static HuabeiProc keyProcTable[] = {
	{0x01, KeyPrevProc},
	{0x02, KeyNextProc},
	{0x03, KeyConfirmProc},
	{0x04, KeyPauseProc},
	{0x05, KeyResumeProc},
	{0x06, KeyExitProc},
	{0x07, KeyBootProc},
	{0x08, KeyShutProc},
	{0x09, KeyVolUpProc},
	{0x0A, KeyVolDownProc},
	{0x0B, KeyAiStartProc},
	{0x0C, KeyAiEndProc},
	{0x0D, KeyWpsProc},
	{0x0E, KeyUnbindProc},
	{0x0F, KeyWechatPlayProc},
	{0x10, KeyWechatStartProc},
	{0x11, KeyWechatEndProc},
};


int KeyProcExec(void *arg)
{
	//int table_len = ARRAY_SIZE(proc_table);
	int ret = -1;
	int i = 0;
	int type;
	HuabeiIotMsg *packet = (HuabeiIotMsg *)arg;
	int operate ;

	operate = packet->operate;

		
	int tbl_len = sizeof(keyProcTable) / sizeof(keyProcTable[0]);

	for(i = 0; i < tbl_len; i++) 
	{
		if( keyProcTable[i].operate == operate)
		{
			if(keyProcTable[i].exec != NULL) {
				ret = keyProcTable[i].exec(arg);
				
			}
			break;
			
		}
	}

	return ret;
}



