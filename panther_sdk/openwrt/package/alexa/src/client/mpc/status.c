/*
 * music player command (mpc)
 * Copyright (C) 2003-2015 The Music Player Daemon Project
 * http://www.musicpd.org
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "status.h"
//#include "charset.h"
//#include "util.h"
//#include "mpc.h"

#include <mpd/client.h>

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "../globalParam.h"
#include "../alert/AlertHead.h"

extern GLOBALPRARM_STRU g;
extern AlertManager *alertManager;
timer_t fade_in_timer;		//定义一个定时器，用于发送信号SIGPROF
int count = 0;

int timer_flag = false ;			//判断定时器是否存在的flag, false 为不存在

int intervalInMilliseconds = 0;

#define AMAZON_PLAYLIST_PATH        "/usr/script/playlists/alexaPlayList.m3u"
#define AMAZON_PLAYLIST_JSON_PATH  "/usr/script/playlists/alexaPlayListInfoJson.txt"
extern	int directiveCount;
void
my_finishCommand(struct mpd_connection *conn)
{
	if (!mpd_response_finish(conn))
		printErrorAndExit(conn);
}

static unsigned
elapsed_percent(const struct mpd_status *status)
{
	unsigned total = mpd_status_get_total_time(status);
	if (total == 0)
		return 0;

	unsigned elapsed = mpd_status_get_elapsed_time(status);
	if (elapsed >= total)
		return 100;

	return (elapsed * 100) / total;
}

unsigned g_dwDleayMS = 0;

static int iACount = 0;
volatile static unsigned g_elapsedRec = 0;
pthread_mutex_t elapsedRec_mtx = PTHREAD_MUTEX_INITIALIZER;
unsigned get_elapsedRec(void){
	unsigned elapsed=0;
	pthread_mutex_lock(&elapsedRec_mtx);
	elapsed=g_elapsedRec;
	pthread_mutex_unlock(&elapsedRec_mtx);
	return elapsed;
}
void set_elapsedRec(unsigned elapsed){
	pthread_mutex_lock(&elapsedRec_mtx);
	g_elapsedRec=elapsed;
	pthread_mutex_unlock(&elapsedRec_mtx);
}

int Abs_int ( int num )
{
	return num > 0 ? num : -num;
}


int
print_status(struct mpd_connection *conn)
{
	#ifdef AMAZON_BOOK_FUNCTION
	static char playOverFlag = 0;
	static char playStartCount = 0;
	extern int directiveCount;
    static struct timeval tv;
	#endif
	static unsigned repeat_count = 0;
	static unsigned stutter_elapsedRec = 0;
	static char is_stutter_flag=0;
	
	if (!mpd_command_list_begin(conn, true) ||
	    !mpd_send_status(conn) ||
	    !mpd_send_current_song(conn) ||
	    !mpd_command_list_end(conn))
			return printErrorAndExit(conn);


	struct mpd_status *status = mpd_recv_status(conn);
	if (status == NULL)
		printErrorAndExit(conn);

	if (mpd_status_get_state(status) == MPD_STATE_PLAY ||
	    mpd_status_get_state(status) == MPD_STATE_PAUSE) {
		if (!mpd_response_next(conn)){
			return printErrorAndExit(conn);
		}

		g.m_mpd_status.elapsed_ms = mpd_status_get_elapsed_ms(status);
		g.m_mpd_status.total_time = mpd_status_get_total_time(status) * 1000;
		if(g.m_mpd_status.elapsed_ms){
			set_elapsedRec(g.m_mpd_status.elapsed_ms);
		}

		if (mpd_status_get_state(status) == MPD_STATE_PLAY)
		{
			//printf("[playing]");
			memset(g.m_mpd_status.mpd_state, 0, 10);
			memcpy(g.m_mpd_status.mpd_state, "PLAYING", strlen("PLAYING"));
		}
		else if(mpd_status_get_state(status) == MPD_STATE_PAUSE)
		{
			//printf("[paused] ");
			memset(g.m_mpd_status.mpd_state, 0, 10);
			memcpy(g.m_mpd_status.mpd_state, "STOPPED", strlen("STOPPED"));
			//memcpy(g.m_mpd_status.mpd_state, "PAUSED", strlen("PAUSED"));
		}

	
		if (g.m_mpd_status.elapsed_ms != 0 && 
			g.m_mpd_status.elapsed_ms > g.CurrentPlayDirective.offsetInMilliseconds+1)
		{
			if  (0 == strncmp(g.m_mpd_status.mpd_state, "PLAYING", strlen("PLAYING")))
			{
				memset(&tv,0,sizeof(struct timeval));
				if (AMAZON_AUDIOPLAYER_PLAY == g.m_PlayerStatus)
				{
					g.m_PlayerStatus = AMAZON_AUDIOPLAYER_PLAYING;
	    		    printf("\033[1;35;40mStart play music. send start playing evevnt :%d, total_time:%d\033[0m\n", g.m_mpd_status.intervalInMilliseconds, g.m_mpd_status.total_time);
					printf("%i:%02i\n", mpd_status_get_total_time(status) / 60, mpd_status_get_total_time(status) % 60);

					PrintSysTime("Start Play");
					g_dwDleayMS = g.m_mpd_status.elapsed_ms;
					stutter_elapsedRec = g_dwDleayMS;
					printf(LIGHT_RED" g_dwDleayMS:%d\n"NONE, g_dwDleayMS);

					if (g.CurrentPlayDirective.intervalInMilliseconds != 0 && g.CurrentPlayDirective.offsetInMilliseconds != 0)
					{
						if (g_dwDleayMS >= 900000)
						{
							intervalInMilliseconds = g.CurrentPlayDirective.intervalInMilliseconds - (g_dwDleayMS % 900000);
						}
						else
						{
							intervalInMilliseconds = g.CurrentPlayDirective.intervalInMilliseconds - g_dwDleayMS;
						}
						//intervalInMilliseconds = g.CurrentPlayDirective.intervalInMilliseconds - g.CurrentPlayDirective.offsetInMilliseconds;
						/*if ((g.CurrentPlayDirective.intervalInMilliseconds % 1000) >= 500)
						{
							intervalInMilliseconds += 1000;
						}*/
						SetSendInterValFlag(1);
					}
					else
					{
						intervalInMilliseconds = g.CurrentPlayDirective.intervalInMilliseconds;
					}
					printf(LIGHT_RED"intervalInMilliseconds:%d, offset:%d\n"NONE, intervalInMilliseconds, g.CurrentPlayDirective.offsetInMilliseconds);

					#ifdef AMAZON_BOOK_FUNCTION
					if(directiveCount > 0)
						playStartCount ++ ;

					if(directiveCount >= AMAZON_BOOK_LIST_MAXNUMBER)
						playOverFlag = 1;

					if(playOverFlag == 1)
					{
						DEBUG_PRINTF("###############playOverFlag ==%d,directiveCount==%d\n",playOverFlag,directiveCount);
						if((directiveCount - playStartCount) <= AMAZON_BOOK_LIST_MINNUMBER)
						{
							playOverFlag = 0;
							playStartCount = 0;
							directiveCount = AMAZON_BOOK_LIST_MINNUMBER;
							Queue_Put(ALEXA_EVENT_PLAYBACK_NEARLYFINISHED);
						}
					}
					#endif
					Queue_Put(ALEXA_EVENT_PLAYBACK_STARTED);

					printf(LIGHT_RED"offsetInMilliseconds:%d, delayInMilliseconds:%d, intervalInMilliseconds:%d\n"NONE, 
						g.CurrentPlayDirective.offsetInMilliseconds, g.CurrentPlayDirective.delayInMilliseconds, g.CurrentPlayDirective.intervalInMilliseconds);
				}
				else if (AMAZON_AUDIOPLAYER_PLAYING == g.m_PlayerStatus)
				{
					if(g.m_mpd_status.elapsed_ms == stutter_elapsedRec){
						repeat_count++;
						if(repeat_count >= 5){
							if (0 == is_stutter_flag){
								Queue_Put(ALEXA_EVENT_PLAYBACK_STUTTERSTARTED);
								is_stutter_flag=1;
							}
						}
					}else{
						repeat_count=0;
						stutter_elapsedRec=g.m_mpd_status.elapsed_ms;
						if(is_stutter_flag){
							Queue_Put(ALEXA_EVENT_PLAYBACK_STUTTERFINISHED);
							is_stutter_flag=0;
						}
					}
				
					// 根据播放器进度条上报delay和interval事件
					if (g.CurrentPlayDirective.delayInMilliseconds > 0)
					{
						// 如果当前时间和delay时间zai500ms以为就应该上报数据了
						if ((-500 <= (g_dwDleayMS - g.CurrentPlayDirective.delayInMilliseconds)) ||
							((g_dwDleayMS - g.CurrentPlayDirective.delayInMilliseconds) >= 500))
						{
							PrintSysTime("Send ProgressReportDelayElapsed Event TO AVS");
							Queue_Put(ALEXA_EVENT_PROGRESSREPORT_DELAYELAPSED);
						}
					}

					if (g.CurrentPlayDirective.intervalInMilliseconds > 0)
					{
						
						/*iACount++;
						if (iACount % 2000)
						{
							iACount = 0;
							printf(LIGHT_RED"(g.m_mpd_status.elapsed_ms - g_dwDleayMS)=%d, g.CurrentPlayDirective.intervalInMilliseconds:%d\n", (g.m_mpd_status.elapsed_ms - g_dwDleayMS), g.CurrentPlayDirective.intervalInMilliseconds);
						}*/
						int iValue = (g.m_mpd_status.elapsed_ms - g_dwDleayMS) - intervalInMilliseconds;
						if (-300 <= iValue || iValue >= 300)
						{
							g_dwDleayMS = g.m_mpd_status.elapsed_ms;
							if (1 == GetSendInterValFlag())
							{
								SetSendInterValFlag(0);
								intervalInMilliseconds = g.CurrentPlayDirective.intervalInMilliseconds;
							}

							PrintSysTime("Send IntervalMilliseconds To AVS");
							Queue_Put(ALEXA_EVENT_PROGRESSREPORT_INTERVALELAPSED);
						}
					}
				}
				/*else
				{
					printf("\033[1;35;40mg.m_PlayerStatus error..\033[0m\n");
				}*/
			}
		}
	}
	else if (mpd_status_get_state(status) == MPD_STATE_STOP)
	{
		char byflag = cdb_get_int("$mpd_playing",0);
		// (g_elapseRec < g.m_mpd_status.total_time - 5 * 1000)
		memset(g.m_mpd_status.mpd_state, 0, 10);
		memcpy(g.m_mpd_status.mpd_state, "FINISHED", strlen("FINISHED"));
		if (1 == byflag)
		{
			struct timeval rt_tv;
			gettimeofday(&rt_tv,NULL);
			if(!tv.tv_sec){
				memcpy(&tv,&rt_tv,sizeof(struct timeval));
			}
			if(rt_tv.tv_sec-tv.tv_sec > 42){
				printf(LIGHT_RED"send playbackfailed(mpd_playing)\n"NONE);
				if(is_stutter_flag){
					is_stutter_flag=0;
				}

				Queue_Put(ALEXA_EVENT_PLAYBACK_FAILED);
				cdb_set_int("$mpd_playing",0);   
				//WIFIAudio_UciConfig_SearchAndSetValueString("xzxwifiaudio.config.playmode", "000");
				goto end;
			}			
			// 计时42S，如果超过这个时间则认为播放失败
		}
		else
		{
			// 这里属于正常停止 则认为播放完成
			if (AMAZON_AUDIOPLAYER_PLAYING == g.m_PlayerStatus)
			{
				g.m_PlayerStatus = AMAZON_AUDIOPLAYER_FINISH;
				g_dwDleayMS = 0;
			    printf("\033[1;35;40mplay music finished..\033[0m\n");
				/*if (1 == g_byPlayFinishedFlag)
				{
					g_byPlayFinishedFlag = 0;
					WIFIAudio_UciConfig_SearchAndSetValueString("xzxwifiaudio.config.playmode", "000");
					//submit_PlaybackFinishedEvent();
				}
				else
				{
					//submit_PlaybackNearlyFinishedEvent();
				}*/
				//myexec_cmd("mpc clear");
				//WIFIAudio_UciConfig_SearchAndSetValueString("xzxwifiaudio.config.playmode", "000");
				//int mode = cdb_get_int("$current_play_mode", 4);
				if (AUDIO_STATUS_PLAY == GetPlayStatus())
				{
					Queue_Put(ALEXA_EVENT_PLAYBACK_NEARLYFINISHED);
				}
				else
				{
					Queue_Put(ALEXA_EVENT_PLAYBACK_FINISHED);
				}

			}
	  }
	}
	if (mpd_status_get_update_id(status) > 0)
		printf("Updating DB (#%u) ...\n",
		       mpd_status_get_update_id(status));

	if (mpd_status_get_volume(status) >= 0)
	{
		//printf("volume:%3i%c   ", mpd_status_get_volume(status), '%');
		g.m_mpd_status.volume = mpd_status_get_volume(status);
	}
	else {
		//printf("volume: n/a   ");
		g.m_mpd_status.volume = 0;
	}

	if (mpd_status_get_error(status) != NULL)
	{
		// 先判断一下是不是amazon的歌曲?
		remove(AMAZON_PLAYLIST_PATH);
		remove(AMAZON_PLAYLIST_JSON_PATH);

		printf("\033[1;35;40m ERROR: %s\033[0m\n", mpd_status_get_error(status));

		myexec_cmd("mpc clearerror");

		set_elapsedRec(0);
		Queue_Put(ALEXA_EVENT_PLAYBACK_FAILED);
		//WIFIAudio_UciConfig_SearchAndSetValueString("xzxwifiaudio.config.playmode", "000");		
		SetPlayStatus(AUDIO_STATUS_STOP);
	}


end:
	mpd_status_free(status);

	my_finishCommand(conn);

	return 0;
}


