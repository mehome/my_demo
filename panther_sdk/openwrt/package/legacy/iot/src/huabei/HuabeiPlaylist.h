#ifndef __HUABEI_PLAYLIST_H__
#define __HUABEI_PLAYLIST_H__


enum {
	PLAYLIST_MODE_PREV = 1,
	PLAYLIST_MODE_NEXT,
};
int HuabeiPlaylistPrev();

int HuabeiPlaylistNext();
int HuabeiPlaylistPrevNext(int mode);
#endif