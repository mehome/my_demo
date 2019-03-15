#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>

#include "../debug.h"
#include "madDecoder.h"
#include "../globalParam.h"
#include "../alert/AlertHead.h"
#include "../wavplay/wav_play.h"

extern AlertManager *alertManager;

extern FT_FIFO * g_DecodeAudioFifo;
extern FT_FIFO * g_PlayAudioFifo;
extern GLOBALPRARM_STRU g;
extern char g_byWakeUpFlag ;

extern char g_playFlag;

#define DECODE_LENGTH 16384 * 2

pthread_t Mp3player_Pthread;

void *Mp3player_pthread(void)
{
	int iLength = 0;
	char *pAudioData = NULL;

	char byCount = 0;

	char byFlag = 0;

	while (1)
	{
	    if (0 == sem_wait(&g.Semaphore.MadPlay_sem))
	    {
			if (NULL == pAudioData)
				pAudioData = malloc(DECODE_LENGTH);

			OnWriteMsgToUartd(ALEXA_IDENTIFY_OK);

			DEBUG_PRINTF("--------------------------------------OK!!!!!");

			if (g_playFlag != 0)
			{
				g_playFlag = 0;
				tone_wav_play("/root/res/med_ui_endpointing._TTH_.wav");
			}

			byCount = 0;

			if (NULL == pAudioData)
			{
	    		DEBUG_PRINTF("malloc failed..");
				continue;
			}

			Queue_Put(ALEXA_EVENT_SPEECH_STARTED);

			DEBUG_PRINTF("<<<<<<<<<<<<<<<< Mp3player_pthread");

			PlayTTSAudio_Close();

			g_PlayAudioFifo = ft_fifo_alloc(PLAY_FIFO_BUFFLTH);

			if (PlayTTSAudio_Init() != 0)
			{
				DEBUG_ERROR("PlayTTSAudio_Init error");
				continue;
			}

			SetTTSAudioStatus(1);

			while (1)
			{
				if (g_DecodeAudioFifo == NULL)
				{
					printf(LIGHT_GREEN"(g_DecodeAudioFifo is NULL), GetAudioRequestStatus:%d, GetSpeechStatus:%d\n"NONE, GetAudioRequestStatus(), GetSpeechStatus());

					SetTTSAudioStatus(0);
					PlayTTSAudio_Close();
					break;
				}
				else
				{
					iLength = ft_fifo_getlenth(g_DecodeAudioFifo);
					if (iLength > 0)
					{
						if (iLength > DECODE_LENGTH)
						{
							iLength = DECODE_LENGTH;
						}

						memset(pAudioData, 0, DECODE_LENGTH);
						ft_fifo_seek(g_DecodeAudioFifo, pAudioData, 0, iLength);

						ft_fifo_setoffset(g_DecodeAudioFifo, iLength);

					    decode(pAudioData, iLength);
					}
					else
					{
						byCount++;
						// 循环12次都可以接收到数据，或者是请求状态位开始状态 则结束播放语音
						//if (24 == byCount || AUDIO_REQUEST_IDLE == GetAudioRequestStatus())
						if ((AUDIO_REQUEST_IDLE == GetAudioRequestStatus()) && (0 == GetSpeechStatus()))
						{
							byCount = 0;

							usleep(1000 * 150);

							if (AUDIO_REQUEST_START == GetAudioRequestStatus())
							{
								printf(LIGHT_GREEN"Audio Processor stopped !\n"NONE);
							}
							else
							{
								DEBUG_PRINTF("Play Alexa Echoes Over..");
								SetSpeechStatus(1);
								Queue_Put(ALEXA_EVENT_SPEECH_FINISHED);
							}

							// 如果正zai录音则不发送AlexaVoicePlayEnd
							if ((3 == g.m_CaptureFlag) || (0 == g.m_CaptureFlag))
							{
								OnWriteMsgToUartd(ALEXA_VOICE_PLAY_END);
							}

							PlayTTSAudio_Close();

							SetTTSAudioStatus(0);

							g.m_AudioFlag = 0;
							//DEBUG_PRINTF("------------g.m_AudioFlag:%d, g.m_PlayMusicFlag:%d", g.m_AudioFlag, g.m_PlayMusicFlag);

							if (1 == g.m_ExpectSpeechFlag)
							{
								g.m_ExpectSpeechFlag = 0;
								DEBUG_PRINTF("ExpectSpeech...");
								SetRecordFlag(1);
								OnWriteMsgToUartd(ALEXA_EXPECTSPEECH);
								//system("uartdfifo.sh tlkon");
								//OnWriteMsgToWakeup("restart");
								if (1 == g_byWakeUpFlag)
								{
									g_byWakeUpFlag = 0;
								}
							}
							else if (1 == g.m_PlayMusicFlag)
							{
								g.m_PlayMusicFlag = 0;

								if (NULL != g.CurrentPlayDirective.m_PlayUrl)
								{
									if (NULL != strstr(g.CurrentPlayDirective.m_PlayUrl, "cid"))
									{
										DEBUG_PRINTF("Play cid..");
										Queue_Put(ALEXA_EVENT_PLAYBACK_NEARLYFINISHED);
									}
									else
									{
										myexec_cmd("mpc repeat off &");

										// 如果当前正zai闹铃，则直接将音乐声音放到最小
										if(alerthandler_get_state(alertManager->handler) != ALERT_FINISHED)
										{
											byFlag = 1;
											MpdVolume(140);
											alerthandler_resume_alerts(alertManager);
										}

										//WIFIAudio_UciConfig_SearchAndSetValueString("xzxwifiaudio.config.playmode", "0");
										SetPlayStatus(AUDIO_STATUS_STOP);
										g.m_PlayerStatus = AMAZON_AUDIOPLAYER_PLAY;
										int iSeekSec = g.CurrentPlayDirective.offsetInMilliseconds / 1000;
										DEBUG_PRINTF("**************play offset Sec:%d, url:%s", iSeekSec, g.CurrentPlayDirective.m_PlayUrl);
										//MpdInit();
										MpdClear();
										if (1 != byFlag)
										{
											MpdVolume(200);
										}
										byFlag = 0;
										MpdAdd(g.CurrentPlayDirective.m_PlayUrl);
										//MpdRunSeek(0, iSeekSec);
										MpdPlay(-1);

										//MpdDeinit();
										//cdb_set_int("$current_play_mode",5);
										SetPlayStatus(AUDIO_STATUS_PLAY);
										//WIFIAudio_UciConfig_SearchAndSetValueString("xzxwifiaudio.config.token", g.CurrentPlayDirective.m_PlayToken);

										memcpy(g.m_mpd_status.user_state, "PLAYING", strlen("PLAYING"));
									}
								}
								else
								{
									myexec_cmd("mpc volume 200");
									if (1 == GetAmazonPlayBookFlag())
									{
										myexec_cmd("mpc play");
										Queue_Put(ALEXA_EVENT_PLAYBACK_STARTED);
									}
								}
								if (1 == g_byWakeUpFlag)
								{
									g_byWakeUpFlag = 0;
									//printf(LIGHT_RED"OnWriteMsgToWakeup(start)\n"NONE);
									//OnWriteMsgToWakeup("start");
								}
							}
							else
							{
								if(alerthandler_get_state(alertManager->handler) == ALERT_INTERRUPTED)
								{
									alerthandler_resume_alerts(alertManager);
								}
								else
								{
									myexec_cmd("mpc volume 200");
								}

							    if (1 == GetAmazonPlayBookFlag())
								{
									myexec_cmd("mpc play");
									Queue_Put(ALEXA_EVENT_PLAYBACK_STARTED);
								}

								if (1 == g_byWakeUpFlag)
								{
									g_byWakeUpFlag = 0;
									//printf(LIGHT_RED"OnWriteMsgToWakeup(start)\n"NONE);
									//OnWriteMsgToWakeup("start");
								}
							}

							break;
						}
					}
				}
			}

			if (g_DecodeAudioFifo)
			{
				printf(LIGHT_BLUE "free g_DecodeAudioFifo\n"NONE);
				ft_fifo_free(g_DecodeAudioFifo);
				g_DecodeAudioFifo = NULL;
			}
			
			if (g_PlayAudioFifo)
			{
				ft_fifo_free(g_PlayAudioFifo);
				g_PlayAudioFifo = NULL;
			}

			if (pAudioData)
			{
				free(pAudioData);
				pAudioData = NULL;
			}
    	}
	}
}

void CreateMp3PlayerPthread(void)
{
	int iRet;

	DEBUG_PRINTF("CreateDecoePthread..");

	pthread_attr_t a_thread_attr; 
	pthread_attr_init(&a_thread_attr);
	pthread_attr_setstacksize(&a_thread_attr, PTHREAD_STACK_MIN * 2);

	iRet = pthread_create(&Mp3player_Pthread, &a_thread_attr, Mp3player_pthread, NULL);
	//iRet = pthread_create(&Mp3player_Pthread, NULL, Mp3player_pthread, NULL);
	if(iRet != 0)
	{
		DEBUG_PRINTF("pthread_create error:%s\n", strerror(iRet));
		exit(-1);
	}
}


