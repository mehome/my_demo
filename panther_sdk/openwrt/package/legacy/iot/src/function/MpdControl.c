#include "MpdControl.h"
#include <pthread.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>

#include "common.h"
#include "mpdcli.h"
#include "uciConfig.h"

#include "myutils/debug.h"

static pthread_mutex_t mpdAudioStatusMtx = PTHREAD_MUTEX_INITIALIZER;

static AudioStatus *mpdAudioStatus = NULL;

AudioStatus *     MpdSuspend()
{
	AudioStatus *mpdAudioStatus = NULL;

	mpdAudioStatus = newAudioStatus();
	if(!mpdAudioStatus)
		return NULL;
	MpdGetAudioStatus(mpdAudioStatus);
	return mpdAudioStatus;
}

void MpdResume(AudioStatus *status)
{ 
	if(status)
	{
		warning("audioStatus->play:%u, audioStatus->duration:%u, audioStatus->progress:%d", 
				status->play,status->duration ,status->progress);
		error("state:%d url:%s",status->state, status->url);
		if(status->state == 2) {
			debug("resume play..");
			PlaylistPlaySeek(status->url, status->progress);

		} 
		else if(status->state == 3)
		{
			debug("resume pause..");
			PlaylistPauseSeek(status->url, status->progress);

		}
		freeAudioStatus(&status);
	}
}

/* 保存当前MPD状态. */
void     SaveMpdState()
{
#ifdef MPG123

    int state = MpdGetPlayState();
    if(state == MPDS_PLAY) 
    {
        exec_cmd("mpc pause");
    }
    

#else
	int mode;
#ifdef TURN_ON_SLEEP_MODE
	if(GetAiMode() == 1)
		return ;
#endif
	
	mode = cdb_get_int("$turing_mpd_play",0);
    info("turing_mpd_play = %d",mode);
	if(mode == 0 || mode == 4) 
	{
		int state = 0;
		state = MpdGetPlayState();
		error("state:%d",state);
		if(state == MPDS_PLAY || state == MPDS_PAUSE) {
			pthread_mutex_lock(&mpdAudioStatusMtx); 
			if(mpdAudioStatus) {
				freeAudioStatus(&mpdAudioStatus);
				mpdAudioStatus = NULL;
			}
			mpdAudioStatus = newAudioStatus();
			if(mpdAudioStatus) {
				MpdGetAudioStatus(mpdAudioStatus);
				error("save mpdAudioStatus->url:%s",mpdAudioStatus->url);
			}
			pthread_mutex_unlock(&mpdAudioStatusMtx);  
		}
		SaveColumnAudio();
	}
	else 
	{
		mode = cdb_set_int("$turing_mpd_play",0);
	}
	info("exit");
#endif    
}







/* 恢复上次保存的MPD状态. */
void ResumeMpdState()
{
#ifdef MPG123
    exec_cmd("mpc play");
#else

	int turingPowerState ;
	
	if ( GetResumeMpd() == 0 ) {
		error("GetResumeMpd() == 0");
		return;
	}
	turingPowerState = cdb_get_int("$turing_power_state",0);
	if (turingPowerState != 0) 
		return ;
	
	int mode = cdb_set_int("$turing_mpd_play",0);
	pthread_mutex_lock(&mpdAudioStatusMtx);  
	if(mpdAudioStatus)
	{
		
		error(" resume state:%d url:%s",mpdAudioStatus->state, mpdAudioStatus->url);
		if(mpdAudioStatus->state == 2) {
			debug("resume play..");
			info("backup:url=%s,progress = %d,duration = %d",mpdAudioStatus->url, mpdAudioStatus->progress,mpdAudioStatus->duration);
			PlaylistPlaySeek(mpdAudioStatus->url, mpdAudioStatus->progress);
			if(mpdAudioStatus->single) {
				exec_cmd("mpc single on");
			} else {
				exec_cmd("mpc single off");
			}
			if(mpdAudioStatus->repeat) {
				exec_cmd("mpc repeat on");
			} else {
				exec_cmd("mpc repeat off");
			}		

		} 
		else if(mpdAudioStatus->state == 3)
		{
			debug("resume pause..");

			PlaylistPauseSeek(mpdAudioStatus->url, mpdAudioStatus->progress);
			if(mpdAudioStatus->single) {
				exec_cmd("mpc single on");
			} else {
				exec_cmd("mpc single off");
			}
			if(mpdAudioStatus->repeat) {
				exec_cmd("mpc repeat on");
			} else {
				exec_cmd("mpc repeat off");
			}
		}
		else 
		{
			cdb_set_int("$turing_stop_time", 1);
		}
		freeAudioStatus(&mpdAudioStatus);
	}
	else 
	{
		error("mpdAudioStatus == NULL");
	}
	pthread_mutex_unlock(&mpdAudioStatusMtx);  
#endif
}


