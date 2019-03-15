#ifndef __MPGPLAYER_H__
#define __MPGPALYER_H__

#include "linklist.h"



enum {
	MPG123_STATE_NONE= 0,
	MPG123_STATE_STARTED,
	MPG123_STATE_ONGING,
	MPG123_STATE_DONE,
	MPG123_STATE_CLOSE,
	MPG123_STATE_CANCLE,
};

typedef enum  {
	PLAYER_STATE_STOP = 0,
	PLAYER_STATE_PAUSE,
	PLAYER_STATE_PLAY
}MpgPlayerState;

typedef struct mpg123_url {		
	char *url;
	struct list_head  list;
}Mpg123Url;

typedef struct playlist_struct
{

	size_t entry; /* entry in the playlist file */
	size_t playcount; /* overall track counter for playback */
	long loop;    /* repeat a track n times */
	size_t size;
	size_t fill;
	size_t pos;
	size_t alloc_step;
	//mpg123_string linebuf;
	//mpg123_string dir;
} playlist_struct;


void DestoryMpg123Thread();
void CreateMpg123Thread(void *arg);

void StartMpg123Thread();
void SetMpg123Cancle();
int IsMpg123Done();
int IsMpg123Finshed();
void SetMpg123State(int state);
int IsMpg123Cancled();
void AddMpg123Url(char *url);

static void freeMpgUrlList(struct list_head *head);
static void SetMpg123Wait(int wait);



void prev_dir(void);
void set_intflag();
void prev_dir(void);
void next_dir(void);
void prev_track(void);
void next_track(void);









#endif


