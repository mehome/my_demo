#ifndef __MPD_CLIENT_H__
#define __MPD_CLIENT_H__

#include <stdio.h>
#include <regex.h> 
#include <mpd/song.h>
#include <mpd/connection.h>
#include <mpd/client.h>

#include "DeviceStatus.h"
#include "list.h"


typedef enum  
{
	MPDS_UNK  = 0,
	MPDS_STOP, 
	MPDS_PLAY, 
	MPDS_PAUSE
}State ;

typedef struct song {

	char *uri;
	char *name;
	char *artist;
	char *album;
	char *title;
	
	char *tracknum;
	char *genre;
	char *artUri;
	unsigned int duration_secs;
	int mpdid;
}Song;


typedef struct mpdStatus
{
	int volume;
	int repeat;
	int random;
	int single;
	int consume;
	int title;
	unsigned int linkip;
	int qlen;
    int qvers;
	State state;
	//unsigned int crossfade;
    //float mixrampdb;
    //float mixrampdelay;
	int songpos;
    int songid;
	unsigned int songelapsedms; //current ms
    unsigned int songlenms; // song millis
    unsigned int kbrate;
	Song * currentsong;
    //Song * nextsong;
	char *errormessage;
}MpdStatus;


// Complete Mpd State
typedef struct MpdState {
    MpdStatus *status;
    list_t *queue;
}MpdState;

typedef struct mpdCli
{
	MpdStatus *m_stat;
	
	struct mpd_connection *m_conn;
	bool m_ok;
	// Saved volume while muted.
    int m_premutevolume;
    // Volume that we use when MPD is stopped (does not return a
    // volume in the status)
    int m_cachedvolume; 
	char *m_host;
    int m_port;
    char *m_password;
	bool m_externalvolumecontrol; 
	regex_t m_tpuexpr;
	 // addtagid command only exists for mpd 0.19 and later.
	bool m_have_addtagid; 
	int m_lastinsertid;
    int m_lastinsertpos;
    int m_lastinsertqvers;
}MPDCli;


MpdStatus *MpdGetStatus();
int MpdGetAudioStatus(AudioStatus *audioStatus);
bool MpdUpdateStatus(void);
bool MpdAdd(char *url) ;
bool MpdPlay(int pos);
bool MpdPause();
bool MpdStop();
int MpdGetVolume();
bool MpdSeek(int seconds);
int MpdGetPlayState();
int MpdCliPlayLocalOrUri();
int MpdInit();
int MpdDeinit();














#endif

