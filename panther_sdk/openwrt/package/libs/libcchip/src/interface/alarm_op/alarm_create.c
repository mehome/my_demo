#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <ctype.h>
#include <unistd.h>
#include <signal.h>
//#include "../../../function/log/slog.h"
#include <function/log/slog.h>
#include <function/common/misc.h>
#include "alarm_create.h"

static pthread_mutex_t alarm_mtx = PTHREAD_MUTEX_INITIALIZER;

/*********************************************************************
*Desc	:	创建音箱闹钟--专辑单曲混合列表
*Param	:	NumID--闹钟编号;State--闹钟状态; Hour--小时
			Min--分钟;Wday--星期; UrlAddr--混合列表路径
*Note	:	混合列表默认路径(/tmp/PlayMixedListJson.json)
*Ret	:	0--成功；-1--失败
*Author	: 	xudh 2018-04-13
************************************************************************/
int create_music_alarm(int NumID, int State, int Hour, \
						int Min, int Wday, char *UrlAddr)
{
	
	if(NumID < 0 || State < 0|| Hour < 0 || Min < 0 || Wday < 0 || UrlAddr == NULL)
	{
		err("param is null!\n");
		return -1;
	}
	
	pthread_mutex_lock(&alarm_mtx);
	
	if(my_popen("alarm mixedmusic %d %d %d %d %d %s", NumID, State, Hour, Min, Wday, UrlAddr) < 0)
	{
		pthread_mutex_unlock(&alarm_mtx);
		
		err("param is null!\n");
		return -1;	
	}
		
	pthread_mutex_unlock(&alarm_mtx);
	
	return 0;
}

/*********************************************************************
*Desc	:	删除音箱闹钟--专辑单曲混合列表
*Param	:	NumID--闹钟编号
*Ret	:	0--成功；-1--失败
*Author	: 	xudh 2018-04-13
************************************************************************/
int delete_music_alarm(int NumID)
{
	if(NumID < 0)
	{
		err("param is null!\n");
		return -1;
	}
	
	pthread_mutex_lock(&alarm_mtx);
	
	if(my_popen("alarm rmmusic %d", NumID) < 0)
	{
		pthread_mutex_unlock(&alarm_mtx);
		
		err("param is null!\n");
		return -1;
	}
	
	pthread_mutex_unlock(&alarm_mtx);
	
	return 0;
}

/***************************************************************
*Desc	:	获取设置音箱闹钟列表信息
*Param	:	void
*Ret	:	返回查询音箱闹钟列表地址 
			(usr/script/playlists/alarm_list.json)
*Author	: 	xudh 2018-04-13
*****************************************************************/
char *get_music_alarm_info(void)
{
	return GET_ALARM_INFO_PATH;
}




