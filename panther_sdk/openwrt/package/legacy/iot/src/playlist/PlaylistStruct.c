#include "PlaylistStruct.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "myutils/debug.h"
/* 分配一个MusicItem */
MusicItem * NewMusicItem(void)
{
	MusicItem *status = NULL;
	
	status = calloc(1, sizeof(MusicItem));
	if(status == NULL) {
		return NULL;
	}

	return status;
}
/* 深度拷贝MusicItem */
MusicItem *DupMusicItem(MusicItem *src)
{
	MusicItem *dst = NewMusicItem();
	
	assert(dst != NULL);
	assert(src != NULL);
	
	if(src->pContentURL) {
		debug("src->pContentURL:%s",src->pContentURL);
		dst->pContentURL = strdup(src->pContentURL);
	}
	if(src->pTitle) {
		debug("src->pTitle:%s",src->pTitle);
		dst->pTitle = strdup(src->pTitle);
	}
	if(src->pId) {
		debug("src->pId:%s",src->pId);
		dst->pId = strdup(src->pId);
	}
	if(src->pArtist) {
		debug("src->pArtist:%s",src->pArtist);
		dst->pArtist = strdup(src->pArtist);
	}
	if(src->pTip) {
		debug("src->pTip:%s",src->pTip);
		dst->pTip = strdup(src->pTip);
	}
	if(src->pAlbum) {
		debug("src->pAlbum:%s",src->pAlbum);
		dst->pAlbum = strdup(src->pAlbum);
	}
	if(src->pCoverURL) {
		debug("src->pCoverURL:%s",src->pCoverURL);
		dst->pCoverURL = strdup(src->pCoverURL);
	}

	dst->type = src->type; 		

	return dst;
}

/* 比例两个MusicItem中的pContentURL */
int MusicItemCmp(void *cmp1, void *cmp2)
{
	int ret = 0;
	MusicItem *src = (MusicItem *)cmp1;
	MusicItem *dst = (MusicItem *)cmp2;
	if(src == NULL && dst == NULL)
		return 0;
	if(src->pContentURL == NULL && dst->pContentURL  == NULL)
		return 0;
	debug("src->pContentURL:%s",src->pContentURL);
	debug("dst->pContentURL:%s",dst->pContentURL);
	debug("dst->pTitle:%s",dst->pTitle);
	debug("src->pTitle:%s",src->pTitle);	
	return !strcmp(src->pContentURL, dst->pContentURL);
}
/* 释放MusicItem */
void FreeMusicItem(void *arg)
{
	int ret = 0;
	int i = 0;
	MusicItem *music = (MusicItem *)arg;
	debug("in");
	if(music) 
	{
		if(music->pArtist)
		{	
			free(music->pArtist);
		}
		if(music->pTitle)
		{
			free(music->pTitle);
		}
		if(music->pContentURL)
		{
			free(music->pContentURL);
		}
		if(music->pTip)
		{
			free(music->pTip);
		}
		if(music->pAlbum)
		{
			free(music->pAlbum);
		}
		if(music->pCoverURL)
		{
			free(music->pCoverURL);
		}
		if(music->pId)
		{
			free(music->pId);
		}
		free(music);
	}
	debug("exit");
}


