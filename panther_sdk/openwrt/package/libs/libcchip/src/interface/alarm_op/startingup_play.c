#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <ctype.h>
#include <unistd.h>
#include <signal.h>
#include <function/log/slog.h>
#include <function/common/misc.h>

#include "startingup_play.h"

/*********************************************************************
*Desc	:	打开/关闭 启动播放音乐
*Param	:	State--0:Disable开机播放音乐;1--Enable开机播放音乐
*Ret	:	0--成功；-1--失败
*Author	: 	xudh 2018-04-13
************************************************************************/
int startingup_play_state(int State)
{
	if(State)
	{
		if(my_popen("uartdfifo.sh autoplay_open") < 0)
		{
			err("param is null!\n");
			return -1;			
		}
	}
	else
	{
		if(my_popen("uartdfifo.sh autoplay_close") < 0)
		{
			err("param is null!\n");
			return -1;	
		}
	}
	
	return 0;
}

int enable_shutdown_set(int Time)
{
	if(Time <= 0)
	{
		err("param is null!\n");
		return -1;
	}
	
	if(my_popen("powoff.sh enable") < 0)
	{
		err("my_popen() error!\n");
		return -1;	
	}

	if(my_popen("powoff.sh settime %d ",Time) < 0)
	{
		err("my_popen() error!\n");
		return -1;	
	}
	
	return 0;
}

int disable_shutdown_set(void)
{
	if(my_popen("powoff.sh disable") < 0)
	{
		err("my_popen() error!\n");
		return -1;	
	}
}

int get_shutdown_remain_time(void)
{
	//cdb获取变量 
    //cdb get powoff_gettime(单位:秒)
	
	int time = 0;
    time = cdb_get_int("$powoff_gettime", 0);
	if(time < 0)
		fprintf(stderr, "time:%d err!\n", time);
	
	return time;
	
}


