#ifndef __MPD_CLIENT_H__
#define __MPD_CLIENT_H__

#include <stdio.h>

#include <mpd/song.h>
#include <mpd/connection.h>

typedef enum  
{
	MPDS_UNK  = 0,
	MPDS_STOP, 
	MPDS_PLAY, 
	MPDS_PAUSE
}State ;

typedef struct song {
	
	char *artist;
	char *album;
	char *title;
	char *uri;
}Song;


typedef struct mpdStatus
{
	int volume;
	//int rept;
	//int random,
	//int single;
	//int consume;
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
	int m_songelapsedms;
	char *m_host;
    int m_port;
    char *m_password;
	 
}MPDCli;


MpdStatus *MpdGetStatus();
//int MpdGetAudioStatus(AudioStatus *audioStatus);
void MpdUpdate(void);
void MpdAdd(char *url) ;
void MpdPlay(int pos);
void MpdPause();
void MpdStop();
int MpdGetVolume();
void MpdSeek(int seconds);
int MpdGetPlayState();
#if 0
int MpdResume();
#endif
int MpdCliPlayLocalOrUri();
int MpdInit();
int MpdDeinit();
int MpdGetCurrentElapsed_ms(void);














#endif

