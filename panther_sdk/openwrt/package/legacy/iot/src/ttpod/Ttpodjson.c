
#include "TtpodJson.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <json/json.h>

#include "SearchMusic.h"


#if 1
int get_search_style_and_search_key(json_object *obj,search_style_t *p_style,search_key_t *p_key)
{
	json_object *semanticIndex = NULL,*solts= NULL,*soltsIndex= NULL, *nameObj= NULL,*valueObj= NULL,*slots_obj= NULL,*category_obj= NULL;	
	int length, i;
	if(NULL == obj) {
		error("json_object_object_get get semantic failed");
		return -1;
	}		
	semanticIndex = json_object_array_get_idx(obj, 0);
	
	if(NULL == semanticIndex) {
		error("json_object_object_get semanticIndex failed");
		return -1;
	}
	solts = 	json_object_object_get(semanticIndex,"slots");
	if(NULL == solts) {
		error("json_object_object_get slots failed");
		return -1;
	}
	length = json_object_array_length(solts);
	if(length <= 0 )
		return -1;
	for(i = 0; i< length ; i++) {
		char *name, *value;
		soltsIndex = json_object_array_get_idx(solts, i);
		nameObj = json_object_object_get(soltsIndex, "name");
		valueObj = json_object_object_get(soltsIndex, "value");
		if(nameObj && valueObj) {
			name = json_object_get_string(nameObj);
			if(strcmp(name,"artist") == 0){
				p_key->song_key.singer_name =  json_object_get_string(valueObj);
			} else if(strcmp(name,"song") == 0) {
				p_key->song_key.music_name =  json_object_get_string(valueObj);
			}
		}
	}
	return 0;	
}

#else
int get_search_style_and_search_key(json_object *obj,search_style_t *p_style,search_key_t *p_key)
{
	struct json_object *semantic_obj,*artist_obj,*song_obj,*slots_obj,*category_obj;
	semantic_obj = json_object_object_get(obj, "semantic");		
	if(NULL == semantic_obj) {
		error("json_object_object_get get semantic_obj failed");
		return -1;
	}		
	slots_obj = json_object_object_get(semantic_obj, "slots");
	if(NULL == slots_obj) {
		error("json_object_object_get slots_obj failed");
		return -1;
	}
	category_obj = 	json_object_object_get(slots_obj,"category");
	if(NULL != category_obj){
		(p_key->sence_key) = json_object_get_string(category_obj);
		*p_style=LIST_SEARCH;
		error("(p_key->sence_key) =%s", (p_key->sence_key) ); 
		return 0;			
	}	
	artist_obj = json_object_object_get(slots_obj, "artist"); 
	if(NULL != artist_obj) {
		(p_key->song_key.singer_name) = json_object_get_string(artist_obj);
		error("(p_key->song_key.singer_name) =%s",(p_key->song_key.singer_name)); 					
	}		
	song_obj = json_object_object_get(slots_obj, "song"); 
	if(NULL != song_obj) {
		(p_key->song_key.music_name) = json_object_get_string(song_obj);
		error("(p_key->song_key.music_name)=%s",(p_key->song_key.music_name)); 
	}
	*p_style=SINGLE_SEARCH;
	return 0;	
}
#endif
int parse_service_music_name_signer(json_object *obj,  char **category, const char **music_name, const char **signer)
{
		struct json_object *semantic_obj,*artist_obj,*song_obj,*slots_obj,*category_obj;
		semantic_obj = json_object_object_get(obj, "semantic");
		
		if(NULL == semantic_obj) {
			error("json_object_object_get get semantic_obj failed");
			return -1;
		}		
		slots_obj = json_object_object_get(semantic_obj, "slots");
		if(NULL == slots_obj) {			
			error("json_object_object_get slots_obj failed");
			return -1;
		}
		category_obj = 	json_object_object_get(slots_obj,"category");
		if(NULL != category_obj){
			*category = json_object_get_string(category_obj);
			error("category=%s", *category); 
			return 0;			
		}
		artist_obj = json_object_object_get(slots_obj, "artist"); 
		if(NULL != artist_obj) {
			*signer = json_object_get_string(artist_obj);
			error("singer=%s", *signer); 
					
		}
		song_obj = json_object_object_get(slots_obj, "song"); 
		if(NULL != song_obj) {
			*music_name =	json_object_get_string(song_obj);
			error("song=%s", *music_name); 
		}
		return 0;
}






DEFINE_SCENE_CMD_TABLE//该宏在头文件中实现 scene_cmd_table 的定义

int parse_json(char *json_str)
{
		int ret = 0 ;
		json_object *root=NULL,*service_entry=NULL,*rc_entry=NULL,*text_obj=NULL;
		json_object *semantic=NULL;
		int rc;
		const char *service=NULL,*text=NULL;
		char *music_name=NULL ,*singer_name=NULL,*scene_key=NULL;
		char *name =NULL;
		root = json_tokener_parse(json_str);
		if (is_error(root)) {
				error("json_tokener_parse failed");
				return -1;
		}	
	
		rc_entry = json_object_object_get(root, "rc");
		if(rc_entry == NULL) {
				ret = -1;
				error("json_object_object_get root failed");
				json_object_put(root);
				return ret;
		}

		text_obj = json_object_object_get(root, "text");
		if(text_obj != NULL) {
				text = json_object_get_string(text_obj);
		}
		
		rc = json_object_get_int(rc_entry);
		
		if (rc == 4 ) {
				ret = -1;
				json_object_put(root);
				return ret;
		} else if ( rc != 0) {
				ret = -1;
				json_object_put(root);
				return rc;
		}
		
		service_entry = json_object_object_get(root, "service");
		if(service_entry == NULL) {
				ret = -1;
				error("json_object_object_get root service failed");
				json_object_put(root);
				return ret;
		}
		
		service = json_object_get_string(service_entry);
		error("service:%s",service);
		if(!strcmp(service, "musicX")) 
		{ //音乐
			semantic = json_object_object_get(root, "semantic");
			search_style_t style={0};
			search_key_t key={0};
			ret=get_search_style_and_search_key(semantic,&style,&key);			
			if(0 == ret){
				char *list_string=NULL;
				warning("artist:%s", key.song_key.singer_name);
				warning("song:%s", key.song_key.music_name);
				list_string=get_song_list_string(&style,NULL,&key);
				warning("song list_string:%s,%d",list_string,strlen(list_string));
				if(strlen(list_string)>EFECTTIVE_LENTH)
				{
					//将列表中的每一首歌添加到播放列表，并从第一首开始播放
					ret=add_every_song_to_playlist(&style,list_string);
					error(" ret :%d",ret);
					HEAP_FREE(list_string);
					error(" HEAP_FREE");
				}
				else
				{
					ret=-1;
				}
			}
		}
		else 
		{
			 ret = -1;
			 error("ret= %d",ret);
		}
		json_object_put(root);
		return ret;		
}


