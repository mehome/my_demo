#include "mpdcli.h"


#include <stdio.h>
#include <pthread.h>
#include <mpd/song.h>

#include <mpd/tag.h>
#include <mpd/error.h>
#include <mpd/status.h>
#include "../globalParam.h"

//#include "common.h"


pthread_mutex_t mpdCliMtx = PTHREAD_MUTEX_INITIALIZER;  
static pthread_cond_t captureCond = PTHREAD_COND_INITIALIZER; 


#define CURRENTLISTPATH "/playlists/CurList.m3u" 
#define CURRENTLIST "CurList" 



//typedef int BOOL;

#define RETRY_CMD(CLI, CMD) {                           \
	int i;												\
    for ( i = 0; i < 2; i++) {                       \
        if ((CMD))                                      \
            break;                                      \
        if (i == 1 || !MPDCliShowError(CLI, #CMD))      \
            return false;                               \
    }                                                   \
}



static MPDCli *mpdcli = NULL;

bool MPDCliOpenConn(MPDCli *mpdcli);
bool MPDCliUpdStatus(MPDCli *mpdcli);


void  FreeSong(Song**song)
{
	if(*song) {
		if((*song)->artist)
			free((*song)->artist);
		if((*song)->artist)
			free((*song)->artist);
		if((*song)->title)
			free((*song)->title);
		if((*song)->uri)
			free((*song)->uri);
		if((*song));
		*song = NULL;
	}
}


void  FreeMpdStatus(MpdStatus **status)
{
	if(*status) {
		if((*status)->currentsong)
			FreeSong(&(*status)->currentsong);
		if((*status)->errormessage)
			free((*status)->errormessage);
		free(*status);
		*status = NULL; 
	}
}

bool MPDCliOk(MPDCli *mpdcli)
{	
	return mpdcli->m_ok && mpdcli->m_conn;
}

bool MPDCliShowError(MPDCli *mpdcli ,char *who)
{
    if (!MPDCliOk(mpdcli)) {
        DEBUG_PRINTF("MpdCliShowError bad state");
        return false;
    }
	DEBUG_PRINTF("MPDCliOk ok");
    int error = mpd_connection_get_error(mpdcli->m_conn);
    if (error == MPD_ERROR_SUCCESS) {
        DEBUG_PRINTF("MPDCliShowError: %s  success !" , who);
        return false;
    }
   
    if (error == MPD_ERROR_SERVER) {
   
       	DEBUG_PRINTF("%s server error: %d ",who ,
               mpd_connection_get_server_error(mpdcli->m_conn));

    }

    if (error == MPD_ERROR_CLOSED)
        if (MPDCliOpenConn(mpdcli))
            return true;
		
    return false;
}

bool MPDCliOpenConn(MPDCli *mpdcli)
{
	if(mpdcli->m_conn) {
		mpd_connection_free(mpdcli->m_conn);
        mpdcli->m_conn = NULL;
	}
	mpdcli->m_conn = mpd_connection_new(mpdcli->m_host, mpdcli->m_port, 0);
	
	if (mpdcli->m_conn == NULL) {
		DEBUG_PRINTF("mpd_connection_new failed. No memory");
		return false;
	}
	DEBUG_PRINTF("mpd_connection_new ok");
	if (mpd_connection_get_error(mpdcli->m_conn) != MPD_ERROR_SUCCESS) {
		MPDCliShowError(mpdcli, "MPDCli::openconn");
		return false;
	}
/*
	if(!(*mpdcli)->m_password) {
		if (!mpd_run_password((*mpdcli)->m_conn, (*mpdcli)->m_password)) {
			DEBUG_PRINTF("Password wrong");
			return false;
		}
	}*/

	return true;
	
}

MPDCli *NewMPDCli(char *host, int port)
{
	MPDCli *cli = NULL;
	
	cli = calloc(1, sizeof(MPDCli));
	if(cli == NULL)
		return NULL;
	cli->m_conn = NULL;
	cli->m_premutevolume = 0;
	cli->m_cachedvolume = 50;
	cli->m_port = port;
	cli->m_host = host;
	cli->m_password = NULL;
	//cli->status = calloc(1, sizeof(MpdStatus));
	cli->m_stat =  NULL;
	if(!MPDCliOpenConn(cli))
		return NULL;
	cli->m_ok = true;
    MPDCliUpdStatus(cli);
	return cli;
}
bool MPDCliUpdSong(MPDCli *mpdcli, int pos)
{
    char *cp = NULL;
	Song *curSong = NULL;
	MpdStatus *stat = NULL;

	DEBUG_PRINTF("MPDCliUpdSong...");
    if (!MPDCliOk(mpdcli))
        return false;
	
	stat = mpdcli->m_stat;
	
	curSong = calloc(1, sizeof(Song));
	if(!curSong) {
		DEBUG_PRINTF("calloc Song failed");
		return false;
	}
	
	if(stat) {
		Song *song = NULL;
		song = stat->currentsong;
		if(song) {
			DEBUG_PRINTF("currentsong != NULL FreeSong...");
			FreeSong(&song);
		}
		
		{
			DEBUG_PRINTF("currentsong == NULL currentsong = curSong");
			stat->currentsong = curSong;
		}
		
	}

	
    struct mpd_song *song = NULL;
    if (pos == -1) {
        RETRY_CMD(mpdcli, song = mpd_run_current_song(mpdcli->m_conn));
    } else {
        RETRY_CMD(mpdcli ,song = mpd_run_get_queue_song_pos(mpdcli->m_conn, (unsigned int)pos));
    }
    DEBUG_PRINTF("mpd_run_current_song");    
    if (song == NULL) {
        DEBUG_PRINTF("mpd_run_current_song failed");
        return false;
    }

  
    cp = mpd_song_get_tag(song, MPD_TAG_ARTIST, 0);
    if (!cp) {
		DEBUG_PRINTF("artist:%s",cp);
		//curSong->artist = strdup(cp);
    }
	

    cp = mpd_song_get_tag(song, MPD_TAG_ALBUM, 0);
    if (!cp) {
		DEBUG_PRINTF("album:%s",cp);
		//curSong->album = strdup(cp);
    }

    cp = mpd_song_get_tag(song, MPD_TAG_TITLE, 0);
    if (!cp) {
		DEBUG_PRINTF("title:%s",cp);
		//curSong->title = strdup(cp);
    }

    cp = mpd_song_get_tag(song, MPD_TAG_TRACK, 0);
    if (!cp) {
		DEBUG_PRINTF("originalTrackNumber:%s",cp);
    }

    cp = mpd_song_get_tag(song, MPD_TAG_GENRE, 0);
    if (!cp) {
		DEBUG_PRINTF("genre:%s",cp);
    }
    
    cp = mpd_song_get_uri(song);
    if (cp != 0) {
		DEBUG_PRINTF("uri:%s",cp);
		curSong->uri = strdup(cp);
	}
	/*if(stat) {
		if(!stat->currentsong) {
			DEBUG_PRINTF("currentsong == NULL mpdcli->m_stat->currentsong = curSong");
			stat->currentsong = curSong;
		}
	}*/
	
    mpd_song_free(song);
	DEBUG_PRINTF("MPDCliUpdSong ok");
    return true;
}

bool MPDCliUpdStatus(MPDCli *mpdcli)
{
	MpdStatus *stat = NULL;
	struct mpd_status  *mpds = NULL;

    if (!MPDCliOk(mpdcli)) {
        DEBUG_PRINTF("MPDCliUpdStatus: bad state\n");
        return false;
    }

	stat = calloc(1, sizeof(MpdStatus));
	if(!stat) {
		 DEBUG_PRINTF("calloc MpdStatus failed\n");
		 return false;
	}

	if(mpdcli->m_stat) {
		//DEBUG_PRINTF("mpdcli->m_stat not null");
		FreeMpdStatus(&mpdcli->m_stat);
	}

	{
		mpdcli->m_stat = stat;
	}

    mpds = mpd_run_status(mpdcli->m_conn);
    if (mpds == NULL) {
        MPDCliOpenConn(mpdcli);
        mpds = mpd_run_status(mpdcli->m_conn);
        if (mpds == NULL) {
            DEBUG_PRINTF("MPDCliUpdStatus: can't get status\n");
            //MPDCliShowError(mpdcli , "MPDCliUpdStatus");
			stat->state = MPDS_UNK;
			return false;
        }
    }

    stat->volume = mpd_status_get_volume(mpds);

    if (stat->volume >= 0) {
        mpdcli->m_cachedvolume = stat->volume;
    } else {
        stat->volume = mpdcli->m_cachedvolume;
    }

	switch (mpd_status_get_state(mpds)) 
	{
	    case MPD_STATE_STOP:  stat->state = MPDS_STOP;  break;
		case MPD_STATE_PLAY:  stat->state = MPDS_PLAY;  break;
		case MPD_STATE_PAUSE: stat->state = MPDS_PAUSE; break;

		case MPD_STATE_UNKNOWN: 
	    default: stat->state = MPDS_UNK;break;
    }

    stat->songelapsedms = mpd_status_get_elapsed_ms(mpds);

	char *err = mpd_status_get_error(mpds);
    if (err != 0) {
        stat->errormessage = strdup(err);
    }

	stat->title = mpd_status_get_title(mpds);

    mpd_status_free(mpds);

    return true;
}

int MPDCliGetVolume(MPDCli *mpdcli)
{
    return mpdcli->m_stat->volume >= 0 ? mpdcli->m_stat->volume : mpdcli->m_cachedvolume;
}

int MPDCliGetElapsed_ms(MPDCli *mpdcli)
{
    return mpdcli->m_stat->songelapsedms >= 0 ? mpdcli->m_stat->songelapsedms : mpdcli->m_songelapsedms;
}


bool MPDCliPlay(MPDCli *mpdcli , int pos)
{
    DEBUG_PRINTF("MPDCliPlay(pos=%d)\n", pos);

	if (!MPDCliOk(mpdcli))
        return false;
    if (pos >= 0) {
        RETRY_CMD(mpdcli , mpd_run_play_pos(mpdcli->m_conn, (unsigned int)pos));
    } else {
        RETRY_CMD(mpdcli , mpd_run_play(mpdcli->m_conn));
    }
    return MPDCliUpdStatus(mpdcli);
}
bool MPDCliTogglePause(MPDCli *mpdcli)
{
    DEBUG_PRINTF("MPDCli::togglePause");
    
	if (!MPDCliOk(mpdcli))
        return false;

    RETRY_CMD(mpdcli, mpd_run_toggle_pause( mpdcli->m_conn));
    return MPDCliUpdStatus(mpdcli);
}

bool MPDCliStop(MPDCli *mpdcli)
{
	DEBUG_PRINTF("MPDCliStop");
	if (!MPDCliOk(mpdcli))
		return false;
    RETRY_CMD(mpdcli ,mpd_run_stop(mpdcli->m_conn));
		return true;
}
bool MPDCliPause(MPDCli *mpdcli , bool onoff)
{
	DEBUG_PRINTF("MPDCliPause");
    if (!MPDCliOk(mpdcli))
		return false;
	RETRY_CMD(mpdcli ,mpd_run_pause(mpdcli->m_conn, onoff));
	return MPDCliUpdStatus(mpdcli);
}
bool MPDCliSeek(MPDCli *mpdcli ,int seconds)
{
   if (!MPDCliUpdStatus(mpdcli))
		return -1;
    DEBUG_PRINTF("MPDCliSeek: pos:%d  seconds:%d", mpdcli->m_stat->songpos,seconds);
	RETRY_CMD(mpdcli, mpd_run_seek_pos(mpdcli->m_conn, mpdcli->m_stat->songpos, (int)seconds));
	//mpd_run_seek_pos(mpdcli->m_conn, mpdcli->m_stat->songpos, (int)seconds);
    return true;
}

bool MPDCliNext(MPDCli *mpdcli)
{
    DEBUG_PRINTF("mpdcli->m_conn");
    if (!MPDCliOk(mpdcli)) {
        return false;
    }
    RETRY_CMD(mpdcli,mpd_run_next(mpdcli->m_conn));
    return true;
}
bool MPDCliPrevious(MPDCli *mpdcli)
{
    DEBUG_PRINTF("MPDCli::previous");
    if (!MPDCliOk(mpdcli)) {
        return false;
    }
    RETRY_CMD(mpdcli,mpd_run_previous(mpdcli->m_conn));
    return true;
}

bool MPDCliSetVolume(MPDCli *mpdcli , int volume, bool isMute)
{
    if (!MPDCliOk(mpdcli)) {
        return false;
    }

    // MPD does not want to set the volume if not active.
    if ((mpdcli->m_stat->state == MPDS_UNK)) {
		DEBUG_PRINTF("MPDCliSetVolume: not active");
		return true;
    }

    if (isMute) {
        if (volume) {
            // Restore premute volume
			if(mpdcli->m_premutevolume)
				volume = mpdcli->m_stat->volume = mpdcli->m_premutevolume;
            DEBUG_PRINTF("MPDCliSetVolume: restoring premute %d" , mpdcli->m_premutevolume );
            mpdcli->m_premutevolume = 0;
        } else {
            if (mpdcli->m_cachedvolume > 0) {
                mpdcli->m_premutevolume = mpdcli->m_cachedvolume;
            }
        }
    }
        
    if (volume < 0)
        volume = 0;
   //// else if (volume > 100)
     //   volume = 100;
   
    RETRY_CMD(mpdcli ,mpd_run_set_volume(mpdcli->m_conn, volume));
	if(volume <= 100) {
		mpdcli->m_stat->volume = volume;
    	mpdcli->m_cachedvolume = volume;
	}
    return true;
}

int MPDCliInsert(MPDCli *mpdcli ,char *uri, int pos)
{
	//UpSong meta;
	//DEBUG_PRINTF("MPDCliInsert at : %d uri: %s" , pos , uri);
	
	if (!MPDCliOk(mpdcli))
		return -1;
	
	if (!MPDCliUpdStatus(mpdcli))
		return -1;
	
	int id;
	RETRY_CMD(mpdcli, (id=mpd_run_add_id_to(mpdcli->m_conn, uri, (unsigned)pos))!=-1);
	
	//meta.clear();
	//meta.title.assign(name);
	
	//send_tag_data(m_lastinsertid, meta);
	//send_tag_data(id, meta);
	
	return id;
}
bool MPDCliSendTag(MPDCli *mpdcli, const char *cid, int tag, char* data)
{                         
    if (!mpd_send_command(mpdcli->m_conn, "addtagid", cid, 
                          mpd_tag_name(tag),
                          data, NULL)) {
        DEBUG_PRINTF("MPDCliSendTag: mpd_send_command failed" );
        return false;
    }

    if (!mpd_response_finish(mpdcli->m_conn)) {
        DEBUG_PRINTF("MPDCliSendTag: mpd_response_finish failed");
        MPDCliShowError(mpdcli , "MPDCliSendTag");
        return false;
    }
    return true;
}


bool MPDCliUpname(MPDCli *mpdcli, char *name)
{
    DEBUG_PRINTF("MPDCli::update name: %s" ,name);
    if (!MPDCliOk(mpdcli))
        return -1;
   if (!MPDCliUpdStatus(mpdcli))
        return false;
   
    RETRY_CMD(mpdcli, mpd_run_update_name(mpdcli->m_conn, name));

    return true;
}

bool MPDCliClear(MPDCli *mpdcli)
{
	DEBUG_PRINTF("MPDCliClear");
    if (!MPDCliOk(mpdcli))
        return -1;

    RETRY_CMD(mpdcli,mpd_run_clear(mpdcli->m_conn));
    return true;
}

bool MPDCliDeleteId(MPDCli *mpdcli ,int id)
{
    DEBUG_PRINTF("MPDCli::deleteId :%d",id);
   	if (!MPDCliOk(mpdcli))
        return -1;

    RETRY_CMD(mpdcli,mpd_run_delete_id(mpdcli->m_conn, (unsigned)id));
    return true;
}

bool MPDCliDeleteRange(MPDCli *mpdcli ,int start, int end)
{
    DEBUG_PRINTF("MPDCli::delete from %d to %d" ,start , end );
	if (!MPDCliOk(mpdcli))
    	return -1;
    RETRY_CMD(mpdcli, mpd_run_delete_range(mpdcli->m_conn, (unsigned)--start, (unsigned)end));
    return true;
}

bool MPDCliMoveId(MPDCli *mpdcli ,int from, int to)
{
	DEBUG_PRINTF("MPDCli::move from %d to %d" ,from , to );
	if (!MPDCliOk(mpdcli))
    	return -1;

    RETRY_CMD(mpdcli,mpd_run_move(mpdcli->m_conn, (unsigned)--from, (unsigned)--to));
    return true;
}

bool MPDCliStatId(MPDCli *mpdcli ,int id)
{
    DEBUG_PRINTF("MPDCli::statId %d", id );
 	if (!MPDCliOk(mpdcli))
    	return -1;

    struct mpd_song *song = mpd_run_get_queue_song_id(mpdcli->m_conn, (unsigned)id);
    if (song) {
        mpd_song_free(song);
        return true;
    }
    return false;
}
int MPDCliCurpos(MPDCli *mpdcli)
{
    if (!MPDCliUpdStatus(mpdcli))
        return -1;
    DEBUG_PRINTF("MPDCli::curpos: pos: %d  id %d ", mpdcli->m_stat->songpos,mpdcli->m_stat->songid);
    return mpdcli->m_stat->songpos;
}


bool MPDCliCurlistdelsave(MPDCli *mpdcli)
{
    if (!MPDCliUpdStatus(mpdcli))
        return false;
    
	unlink(CURRENTLISTPATH);
	mpd_run_save(mpdcli->m_conn, CURRENTLIST);

	return true;
}


MpdStatus * MPDCliGetStatus(MPDCli *mpdcli)
{
	DEBUG_PRINTF("MpdCliGetStatus\n");
	if (true == MPDCliUpdStatus(mpdcli))
	{
		return mpdcli->m_stat;
	}

	return NULL;
}

int MPDCliGetPlayState(MPDCli *mpdcli)
{
	MpdStatus *mpds = NULL;
	mpds = MPDCliGetStatus(mpdcli);
	return (mpds->state);
}

bool MPDCliRunSeek(MPDCli *mpdcli, int pos, unsigned int progress)
{

	if (!MPDCliOk(mpdcli))
        return false;
    if (pos >= 0) {
        RETRY_CMD(mpdcli , mpd_run_seek_pos(mpdcli->m_conn, (unsigned int)pos, progress));
    } 
	return MPDCliUpdStatus(mpdcli);
}

bool MPDCliRunPauseSeek(MPDCli *mpdcli, int pos, unsigned int progress)
{

	DEBUG_PRINTF("MPDCliRunPauseSeek");

	if (!MPDCliOk(mpdcli))
        return false;
    if (pos >= 0) {
		//RETRY_CMD(mpdcli , mpd_run_pause_seek(mpdcli->m_conn, (unsigned int)pos, progress));
		RETRY_CMD(mpdcli , mpd_run_seek_pos(mpdcli->m_conn, (unsigned int)pos, progress));
		RETRY_CMD(mpdcli , mpd_run_toggle_pause(mpdcli->m_conn));
    } 

	return MPDCliUpdStatus(mpdcli);
}

void MpdRunSeek(int pos, unsigned int progress)
{
	
	pthread_mutex_lock(&mpdCliMtx);
	MPDCliRunSeek(mpdcli, pos, progress);
	pthread_mutex_unlock(&mpdCliMtx);
}


void MpdRunPauseSeek(int pos, unsigned int progress)
{
	
	pthread_mutex_lock(&mpdCliMtx);
	MPDCliRunPauseSeek(mpdcli, pos, progress);
	pthread_mutex_unlock(&mpdCliMtx);
}

int MpdInit()
{
	if(!mpdcli) {
		mpdcli = NewMPDCli("localhost",6600);
		if(!mpdcli) {
			DEBUG_PRINTF("NewMPDCli error");
			return -1;
		}
		if(!MPDCliOk(mpdcli)) {
			DEBUG_PRINTF("Cli connection failed");
			return -1;
		}
	}
	DEBUG_PRINTF("mpdcli init ok\n");

	return 0;
}

void FreeMPDCli(MPDCli **cli)
{
	if(*cli) {
		FreeMpdStatus(&((*cli)->m_stat));
		free(*cli);
		*cli = NULL;
	}
}

int MpdDeinit()
{
	pthread_mutex_lock(&mpdCliMtx);
	if(mpdcli) {
		 if (mpdcli->m_conn) 
        	mpd_connection_free(mpdcli->m_conn);
		FreeMPDCli(&mpdcli);
	}
	pthread_mutex_unlock(&mpdCliMtx);
	DEBUG_PRINTF("mpdcli deinit ...");
	return 0;
}

MpdStatus *MpdGetStatus()
{	
	MpdStatus *stat = NULL;

	pthread_mutex_lock(&mpdCliMtx);
	stat = MPDCliGetStatus(mpdcli);
	pthread_mutex_unlock(&mpdCliMtx);
	return stat;
}

#if 0
int MpdGetAudioStatus(AudioStatus *audioStatus)
{
	MpdStatus *stat = NULL;
	Song *song = NULL;
	
	pthread_mutex_lock(&mpdCliMtx);
	stat = MPDCliGetStatus(mpdcli);
	pthread_mutex_unlock(&mpdCliMtx);
	if(!stat)
		return -1;
	
	audioStatus->play 		=  stat->state - 1;
	audioStatus->state      =  stat->state ;
	audioStatus->duration	=  stat->songlenms/1000;
	audioStatus->progress   =  (int)(stat->songelapsedms/1000.0);
	DEBUG_PRINTF("audioStatus->state:%u, audioStatus->duration:%u, audioStatus->progress:%d", 
			audioStatus->state,audioStatus->duration ,audioStatus->progress);



	if(!audioStatus->url) {
		DEBUG_PRINTF("audioStatus->url == NULL");
		song = stat->currentsong;
		if(song) {
			DEBUG_PRINTF("song != NULL");
			char *url = NULL;
			url = song->uri;
			audioStatus->url =  strdup(url);
			DEBUG_PRINTF("audioStatus->url:%s",audioStatus->url);
			DEBUG_PRINTF("audioStatus->title:%s",song->title);
		}
	}
	DEBUG_PRINTF("MpdGetAudioStatus ok");
	return 0;
}
#endif

void MpdUpdate(void)
{
	pthread_mutex_lock(&mpdCliMtx);
	MPDCliUpdStatus(mpdcli);
	pthread_mutex_unlock(&mpdCliMtx);
}

void MpdAdd(char *url) 
{
	pthread_mutex_lock(&mpdCliMtx);
	MPDCliInsert(mpdcli, url ,0);
	pthread_mutex_unlock(&mpdCliMtx);
}
void MpdPlay(int pos)
{
	pthread_mutex_lock(&mpdCliMtx);
	MPDCliPlay(mpdcli,pos);
	pthread_mutex_unlock(&mpdCliMtx);
}
void MpdTogglePause()
{
	pthread_mutex_lock(&mpdCliMtx);
	MPDCliTogglePause(mpdcli);
	pthread_mutex_unlock(&mpdCliMtx);
}

void MpdPause()
{
	pthread_mutex_lock(&mpdCliMtx);
	MPDCliPause(mpdcli, true);
	pthread_mutex_unlock(&mpdCliMtx);
}
void MpdStop()
{
	pthread_mutex_lock(&mpdCliMtx);
	MPDCliStop(mpdcli);
	pthread_mutex_unlock(&mpdCliMtx);

}


int MpdGetVolume()
{		
	int volume ;

	MpdUpdate();

	pthread_mutex_lock(&mpdCliMtx);
	volume = MPDCliGetVolume(mpdcli);
	pthread_mutex_unlock(&mpdCliMtx);

	return volume;
}

int MpdGetCurrentElapsed_ms(void)
{		
	int iElapsed_ms ;

	MpdUpdate();

	pthread_mutex_lock(&mpdCliMtx);
	iElapsed_ms = MPDCliGetElapsed_ms(mpdcli);
	pthread_mutex_unlock(&mpdCliMtx);

	return iElapsed_ms;
}


void MpdVolume(int volume)
{	
	pthread_mutex_lock(&mpdCliMtx);
	volume = MPDCliSetVolume(mpdcli, volume, false);
	pthread_mutex_unlock(&mpdCliMtx);
}

void MpdSeek(int seconds)
{
	pthread_mutex_lock(&mpdCliMtx);
	MPDCliSeek(mpdcli,seconds);
	pthread_mutex_unlock(&mpdCliMtx);
}
	
int MpdGetPlayState()
{
	int state;
	pthread_mutex_lock(&mpdCliMtx);
	state = MPDCliGetPlayState(mpdcli);
	pthread_mutex_unlock(&mpdCliMtx);
	return state;
}

void MpdClear()
{
	pthread_mutex_lock(&mpdCliMtx);
	MPDCliClear(mpdcli);
	pthread_mutex_unlock(&mpdCliMtx);
}

#if 0
int MpdResume()
{
	int state ;
	pthread_mutex_lock(&mpdCliMtx);
	state = MPDCliGetPlayState(mpdcli);
	pthread_mutex_unlock(&mpdCliMtx);
	if(state == MPDS_PAUSE)
	{
		eval("mpc", "play");
		//MPDCliPlay(mpdcli,-1);
	}
}
#endif

// 0  1: local 2: uri
/*int MpdCliPlayLocalOrUri()
{
	MpdStatus *stat = NULL;
	pthread_mutex_lock(&mpdCliMtx);
	stat = MPDCliGetStatus(mpdcli);
	pthread_mutex_unlock(&mpdCliMtx);
	if(stat) {
		if(stat->state == MPDS_PLAY) {
			DEBUG_PRINTF("stat->state == MPDS_PLAY");
			if(stat->currentsong) {
				char *uri = NULL;
				uri = stat->currentsong->uri;
				DEBUG_PRINTF("uri:%s",uri);
				if(stat->currentsong->uri) {
					uri = stat->currentsong->uri;
					DEBUG_PRINTF("uri:%s",uri);
					if(UriHasScheme(uri))
						return 2;
					else 
						return 1;
				}
			} else {
				DEBUG_PRINTF("stat->currentsong == NULL");
			}
		}
	}
	return 0;
}*/




