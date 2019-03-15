#include <stdio.h>
#include <malloc.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <fcntl.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <time.h>
#include <locale.h>
#include <alsa/asoundlib.h>
#include <assert.h>
#include <termios.h>
#include <signal.h>
#include <sys/poll.h>
#include <sys/uio.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <endian.h>
#include <termios.h>

#ifdef ALEXA_FF

#include "aconfig.h"
#include "gettext.h"
#include "formats.h"
#include "version.h"

#include "capture.h"

#include "resample_441_to_16.h"
#include "../globalParam.h"
#include "../alert/AlertHead.h"

#include "../wakeup/audio.h"

static snd_pcm_t *C20921_handle;
short audiobuf20921[512] = {0};


#define WRITE_20921_FILE

#define AUDIO_BUFFERSZ 250
#define SAMPLERATE 16000
/*int wantSamples = AUDIO_BUFFERSZ / 1000. * SAMPLERATE;*/

#define UPLOAD_FILE_NAME     "/tmp/16k_10921.raw"
static snd_pcm_uframes_t chunk_size = 0;

extern audio_t aud;
extern AlertManager *alertManager;
extern GLOBALPRARM_STRU g;
extern char g_byWakeUpFlag;
extern FT_FIFO * g_CaptureFifo;
#define POST_MAX_NUMBER 16384 * 3


extern audio_t aud;

#if 0
static int openKnownAudio() {
  int i = 0;
  int rc;
  char byFlag = 0;
  unsigned int val;
  int dir=0;
  snd_pcm_t *handle;
  snd_pcm_hw_params_t *hw_params=NULL;
  snd_pcm_uframes_t frames;
  size_t esz = 256;
  char err[esz];
  
  	//audiobuf20921 = malloc(wantSamples*sizeof(short));

  /* Open PCM device for recording (capture). */ 
  

	for (i = 0; i < 3; i++)
	{
		if ((rc=snd_pcm_open(&C20921_handle, "plughw:1,0",SND_PCM_STREAM_CAPTURE, 0))<0) 
		{
			printf("unable to open pcm device for recording: %s, try again..\n",snd_strerror(rc));
			//return -1;
			byFlag = 1;
			usleep(1000 * 100);
		}
		else
		{
			byFlag = 0;
			break;
		}
	}

	if (1 == byFlag)
	{
		return -1;
	}

	handle=C20921_handle;

  /* Configure hardware parameters  */
  if((rc=snd_pcm_hw_params_malloc(&hw_params)) < 0) {
    printf("unable to malloc hw_params: %s\n",snd_strerror(rc));
  }
  if((rc=snd_pcm_hw_params_any(handle, hw_params))<0) {
    printf("unable to setup hw_params: %s\n",snd_strerror(rc));
  }
  if((rc=snd_pcm_hw_params_set_access(handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED))<0) {
    printf("unable to set access mode: %s\n",snd_strerror(rc));
  }
  if((rc=snd_pcm_hw_params_set_format(handle, hw_params, SND_PCM_FORMAT_S16_LE))<0) {
    printf("unable to set format: %s\n",snd_strerror(rc));
  }
  if((rc=snd_pcm_hw_params_set_channels(handle, hw_params, 1))<0) {
    printf("unable to set channels: %s\n",snd_strerror(rc));
  }
  val = 16000;
  dir = 0;
  if((rc=snd_pcm_hw_params_set_rate(handle, hw_params, 16000,0))<0) {
    printf("unable to set samplerate: %s\n",snd_strerror(rc));
  }
  if (val!=SAMPLERATE) {
    printf("unable to set requested samplerate: requested=%i got=%i\n",SAMPLERATE,val);
  }


  
/*  frames = 32; 
  if ((rc=snd_pcm_hw_params_set_period_size_near(handle,hw_params, &frames, &dir))<0) {
    snprintf(err, esz, "unable to set period size: %s\n",snd_strerror(rc));
    audioPanic(err);
  }
  frames = 4096; 
  if ((rc=snd_pcm_hw_params_set_buffer_size_near(handle,hw_params, &frames))<0) {
    snprintf(err, esz, "unable to set buffer size: %s\n",snd_strerror(rc));
    audioPanic(err);
  }*/

  if ((rc = snd_pcm_hw_params(handle, hw_params))<0) {
    printf("unable to set hw parameters: %s\n",snd_strerror(rc));
  }

  snd_pcm_hw_params_get_period_size(hw_params, &chunk_size, 0);
#if 0
  aboutAlsa(handle,hw_params); // useful debugging
#endif
  snd_pcm_hw_params_free(hw_params);

  return 1;
}
#endif

