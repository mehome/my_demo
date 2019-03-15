
#include "mpdcli.h"


#include <stdio.h>
#include <pthread.h>
#include <mpd/song.h>
#include <mpd/tag.h>
#include <mpd/error.h>
#include <mpd/status.h>

#include "common.h"
#include "myutils/debug.h"

#define M_CONN (mpdcli->m_conn)

static pthread_mutex_t mpdCliMtx = PTHREAD_MUTEX_INITIALIZER;  
static pthread_cond_t captureCond = PTHREAD_COND_INITIALIZER; 


#define CURRENTLISTPATH "/usr/script/playlists/CurList.m3u" 
#define CURRENTLIST "CurList" 

#if 1
#define RETRY_CMD(CLI, CMD) {                           \
	int i;												\
    for ( i = 0; i < 2; i++) {                          \
        if ((CMD))                                      \
            break;                                      \
        if (i == 1 || !MPDCliShowError(CLI, #CMD))      \
            return false;                               \
    }                                                   \
}
#else
#define RETRY_CMD(CLI, CMD) CMD
#endif

#define RETRY_CMD_WITH_SLEEP(CLI, CMD) {                \
	int i;												\
    for ( i = 0; i < 2; i++) {                       	\
        if ((CMD))                                      \
            break;                                      \
        sleep(1);                                       \
        if (i == 1 || !MPDCliShowError(CLI, #CMD))      \
            return false;                               \
    }                                                   \
    }

static MPDCli *mpdcli = NULL;

bool MPDCliOpenConn(MPDCli *mpdcli);
bool MPDCliUpdStatus(MPDCli *mpdcli);
bool MPDCliGetQueueSongs(MPDCli *mpdcli, list_t *songs);
bool MPDCliSendTag(MPDCli *mpdcli, const char *cid, int tag, const char* data);
bool MPDCliSendTagData(MPDCli *mpdcli, int id, Song *meta);

void    FreeSong(Song *song)
{
	if(song) {
		if(song->uri) {
			free(song->uri);
			song->uri = NULL;
		}
		if(song->name) {
			free(song->name);
			song->name = NULL;
		}
		if(song->artist) {
			free(song->artist);
			song->artist = NULL;
		}
		if(song->album) {
			free(song->album);
			song->album = NULL;
		}
		if(song->title) {
			free(song->title);
			song->title = NULL;
		}
		if(song->tracknum) {
			free(song->tracknum);
			song->tracknum = NULL;
		}
		if(song->genre) {
			free(song->genre);
			song->genre = NULL;
		}
		if(song->artUri) {
			free(song->artUri);
			song->artUri = NULL;
		}
		free(song);
	}
}


void    FreeMpdStatus(MpdStatus **status)
{
	if(*status) {
		if((*status)->currentsong)
			FreeSong((*status)->currentsong);
		if((*status)->errormessage)
			free((*status)->errormessage);
		free(*status);
		*status = NULL; 
	}
}

bool MPDCliLooksLikeTransportURI(MPDCli *mpdcli, char *path)
{
    return (regexec(&(mpdcli->m_tpuexpr), path, 0, 0, 0) == 0);
}


bool MPDCliOk(MPDCli *mpdcli)
{	
	if(mpdcli == NULL){
		return false;
	}

	return mpdcli->m_ok && mpdcli->m_conn;
}

bool MPDCliShowError(MPDCli *mpdcli ,char *who)
{
    if (!MPDCliOk(mpdcli)) {
        error("MpdCliShowError bad state");
        return false;
    }
	debug("MPDCliOk ok");
    int error = mpd_connection_get_error(mpdcli->m_conn);
    if (error == MPD_ERROR_SUCCESS) {
        error("MPDCliShowError: %s  success !" , who);
        return false;
    }
   
    if (error == MPD_ERROR_SERVER) {
   
       	error( "%s server error: %d ",who ,
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
	//error("mpdcli->m_conn:%p",mpdcli->m_conn);
	if (mpdcli->m_conn == NULL) {
		error("mpd_connection_new failed. No memory");
		return false;
	}
	//debug("mpd_connection_new ok");
	if (mpd_connection_get_error(mpdcli->m_conn) != MPD_ERROR_SUCCESS) {
		MPDCliShowError(mpdcli, "MPDCli::openconn");
		
		return false;
	}

	/*if(!(*mpdcli)->m_password) {
		if (!mpd_run_password((*mpdcli)->m_conn, (*mpdcli)->m_password)) {
			error("Password wrong");
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
	MPDCliOpenConn(cli);
	cli->m_ok = true;
	
	//regcomp(&(cli->m_tpuexpr), "^[[:alpha:]]+://.+", REG_EXTENDED|REG_NOSUB);

	return cli;
}
static void
print_tag(const struct mpd_song *song, enum mpd_tag_type type,
	  const char *label)
{
	unsigned i = 0;
	const char *value;

	while ((value = mpd_song_get_tag(song, type, i++)) != NULL)
		error("%s: %s", label, value);
}
static void
print_song(const struct mpd_song *song)
{
	error("uri: %s\n", mpd_song_get_uri(song));
	print_tag(song, MPD_TAG_ARTIST, "artist");
	print_tag(song, MPD_TAG_ALBUM, "album");
	print_tag(song, MPD_TAG_TITLE, "title");
	print_tag(song, MPD_TAG_TRACK, "track");
	print_tag(song, MPD_TAG_NAME, "name");
	print_tag(song, MPD_TAG_DATE, "date");

	if (mpd_song_get_duration(song) > 0)
	  error("time: %i", mpd_song_get_duration(song));

	if (mpd_song_get_duration_ms(song) > 0)
	  error("duration: %i", mpd_song_get_duration_ms(song));

	error("pos: %u", mpd_song_get_pos(song));
}

bool MPDCliUpdSong(MPDCli *mpdcli, int pos)
{
    char *cp = NULL;
	Song *curSong = NULL;
	MpdStatus *stat = NULL;
	debug("enter");
	debug("MPDCliUpdSong...");
    if (!MPDCliOk(mpdcli))
        return false;
	
	stat = mpdcli->m_stat;
	
	debug("calloc  Song");	
	curSong = calloc(1, sizeof(Song));
	if(!curSong) {
		error("calloc Song failed");
		return false;
	}
	debug("calloc  ...");
	if(stat) {
		Song *song = NULL;
		song = stat->currentsong;
		if(song) {
			debug("currentsong != NULL FreeSong...");
			FreeSong(song);
		}
		
		{
			debug("currentsong == NULL ");
			stat->currentsong = curSong;
		}
		
	}

    struct mpd_song *song = NULL;
    if (pos == -1) {
        RETRY_CMD(mpdcli, song = mpd_run_current_song(mpdcli->m_conn));
    } else {
        RETRY_CMD(mpdcli ,song = mpd_run_get_queue_song_pos(mpdcli->m_conn, (unsigned int)pos));
    }
    //info("mpd_run_current_song");    
    if (song == NULL) {
        error("mpd_run_current_song failed");
        return false;
    }
/*
	//print_song(song);
    cp = mpd_song_get_tag(song, MPD_TAG_ARTIST, 0);
    if (!cp) {
		debug("artist:%s",cp);
		//curSong->artist = strdup(cp);
    }
	
	
    cp = mpd_song_get_tag(song, MPD_TAG_ALBUM, 0);
    if (!cp) {
		debug("album:%s",cp);
		//curSong->album = strdup(cp);
    }

    cp = mpd_song_get_tag(song, MPD_TAG_TITLE, 0);
    if (!cp) {
		debug("title:%s",cp);
		//curSong->title = strdup(cp);
    }

    cp = mpd_song_get_tag(song, MPD_TAG_TRACK, 0);
    if (!cp) {
		debug("originalTrackNumber:%s",cp);
    }

    cp = mpd_song_get_tag(song, MPD_TAG_GENRE, 0);
    if (!cp) {
		debug("genre:%s",cp);
    }
 */
    cp = mpd_song_get_uri(song);
    if (cp != 0) {
		debug("uri:%s",cp);
		curSong->uri = strdup(cp);
	}
	curSong->duration_secs = mpd_song_get_duration(song);
    curSong->mpdid = mpd_song_get_id(song);
    mpd_song_free(song);
	debug("MPDCliUpdSong ok");
	debug("exit");
    return true;
}

bool MPDCliUpdStatus(MPDCli *mpdcli)
{
	MpdStatus *stat = NULL;
	struct mpd_status  *mpds = NULL;
	debug("enter");

    if (!MPDCliOk(mpdcli))
    {
        error("MPDCliUpdStatus: bad state");
        return false;
    }
	
	stat = calloc(1, sizeof(MpdStatus));
	if(!stat) 
    {
		 error("calloc MpdStatus failed");
		 return false;
	}

	if(mpdcli->m_stat)
    {
		debug("mpdcli->m_stat not null");
		FreeMpdStatus(&mpdcli->m_stat);
	}

	{
		debug("mpdcli->m_stat null");
		mpdcli->m_stat = stat;
	}
    
    mpds = mpd_run_status(mpdcli->m_conn);
    if (mpds == NULL) 
    {
		debug("mpd_run_status NULL");
        MPDCliOpenConn(mpdcli);
        mpds = mpd_run_status(mpdcli->m_conn);
        if (mpds == NULL) 
        {
            error("MPDCliUpdStatus: can't get status");
            MPDCliShowError(mpdcli , "MPDCliUpdStatus");
			stat->state = MPDS_UNK;
			return false;
        }
    }
    

	debug("mpd_run_status ok");
    stat->volume = mpd_status_get_volume(mpds);
    debug("volume:%d",stat->volume);

    if (stat->volume >= 0) 
    {
        mpdcli->m_cachedvolume = stat->volume;
    } 
    else 
    {
        stat->volume = mpdcli->m_cachedvolume;
    }

    stat->repeat = mpd_status_get_repeat(mpds);
    stat->random = mpd_status_get_random(mpds);
   	stat->single = mpd_status_get_single(mpds);
    stat->consume = mpd_status_get_consume(mpds);
    stat->qlen = mpd_status_get_queue_length(mpds);
    stat->qvers = mpd_status_get_queue_version(mpds);
    stat->linkip = mpd_status_get_linkip(mpds);
    int state = mpd_status_get_state(mpds);
	debug("state:%d",state);
    debug("volume:%d",stat->volume);
    debug("repeat:%d",stat->repeat);
    debug("random:%d",stat->random);
    debug("single:%d",stat->single);
	switch (mpd_status_get_state(mpds)) {
    case MPD_STATE_STOP:  stat->state = MPDS_STOP;break;
    case MPD_STATE_PLAY:  stat->state = MPDS_PLAY;break;
    case MPD_STATE_PAUSE: stat->state = MPDS_PAUSE;break;
    case MPD_STATE_UNKNOWN: 
    default:
        stat->state = MPDS_UNK;
        break;
    }
	debug("mpd_status_get_state ok");
   // stat->crossfade = mpd_status_get_crossfade(mpds);
   // stat->ixrampdb = mpd_status_get_mixrampdb(mpds);
   // stat->mixrampdelay = mpd_status_get_mixrampdelay(mpds);
    stat->songpos = mpd_status_get_song_pos(mpds);
   	debug("stat->songpos:%d", stat->songpos);
    stat->songid = mpd_status_get_song_id(mpds);
	debug("stat->songid :%d", stat->songid );
	
    if (stat->songpos >= 0) {
        MPDCliUpdSong(mpdcli ,stat->songpos);
      //  updSong(mpdcli->m_stat->nextsong, mpdcli->m_stat->songpos + 1);
    }
	
    stat->songelapsedms = mpd_status_get_elapsed_ms(mpds);
    stat->songlenms = mpd_status_get_total_time(mpds) * 1000;
    stat->kbrate = mpd_status_get_kbit_rate(mpds);
	debug("stat->songelapsedms:%u, stat->songlenms:%u, stat->kbrate:%u", stat->songelapsedms,
				stat->songlenms , stat->kbrate);

	char *err = mpd_status_get_error(mpds);
    if (err != 0) {
        stat->errormessage = strdup(err);
    }

	stat->title = mpd_status_get_title(mpds);
	

    mpd_status_free(mpds);
	debug("MPDCliUpdStatus ok");

#if 0
    MPDCli * mympd = NULL;
    struct mpd_status *my_status = NULL;

    mympd = NewMPDCli("localhost",6600);
    if(mympd == NULL)
        info("error");
    MPDCliOpenConn(mympd);


    my_status = mpd_run_status(mympd->m_conn);
    info("status = %d",mpd_status_get_state(my_status));
    info("repeat = %d",mpd_status_get_repeat(my_status));
    info("length = %d",mpd_status_get_queue_length(my_status));
    info("volume = %d",mpd_status_get_volume(my_status));
    info("random = %d",mpd_status_get_random(my_status));
    mpd_status_free(my_status);
#endif



    
	debug("exit");
    return true;
}
int MPDCliGetPos(MPDCli *mpdcli)
{
    if(MPDCliUpdStatus(mpdcli) == true)
    {
        //正在语音点播单曲
        if((1 == (mpdcli->m_stat->songpos+1)) && (2 == (mpdcli->m_stat->qlen)))
        {
            return 0;
        }
        else if((2 == (mpdcli->m_stat->songpos+1)) && (2 == (mpdcli->m_stat->qlen)))
        {
            return 1;
        }
    }
}


int MPDCliGetVolume(MPDCli *mpdcli)
{
	if(MPDCliUpdStatus(mpdcli) == false) {
		int volume =  cdb_get_int("$ra_vol", 50);
		return volume;
	}
    return mpdcli->m_stat->volume >= 0 ? mpdcli->m_stat->volume : mpdcli->m_cachedvolume;
}
bool MPDCliPlay(MPDCli *mpdcli , int pos)
{
    debug("MPDCliPlay(pos=%d)", pos);

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
    debug("MPDCli::togglePause");
    
	if (!MPDCliOk(mpdcli))
        return false;

    RETRY_CMD(mpdcli, mpd_run_toggle_pause( mpdcli->m_conn));
    return MPDCliUpdStatus(mpdcli);
}

bool MPDCliStop(MPDCli *mpdcli)
{
	debug("MPDCliStop");
	if (!MPDCliOk(mpdcli))
		return false;
    RETRY_CMD(mpdcli ,mpd_run_stop(mpdcli->m_conn));
		return true;
}
bool MPDCliPause(MPDCli *mpdcli , bool onoff)
{
	debug("MPDCliPause");
    if (!MPDCliOk(mpdcli))
		return false;
	RETRY_CMD(mpdcli ,mpd_run_pause(mpdcli->m_conn, onoff));
	return MPDCliUpdStatus(mpdcli);
}
bool MPDCliSeek(MPDCli *mpdcli ,int seconds)
{
	debug("MPDCliSeek");
   if (!MPDCliUpdStatus(mpdcli))
		return false;
    debug("MPDCliSeek: pos:%d  seconds:%d", mpdcli->m_stat->songpos,seconds);
	RETRY_CMD(mpdcli, mpd_run_seek_pos(mpdcli->m_conn, mpdcli->m_stat->songpos, (int)seconds));
	//mpd_run_seek_pos(mpdcli->m_conn, mpdcli->m_stat->songpos, (int)seconds);
    return true;
}

bool MPDCliNext(MPDCli *mpdcli)
{
    debug("MPDCliNext");
    if (!MPDCliOk(mpdcli)) {
        return false;
    }
    RETRY_CMD(mpdcli,mpd_run_next(mpdcli->m_conn));
    return true;
}
bool MPDCliPrevious(MPDCli *mpdcli)
{
    debug("MPDCli::previous");
    if (!MPDCliOk(mpdcli)) {
        return false;
    }
    RETRY_CMD(mpdcli,mpd_run_previous(mpdcli->m_conn));
    return true;
}

bool MPDCliLoad(MPDCli *mpdcli ,char *filename)
{
	debug("MPDCliLoad");
	if (!MPDCliOk(mpdcli))
		return false;
    RETRY_CMD(mpdcli ,mpd_run_load(mpdcli->m_conn, filename));
	return true;
}
bool MPDCliSave(MPDCli *mpdcli ,char *filename)
{
	debug("MPDCliLoad");
	if (!MPDCliOk(mpdcli))
		return false;
    RETRY_CMD(mpdcli ,mpd_run_save(mpdcli->m_conn, filename));
	return true;
}

bool MPDCliUpdate(MPDCli *mpdcli ,char *path)
{
	debug("MPDCliUpdate");
	if (!MPDCliOk(mpdcli))
		return false;
    RETRY_CMD(mpdcli ,mpd_run_update(mpdcli->m_conn, path));
	return true;
}

bool MPDCliRepeat(MPDCli *mpdcli ,bool mode)
{
	debug("MPDCliRepeat %d", mode);
	if (!MPDCliOk(mpdcli))
		return false;
    RETRY_CMD(mpdcli ,mpd_run_repeat(mpdcli->m_conn, mode));
	return true;
}
bool MPDCliRandom(MPDCli *mpdcli ,bool mode)
{
	debug("MPDCliRandom");
	if (!MPDCliOk(mpdcli))
		return false;
    RETRY_CMD(mpdcli ,mpd_run_random(mpdcli->m_conn, mode));
	return true;
}
bool MPDCliSingle(MPDCli *mpdcli ,bool mode)
{
	debug("MPDCliSingle");
	if (!MPDCliOk(mpdcli))
		return false;
    RETRY_CMD(mpdcli ,mpd_run_single(mpdcli->m_conn, mode));
	return true;
}
bool MPDCliConsume(MPDCli *mpdcli ,bool on)
{
    debug("MPDCliconsume %d",on);
   if (!MPDCliOk(mpdcli))
        return false;

    RETRY_CMD(mpdcli , mpd_run_consume(mpdcli->m_conn, on));
    return true;
}

bool MPDCliSetVolume(MPDCli *mpdcli , int volume, bool isMute)
{
	debug("MPDCliSetVolume");
    if (!MPDCliOk(mpdcli)) {
        return false;
    }

    // MPD does not want to set the volume if not active.
    if ((mpdcli->m_stat->state == MPDS_UNK)) {
		error("MPDCliSetVolume: not active");
		return true;
    }

    if (isMute) {
        if (volume) {
            // Restore premute volume
			if(mpdcli->m_premutevolume)
				volume = mpdcli->m_stat->volume = mpdcli->m_premutevolume;
            warning("MPDCliSetVolume: restoring premute %d" , mpdcli->m_premutevolume );
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
bool MPDCliAdd(MPDCli *mpdcli ,char *uri)
{
	
	if (!MPDCliOk(mpdcli))
		return false;
	
	if (!MPDCliUpdStatus(mpdcli))
		return false;
	
	RETRY_CMD(mpdcli, mpd_run_add(mpdcli->m_conn, uri));
	return true;
}


bool MPDCliInsert(MPDCli *mpdcli ,char *uri, int pos)
{	
	if (!MPDCliOk(mpdcli))
		return false;
	
	if (!MPDCliUpdStatus(mpdcli))
		return false;
	int id;
	mpdcli->m_lastinsertpos = pos;
	RETRY_CMD(mpdcli, (mpdcli->m_lastinsertid =
		mpd_run_add_id_to(mpdcli->m_conn, uri, (unsigned)pos))!=-1);
	mpdcli->m_lastinsertqvers = mpdcli->m_stat->qvers;
	return true;
}

int MPDCliInsertSong(MPDCli *mpdcli ,char *uri, int pos, Song *song)
{	
	if (!MPDCliOk(mpdcli))
		return false;
	
	RETRY_CMD(mpdcli, (mpdcli->m_lastinsertid =
		mpd_run_add_id_to(mpdcli->m_conn, uri, (unsigned)pos))!=-1);
	
    if (mpdcli->m_have_addtagid)
        MPDCliSendTagData(mpdcli, mpdcli->m_lastinsertid, song);

	mpdcli->m_lastinsertpos = pos;
	
	MPDCliUpdStatus(mpdcli);
	
	mpdcli->m_lastinsertqvers = mpdcli->m_stat->qvers;
	
	return mpdcli->m_lastinsertid;
}

int MPDCliInsertSongAfterId(MPDCli *mpdcli,char *uri, int id, Song* meta)
{
    debug("MPDCliInsertAfterId: id %d uri %s", id, uri );
   if (!MPDCliOk(mpdcli))
		return false;

    // id == 0 means insert at start
    if (id == 0) {
        return MPDCliInsertSong(mpdcli, uri, 0, meta);
    }
	
    MPDCliUpdStatus(mpdcli);

    int newpos = 0;
    if (mpdcli->m_lastinsertid == id && mpdcli->m_lastinsertpos >= 0 &&
        mpdcli->m_lastinsertqvers == mpdcli->m_stat->qvers) {
        newpos = mpdcli->m_lastinsertpos + 1;
    } else {
        // Translate input id to insert position
        list_t *songs = list_new_free(mpd_song_free);
        if (!MPDCliGetQueueSongs(mpdcli, songs)) {
			list_destroy(songs);
            return false;
        }
		unsigned int pos = 0;
        for (pos = 0; pos < list_length(songs); pos++) {
			struct mpd_song*song= NULL;
			song = list_at_value(songs, pos);
            unsigned int qid = mpd_song_get_id(song);
            if (qid == (unsigned int)id || pos == list_length(songs) -1) {
                newpos = pos + 1;
                break;
            }
        }
        list_destroy(songs);
    }
    return  MPDCliInsertSong(mpdcli, uri, newpos, meta);
}

bool MPDCliInsertUri(MPDCli *mpdcli ,char *uri)
{	
	struct mpd_status *status = NULL;
	if (!MPDCliOk(mpdcli))
		return false;
	
	if (!MPDCliUpdStatus(mpdcli))
		return false;
	
	status = mpd_run_status(mpdcli->m_conn);
	if(status == NULL)
		return false;
	const unsigned from = mpd_status_get_queue_length(status);
	const int cur_pos = mpd_status_get_song_pos(status);
	mpd_status_free(status);

	status = mpd_run_status(mpdcli->m_conn);
	const unsigned end = mpd_status_get_queue_length(status);
	mpd_status_free(status);	

	MPDCliAdd(mpdcli, uri);
	
	if (end == from)
		return 0;

	/* move those songs to right after the current one */
	if (!mpd_run_move_range(mpdcli->m_conn, from, end, cur_pos + 1))
		return false;

	return true;
}

bool MPDCliUpname(MPDCli *mpdcli, char *name)
{
    debug("MPDCli::update name: %s" ,name);
    if (!MPDCliOk(mpdcli))
        return false;
   if (!MPDCliUpdStatus(mpdcli))
        return false;
   
    RETRY_CMD(mpdcli, mpd_run_update_name(mpdcli->m_conn, name));

    return true;
}

bool MPDCliClear(MPDCli *mpdcli)
{
	debug("MPDCliClear");
    if (!MPDCliOk(mpdcli))
        return false;

    RETRY_CMD(mpdcli,mpd_run_clear(mpdcli->m_conn));
    return true;
}
bool MPDCliDeleteId(MPDCli *mpdcli ,int id)
{
    debug("MPDCli::deleteId :%d",id);
   	if (!MPDCliOk(mpdcli))
        return false;

    RETRY_CMD_WITH_SLEEP(mpdcli,mpd_run_delete_id(mpdcli->m_conn, (unsigned)id));
    return true;
}

bool MPDCliDeletePosRange(MPDCli *mpdcli ,int start, int end)
{
    debug("MPDCli::delete from %d to %d" ,start , end );
	if (!MPDCliOk(mpdcli))
    	return false;
    RETRY_CMD(mpdcli, mpd_run_delete_range(mpdcli->m_conn, (unsigned)--start, (unsigned)end));
    return true;
}

bool MPDCliMoveId(MPDCli *mpdcli ,int from, int to)
{
	debug("MPDCli::move from %d to %d" ,from , to );
	if (!MPDCliOk(mpdcli))
    	return false;

    RETRY_CMD(mpdcli,mpd_run_move(mpdcli->m_conn, (unsigned)--from, (unsigned)--to));
    return true;
}

bool MPDCliStatId(MPDCli *mpdcli ,int id)
{
    debug("MPDCli::statId %d", id );
 	if (!MPDCliOk(mpdcli))
    	return false;

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
        return false;
    debug("MPDCli::curpos: pos: %d  id %d ", mpdcli->m_stat->songpos,mpdcli->m_stat->songid);
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
	if(!MPDCliUpdStatus(mpdcli)) {
		error("MPDCliUpdStatus failed");
		return NULL;
	}
	return mpdcli->m_stat;
}

int MPDCliGetPlayState(MPDCli *mpdcli)
{
	MpdStatus *mpds = NULL;
	mpds = MPDCliGetStatus(mpdcli);
	if(mpds) {
		return (mpds->state);
	}
	else
		return 0;
}

bool MPDCliRunSeek(MPDCli *mpdcli, int pos, unsigned int progress)
{
	debug("enter");
	if (!MPDCliOk(mpdcli))
        return false;
    if (pos >= 0) {
        RETRY_CMD(mpdcli , mpd_run_seek_pos(mpdcli->m_conn, (unsigned int)pos, progress));
    } else {
    	  RETRY_CMD(mpdcli , mpd_run_play(mpdcli->m_conn));
	}
	return MPDCliUpdStatus(mpdcli);
}

bool MPDCliRunPauseSeek(MPDCli *mpdcli, int pos, unsigned int progress)
{

	debug("MPDCliRunPauseSeek");
	
	if (!MPDCliOk(mpdcli))
        return false;
    if (pos >= 0) {
		//RETRY_CMD(mpdcli , mpd_run_pause_seek(mpdcli->m_conn, (unsigned int)pos, progress));
		RETRY_CMD(mpdcli , mpd_run_seek_pos(mpdcli->m_conn, (unsigned int)pos, progress));
		RETRY_CMD(mpdcli , mpd_run_toggle_pause(mpdcli->m_conn));
    }  else {
    	RETRY_CMD(mpdcli , mpd_run_play(mpdcli->m_conn));
		RETRY_CMD(mpdcli , mpd_run_toggle_pause(mpdcli->m_conn));
	}
	return MPDCliUpdStatus(mpdcli);
}
bool MPDCliSendTag(MPDCli *mpdcli, const char *cid, int tag, const char *data)
{
    if (!mpd_send_command(M_CONN, "addtagid", cid, 
                          mpd_tag_name((enum mpd_tag_type)tag),
                          data, NULL)) {
        error("MPDCliSendtag: mpd_send_command failed");
        return false;
    }

    if (!mpd_response_finish(M_CONN)) {
        error("MPDCli::send_tag: mpd_response_finish failed");
        MPDCliShowError(mpdcli, "MPDCliSendTag");
        return false;
    }
    return true;
}

bool MPDCliSendTagData(MPDCli *mpdcli, int id, Song *meta)
{
    debug("MPDCliSendTagData");
    if (!mpdcli->m_have_addtagid)
        return false;

    char cid[30];
    sprintf(cid, "%d", id);

    if (meta->artist && !MPDCliSendTag(mpdcli, cid, MPD_TAG_ARTIST, meta->artist))
        return false;
    if (meta->album && !MPDCliSendTag(mpdcli, cid, MPD_TAG_ALBUM, meta->album))
        return false;
    if (meta->title && !MPDCliSendTag(mpdcli, cid, MPD_TAG_TITLE, meta->title))
        return false;
    if (meta->tracknum && !MPDCliSendTag(mpdcli, cid, MPD_TAG_TRACK, meta->tracknum))
        return false;
    //if (meta->tracknum && !send_tag(cid, MPD_TAG_COMMENT, upmpdcli_comment))
      //  return false;
    return true;
}

Song * MPDCliMapSong(struct mpd_song *song)
{
    char *cp = NULL;
	Song *upsong  = NULL;
	upsong = calloc(1, sizeof(Song));
	if(upsong == NULL)
		return NULL;
    cp = mpd_song_get_uri(song);
    if (cp != 0)
        upsong->uri = strdup(cp);
    // If the URI looks like a local file
    // name, replace with a bogus http uri. This is to fool
    // Bubble UPnP into accepting to play them (it does not
    // actually need an URI as it's going to use seekid, but
    // it believes it does).
   // if (!MPDCliLooksLikeTransportURI(upsong.uri)) {
        //LOGDEB("MPDCli::mapSong: id " << upsong.mpdid << 
        // " replacing [" << upsong.uri << "]" << endl);
    //    upsong.uri = "http://127.0.0.1/" + upsong.uri;
  // }
    cp = mpd_song_get_tag(song, MPD_TAG_NAME, 0);
    if (cp != NULL)
        upsong->name = strdup(cp);
    cp = mpd_song_get_tag(song, MPD_TAG_ARTIST, 0);
    if (cp != NULL)
        upsong->artist = strdup(cp);
	
   	if (upsong->artist == NULL && upsong->name)
        upsong->artist = strdup(upsong->name);
	
    cp = mpd_song_get_tag(song, MPD_TAG_ALBUM, 0);
    if (cp != NULL)
        upsong->album = strdup(cp);

    cp = mpd_song_get_tag(song, MPD_TAG_TITLE, 0);
    if (cp != NULL) 
        upsong->title = strdup(cp);

    cp = mpd_song_get_tag(song, MPD_TAG_TRACK, 0);
    if (cp != NULL)
        upsong->tracknum = strdup(cp);

    cp = mpd_song_get_tag(song, MPD_TAG_GENRE, 0);
    if (cp != NULL)
        upsong->genre = strdup(cp);

    upsong->duration_secs = mpd_song_get_duration(song);
    upsong->mpdid = mpd_song_get_id(song);
	
    return upsong;
}

bool MPDCliGetQueueSongs(MPDCli *mpdcli, list_t *songs)
{
    info("MPDCliGetQueueSongs");
    list_clear(songs);

    RETRY_CMD(mpdcli ,mpd_send_list_queue_meta(M_CONN));

    struct mpd_song *song;
    while ((song = mpd_recv_song(M_CONN)) != NULL) {
        list_add(songs, song);
    }
    
    if (!mpd_response_finish(M_CONN)) {
        error("MPDCliGetQueueSongs: mpd_list_queue_meta failed");
        return false;
    }
    debug("MPDCliGetQueueSongs: " , list_length(songs));
    return true;
}

bool MPDCliGetQueueData(MPDCli *mpdcli, list_t *queue)
{
    info("MPDCliGetQueueData");
    Song *upsong = NULL;
    RETRY_CMD(mpdcli ,mpd_send_list_queue_meta(M_CONN));

    struct mpd_song *song;
    while ((song = mpd_recv_song(M_CONN)) != NULL) {
		upsong = MPDCliMapSong(song);
		if(upsong)
			list_add(queue, upsong);
		mpd_song_free(song);
    }
    
    if (!mpd_response_finish(M_CONN)) {
        error("MPDCliGetQueueSongs: mpd_list_queue_meta failed");
        return false;
    }
    info("MPDCliGetQueueSongs: songs.size:%d", list_length(queue));
    return true;
}


bool MPDCliSaveState(MPDCli *mpdcli, MpdState *st, int seekms)
{
    if (!MPDCliUpdStatus(mpdcli)) {
        error("MPDCliSaveState can't retrieve current status\n");
        return false;
    }
    st->status = mpdcli->m_stat;//dup
    if (seekms > 0) {
        st->status->songelapsedms = seekms;
    }
    list_clear(st->queue);

   	if (!MPDCliGetQueueData(mpdcli, st->queue)) {
        error("MPDCliSaveState: can't retrieve current playlist\n");
        return false;
    }
    return true;
}

bool MPDCliRestoreState(MPDCli *mpdcli, MpdState *st)
{
    debug("MPDCliRestoreState: seekms:%u ", st->status->songelapsedms);
    MPDCliClear(mpdcli);
	unsigned int i = 0;
    for (i = 0; i < list_length(st->queue); i++) {
		Song *song = list_at_value(st->queue, i);
		if(song == NULL)
			continue;
        if (MPDCliInsertSong(mpdcli, song->uri, i, song) < 0) {
             error("MPDCliRestoreState: insert failed");
             return false;
       }
    }
    MPDCliRepeat(mpdcli, st->status->repeat);
    MPDCliRandom(mpdcli, st->status->random);
    MPDCliSingle(mpdcli, st->status->single);
    MPDCliConsume(mpdcli, st->status->consume);
    mpdcli->m_cachedvolume = st->status->volume;
    //no need to set volume if it is controlled external
    if (! mpdcli->m_externalvolumecontrol)
        mpd_run_set_volume(M_CONN, st->status->volume);

    if (st->status->state == MPDS_PAUSE ||
        st->status->state == MPDS_PLAY) {
        // I think that the play is necessary and we can't just do
        // pause/seek from stop state. To be verified.
        MPDCliPlay(mpdcli, st->status->songpos);
        if (st->status->songelapsedms > 0)
             MPDCliSeek(mpdcli, st->status->songelapsedms/1000);
        if (st->status->state == MPDS_PAUSE)
             MPDCliPause(mpdcli, true);
        if (!mpdcli->m_externalvolumecontrol)
            mpd_run_set_volume(M_CONN, st->status->volume);
    }
    return true;
}
static bool copyMpd(MPDCli *src, MPDCli *dest, int seekms)
{
    if (!src || !dest) {
        error("copyMpd: src or dest is null\n");
        return false;
    }
    MpdState *st = calloc(1, sizeof(MpdState));
    return MPDCliSaveState(src, st, seekms) && MPDCliRestoreState(dest, st);
}

int MpdInit()
{
	if(!mpdcli) 
    {
		mpdcli = NewMPDCli("localhost",6600);
		if(!mpdcli) 
        {
			error("NewMPDCli error");
			return -1;
		}
		if(!MPDCliOk(mpdcli)) 
        {
			error("Cli connection failed");
			return -1;
		}
	}
	//MpdUpdate()
	exec_cmd("mpc update");
	debug("mpdcli init ok");
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
	debug("mpdcli init ok");
	return 0;
}

MpdStatus *MpdGetStatus()
{	MpdStatus *stat = NULL;
	debug("MpdGetStatus");
	pthread_mutex_lock(&mpdCliMtx);
	stat = MPDCliGetStatus(mpdcli);
	pthread_mutex_unlock(&mpdCliMtx);
	return stat;
}

bool    MpdRunSeek(int pos, unsigned int progress)
{
	int ret;
	pthread_mutex_lock(&mpdCliMtx);
	ret = MPDCliRunSeek(mpdcli, pos, progress);
	pthread_mutex_unlock(&mpdCliMtx);
	return ret;
}


bool MpdRunPauseSeek(int pos, unsigned int progress)
{
	int ret;
	pthread_mutex_lock(&mpdCliMtx);
	ret = MPDCliRunPauseSeek(mpdcli, pos, progress);
	pthread_mutex_unlock(&mpdCliMtx);
	return ret;
}

int MpdGetAudioStatus(AudioStatus *audioStatus)
{
	MpdStatus *stat = NULL;
	Song *song = NULL;
	debug("enter");
	pthread_mutex_lock(&mpdCliMtx);
	stat = MPDCliGetStatus(mpdcli);
	pthread_mutex_unlock(&mpdCliMtx);
	if(!stat) {
		error("stat == Null");
		return -1;
	}

	
	audioStatus->play 		=  stat->state - 1;
	audioStatus->state      =  stat->state ;
	audioStatus->duration	=  stat->songlenms/1000;
	audioStatus->progress   =  (int)(stat->songelapsedms/1000.0);
	audioStatus->repeat     =  stat->repeat;
	audioStatus->random     =  stat->random;
	audioStatus->single     =  stat->single;
	warning("audioStatus->state:%u, audioStatus->duration:%u, audioStatus->progress:%d", 
			audioStatus->state,audioStatus->duration ,audioStatus->progress);

	if(!audioStatus->url) {
		info("audioStatus->url == NULL");
		song = stat->currentsong;
		info("stat->currentsong");
		if(song) {
			info("song != NULL");
			char *url = NULL;
			url = song->uri;
			if(url) {
				audioStatus->url =  strdup(url);
				info("audioStatus->url:%s", audioStatus->url);
			}
		} else {
			info("song == NULL");
		}
	}
	info("exit...");
	return 0;
}



bool MpdUpdateStatus(void)
{
	int ret = 0;
	pthread_mutex_lock(&mpdCliMtx);
	ret = MPDCliUpdStatus(mpdcli);
	pthread_mutex_unlock(&mpdCliMtx);
	return ret;
}

bool MpdAdd(char *url) 
{
	int ret;
	pthread_mutex_lock(&mpdCliMtx);
	ret = MPDCliAdd(mpdcli, url);
	pthread_mutex_unlock(&mpdCliMtx);
	return ret;
}

bool MpdPlay(int pos)
{
	int ret;
	pthread_mutex_lock(&mpdCliMtx);
	ret = MPDCliPlay(mpdcli, pos);
	pthread_mutex_unlock(&mpdCliMtx);
	return ret;
}
bool MpdTogglePause()
{
	int ret;
	pthread_mutex_lock(&mpdCliMtx);
	ret = MPDCliTogglePause(mpdcli);
	pthread_mutex_unlock(&mpdCliMtx);
	return ret;
}

bool MpdPause()
{
	int ret;
	pthread_mutex_lock(&mpdCliMtx);
	ret = MPDCliPause(mpdcli, true);
	pthread_mutex_unlock(&mpdCliMtx);
	return ret;
}
bool MpdStop()
{
	int ret;
	pthread_mutex_lock(&mpdCliMtx);
	ret = MPDCliStop(mpdcli);
	pthread_mutex_unlock(&mpdCliMtx);
	return ret;
}
bool MpdNext()
{
	int ret;
	pthread_mutex_lock(&mpdCliMtx);
	ret = MPDCliNext(mpdcli);
	pthread_mutex_unlock(&mpdCliMtx);
	return ret;
}
bool MpdInset(char *url,int pos)
{
	int ret;
	pthread_mutex_lock(&mpdCliMtx);
	ret = MPDCliInsert(mpdcli, url,pos );
	pthread_mutex_unlock(&mpdCliMtx);
	return ret;
}
bool MpdSave(char *file)
{
	int ret;
	pthread_mutex_lock(&mpdCliMtx);
	ret = MPDCliSave(mpdcli, file);
	pthread_mutex_unlock(&mpdCliMtx);
	return ret;
}


int MpdGetVolume()
{		
	int volume ;

	pthread_mutex_lock(&mpdCliMtx);
	volume = MPDCliGetVolume(mpdcli);
	pthread_mutex_unlock(&mpdCliMtx);

	return volume;
}
bool MpdVolume(int volume)
{	
	int ret;
	pthread_mutex_lock(&mpdCliMtx);
	ret = MPDCliSetVolume(mpdcli, volume, false);
	pthread_mutex_unlock(&mpdCliMtx);
	return ret;
	
}

bool MpdSeek(int seconds)
{
	int ret;
	pthread_mutex_lock(&mpdCliMtx);
	ret =  MPDCliSeek(mpdcli,seconds);
	pthread_mutex_unlock(&mpdCliMtx);
	return ret;
}

//0:未知    1:stop 2:play 3:pause
int MpdGetPlayState()
{
	int state;
	pthread_mutex_lock(&mpdCliMtx);
	state = MPDCliGetPlayState(mpdcli);
	pthread_mutex_unlock(&mpdCliMtx);
	return state;
}

bool   MpdClear()
{
	int ret;
	pthread_mutex_lock(&mpdCliMtx);
	ret = MPDCliClear(mpdcli);
	pthread_mutex_unlock(&mpdCliMtx);
	return ret;
}
bool MpdLoad(char *filename)
{
	int ret;
	pthread_mutex_lock(&mpdCliMtx);
	ret = MPDCliLoad(mpdcli,filename);
	pthread_mutex_unlock(&mpdCliMtx);
	return ret;
}


bool MpdUpdate(char *path)
{
	int ret;
	pthread_mutex_lock(&mpdCliMtx);
	ret = MPDCliUpdate(mpdcli,path);
	pthread_mutex_unlock(&mpdCliMtx);
	return ret;
}
bool MpdRandom(bool mode)
{
	int ret;
	pthread_mutex_lock(&mpdCliMtx);
	ret = MPDCliRandom(mpdcli,mode);
	pthread_mutex_unlock(&mpdCliMtx);
	return ret;
}
bool MpdRepeat(bool mode)
{
	int ret;
	pthread_mutex_lock(&mpdCliMtx);
	ret = MPDCliRepeat(mpdcli,mode);
	pthread_mutex_unlock(&mpdCliMtx);
	return ret;
}
bool MpdSingle(bool mode)
{
	int ret;
	pthread_mutex_lock(&mpdCliMtx);
	ret = MPDCliSingle(mpdcli,mode);
	pthread_mutex_unlock(&mpdCliMtx);
	return ret;
}


bool MpdTttsPlayOver()
{
    int ret = 0;
    while(!ret)
    {
        pthread_mutex_lock(&mpdCliMtx);
        ret = MPDCliGetPos(mpdcli);
        pthread_mutex_unlock(&mpdCliMtx);
        usleep(100*1000);
    }

    return ret;
}


// 0  1: local 2: uri
int MpdCliPlayLocalOrUri()
{
	MpdStatus *stat = NULL;
	pthread_mutex_lock(&mpdCliMtx);
	stat = MPDCliGetStatus(mpdcli);
	pthread_mutex_unlock(&mpdCliMtx);
	if(stat) {
		if(stat->state == MPDS_PLAY) {
			debug("stat->state == MPDS_PLAY");
			if(stat->currentsong) {
				char *uri = NULL;
				uri = stat->currentsong->uri;
				debug("uri:%s",uri);
				if(stat->currentsong->uri) {
					uri = stat->currentsong->uri;
					debug("uri:%s",uri);
					if(UriHasScheme(uri))
						return 2;
					else 
						return 1;
				}
			} else {
				warning("stat->currentsong == NULL");
			}
		}
	}
	return 0;
}





