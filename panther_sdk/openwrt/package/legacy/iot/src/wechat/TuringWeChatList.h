#ifndef __TURING_WECHAT_LIST__

#define __TURING_WECHAT_LIST__

#include "WeChatListParser.h"

extern void    TuringWeChatListDeinit();
extern int TuringWeChatListInit();
extern int TuringWeChatListPlay(void *pv);
extern int TuringWeChatListAddItem(WeChatItem *item);
extern int TuringWeChatListAdd(time_t currTime ,char *url, char *fromUser, char *tip, char *relationName, int relationId);
#endif