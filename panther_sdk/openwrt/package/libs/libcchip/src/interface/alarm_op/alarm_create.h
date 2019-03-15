#ifndef __ALARM_CREATE_H__
#define __ALARM_CREATE_H__


#define ALARM_THREAD_NAME				"alarm" 
#define CREATE_ALARM					"mixedmusic"
#define DELETE_ALARM					"rmmusic"

#define GET_ALARM_INFO_PATH				"usr/script/playlists/alarm_list.json"


typedef struct _alarm_table
{
	char 	num;
	char 	state;
	char 	hour;
	char 	minute;
	int 	second;
	char 	wday;
	char 	*url;
	char 	*title;
	char 	*artist;
	char 	*album;
	char 	*album_pic;
	char 	*platform_name;
	int 	column;
	int 	program;
	
}alarm_table;




/*********************************************************************
*Desc	:	创建音箱闹钟--专辑单曲混合列表模式
*Param	:	NumID--闹钟编号;State--闹钟状态; Hour--小时
			Min--分钟;Wday--星期; UrlAddr--混合列表路径
*Note	:	混合列表默认路径(/tmp/PlayMixedListJson.json)
*Ret	:	0--成功；-1--失败
*Author	: 	xudh 2018-04-13
************************************************************************/
extern int CreateMusicAlarm(int NumID, int State, int Hour, \
						int Min, int Wday, char *UrlAddr);


/*********************************************************************
*Desc	:	删除音箱闹钟--专辑单曲混合列表
*Param	:	NumID--闹钟编号
*Ret	:	0--成功；-1--失败
*Author	: 	xudh 2018-04-13
************************************************************************/
extern int DeleteMusicAlarm(int NumID);

/***************************************************************
*Desc	:	查询设置音箱闹钟列表信息
*Param	:	void
*Ret	:	返回查询音箱闹钟列表地址 
			(usr/script/playlists/alarm_list.json)
*Author	: 	xudh 2018-04-13
*****************************************************************/
extern char *GetMusicAlarmInfo(void);



#endif 
