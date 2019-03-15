#include <stdio.h>
//#include <json/json.h>
#include <json-c/json.h>
#include "playlist_info_json.h"

int WIFIAudio_RealTimeInterface_pGetCurrentPlaylistFree(WARTI_pMusicList *ppmusiclist)
{
	int i;
	int ret = 0;
	
	if((NULL == ppmusiclist)||(NULL == *ppmusiclist))
	{
		PRINTF("Error of parameter : NULL == ppmusiclist or NULL == *ppmusiclist in WIFIAudio_RealTimeInterface_pGetCurrentPlaylistFree\r\n");
		ret = -1;
	}
	else
	{
		if(NULL != (*ppmusiclist)->pRet)
		{
			free((*ppmusiclist)->pRet);
			(*ppmusiclist)->pRet = NULL;
		}
		if(NULL != (*ppmusiclist)->pMusicList)
		{
			for(i = 0; i<((*ppmusiclist)->Num); i++)
			{
				if(NULL != (((*ppmusiclist)->pMusicList)[i]))
				{
					WIFIAudio_RealTimeInterface_pMusicFree(&(((*ppmusiclist)->pMusicList)[i]));
				}
			}
			free((*ppmusiclist)->pMusicList);
			(*ppmusiclist)->pMusicList = NULL;
		}
		free(*ppmusiclist);
		*ppmusiclist = NULL;
	}
		
	return ret;
}


int WIFIAudio_RealTimeInterface_pMusicFree(WARTI_pMusic *ppmusic)
{
	int ret = 0;
	int i = 0;
	
	if((NULL == ppmusic) || (NULL == *ppmusic))
	{
		PRINTF("Error of Parameter : NULL == ppmusic or NULL == *ppmusic in WIFIAudio_RealTimeInterface_pMusicFree\r\n");
		ret = -1;
	}
	else
	{
		
		if(NULL != (*ppmusic)->pArtist)
		{
			free((*ppmusic)->pArtist);
			(*ppmusic)->pArtist = NULL;
		}
		if(NULL != (*ppmusic)->pTitle)
		{
			free((*ppmusic)->pTitle);
			(*ppmusic)->pTitle = NULL;
		}if(NULL != (*ppmusic)->pAlbum)
		{
			free((*ppmusic)->pAlbum);
			(*ppmusic)->pAlbum = NULL;
		}
		if(NULL != (*ppmusic)->pContentURL)
		{
			free((*ppmusic)->pContentURL);
			(*ppmusic)->pContentURL = NULL;
		}
		if(NULL != (*ppmusic)->pCoverURL)
		{
			free((*ppmusic)->pCoverURL);
			(*ppmusic)->pCoverURL = NULL;
		}
		free(*ppmusic);
		*ppmusic = NULL;
	}
	
	return ret;
}


WARTI_pMusic WIFIAudio_RealTimeInterface_pJsonObjectTopMusic(struct json_object *p)
{
	WARTI_pMusic ret;
	struct json_object *getobject;
	
	if(p==NULL)
	{
		PRINTF("传入指向struct json_object结构体指针为NULL，出错返回\r\n");
		return NULL;
	}
	
	ret = (WARTI_pMusic)calloc(1,sizeof(WARTI_Music));  //yes
	if(ret==NULL)
	{
		PRINTF("分配空间返回为空，出错返回\r\n");
		return NULL;
	}
	//根据需要分配空间
	
	getobject = json_object_object_get(p,"artist");//获取的是JSON的对象，要转换成声明值要在做转换
	if(NULL != getobject)
	{
		ret->pArtist = strdup(json_object_get_string(getobject));
	}
	else
	{
		ret->pArtist = NULL;
	}
	getobject = json_object_object_get(p,"title");//获取的是JSON的对象，要转换成声明值要在做转换
	if(NULL != getobject)
	{
		ret->pTitle = strdup(json_object_get_string(getobject));
	}
	else
	{
		ret->pTitle = NULL;
	}
	
		getobject = json_object_object_get(p,"album");//获取的是JSON的对象，要转换成声明值要在做转换
	if(NULL != getobject)
	{
		ret->pAlbum = strdup(json_object_get_string(getobject));
	}
	else
	{
		ret->pAlbum = NULL;
	}
	
	getobject = json_object_object_get(p,"content_url");
	if(NULL != getobject)
	{
		ret->pContentURL = strdup(json_object_get_string(getobject));
	}
	else
	{
		ret->pContentURL = NULL;
	}
	
	getobject = json_object_object_get(p,"cover_url");
	if(NULL != getobject)
	{
		ret->pCoverURL = strdup(json_object_get_string(getobject));
	}
	else
	{
		ret->pCoverURL = NULL;
	}
	
    getobject = json_object_object_get(p,"token");
	if(NULL != getobject)
	{
		ret->pToken = strdup(json_object_get_string(getobject));
	}
	else
	{
		ret->pToken = NULL;
	}
	
	return ret;
}


