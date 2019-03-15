
#include <json-c/json.h>
#include <curl/curl.h>

#include "myutils/debug.h"
#include "SearchMusic.h"

#define TMP_FILE "/tmp/tmp.txt"



static size_t write_callback(void *ptr, size_t size, size_t nmemb, void *stream) {
    int len = size * nmemb;
    int written = len;
    FILE *fp = NULL;
    if (written == 0) 
    	return 0;
	printf("size:%d,nmemb:%d\n",size,nmemb);// size:1,nmemb:1247
	printf("stream---------:%s\n",stream);// /tmp/url_tmp_file.txt
	printf("ptr--------:%s\n",(char *)ptr);
    if (access((char*) stream, 0) == -1) {
        fp = fopen((char*) stream, "wb");
    } else {
        fp = fopen((char*) stream, "ab");
    }
    if (fp) {
        fwrite(ptr, size, nmemb, fp);
    }

    fclose(fp);
    return written;
}

static char *curl_get(char* url)
{
	CURL *curl;
	CURLcode res;
	long length = 0;

	char * response = NULL;
	unlink(TMP_FILE);
	curl = curl_easy_init();
	if (curl)
	{
		curl_easy_setopt(curl, CURLOPT_URL, url);
		curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, TMP_FILE);
		res = curl_easy_perform(curl);
		curl_easy_cleanup(curl);
		if (CURLE_OK == res) {
			length = getFileSize(TMP_FILE);
			info("length:%ld",length);
			response = calloc(1, length+2);
			if(response == NULL) {
				error("calloc failed");
				return NULL;
			}
			FILE *fp = NULL;
			fp = fopen(TMP_FILE, "r");
			if (fp) {
				int read=0;
				if( (read=fread(response, 1, length, fp)) != length)
				{
					info("fread failed read:%d",read);
					if(response)
						free(response);
					fclose(fp);
					return NULL;
				}
				fclose(fp);
				info("fread ok read:%d",read);
			} else {
				if(response)
					free(response);
				info("fopen failed");
				return NULL;
			}  
		}
	}	
	return response;
}
DEFINE_SCENE_CMD_TABLE//该宏在头文件中实现 scene_cmd_table 的定义
char* get_sence_name(char *sence_key){
	int item_sence= (sizeof(scene_cmd_table)/sizeof(scene_item_t));//scene_cmd_table 定义由宏：DEFINE_SCENE_CMD_TABLE 实现
	int i=0;
	for(i=0;i<item_sence-1;i++){
		if(0==strcmp(sence_key,scene_cmd_table[i].key)){
			return (scene_cmd_table[i].name);
		}		
	}
	return scene_cmd_table[item_sence-1].name;
}

char* GetSenceName(char *asr)
{
	int item_sence= (sizeof(scene_cmd_table)/sizeof(scene_item_t));//scene_cmd_table 定义由宏：DEFINE_SCENE_CMD_TABLE 实现
	int i=0;
	for(i=0;i<item_sence-1;i++){
		if(strstr(asr, scene_cmd_table[i].key)) {
			return scene_cmd_table[i].key;
		}
	
	}
	return NULL;
}

char* get_song_list_string(search_style_t *p_style,char *scene_name,search_key_t *p_key)
{
	char buf[512]={0};	
	if(p_key->sence_key) {
		*p_style = LIST_SEARCH;
	}
	
	if(*p_style == SINGLE_SEARCH){//单曲
		const char *url_com = "http://iapi.erduoav.com:18080/api/v1/searchsongs?key=";
		const char *url_ctrl = "&limit=10&page=1";
		if(p_key->song_key.music_name && p_key->song_key.singer_name)
			snprintf(buf,sizeof(buf),"%s%s+%s%s",url_com,p_key->song_key.music_name,p_key->song_key.singer_name,url_ctrl);
		else if (p_key->song_key.music_name)
			snprintf(buf,sizeof(buf),"%s%s%s",url_com,p_key->song_key.music_name,url_ctrl);
		else if (p_key->song_key.singer_name)
			snprintf(buf,sizeof(buf),"%s%s%s",url_com,p_key->song_key.singer_name,url_ctrl);
		
	}else if(*p_style == LIST_SEARCH){//列表
		const char *url_com = "http://iapi.erduoav.com:18080/api/v1/scenesongs?name=";
		const char *url_ctrl = "&limit=100&page=1";	
		if(scene_name){
			snprintf(buf,sizeof(buf),"%s%s%s",url_com,scene_name,url_ctrl);
		}else{
			snprintf(buf,sizeof(buf),"%s%s%s",url_com,get_sence_name(p_key->sence_key),url_ctrl);
		}
	}
	error("first curl buf:%s",buf);
	return curl_get(buf);
	
}

