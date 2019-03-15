#include "WeChatListParser.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <json-c/json.h>


#include "list.h"
#include "list_iterator.h"
#include "myutils/debug.h"


/*  释放WeChatItem */
void FreeWeChatItem(void *data)
{
	WeChatItem *item = (WeChatItem *)data;

	if(item) {
		if(item->url) 
			free(item->url);
		if(item->fromUser) 
			free(item->fromUser);
		if(item->mediaId) 
			free(item->mediaId);
		if(item->relationName) 
			free(item->relationName);
		if(item->tip) 
			free(item->tip);
		free(item);
	}
}
/*  初始化WeChatItem */
WeChatItem* NewWeChatItem(time_t currTime ,char *url, char *fromUser, char *tip, char *relationName, int relationId)   
{

	WeChatItem *item = NULL;
	
	item = calloc(1, sizeof(WeChatItem));
	if(item == NULL) {
		return NULL;
	}
	
	item->url = url;
	item->time = currTime;
	item->fromUser = fromUser;
	item->tip = tip;
	item->relationName = relationName;
	item->relationId = relationId;
	return item;

}
/*  向list中添加的WeChatItem */
int WeChatListAddItem(list_t *list, WeChatItem *item)
{
	int length = 0;

	length = list_length(list);
	info("list add befor length :%d" ,length);
	//如果链表中，微聊语音条数 >8 条后，将删除掉之前存储的微聊语音
	if(length >= 8) {
		list_node_t *node = NULL;
		node = list_lpop(list);		//找出最前面的那条信息，并将其返回
		if(node) {
			FreeWeChatItem(node->val);	//free
			free(node);
		}
	}
	list_add(list,item);	//将微聊语音放入链表中
	length = list_length(list);//获取len的语音条数
	info("list add after length:%d" ,length);
	return 0;
} 

 /* 将list中的WeChatItem解析为json字符 */
char *WeChatListToJson(list_t *list)
{
	char *data = NULL;
	json_object *allLists=NULL;

	list_iterator_t *it = NULL;

	allLists = json_object_new_array();
	if(NULL == allLists) {
		error("json_object_new_array failed");
		return NULL;
	}
	
	it = list_iterator_new(list,LIST_HEAD);

	while(list_iterator_has_next(it )) {
		WeChatItem *item = NULL;
		json_object *val = NULL;
	
		item = (WeChatItem *)list_iterator_next_value(it);

		val = json_object_new_object();
		if(NULL == val) {
			error("json_object_new_object failed");
			return NULL;
		}
		json_object_object_add(val, "time", json_object_new_int64(item->time));
		if(item->url)
			json_object_object_add(val, "url", json_object_new_string(item->url));
		
		if(item->fromUser)
			json_object_object_add(val, "fromUser", json_object_new_string(item->fromUser));
		if(item->tip)
			json_object_object_add(val, "tip", json_object_new_string(item->tip));
		if(item->relationName)
			json_object_object_add(val, "relationName", json_object_new_string(item->relationName));

		json_object_object_add(val, "relationId", json_object_new_int(item->relationId));
		json_object_array_add(allLists,val);
	
	}
	if(it)
		list_iterator_destroy(it);
	if(allLists) {
		data = strdup(json_object_to_json_string(allLists));
		json_object_put(allLists);
	}
	return data;
}


/* 解析json数据中的WeChatItem, 添加的list中 */
int GetWeChatListFromJson(list_t *list, char *p)
{
	int i = 0, err = 0;
	json_object *all;
	int length = 0;
	all = json_tokener_parse(p);
	if (is_error(all)) {
		error("json_tokener_parse failed ");
		return NULL;
	} 	
	length = json_object_array_length(all);
	info("length:%d ", length);
	if(length <= 0 ) {
		error("json_object_array_length failed ");
		err = -1;
		goto exit;
	}
	for ( i = 0; i < length ; i++) {
		time_t time = NULL;
		char *url = NULL ;
		char *fromUser = NULL;
		char *tip = NULL;
		char *relationName = NULL;
		int relationId = 0;
		WeChatItem *item = NULL;
		
		json_object *obj=NULL;
		json_object *timeObj=NULL ,*urlObj=NULL, *fromObj = NULL;
		json_object *tipObj=NULL ,*relationIdObj=NULL, *relationNameObj = NULL;
		obj = json_object_array_get_idx(all, i);  
		info("obj:%s",json_object_to_json_string(obj));
		timeObj		= json_object_object_get(obj , "time");
		urlObj 		= json_object_object_get(obj , "url");
		fromObj     = json_object_object_get(obj , "fromUser");
		relationNameObj =  json_object_object_get(obj , "relationName");
		relationIdObj =  json_object_object_get(obj , "relationId");
		tipObj = json_object_object_get(obj , "tip");
		if(timeObj)
			time = json_object_get_int64(timeObj);
		info("time:%ld",time);
		if(urlObj)
			url	= strdup((char *)json_object_get_string(urlObj));
		info("url:%s",url);
		if(tipObj)
			tip	= strdup((char *)json_object_get_string(tipObj));
		info("tip:%s",tip);
		if(fromObj) {
			fromUser = strdup((char *)json_object_get_string(fromObj));
		}
		if(relationNameObj)
			relationName = strdup((char *)json_object_get_string(relationNameObj));
		info("relationName:%s",relationName);
		if(relationIdObj)
			relationId =json_object_get_int(relationIdObj);
		
		info("time:[%ld] , url:[%s]",time, url);
		item = NewWeChatItem(time, url, fromUser, tip, relationName, relationId);
		list_add(list, item);		//添加到链表中
	}

exit:

	if(all) {
		json_object_put(all);
		all = NULL;
	}
	return err;
	
}