int GetResumeMpdState()
{	int ret = 0;
	pthread_mutex_lock(&mpdAudioStatusMtx);  
	if(mpdAudioStatus && mpdAudioStatus->url)
		ret = 1;
	pthread_mutex_unlock(&mpdAudioStatusMtx);  
	return ret;
}
void FreeMpdState()
{
	pthread_mutex_lock(&mpdAudioStatusMtx);  
	if(mpdAudioStatus)
	{
		freeAudioStatus(&mpdAudioStatus);
	}
	pthread_mutex_unlock(&mpdAudioStatusMtx);  
}

void SaveMpdStateBeforeShutDown()
{
	AudioStatus *audioStatus;
	audioStatus = newAudioStatus();
	char buf[64] = {0};
	if(audioStatus) {
		char linkFile[512] = {0};
		MpdGetAudioStatus(audioStatus);
		if(audioStatus->state == MPDS_PLAY) 
		{
			if(audioStatus->url)
			{
				WIFIAudio_UciConfig_SearchAndSetValueString("xzxwifiaudio.config.current_contentURL", audioStatus->url);


				bzero(buf, 64);	
				snprintf(buf, 64, "%d",  audioStatus->progress);
				WIFIAudio_UciConfig_SearchAndSetValueString("xzxwifiaudio.config.current_progress", buf);
				bzero(buf, 64);	
				snprintf(buf, 64, "%d",  audioStatus->state);
				WIFIAudio_UciConfig_SearchAndSetValueString("xzxwifiaudio.config.current_state", buf);
			}
			if(audioStatus->title)
			{
				WIFIAudio_UciConfig_SearchAndSetValueString("xzxwifiaudio.config.current_title",  audioStatus->title);
			}
			if(readlink(CURRENT_PLAYLIST_JSON, linkFile ,512) > 0)
			{
				WIFIAudio_UciConfig_SearchAndSetValueString("xzxwifiaudio.config.current_playlist",  linkFile);
			}
		}
		else 
		{
			bzero(buf, 64);	
			snprintf(buf, 64, "%d",  audioStatus->state);
			WIFIAudio_UciConfig_SearchAndSetValueString("xzxwifiaudio.config.current_contentURL","0");
			WIFIAudio_UciConfig_SearchAndSetValueString("xzxwifiaudio.config.current_title",  " ");
			WIFIAudio_UciConfig_SearchAndSetValueString("xzxwifiaudio.config.current_state", buf);
			WIFIAudio_UciConfig_SearchAndSetValueString("xzxwifiaudio.config.current_playlist",  " ");
		}

	}

	freeAudioStatus(&audioStatus);
	MpdClear();
	sleep(2);
	eval("uartdfifo.sh", "tunoff");
}


char* JsonFileToM3u(char *jsonfile)
{	
	char *file = NULL;
	char *suffix = NULL;
	suffix = strrchr(jsonfile, '.');
	if(suffix == NULL)
		return NULL;
	error("suffix:%s",suffix);
	int length = suffix - jsonfile;
	file = calloc(length + 4, 1);
	if(file == NULL)
		return NULL;
	memcpy(file, jsonfile, length);
	error("file:%s",file);
	strcat(file, ".m3u");
	return file;
	
}
void ResumeMpdStateStartingUp()
{
	int state = 0;
	int progress = 0; 
	char *url; 
	char *pProgress;
	char *pState;
	char *title;
	char *pPlaylistJson;
	int hasPlaylist = 0;
	pPlaylistJson = WIFIAudio_UciConfig_SearchValueString("xzxwifiaudio.config.current_playlist");
	if(pPlaylistJson) {
		hasPlaylist = 1;
		error("pPlaylistJson:%s", pPlaylistJson);
	}
	pState = WIFIAudio_UciConfig_SearchValueString("xzxwifiaudio.config.current_state");
	if(pState) {
		error("pState:%s", pState);
		state = atoi(pState);
		WIFIAudio_UciConfig_FreeValueString(&pState);
	}
	pProgress = WIFIAudio_UciConfig_SearchValueString("xzxwifiaudio.config.current_progress");
	if(pProgress) {
		error("pProgress:%s", pProgress);
		progress = atoi(pProgress);
		WIFIAudio_UciConfig_FreeValueString(&pProgress);
	}
	url = WIFIAudio_UciConfig_SearchValueString("xzxwifiaudio.config.current_contentURL");
	if(state == MPDS_PLAY && url != NULL && strcmp(url, "0") ) 
	{
	
		error("url:%s", url);
		title =  WIFIAudio_UciConfig_SearchValueString("xzxwifiaudio.config.current_title");
		if(MpdCliPlayLocalOrUri() == 1){
			exec_cmd("mpc update");
		}
		if(pPlaylistJson) {
			if(access(pPlaylistJson ,F_OK) == 0) {
				unlink(CURRENT_PLAYLIST_JSON);
				unlink(CURRENT_PLAYLIST_M3U);
				char *file = JsonFileToM3u(pPlaylistJson);
				if(file) {
					error("file:%s",file);
					symlink(file, CURRENT_PLAYLIST_M3U);
					free(file);
				}
				symlink(pPlaylistJson, CURRENT_PLAYLIST_JSON);
				PlaylistPlaySeek(url, progress);
			}
			WIFIAudio_UciConfig_FreeValueString(&pPlaylistJson);
		} else {
			
			MpdClear();
			MpdAdd(url);
			MpdRunSeek(0, progress);
		}
		
		if(title)
			WIFIAudio_UciConfig_FreeValueString(&title);
		WIFIAudio_UciConfig_FreeValueString(&url);
	}
	
}
#ifdef ENABLE_HUABEI
static int huabeiIndex = 0;
static int maxIndex = 10;

