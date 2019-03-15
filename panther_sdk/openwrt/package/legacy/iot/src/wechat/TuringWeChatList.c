
#include "TuringWeChatList.h"
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <assert.h>

#include "list.h"

#include "myutils/debug.h"
#include "FileUtil.h"

#define WECHAT_LIST_FILE "/etc/config/wechat.json"

static list_t *weChatList = NULL;

/*
 * 初始化weChatList
 */
int TuringWeChatListInit()
{
	int ret = 0;
	if(weChatList) {
		list_destroy(weChatList);
		weChatList = NULL;
	}
	warning("list_new_free");
	weChatList = list_new_free(FreeWeChatItem); //yes
	if(weChatList == NULL)
	{
		error("calloc for WeChatList failed ");
		return -1;
	}
	warning("ReadData");
	char *data = ReadData(WECHAT_LIST_FILE);
	if(data) {
		error("data:%s", data);
		ret = GetWeChatListFromJson(weChatList, data);
		free(data);
	}	
	return ret;
}
/* 向weChatList添加WeChatItem. */
int TuringWeChatListAddItem(WeChatItem *item)
{
	char *data = NULL;
	int ret = 0;
	assert(weChatList != NULL);

	WeChatListAddItem(weChatList, item);

	data = WeChatListToJson(weChatList);
	if(data) {
		info("data:%s", data);
		ret = WriteData(WECHAT_LIST_FILE, data);
		free(data);
	}
	return 0;
	
}

/* 向weChatList添加WeChatItem. */
//curr:当前时间		 urlStr:url链接	  tiptr:NULL	relationNameStr:NULL	relationIdInt:0
int TuringWeChatListAdd(time_t currTime ,char *url, char *fromUser, char *tip, char *relationName, int relationId)   
{
	char *data = NULL;
	int ret = 0;
	assert(weChatList != NULL);
	WeChatItem *item = NULL;
	item = NewWeChatItem(currTime,  url, fromUser, tip, relationName, relationId);
	if(item == NULL)
		return -1;
	//static list_t *weChatList = NULL
	WeChatListAddItem(weChatList, item);		//将歌曲信息放入微聊链表中

	data = WeChatListToJson(weChatList);
	if(data) {
		info("data:%s", data);
//
		ret = WriteData(WECHAT_LIST_FILE, data);
		free(data);
	}
	return 0;
	
}

/* 释放weChatList. */
void    TuringWeChatListDeinit()
{
	if(weChatList) {
		list_destroy(weChatList);
		weChatList = NULL;
	}
}

/* 播放weChatList最后一个WeChatItem. */
int TuringWeChatListPlay(void *pv)
{
	char *url = NULL;
	char *tip = NULL;
	int ret = -1;
	int i;
	int relationId;
	
	if(weChatList) {
		info("weChatList != NULL");
		WeChatItem *item = NULL;
		list_node_t *node = list_at(weChatList, -1);	//获取最后一条微信语音消息
		if (node) {
			ret = 0;
			item = (WeChatItem *)node->val;
			url = item->url;
			tip = item->tip;
			relationId = item->relationId;
			info("item->url:%s", item->url);
			info("item->relationId:%d", item->relationId);
			if(url) {
				int wanif_state = 0;
				wanif_state = cdb_get_int("$wanif_state", 0);
				if(wanif_state != 2) {
					wav_play2("/tmp/res/speaker_not_connected.wav");
					return 0;
				}
				unlink("/root/chat.raw");
				unlink("/root/tip.raw");
				HttpDownLoadFile(url,"/tmp/chat.amr");	//去http服务器上下载"url"的链接
				amr_dec("/tmp/chat.amr", "/root/chat.raw");//转换格式
				if(tip) {
					HttpDownLoadFile(tip,"/tmp/tip.amr");
					amr_dec("/tmp/tip.amr", "/root/tip.raw");
				}
				eval("mpc", "volume", "101");
				if(relationId != 0) {		
					eval("aplay", "-r", "8000", "-f" , "s16_le", "-c" ,"1", "/root/tip.raw");		
				} 
				eval("aplay", "-r", "8000", "-f" , "s16_le", "-c" ,"1", "/root/chat.raw");	
				ret = 0;
				eval("mpc", "volume", "200");
			
			}
		}
	}
	return ret;
}