static char* decrypt_playurl(char* crypto_url)
{
	char byUrlBuf[512] = {0};
	sprintf(byUrlBuf, "http://iapi.erduoav.com:18080/decryptplayurl?key=%s", crypto_url);
	info(" byUrlBuf:%s", byUrlBuf);
	char *pJsonStr = curl_get(byUrlBuf);
	if (NULL == pJsonStr){
		return NULL;
	}
	info("pJsonStr:%s", pJsonStr);
	return pJsonStr;
}

int add_one_song_to_playlist(song_key_t *key, search_style_t* p_style,const char *json_str,int* p_add_count)
{
	const char *url = NULL;
	const char *author = NULL;
	const char *album_title = NULL;
	const char *pic_url = NULL;
	const char *title = NULL;
	const char *id = NULL;
	json_object *root=NULL, *bitrate=NULL, *show_link=NULL, *date=NULL;
	json_object *title_obj=NULL, *author_obj=NULL, *album_title_obj=NULL, *pic_url_obj=NULL,*play_url_obj=NULL;
	json_object *id_obj=NULL;
	int ret = -1;
	root = json_tokener_parse(json_str);
	if (is_error(root)){
		error("json_tokener_parse failed");
		return -1;
	}
	
	info("one json_str:%s",json_object_to_json_string(root));		
	date = json_object_object_get(root, "data");
	if(is_error(date)) {//没有找到data字段,可能是为没有加密的歌曲信息
		//DEBUG_INFO("one json_str:%s",json_object_to_json_string(root));
		date=root;
	}
	play_url_obj=json_object_object_get(date, "playUrl");
	if(is_error(play_url_obj)) {//没找到playUrl字段,无效的歌曲信息
		json_object_put(root);	
		return -1;
	}
	url = json_object_get_string(play_url_obj);
	info("play_url:%s",url);	
	author_obj = json_object_object_get(date, "artist");
	if(!is_error(author_obj)) {
		author = json_object_get_string(author_obj);
		info("author:%s",author);
	}
	album_title_obj = json_object_object_get(date, "album");
	if(!is_error(album_title_obj)) {
		album_title = json_object_get_string(album_title_obj);
		info("album_title:%s",album_title);
	}
	pic_url_obj = json_object_object_get(date, "artworkUrl");
	if(!is_error(pic_url_obj)) {
		pic_url = json_object_get_string(pic_url_obj);
		info("pic_url:%s",pic_url);
	}
	title_obj= json_object_object_get(date, "title");
	if(!is_error(title_obj)) {
		title = json_object_get_string(title_obj);
		info("title:%s",title);
	}

	id_obj= json_object_object_get(date, "id");
	if(id_obj) {
		id  = json_object_get_string(id_obj);
	} else {
		id= "null";
	}
	if(author == NULL||  strcmp(author, "")== 0) {
		author = "null";
	}
	if(album_title == NULL ||  strcmp(album_title, "")== 0) {
		album_title = "null";
	}
	if(title == NULL ||  strcmp(title, "")== 0) {
		title = "null";
	}
	if(pic_url == NULL || strcmp(pic_url, "")== 0) {
		pic_url = "null";
	}
	if(key->music_name && key->singer_name == NULL) {
		if(strcmp(key->music_name, title)) {
			ret = -1;
			goto exit;
		}	
	} else if(key->singer_name  && key->music_name == NULL)  {
		if(strcmp(key->singer_name, author)) {
			ret = -1;
			goto exit;
		}
	} else if(key->singer_name  && key->music_name )  {
		if(strcmp(key->singer_name, author) || strcmp(key->music_name, title)) {
			ret = -1;
			goto exit;
		}
	}
	if(url != NULL) 
	{
		
		char buf[2048]={0};	
		if(!(*p_add_count))
		{			
			//snprintf(buf, sizeof(buf), "即将播放%s的%s",author, title);
			//text_to_sound(buf, "/tmp/answer.wav");
			//memset(buf,0,sizeof(buf));
				
		}
		if(*p_style==LIST_SEARCH)
		{
			if(!(*p_add_count))
			{			
				//playlist_clear();
			}
			error("url:%s title:%s", url,title);
			
			snprintf(buf, 2048, "creatPlayList iot \"%s\" \"%s\" \"%s\" null \"%s\" \"%s\" \"%s\" 2 symlink", url,title,author,id,album_title,pic_url);
			exec_cmd(buf);	
			if(!(*p_add_count)) {
				TuringPlaylistPlay(url);
			}		
		}
		else
		{
			error("url:%s title:%s", url,title);
			snprintf(buf, 2048, "creatPlayList iot \"%s\" \"%s\" \"%s\" null \"%s\" \"%s\" \"%s\" 2 symlink", url,title,author,id,album_title,pic_url);
			exec_cmd(buf);
			TuringPlaylistPlay(url);
			//MpdClear();
			//MpdAdd(url);
			//MpdPlay(-1);
		}			
		(*p_add_count)++;		
		ret=0;	
	}
	else 
	{
		ret = -1;
	}
exit:
	json_object_put(root);	
	return ret;
}

