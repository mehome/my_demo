#include "MusicItemParser.h"
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <assert.h>
#include <json/json.h>

#include "FileUtil.h"
#include "list.h"
#include "myutils/debug.h"
#include "common.h"
int   MusicListShowList(list_t *list);
/* 将MusicItem转成json对象. */
static json_object* MusicItemToJsonObject(MusicItem *p)
{
	json_object *object;
	
	if(p == NULL)
	{
		error(" MusicItem is NULL");
		return NULL;
	}
	object = json_object_new_object();
	
	if(p->pArtist)
	{
		json_object_object_add(object,"artist", json_object_new_string(p->pArtist));
	}
	
	if(p->pTitle)
	{
		json_object_object_add(object,"title", json_object_new_string(p->pTitle));
	}
	if(p->pContentURL)
	{
		json_object_object_add(object,"content_url", json_object_new_string(p->pContentURL));
	}
	
	
	if(p->pTip)
	{
		json_object_object_add(object,"tip", json_object_new_string(p->pTip));
	}
	
	if(p->pId) 
	{
		json_object_object_add(object,"id", json_object_new_string(p->pId));
	}
	
	if(p->pAlbum)
	{
		json_object_object_add(object,"album", json_object_new_string(p->pAlbum));
	}

	if(p->pCoverURL)
	{
		json_object_object_add(object,"cover_url", json_object_new_string(p->pCoverURL));
	}

	
	json_object_object_add(object,"type", json_object_new_int(p->type));

	
	return object;
}
/* 将json对象转成MusicItem. */
static MusicItem* JsonObjectToMusicItem(json_object* obj)
{
	MusicItem *item = NULL;
	char *artist = NULL ;
	char *title = NULL;
	char *tip = NULL;
	char *album = NULL;
	char *coverUrl = NULL;
	char *contentUrl = NULL;
	char *mediaId = NULL;
	int type = 0;
	json_object *artistObj=NULL ,*titleObj=NULL, *contentUrlObj = NULL;
	json_object *tipObj=NULL ,*albumObj=NULL, *coverUrlObj = NULL;
	json_object *typeObj=NULL;json_object *mediaIdObj=NULL;

	item = NewMusicItem();
	
	if(item == NULL)
		return NULL;
	
	artistObj		= json_object_object_get(obj , "artist");
	titleObj 		= json_object_object_get(obj , "title");
	contentUrlObj     = json_object_object_get(obj , "content_url");
	tipObj =  json_object_object_get(obj , "tip");
	albumObj =  json_object_object_get(obj , "album");
	coverUrlObj = json_object_object_get(obj , "cover_url");
	typeObj = json_object_object_get(obj , "type");
	mediaIdObj = json_object_object_get(obj , "id");

	if(mediaIdObj) 
	{
		mediaId = json_object_get_string(mediaIdObj);
		if(mediaId && strlen(mediaId) > 0) {
			item->pId = strdup(mediaId);
			debug("mediaId:%s" ,mediaId);
		}
		
	}

	if(artistObj) 
	{
		artist = (char *)json_object_get_string(artistObj);
		
		if(artist && strlen(artist) > 0) {
			debug("artist:%s" ,artist);
			item->pArtist = strdup(artist);
		}
	}
	
	if(titleObj) 
	{
		title = (char *)json_object_get_string(titleObj);
		if(title && strlen(title) > 0) {
			debug("title:%s" ,title);
			item->pTitle = strdup(title);
		}

	}
	
	if(tipObj) 
	{
		tip	= (char *)json_object_get_string(tipObj);
		if(tip && strlen(tip) > 0) {
			debug("tip:%s" ,tip);
			item->pTip = strdup(tip);
		}
	}
	if(albumObj) 
	{
		album = (char *)json_object_get_string(albumObj);

		if(album && strlen(album) > 0) {
			item->pAlbum = strdup(album);
			debug("album:%s",album);
		}
	}
	if(coverUrlObj) {
		coverUrl = (char *)json_object_get_string(coverUrlObj);
		
		if(coverUrl && strlen(coverUrl) > 0) {
			item->pCoverURL = strdup(coverUrl);
			debug("coverUrl:%s",coverUrl);
		}
		
	}
	if(contentUrlObj) {
		contentUrl = (char *)json_object_get_string(contentUrlObj);
		if(contentUrl && strlen(contentUrl) > 0) {
			item->pContentURL = strdup(contentUrl);
			debug("contentUrl:%s",contentUrl);
		}
	}
	
	if(typeObj) 
	{
		type =json_object_get_int(typeObj);
		item->type = type;
		debug("type:%d",type);
	}

	return item;
}
/* 将list_t中所有MusicItem转成json字符串. */
static char *MusicItemListToJson(list_t *list)
{
	info("enter");
	char *data = NULL;
	int length =  0;
	json_object *root = NULL;
	json_object *allLists=NULL;
	list_iterator_t *it = NULL;
	root = json_object_new_object();
	if(NULL == root) {
		error("json_object_new_array failed");
		return NULL;
	}
	allLists = json_object_new_array();
	if(NULL == allLists) {
		error("json_object_new_array failed");
		goto exit;
	}
	it = list_iterator_new(list,LIST_HEAD);

	while(list_iterator_has_next(it )) {
		MusicItem *item = NULL;
		json_object *val = NULL;
	
		item = (MusicItem *)list_iterator_next_value(it);
		if(item) {
			debug("item:%p", item);
			val = MusicItemToJsonObject(item);
			if(val) {
				debug("val:%s", json_object_to_json_string(val));
				json_object_array_add(allLists, val);
			}
		}
		
	}
	
	length = json_object_array_length(allLists);			
	debug("allLists:%s", json_object_to_json_string(allLists));
	json_object_object_add(root,"num", json_object_new_int((length)));
	json_object_object_add(root,"musiclist",allLists);
	
	data = strdup(json_object_to_json_string(root));
	
	json_object_put(root);
	list_iterator_destroy(it);
	return data;
exit:
	if(it)
		list_iterator_destroy(it);
	if(root) {
		json_object_put(root);
	}
	if(allLists) {
		json_object_put(allLists);
	}
	info("exit");
	return NULL;
}

