#ifndef __WECHAT_LIST_PARSER_H__

#define  __WECHAT_LIST_PARSER_H__

#include "common.h"

#include "list.h"

typedef struct weChatItem {
	time_t time;
	int relationId;
	char *url;
	char *mediaId;
	char *fromUser;
	char *tip;
	char *relationName;
}WeChatItem;


void FreeWeChatItem(void *data);

WeChatItem* NewWeChatItem(time_t currTime ,char *url, char *fromUser, char *tip, char *relationName, int relationId);
int GetWeChatListFromJson(list_t *list, char *p);
int WeChatListAddItem(list_t *list, WeChatItem *item);
char *WeChatListToJson(list_t *list);








#endif




