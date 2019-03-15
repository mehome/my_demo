
#ifndef __MUSIC_ITEM_PARSER_H__
#define __MUSIC_ITEM_PARSER_H__




#include "PlaylistStruct.h"

extern int MusicItemlistAdd(char *jsonFile, char *m3uFile, MusicItem *prev , MusicItem *next);

extern int MusicItemlistInsert(char *jsonFile, char *m3uFile, MusicItem *prev , MusicItem *next);


#endif