/* 从json字符串解析MusicItem,并添加到list中. */
static int     MusicItemListParser(list_t *list, char *p)
{
	int i = 0, err = 0;
	json_object *all;
	json_object *allList;
	int length = 0;
	all = json_tokener_parse(p);
	if (is_error(all)) 
	{
		error("json_tokener_parse failed ");
		return NULL;
	} 	
	allList = json_object_object_get(all, "musiclist");
	if(allList == NULL) 
	{
		err = -1;
		goto exit;
	}
	length = json_object_array_length(allList);
	info("length:%d ", length);
	if(length <= 0 ) 
	{
		error("json_object_array_length failed ");
		err = -1;
		goto exit;
	}
	debug("allList:%s ", json_object_to_json_string(allList));//打印所有列表json数据
	int count = 0;
	for ( i = 0; i < length ; i++) 
	{
		json_object* obj = NULL;
		MusicItem *item = NULL;
		
		obj = json_object_array_get_idx(allList, i);  
		item = JsonObjectToMusicItem(obj);
		if(item) {
			debug("item:%p", item);
			debug("item->Title:%s",item->pTitle);
			list_add(list, item);
			count++;
		}
	}
	info("count%d, length:%d", count, length);
exit:

	if(all) {
		json_object_put(all);
		all = NULL;
	}
	info("exit");
	return err;
	
}

/* 将MusicItem插入jsonFile的头部,同时键url插入到m3uFile. */
int MusicItemlistInsert(char *jsonFile, char *m3uFile, MusicItem *prev , MusicItem *next)
{
	int ret = 0;
	list_t *list = NULL;
	char *data = NULL, *json = NULL;
	list = list_new_free_match(FreeMusicItem, MusicItemCmp);	//申请空间
	if(list == NULL) {
		error("calloc for MusicItem list failed ");
		return -1;
	}
	data = ReadData(jsonFile);				//将原本的json数据获取出来
	if(data) {
		ret = MusicItemListParser(list, data);
		if(ret < 0) {
			error("MusicItemListParser failed");
			ret = -1;
			goto exit;
		}
	}
	
	MusicItem *val = DupMusicItem(next);	//将当前列表的信息拷贝到MusicItem *val中
	if(val == NULL) {
		error("DupMusicItem failed");
		goto exit;
	}
	list_remove(list, val);					//遍历list链表，如果之前有保存该歌曲，则重复了，需将其删掉，然后重新插入
	
	ret = list_insert(list, prev, val);		//将当前的列表歌曲信息添加到链表中
	if(ret < 0) {
		error("list_add failed");
		ret = -1;
		goto exit;
	}
	json = MusicItemListToJson(list);
	if(json) {
		WriteData(jsonFile, json);			//写入json文件
	}
	MusicListToPlaylist(list , m3uFile);	//写入m3u文件中
exit:
	if(list) {
		list_destroy(list);
	}
	if(data)
		free(data);
	if(json)
		free(json);
	info("exit");
	return ret;
}

