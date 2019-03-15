#ifndef __TURING_PLAYLIST_H__
#define __TURING_PLAYLIST_H__

#include "PlaylistStruct.h"


enum {
	TURING_AUDIO_TYPE_WECHAT = 0,
	TURING_AUDIO_TYPE_SERVER,
	TURING_AUDIO_TYPE_LOCAL,
};

extern int TuringPlaylistInsert(char *file, MusicItem *music, int play);
extern int TuringPlaylistAdd(char *file, MusicItem *music, int play);
extern int Turing_list_Insert(char *file, MusicItem *music, int play);



#endif











