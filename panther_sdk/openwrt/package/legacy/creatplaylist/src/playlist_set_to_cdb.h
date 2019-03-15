#ifndef _PLAYLIST_SET_TO_CDB_H_
#define _PLAYLIST_SET_TO_CDB_H_


//#define __DEBUGPRINTF__

#ifdef  __DEBUGPRINTF__
#define PRINTF printf
#else
#define PRINTF //
#endif

int playlist_set_to_cdb(char *cdb_playlist);
int json_playlist_set_to_cdb(char *json_playlist);





#endif