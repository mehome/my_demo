
#include "MiguMusic.h"
#include <stdio.h> /* printf, sprintf */

#include <json/json.h>


#include "myutils/debug.h"
#include "common.h"
#include "cJSON.h"
#include "StringUtil.h"
#include "CharUtil.h"
#include "PlaylistStruct.h"


/*int GetMac(char *id)
{

	char *device = "br0"; 
	unsigned char macaddr[ETH_ALEN]; 
	int i,s;  
	s = socket(AF_INET,SOCK_DGRAM,0); 
	struct ifreq req;  
	int err,rc;  
	rc  = strcpy(req.ifr_name, device); 
	err = ioctl(s, SIOCGIFHWADDR, &req); 
	close(s);  

	if (err != -1 )  
	{  
		memcpy(macaddr, req.ifr_hwaddr.sa_data, ETH_ALEN); 
		
		for(i = 0; i < ETH_ALEN; i++)  
		{
			//sprintf(&id[(3+i) * 2], "%02x", macaddr[i]);
			if(i == ETH_ALEN - 1)
				sprintf(id+(3*i),"%02x",macaddr[i]);
			else
				sprintf(id+(3*i),"%02x:",macaddr[i]);
		}
		
	}
	return 0;
}*/


int FindBrace(char *str)
{
	int i ;
	int len = strlen(str);
	info("len:%d",len);

    for(i = len - 1; i > 0; i--)
    {
    	if(str[i] != '}' )
			str[i] = '\0';
		else
			break;
    }

    return 0;
}
char *DupStrline(char *str)
{
	char *dst = NULL;
	int len = strlen(str);
	info("len:%d", len);
	char ch;
	int i , j = 0;
	dst = calloc (1, len+1);
    for(i = 0; i < len; i++)
    {
    	char ch  = (char)str[i];
		if(IsWhitespaceNotNull(ch))
			continue;    	
			dst[j] = ch;	
			j++;
    }
	return dst;
}

static int PraseMusicInfo(json_object *index, search_key_t *key, search_style_t style, MusicItem *item)
{
	int find = 0;
	int ret = 0;
	json_object *val = NULL;
	
	char *musicId = NULL, *lrcUrl = NULL, *isCpAuth = NULL;
	char *musicName = NULL, *singerName = NULL, *listenUrl = NULL;
	
	char *music = key->song_key.music_name;
	char *singer = key->song_key.singer_name;

	val =  json_object_object_get(index, "listenUrl"); 
	if(val == NULL)
		return -1;
	
	listenUrl = json_object_get_string(val);
	if (strlen(listenUrl) <= 0)	
		return -1;

	val =  json_object_object_get(index, "musicId"); 
	if(val) {
		musicId = json_object_get_string(val);
	}
	
	val =  json_object_object_get(index, "musicName"); 
	if(val) {
		musicName = json_object_get_string(val);
	}
	
	val =  json_object_object_get(index, "singerName"); 
	if(val) {
		singerName = json_object_get_string(val);
	}

	val =  json_object_object_get(index, "lrcUrl"); 
	if(val) {
		lrcUrl = json_object_get_string(val);
	} 
	val =  json_object_object_get(index, "isCpAuth"); 
	if(val) {
		isCpAuth = json_object_get_string(val);
	}

	
	if(style == SONG_SEARCH) 
	{
		if(musicName == NULL)
			return -1;
		if(singerName == NULL)
			singerName = "null";
		if( !strcmp(music ,musicName) ||  !strcmp(musicName ,music) 
			|| strstr(musicName, music)) 
		{
			find = 1;
		}
	}
	if(style == SINGER_SEARCH) 
	{
		
		if(singerName == NULL)
			return -1;
		if(musicName == NULL)
			musicName = "null";
		if( !strcmp(singer ,singerName) ||  !strcmp(singerName ,singer) 
			|| strstr(singerName, singer)) 
		{
			find = 1;
		}
	}
	if(style == SINGLE_SEARCH) 
	{
		int musicFind = 0;
		int singerFind = 0;

		if( musicName == NULL || singerName == NULL)
			return -1;
		if( !strcmp(music ,musicName) ||  !strcmp(musicName ,music) 
			|| strstr(musicName, music)) 
		{
			musicFind = 1;
		} 	
		if( !strcmp(singer ,singerName) ||  !strcmp(singerName ,singer) 
			|| strstr(singerName, singer)) 
		{
			singerFind = 1;
		}
		if(musicFind && singerFind) 
		{
		 	find = 1;
		}
	}
	if(find) {
		ret = 0;
		info("musicId:%s",musicId);
		info("musicName:%s",musicName);
		info("singerName:%s",singerName);
		info("listenUrl:%s",listenUrl);
		info("lrcUrl:%s",lrcUrl);
		info("isCpAuth:%s",isCpAuth);

		if(listenUrl) {
			item->pContentURL = strdup(listenUrl);
			info("item->pContentURL:%s",item->pContentURL);
		}
		if(musicName && strlen(musicName) > 0) {
			item->pTitle = strdup(musicName);
			info("item->pTitle:%s",item->pTitle);
		}
		if(singerName && strlen(singerName) > 0 ) {
			item->pArtist = strdup(singerName);
			info("item->pArtist:%s",item->pArtist);
		}
		if(musicId && strlen(musicName) > 0) {
			item->pId = strdup(musicId);
			info("item->pId:%s" ,item->pId);
		}
		
		item->type = MUSIC_TYPE_MIGU;
	}
	
	return ret;
	
}