int add_every_song_to_playlist(song_key_t * key, search_style_t *p_style,char *pJsonStr)
{
	int  ret =-1,i=0, times = 0, dateLength,add_count=0;
	const char *encrypt_str,*pDecryptJsonStr, *singer,*crypto_url;
	const struct json_object *root, *song, *index, *song_url, *encrypt;

	root = json_tokener_parse(pJsonStr);
	if (is_error(root)){
		error("json_tokener_parse failed");
		return -1;
	}	
	error("pJsonStr:%s", json_object_to_json_string(root));
	//json_object_put(root);
	//return 0;
	// 从json中按名字取date对象。
	song = json_object_object_get(root, "data");
	if(is_error(song)) {
		error("json_object_object_get data failed");
		return -1;
	}
	// 获取date数组的长度
	dateLength =  json_object_array_length(song);
	info("dateLength:%d", dateLength);
	if(key->music_name)
	{
		for (i = 0; i < dateLength; i++) {
			// 从数组中，按下标取JSON值对象。
			index = json_object_array_get_idx(song, i);
			if(is_error(index)) {
				error("json_object_array_get_idx %d failed", i);
				return -1;
			}
			encrypt = json_object_object_get(index, "playUrlEncrypt");
			if (is_error(encrypt)){
				error("encrypt == NULL");
			    continue;
			}
			encrypt_str = json_object_get_string(encrypt);
			info(" encrypt_str:%s", encrypt_str);
			if(0 == strcmp(encrypt_str,"true")){//如果playUrl加密
				song_url = json_object_object_get(index, "playUrl");
				if (is_error(song_url)){
					error("song_url == NULL");
				    continue;
				}
				crypto_url = json_object_get_string(song_url);		
				//解密请求
				pDecryptJsonStr = decrypt_playurl(crypto_url);
				info(" pDecryptJsonStr:%s", pDecryptJsonStr);	
				if(NULL == pDecryptJsonStr){
					continue;
				}
			}else if(0 == strcmp(encrypt_str,"false")){
				pDecryptJsonStr=compat_strdup(json_object_get_string(index));
			}else{
				continue;
			}
			//添加到播放列表
			ret = add_one_song_to_playlist(key, p_style, pDecryptJsonStr, &add_count);
			if(ret== 0 && *p_style== SINGLE_SEARCH )
				break;
			FREE(pDecryptJsonStr);
			error(" add_count:%d ret=%d", add_count,ret);
			if(add_count > 0){
				ret = 0;
			}
		}
	}
	else 
	{
			int count = 0;
			while(count < dateLength) {
			srand((unsigned)time(NULL));
			i = rand()%dateLength;
			count++;
			index = json_object_array_get_idx(song, i);
			if(is_error(index)) {
				error("json_object_array_get_idx %d failed", i);
				return -1;
			}
			encrypt = json_object_object_get(index, "playUrlEncrypt");
			if (is_error(encrypt)){
				error("encrypt == NULL");
			    continue;
			}
			encrypt_str = json_object_get_string(encrypt);
			info(" encrypt_str:%s", encrypt_str);
			if(0 == strcmp(encrypt_str,"true")){//如果playUrl加密
				song_url = json_object_object_get(index, "playUrl");
				if (is_error(song_url)){
					error("song_url == NULL");
				    continue;
				}
				crypto_url = json_object_get_string(song_url);		
				//解密请求
				pDecryptJsonStr = decrypt_playurl(crypto_url);
				info(" pDecryptJsonStr:%s", pDecryptJsonStr);	
				if(NULL == pDecryptJsonStr){
					continue;
				}
			}else if(0 == strcmp(encrypt_str,"false")){
				pDecryptJsonStr=compat_strdup(json_object_get_string(index));
			}else{
				continue;
			}
			//添加到播放列表
			ret = add_one_song_to_playlist(key, p_style,pDecryptJsonStr,&add_count);
			if(ret== 0 && *p_style== SINGLE_SEARCH )
				break;
			FREE(pDecryptJsonStr);
			error(" add_count:%d ret=%d", add_count,ret);
			if(add_count > 0){
				ret = 0;
			}
		}
	
	}

	json_object_put(root);
	return ret;
}


int SearchPopularMusic(search_key_t *key)
{
	int ret;
	search_style_t style={0};
	char *list_string=NULL;
	char *data = NULL;
	warning("music_name:%s,singer_name:%s",key->song_key.music_name,key->song_key.singer_name);
	list_string=get_song_list_string(&style,NULL,key);
	warning("song list_string:%s,[%d]",list_string,strlen(list_string));
	if(strlen(list_string)>EFECTTIVE_LENTH)
	{
		//将列表中的每一首歌添加到播放列表，并从第一首开始播放
		data = SkipToBraces(list_string);
		warning("data:%s,[%d]",data,strlen(data));
		ret=add_every_song_to_playlist(&(key->song_key) ,&style,data);
		error(" ret :%d",ret);
		HEAP_FREE(list_string);
		error(" HEAP_FREE");
	}
	else
	{
		ret=-1;
	}

	return ret;
}