WARTI_pMusicList WIFIAudio_RealTimeInterface_pStringTopGetCurrentPlaylist(char *p)
{
	WARTI_pMusicList ret = NULL;
	int i = 0;
	struct json_object *getobject;
	struct json_object *object;
	
	
	
	ret = (WARTI_pMusicList)calloc(1,sizeof(WARTI_MusicList)); //yes
	if(ret==NULL)
	{
		PRINTF("分配空间返回为空，出错返回\r\n");
		return NULL;
	}
	
	object = json_tokener_parse(p);//根据标志将字符串转为JSON对象，之前一直不理解这个函数是什么意思
	
	//这个主要是解析推送过来的列表，所以这边暂时不解析ret这个条件，因为推送过来的东西不含有ret这个字段
	getobject = json_object_object_get(object,"ret");//获取的是JSON的对象，要转换成声明值要在做转换
	if(NULL == getobject)
	{
		ret->pRet = NULL;
	}
	else
	{
		ret->pRet = strdup(json_object_get_string(getobject));
	}
	
	getobject = json_object_object_get(object,"num");
	if(getobject==NULL)
	{
		ret->Num = WIFIAUDIO_REALTIMEINTERFACE_INTERRORCODE;
	}
	else
	{
		ret->Num = json_object_get_int(getobject);
		PRINTF("ret->Num =%d\n",ret->Num);
	}
	
	getobject = json_object_object_get(object,"index");
	if(getobject==NULL)
	{
		ret->Index = WIFIAUDIO_REALTIMEINTERFACE_INTERRORCODE;
	}
	else
	{
		ret->Index = json_object_get_int(getobject);
		PRINTF("ret->Index =%d\n",ret->Index);
	}
	
	ret->pMusicList = (WARTI_pMusic)calloc(1,(ret->Num)*sizeof(WARTI_pMusic));  //yes
	
	if(NULL == ret->pMusicList)
	{
		PRINTF("Error of calloc 111111: NULL == ret in WIFIAudio_RealTimeInterface_pStringTopGetCurrentPlaylist\r\n");
		system("rm /usr/script/playlists/httpPlayListInfoJson.txt");
		WIFIAudio_RealTimeInterface_pGetCurrentPlaylistFree(&ret);		
		ret = NULL;
	}
	
	getobject = json_object_object_get(object,"musiclist");
	
	if(strlen(json_object_to_json_string(getobject)) < 10)
	{
		printf("str is null\n");
		
		//free(p);
		//free(ret->pMusicList);
		exit(1);
	}
	
	for(i = 0;i<(ret->Num);i++)
	{
		(ret->pMusicList)[i] = WIFIAudio_RealTimeInterface_pJsonObjectTopMusic(json_object_array_get_idx(getobject,i));
	}
	
	//json_object_put(object);//转换成结构体之后将JSONobject释放

	
	return ret;
}