int open_file(const char *name)
{
	int fd;
	
	if(access(name,0) == 0){
			DEBUG_PRINTF("remove %s",name);
			remove(name);
	}
	
	fd = open(name, O_WRONLY | O_CREAT | O_APPEND, 0644);
	if (fd == -1) {
		printf("open error\n");
	}
	return fd;
}

static void SendMSGToSendPthread(void)
{
	if (1 == GetAmazonConnectStatus())
	{
		Queue_Put(ALEXA_EVENT_SPEECH_RECOGNIZE);
	}
	else
	{
		myexec_cmd("uartdfifo.sh Identify_ERR");
//		OnWriteMsgToUartd("Identify_ERR");
//		OnWriteMsgToUartd("tlkoff");

//		myexec_cmd("aplay /root/res/alexa_touble.wav");
//		MpdVolume(200);
      myexec_cmd("uartdfifo.sh tlkoff && aplay /root/res/alexa_touble.wav && mpc volume 200");

		/*if (1 == g_byWakeUpFlag)
		{
			g_byWakeUpFlag = 0;
			printf(LIGHT_RED"OnWriteMsgToWakeup(start)\n"NONE);
			OnWriteMsgToWakeup("start");
		}*/
	}
}

static void exchangeData(short samples[], int len)
{
	int i;
	unsigned char *p;
	unsigned char temp;
	for(i = 0; i < len; i++) 
	{
		p = &samples[i];
		temp = p[0];
		p[0] = p[1];
		p[1] = temp;
	}
}

