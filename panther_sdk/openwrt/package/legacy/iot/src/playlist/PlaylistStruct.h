#ifndef __PLAYLIST_STRUCT_H__
#define __PLAYLIST_STRUCT_H__

typedef struct MusicItem
{
	char *pContentURL;	//歌曲链接
	char *pTitle;		//歌曲名
	char *pArtist;		//作者
	char *pTip;			//歌曲名链接

	char *pId;			//id号
	char *pAlbum;		//
	char *pCoverURL;	//
	//char *pLrcURL;

	int type;			//type类型
}MusicItem;



void FreeMusicItem(void *music);
MusicItem * NewMusicItem(void);
int MusicItemCmp(void *cmp1, void *cmp2);




#endif