static int PraseJson(char *data, search_key_t *key, search_style_t style)
{
	json_object *val = NULL;
	int ret = -1;
	int length, i;
	char *randUrl = NULL;
	char *singer = key->song_key.music_name;
	char *song = key->song_key.singer_name;
	json_object *root = json_tokener_parse(data);
	if (is_error(root))  {
		error("json_tokener_parse failed");
		return -1;
	}
	info("root:%s",json_object_to_json_string(root));

	val =  json_object_object_get(root, "resCode");
	if(val == NULL)
		goto exit;
		
	ret = atoi(json_object_get_string(val));
	info("resCode:%d",ret);
	if(ret != 0)
		goto exit;
	
	val =  json_object_object_get(root, "musicInfos"); 
	if(val == NULL)
		goto exit;
	length =  json_object_array_length(val);
	info("length:%d",length);
	info("style:%d", style);
	if(length <= 0 )
		goto exit;
	srand( (unsigned)time(NULL) );	 
	int count = rand()%length + 1;
	info("count:%d", count);
	for(i = 0;  i < length ; i++) 
	{
		char *url = NULL;
		MusicItem *item = NULL;
		json_object *index = NULL;
		item = NewMusicItem();
		if( item == NULL)
			goto exit;
		
		index = json_object_array_get_idx(val, i);
		if(index == NULL)
			continue;
	
		ret = PraseMusicInfo(index, key, style, item);
		if(style == SONG_SEARCH || style == SINGLE_SEARCH) 
		{
			info("SINGLE_SEARCH");
			if(ret == 0) 
			{
				info("url:%s", url);
				TuringPlaylistInsert(TURING_PLAYLIST, item, 1);
				break;
			}
			
		} 
		else if(style == LIST_SEARCH || style == SINGER_SEARCH) 
		{	
			info("LIST_SEARCH");
			if(ret == 0) 
			{

				TuringPlaylistInsert(TURING_PLAYLIST, item, 1);
				break;
			}
			
		}
		if(item)
			FreeMusicItem(item);
	}

exit:
	json_object_put(root);
	return ret;
}

int test(char *singer, char *music)
{
	search_key_t * songKey = NULL;
	songKey = calloc(1, sizeof(search_key_t));
	songKey->song_key.singer_name = NULL;
	songKey->song_key.music_name = NULL;
	if(music) {
		songKey->song_key.music_name = strdup(music);
	}
	if(singer) {
		songKey->song_key.singer_name = strdup(singer);
	}
	SearchMiguMusic(songKey);
	FreeSearchKey(&songKey);
}

/* 从MIGU中搜索歌曲 */
int SearchMiguMusic(search_key_t *key)
{
	char *data = NULL;
	char *postdata = NULL;
	char mac[64] = {0};
	search_style_t style;
	int ret = -1;
	
	char *music = key->song_key.music_name;
	char *singer = key->song_key.singer_name;

	GetMac(mac);
	
	if(singer && music) {
		style = SINGLE_SEARCH;
		postdata =  join(singer, music);
	} else if(singer && music == NULL) {
		style = SINGER_SEARCH;
		postdata = strdup(singer);
	} else if(music && singer == NULL)  {
		style = SONG_SEARCH;
		postdata = strdup(music);
	}
	if(postdata == NULL)
		return -1;
	info("postdata:%s", postdata);
	//data = miguMusic("296367b6dabf4fedb2e1d2c77565507f", "be86181d3faca3b96c1f5870ec51de31","00:0C:29:52:59:2A", "\xe5\x88\x98\xe5\xbe\xb7\xe5\x8d\x8e\xe5\x86\xb0\xe9\x9b\xa8");		//zhangyu
	//data = miguMusic("296367b6dabf4fedb2e1d2c77565507f", "be86181d3faca3b96c1f5870ec51de31","00:0C:29:52:59:2A", "\xE5\x88\x98\xE5\xBE\xB7\xE5\x8D\x8E\xE5\x86\xB0\xE9\x9B\xA8");		//zhangyu
	data =	miguMusic("296367b6dabf4fedb2e1d2c77565507f", "be86181d3faca3b96c1f5870ec51de31",mac, postdata);		//zhangyu
	
	if(data) 
	{
		info("data:%s", data);
		FindBrace(data);
		char *tmp = DupStrline(data);
		if(tmp) 
		{
			ret = PraseJson(tmp, key, style);
			if(tmp)
				free(tmp);
		}
		
		free(data);
	}
	if(postdata)
		free(postdata);
	return ret;
}