long capture20921()
{
    int rc;
	int fd1;
	long int startTime = 0;
	long int endTime = 0;

	char byFlag = 0;
	char byVadFlag = 0;
	char byStart = 0;
	g.m_CaptureFlag = 1;
	//char byVADCont = 0;
	static int iCount = 0;
	char byVadEndFlag = 0;
	char byVAD_StartFlag = 0;
	long pcm_count = 0;

	SetVADFlag(RECORD_VAD_START);
	
	// 开始录音，判断是否有闹钟，有的话则中断
	if (alertmanager_has_activealerts(alertManager))
	{
		DEBUG_PRINTF("<<<<<<<<<alerthandler_set_state ALERT_INTERRUPTED");
		alerthandler_set_state(alertManager->handler, ALERT_INTERRUPTED);
	}

	/*g.m_RemoteVadFlag = REMOTE_VAD_START;

	if (openKnownAudio() < 0)
	{
		myexec_cmd("uartdfifo.sh " ALEXA_IDENTIFY_ERR);
		goto end;
	}*/

	DEBUG_PRINTF("chunk_size:%d", g.m_chunk_size);

	memset(audiobuf20921, 0, sizeof(audiobuf20921));

	/*if (1 == g_byWakeUpFlag)
	{
		if (g.m_chunk_size >= 2000)
		{
			byVADCont = 12;
		}
		else if (g.m_chunk_size >= 1000)
		{
			byVADCont = 15;
		}
		else
		{
			byVADCont = 35;
		}
	}
	else
	{
		byVADCont = 28;
	}*/

#ifdef WRITE_20921_FILE
	fd1 = open_file(UPLOAD_FILE_NAME);
#endif

	struct timeval tv; 
	gettimeofday(&tv,NULL);	
	startTime = tv.tv_sec;

	while (1)
	{
		g.m_CaptureFlag = 2;

		if ((rc = snd_pcm_readi(aud.inhandle, audiobuf20921, g.m_chunk_size)) < 0) 
		{
			printf("read failed (%s)\n", snd_strerror (rc));
			return;
		}

		// VAD判断
		if (RECORD_VAD_END == GetVADFlag())
		{
			if (0 == byFlag)
			{
				byFlag = 1;
				DEBUG_PRINTF("getRecord break ...sem_post Request_sem ..");
				SendMSGToSendPthread();
			}
			iCount = 0;
			g.m_CaptureFlag = 3;

			break;
		}

		// 超时判断
		gettimeofday(&tv, NULL); 
		endTime = tv.tv_sec; 	 
		if ((endTime-startTime) >= 15)
		{
			if (0 == byFlag)
			{
				byFlag = 1;
				DEBUG_PRINTF("time out sem_post Request_sem ..");
				SendMSGToSendPthread();
			}

			DEBUG_PRINTF("record time out = :%ld， send tlkoff...\n", endTime-startTime);
			iCount = 0;

			SetVADFlag(RECORD_VAD_END);

			g.m_CaptureFlag = 3;

			break;
		}
		
		if (NULL == g_CaptureFifo)
		{
			DEBUG_PRINTF("<g_CaptureFifo is NULL..>");
			SetVADFlag(RECORD_VAD_END);
			continue;
		}
		else
		{
			exchangeData(audiobuf20921, rc);
			ft_fifo_put(g_CaptureFifo, (char *)audiobuf20921, rc*2);
		}

		if ((0 == byVadEndFlag) && (1 == byVAD_StartFlag))
		{
			if (0 == byStart)
			{
				// vad返回值为1时为is voice
				if (1 == vad(audiobuf20921, rc))
				{
					//DEBUG_PRINTF("Vad ================================== Start");
					byStart = 1;
				}
			}
			else if (0 == vad(audiobuf20921, rc))
			{
				iCount++;
				//DEBUG_PRINTF("Vad iCount:%d==================================", iCount);
				if (g.m_byVADCont <= iCount)
				{
					if (0 == byVadFlag)
					{
						byVadFlag = 1;
						iCount = 0;

						byVadEndFlag = 1; // 此次本地VAD结束，任务完成
						byVAD_StartFlag = 0;
						DEBUG_PRINTF("#################################-----------------------------------------------------------is not voice...");
						SetVADFlag(RECORD_VAD_END);
					}
				}
			}
			else
			{
				//DEBUG_PRINTF("---------------is voice=============", iCount);
				iCount = 0;
			}
		}
		
#ifdef WRITE_20921_FILE
		if (write(fd1, (char *)audiobuf20921, rc*2) != rc*2) 
		{
			printf("16k_capture.wav error\n");
			break;
		}
#endif

		pcm_count += (long)rc*2;
		//if (pcm_count >= 22400)
		if (pcm_count >= POST_MAX_NUMBER)
		{
			byVAD_StartFlag = 1;
		}
		if (0 == byFlag && pcm_count >= 16384)
		{
			// 缓存4帧数据
			if (1 == GetAmazonConnectStatus())
			{
				byFlag = 1;
				Queue_Put(ALEXA_EVENT_SPEECH_RECOGNIZE);
			}
		}
	}

	myexec_cmd("uartdfifo.sh " ALEXA_CAPTURE_END);
//	OnWriteMsgToUartd(ALEXA_CAPTURE_END);

end:
	/*if(C20921_handle){
		snd_pcm_drain(C20921_handle); 
		snd_pcm_close(C20921_handle);
		C20921_handle  = NULL;
	}	*/

	/*if(audiobuf20921)
	{
		free(audiobuf20921);
		audiobuf20921 =NULL;
	}*/

	g_byWakeUpFlag = 0;
	printf(LIGHT_RED"capture_voice exit ...\n"NONE);
	printf(LIGHT_RED"OnWriteMsgToWakeup(start)\n"NONE);
	//OnWriteMsgToWakeup("start");
	sem_post(&g.Semaphore.WakeUp_sem);
#ifdef WRITE_20921_FILE
	close(fd1);
#endif
}

#endif