int HuabeiLocalPrevNext(int arg)
{	char buf[4] = {0};
	if(arg == 1) {
		huabeiIndex = huabeiIndex-1;
	} else if(arg == 2) {
		huabeiIndex = huabeiIndex+1;
	}
	if(huabeiIndex > maxIndex)
		huabeiIndex = 0;
	if(huabeiIndex < 0)
		huabeiIndex = 0;
	snprintf(buf,4, "%02d", huabeiIndex);
	return HuabeiPlayIndex(buf);
}

int HuabeiPlayIndex(char        *index)
{
	FILE *dp =NULL;
	int ret = -1;
	size_t len = 0;
	int num = 0;
	ssize_t read;  
	char line[1024];
	char cmd[1024];
	char data[16] = {0};

	snprintf(data, 16, "_%s.", index);
	info("data:%s",data);
	dp= fopen(CURRENT_PLAYLIST_M3U, "r");
	if(dp == NULL)
		return -1;
	///while ((read = getline(&line, &len, dp)) != -1)  
	while(!feof(dp))
    {  
   	   	memset(line, 0 , 1024);
      	char *tmp = NULL;
		tmp = fgets(line, sizeof(line), dp);
		if(tmp == NULL)
			break;
		int len =  strlen(line) -1;
		char ch = line[len];
		if(ch  == '\n' || ch  == '\r')
			line[strlen(line)-1] = 0;
	    line[strlen(line)] = 0;
		num++;
		maxIndex = num;
		info("num:%d",num);
		info("line:%s",line);
		//if(num == index)
		if(strstr(line, data))
		{	info("line:%s",line);
			exec_cmd("mpc clear");
			exec_cmd("mpc single on");
			exec_cmd("mpc repeat off");
			snprintf(cmd, 1024, "mpc add %s && mpc play" , line);
			exec_cmd(cmd);
			//break;
		}

	
    }  
	ret  = num;
	if(dp) {
		fclose(dp);
	}

	return ret;
}

int HuabeiPlayHttp(char *index)
{
	char *url = NULL;
	char *dirName = NULL;
	char data[512] = {0};
	info("...");
	DEBUG_INFO("...");
	url = GetHuabeiDataUrl();
	if(url == NULL) 
		return -1;
	info("url:%s",url);
	DEBUG_INFO("url:%s",url);
	dirName = GetHuabeiDataDirName();
	if(dirName == NULL) {
		return -1;
	}
	info("dirName:%s",dirName);
	if(strcmp(index, "mc") == 0) {
		SetHuabeiSendStop(0);
		cdb_set_int("$turing_mpd_play", 0);
	} else {
		SetHuabeiSendStop(1);
		cdb_set_int("$turing_mpd_play", 3);
	}
	snprintf(data, 512 , "%s/%s_%s.mp3", url, dirName,  index);
	warning("data:%s", data);
//#ifdef USE_MPDCLI
#if 0
	MpdVolume(200);
	MpdClear();
	MpdSingle(1);
	MpdRepeat(0);
	MpdAdd(data);
	MpdPlay(0);
#else
	exec_cmd("mpc volume 200");
	exec_cmd("mpc clear");
	exec_cmd("mpc single on");
	exec_cmd("mpc repeat off");
	eval("mpc", "add", data);
	exec_cmd("mpc play");
#endif

}
#endif
