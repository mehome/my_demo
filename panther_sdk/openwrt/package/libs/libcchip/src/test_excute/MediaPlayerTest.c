#include <libcchip.h>
#include <stdio.h>
#if 0
typedef struct __data_proc_t {
    const char *name;
    int (*exec_proc)(char *cmd[], int iParam);
} data_proc_t;

static int OnProcessMediaAllStatus(char *cmd, int iParam) {
	PLAYER_STATUS *playerStatus  = MediaPlayerAllStatus();
	if (playerStatus != NULL)
	{
		printf("playerStatus->mainmode:%d\n",playerStatus->mainmode);	
		printf("playerStatus->nodetype:%d\n",playerStatus->nodetype);	
		printf("playerStatus->sw:%d\n",playerStatus->sw);		
		printf("playerStatus->status:%d\n", playerStatus->status);
		printf("playerStatus->curpos:%d\n", playerStatus->curpos);		
		printf("playerStatus->totlen:%d\n",playerStatus->totlen);		
		printf("playerStatus->Title:%s\n",playerStatus->Title); 	
		printf("playerStatus->Artist:%s\n",playerStatus->Artist);	
		printf("playerStatus->Album:%s\n",playerStatus->Album); 	
		printf("playerStatus->Year:%d\n",playerStatus->Year);		
		printf("playerStatus->Track:%d\n",playerStatus->Track); 	
		printf("playerStatus->Genre%s\n",playerStatus->Genre); 	
		printf("playerStatus->TFflag:%d\n",playerStatus->TFflag);		
		printf("playerStatus->plicount:%d\n",playerStatus->plicount);	
		printf("playerStatus->plicurr:%d\n",playerStatus->plicurr); 
		printf("playerStatus->vol:%d\n",playerStatus->vol); 	
		printf("playerStatus->mute:%d\n",playerStatus->mute);		
		printf("playerStatus->uri:%s\n",playerStatus->uri);		
		printf("playerStatus->cover%s\n",playerStatus->cover);	
		printf("playerStatus->sourcemode:%d\n",playerStatus->sourcemode);	
		printf("playerStatus->uuid:%s\n",playerStatus->uuid);	

		MediaPlayerAllStatusFreeSturct(playerStatus);
		printf("MediaPlayerAllStatusFreeSturct\n");
	}
}

static int OnProcessMediaPlay(char *cmd, int iParam) {
	MediaPlayerPlay();
}

static int OnProcessMediaPause(char *cmd, int iParam) {
	MediaPlayerPause();
}
	
static int OnProcessMediaStop(char *cmd, int iParam) {
	MediaPlayerStop();
}

static int OnProcessMediaNext(char *cmd, int iParam) {
	MediaPlayerNext();
}

static int OnProcessMediaPrevious(char *cmd, int iParam) {
	MediaPlayerPrevious();
}

static int OnProcessMediaSeek(char *cmd, int iParam) {
	MediaPlayerSeek(iParam);
}
	
static int OnProcessMediaSetVolume(char *cmd, int iParam) {
	MediaPlayerSetVolume(iParam);
}

static int OnProcessMediaGetVolume(char *cmd, int iParam) {
	printf("OnProcessMediaGetVolume:%d", MediaPlayerGetVolume());
}

static int OnProcessMediaSetMute(char *cmd, int iParam) {
	if (1 == iParam) {
		MediaPlayerSetMute(true);
	} else {
		MediaPlayerSetMute(false);
	}
}

static int OnProcessMediaMuteStatus(char *cmd, int iParam) {
	printf("OnProcessMediaMuteStatus:%d", MediaPlayerMuteStatus());
}
	
static int OnProcessMediaSetPlayMode(char *cmd, int iParam) {
	MediaPlayerSetPlayMode(iParam);
}

static int OnProcessMediaGetPlayMode(char *cmd, int iParam) {
	printf("OnProcessMediaGetPlayMode:%d", MediaPlayerGetPlayMode());
}

static int OnProcessMediaSetEqualizerMode(char *cmd, int iParam) {
	MediaPlayerSetEqualizerMode(iParam);
}

static int OnProcessMediaGetEqualizerMode(char *cmd, int iParam) {
	printf("OnProcessMediaGetEqualizerMode:%d", MediaPlayerGetEqualizerMode());

}
	
static int OnProcessMediaSetChannel(char *cmd, int iParam) {
	MediaPlayerSetChannel(iParam);
}

static int OnProcessMediaGetChannel(char *cmd, int iParam) {
	printf("OnProcessMediaGetChannel:%d", MediaPlayerGetChannel());
}


static  data_proc_t date_tbl[] = {
	{"AllStatus", OnProcessMediaAllStatus},
	{"Play", OnProcessMediaPlay},
	{"Pause", OnProcessMediaPause},	
	{"Stop", OnProcessMediaStop},
	{"Next", OnProcessMediaNext},
	{"Previous", OnProcessMediaPrevious},
	{"Seek", OnProcessMediaSeek},	
	{"SetVolume", OnProcessMediaSetVolume},
	{"GetVolume", OnProcessMediaGetVolume},
	{"SetMute", OnProcessMediaSetMute},
	{"MuteStatus", OnProcessMediaMuteStatus},	
	{"SetPlayMode", OnProcessMediaSetPlayMode},
	{"GetPlayMode", OnProcessMediaGetPlayMode},
	{"SetEqualizerMode", OnProcessMediaSetEqualizerMode},
	{"GetEqualizerMode", OnProcessMediaGetEqualizerMode},	
	{"SetChannel", OnProcessMediaSetChannel},
	{"GetChannel", OnProcessMediaGetChannel}
};

void OnProcessMediaControl(char *pCmd, int iParma) {
	int iRet = -1;
	int i;

	printf("command: %s, iParma:%d\r\n", pCmd, iParma);

	int iLength = sizeof(date_tbl) / sizeof(data_proc_t);
	for (i = 0; i < iLength; i++) {
		if (0 == strncmp(pCmd, date_tbl[i].name, strlen(date_tbl[i].name))) {
			if (date_tbl[i].exec_proc != NULL) {
				iRet = date_tbl[i].exec_proc(pCmd, iParma);
			}
			break;
		}
	}
}
#endif