/* 将MusicItem插入jsonFile中的尾部,同时将url插入到m3uFile. */
int MusicItemlistAdd(char *jsonFile, char *m3uFile, MusicItem *prev , MusicItem *next)
{
	int ret = 0;
	list_t *list = NULL;
	char *data = NULL, *json = NULL;
	list = list_new_free_match(FreeMusicItem, MusicItemCmp);
	if(list == NULL) {
		error("calloc for MusicItem list failed ");
		return -1;
	}
	data = ReadData(jsonFile);
	if(data) {
		ret = MusicItemListParser(list, data);
		if(ret < 0) {
			error("MusicItemListParser failed ");
			ret = -1;
			goto exit;
		}
	}
	MusicItem *val = DupMusicItem(next);

	list_remove(list, val);
	ret = list_add(list, val);
	if(ret < 0) {
		error("list_add failed");
		ret = -1;
		goto exit;
	}
	
	json = MusicItemListToJson(list);
	if(json) {
		WriteData(jsonFile, json);
	}
	
	MusicListToPlaylist(list , m3uFile);
exit:
	if(list) {
		list_destroy(list);
	}
	if(data)
		free(data);
	if(json)
		free(json);
	return ret;
}
/* 根据list每个元素的url写到file文件中. */
int MusicListToPlaylist(list_t *list, char *file)
{	
	int ret = 0;
	FILE *fp = NULL;
	list_iterator_t *it = NULL;
	fp = fopen(file, "w+");
	if(fp == NULL) {
		error("fopen %s filed", file);
		return -1;
	}
	it = list_iterator_new(list, LIST_HEAD);
	if(it == NULL) {
		error("list_iterator_new failed");
		ret = -1;
		goto exit;
	}
	
	while(list_iterator_has_next(it)) {
		MusicItem *item = NULL;
		item = (MusicItem *)list_iterator_next_value(it);
		if(item) {
			debug("item->pContentURL :%s",item->pContentURL);
			fputs(item->pContentURL, fp);
			fputs("\n", fp);
		}
	}
exit:	

	if(fp)
		fclose(fp);
	if(it)
		list_iterator_destroy(it);
	info("exit");
	return ret;
}
/* 显示turingPlayList.json文件中每个元素的信息. */
int   MusicListShow()
{
	int ret = 0;
	char *file = "/usr/script/playlists/turingPlayList.json";
	list_t *list = NULL;
	char *data = NULL, *json = NULL;
	list = list_new_free_match(FreeMusicItem, MusicItemCmp);
	if(list == NULL) {
		error("calloc for MusicItem list failed ");
		return -1;
	}
	data = ReadData(file);
	if(data) {
		ret = MusicItemListParser(list, data);
		if(ret < 0) {
			error("MusicItemListParser failed ");
			ret = -1;
			goto exit;
		}
	}
	list_iterator_t *it = NULL;

	it = list_iterator_new(list, LIST_HEAD);
	if(it == NULL) {
		error("list_iterator_new failed");
		ret = -1;
		goto exit;
	}
	
	while(list_iterator_has_next(it)) {
		MusicItem *item = NULL;
		item = (MusicItem *)list_iterator_next_value(it);
		if(item) {
			error("item:%p",item);
			error("item->pTitle :%s",item->pTitle);
		}
	}

exit:
	if(it)
		list_iterator_destroy(it);

	if(list) {
		list_destroy(list);
	}
	if(data)
		free(data);
	if(json)
		free(json);
	return ret;
}
/* 显示list中每个元素的信息. */
int   MusicListShowList(list_t *list)
{	
	int ret= 0;
	list_iterator_t *it = NULL;

	it = list_iterator_new(list, LIST_HEAD);
	if(it == NULL) {
		error("list_iterator_new failed");
		ret = -1;
		goto exit;
	}
	
	while(list_iterator_has_next(it)) {
		MusicItem *item = NULL;
		item = (MusicItem *)list_iterator_next_value(it);
		if(item) {
			error("item:%p",item);
			error("item->pTitle :%s",item->pTitle);
		}
	}

exit:
	if(it)
		list_iterator_destroy(it);

	return ret;
}


