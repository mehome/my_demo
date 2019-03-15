#ifndef __STARTINGUP_PLAY_H__
#define __STARTINGUP_PLAY_H__


#define STARTINGUP_THREAD_NAME				"uartdfifo.sh" 
#define ENABLE_STARTINGUP					"autoplay_open"
#define DISABLE_STARTINGUP					"autoplay_close"


/*********************************************************************
*Desc	:	打开/关闭 启动播放音乐
*Param	:	State--0:Disable开机播放音乐;1--Enable开机播放音乐
*Ret	:	0--成功；-1--失败
*Author	: 	xudh 2018-04-13
************************************************************************/
int StartingupPlayState(int State);


#endif 
