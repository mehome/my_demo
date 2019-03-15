#include "tts.h"
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <pthread.h>
#include <semaphore.h>
#include <byteswap.h>
#include <alsa/asoundlib.h>

#include "qtts.h"
#include "qisr.h"
#include "msp_cmn.h"
#include "msp_errors.h"
#include "speech.h"
#include "myutils/debug.h"


static int ifly_is_login = 0; 

static pthread_t iflytekPthread = 0;
static pthread_t ttsWait = 0;
static sem_t ttsSem;
static int ttsState = IFLYTEK_NONE;
static char * text = NULL;

static pthread_mutex_t ttsMtx = PTHREAD_MUTEX_INITIALIZER;




int IsIflytekCancled()
{
	int ret;
	pthread_mutex_lock(&ttsMtx);
	ret = (ttsState == IFLYTEK_CANCLE);
	pthread_mutex_unlock(&ttsMtx);
	return ret;
}
int IsIflytekRuning()
{
	int ret;
	pthread_mutex_lock(&ttsMtx);
	ret = (ttsState == IFLYTEK_ONGING);
	pthread_mutex_unlock(&ttsMtx);
	return ret;
}

void SetIflytekState(int state)
{
	pthread_mutex_lock(&ttsMtx);
	ttsState = state;
	pthread_mutex_unlock(&ttsMtx);
}


int IsIflytekFinshed()
{
	int ret;
	pthread_mutex_lock(&ttsMtx);
	ret = (ttsState == IFLYTEK_NONE);
	pthread_mutex_unlock(&ttsMtx);
	return ret;
}
int IsIflytekDone()
{
	int ret;
	pthread_mutex_lock(&ttsMtx);
	ret = (ttsState == IFLYTEK_DONE);
	pthread_mutex_unlock(&ttsMtx);
	return ret;
}
void SetIflytekWait(int wait)
{
	pthread_mutex_lock(&ttsMtx);
	ttsWait = wait;
	pthread_mutex_unlock(&ttsMtx);
}

void StartIflytek(char *value)
{
	pthread_mutex_lock(&ttsMtx);
	text = strdup(value);
	if(ttsWait == 0) {
		sem_post(&ttsSem);
	}
	pthread_mutex_unlock(&ttsMtx);

}

void WaitForIflytekFinshed()
{		 
	if(!IsIflytekFinshed()) {
		info("Set IsIflytek Cancle...");
		SetIflytekState(IFLYTEK_CANCLE);
		//while(!IsIflytekFinshed()) {
		//
		//}
	}
	info("new IsIflytek start...");
}


static short int swapInt16(short int  * pvalue)  
{  
    int low_byte = 0xff;
    unsigned short high_byte = 0xff00;

    *(pvalue) = ( (*pvalue) & low_byte ) << 8 | ( (*pvalue) & high_byte ) >> 8; 

    return *pvalue;
}


static int swapInt32(int *pvalue)  
{  
    short int tmp;
    short int * swap;
    
    swap = (short int *)pvalue;

    tmp =  swap[0];
    swap[0] =  swap[1];
    swap[1] = tmp;

    swapInt16(&swap[0]);
    swapInt16(&swap[1]);

    return (*pvalue);
}


static int endian_conv(void *data,int len, void **pout)
{
    int slen = len;
    int low_byte = 0xff;
    unsigned short high_byte = 0xff00;
    void  *porig = NULL;

    *pout = malloc(len);
    if(*pout  ==NULL )
    {
        printf("malloc failed!  ");
        return -1;
    }

    memcpy(*pout,data,len);

    porig = *pout;
     
    while( slen > 0 )
    {
       *((unsigned short *)porig) = ( (*((unsigned short *)porig)) & low_byte ) << 8 | ( (*((unsigned short *)porig)) & high_byte ) >> 8; 
        
       porig += sizeof(unsigned short);

       slen -= sizeof(unsigned short);
    }
    
    return 0;
}

static int _text_to_sound(const char *buf, const char *snd_path, const char *params)
{
	int ret = -1;
	const char *sessionID    = NULL;
	unsigned int audio_len    = 0;
	void *pdata = NULL;
	snd_pcm_t *pcm = NULL;
	
	int synth_status = MSP_TTS_FLAG_STILL_HAVE_DATA;
	info("...");
	if ( !buf || !snd_path)
	{
		error("!buf || !snd_path");
		return ret;
	}

	pcm_stream_init(&pcm);
	info(" after pcm_stream_init");
	if( !pcm ){
		error("codec initialized failed.");
    	return ret;
	}
	
	sessionID = QTTSSessionBegin(params, &ret);
	if (ret != MSP_SUCCESS )
	{
		error("QTTSSessionBegin failed, error = %d.", ret);
  		goto tts_exit;
	}
	ret = QTTSTextPut(sessionID, buf, (unsigned int)strlen(buf), NULL);
	if (MSP_SUCCESS != ret)
	{
		error("QTTSTextPut failed, error code: %d.",ret);
		QTTSSessionEnd(sessionID, "TextPutError");
		goto tts_exit;
	}
	SetIflytekState(IFLYTEK_ONGING);
	while (1) 
	{
		if(IsIflytekCancled()) {
			info("_text_to_sound  Cancle...");
			break;
		}
		const void *paud = QTTSAudioGet(sessionID, &audio_len, &synth_status, &ret);
		if (MSP_SUCCESS != ret)
			break;
		if (NULL != paud)
		{
		    endian_conv(paud, audio_len, &pdata);
       	 	pcm_stream_input(pcm , pdata, audio_len / 2);
            
        	free(pdata);
		}
		if (MSP_TTS_FLAG_DATA_END == synth_status)
			break;
		usleep(7000); 
	}

	if (MSP_SUCCESS != ret)
	{
		error("QTTSAudioGet failed, error code: %d.",ret);
		QTTSSessionEnd(sessionID, "AudioGetError");
		goto tts_exit;
	}

	ret = QTTSSessionEnd(sessionID, "Normal");
	if (MSP_SUCCESS != ret)
	{
		error("QTTSSessionEnd failed, error = %d.",ret);
	}

tts_exit:
	if( pcm )
		pcm_stream_close(pcm);
    return ret;
}
static int tts_to_sound(const char *buf, const char *snd_path, const char *params)
{
	int ret = -1;
	const char *sessionID    = NULL;
	unsigned int audio_len    = 0;
	void *pdata = NULL;
	
	void *wav = NULL;
	snd_pcm_t *pcm = NULL;
	int synth_status = MSP_TTS_FLAG_STILL_HAVE_DATA;
	info("...");
	if ( !buf || !snd_path)
	{
		error("!buf || !snd_path");
		return ret;
	}
	wav = wav_write_open(snd_path, 16000, 16, 1);
	if(wav == NULL) {
		error("wav_write_open failed.");
  		goto tts_exit;
	}
		
	sessionID = QTTSSessionBegin(params, &ret);
	if (ret != MSP_SUCCESS )
	{
		error("QTTSSessionBegin failed, error = %d.", ret);
  		goto tts_exit;
	}
	ret = QTTSTextPut(sessionID, buf, (unsigned int)strlen(buf), NULL);
	if (MSP_SUCCESS != ret)
	{
		error("QTTSTextPut failed, error code: %d.",ret);
		QTTSSessionEnd(sessionID, "TextPutError");
		goto tts_exit;
	}
	SetIflytekState(IFLYTEK_ONGING);
	while (1) 
	{
		if(IsIflytekCancled()) {
			info("_text_to_sound  Cancle...");
			break;
		}
		const void *paud = QTTSAudioGet(sessionID, &audio_len, &synth_status, &ret);
		if (MSP_SUCCESS != ret)
			break;
		if (NULL != paud)
		{
			endian_conv(paud, audio_len, &pdata);
	       	//	pcm_stream_input(pcm , pdata, audio_len / 2);
	        wav_write_data(wav, pdata, audio_len);    
	        //	free(pdata);
        	
		}
		if (MSP_TTS_FLAG_DATA_END == synth_status)
			break;
		usleep(7000); 
	}

	if (MSP_SUCCESS != ret)
	{
		error("QTTSAudioGet failed, error code: %d.",ret);
		QTTSSessionEnd(sessionID, "AudioGetError");
		goto tts_exit;
	}

	ret = QTTSSessionEnd(sessionID, "Normal");
	if (MSP_SUCCESS != ret)
	{
		error("QTTSSessionEnd failed, error = %d.",ret);
	}

tts_exit:
	if(wav)
		wav_write_close(wav);
	if( pcm )
		pcm_stream_close(pcm);
    return ret;
}

static int ifly_login(const char *param)
{
    int ret = 0;

    if(ifly_is_login == 0 )
    {
    	ret = MSPLogin(NULL, NULL, param);
	    if ( ret != MSP_SUCCESS )
	    {
		    printf("MSPLogin failed , Error code %d.\n",ret);
    		return -1;
	    }
        ifly_is_login = 1;
        return 0;
    }
    return 0;
}


static int ifly_logout(void)
{
    if(ifly_is_login == 1)
    {
        MSPLogout();
        ifly_is_login = 0 ;
    }
    return 0;
}

static int ifly_text_to_sound(char *buf,char *snd_path)
{

    int ret  = MSP_SUCCESS;
	const char* session_begin_params = "voice_name=nannan, text_encoding=utf8,aue=speex-wb;7, rate=16000 ,sample_rate=16000, speed=70, volume=50, pitch=50, rdn=2";

	ret = _text_to_sound(buf, snd_path, session_begin_params);
	if (ret != MSP_SUCCESS )
	{
		error("_text_to_sound failed, error = %d.", ret);
	}

	return ret;
}
int iflytek_tts(char *word)
{
	int ret = -1;
	int ifly_login_flag = 0;
	const char* session_begin_params = "voice_name=nannan, text_encoding=utf8,aue=speex-wb;7, rate=16000 ,sample_rate=16000, speed=70, volume=50, pitch=50, rdn=2";
	char *snd_path = "/tmp/tts.wav";
	
	ifly_login_flag = ifly_login("appid=57baa402, work_dir=/tmp");
	if(ifly_login_flag >=0 )
	{
		info("ifly login success");
		
		ret = tts_to_sound(word, snd_path, session_begin_params);
		ifly_logout();
	
	}
	
	return ret;
}

int ifly_tts(char *word)
{
	int ret = -1;
	int ifly_login_flag = 0;
	
	ifly_login_flag = ifly_login("appid=57baa402, work_dir=/tmp");
	if(ifly_login_flag >=0 )
	{
		info("ifly login success");
		ret = ifly_text_to_sound(word, "/tmp/tts.wav");
		ifly_logout();
	
	}
	
	return ret;
}


static void *IflytekPthread(void *arg)
{ 	
	int ret ;
	while(1){
		warning("IFLYTEK_NONE ...");
		SetIflytekState(IFLYTEK_NONE);
		warning("sem_wait ttsMtx ");	
		SetIflytekWait(0);
		sem_wait(&ttsSem);
		SetIflytekWait(1);
		SetIflytekState(IFLYTEK_STARTED);
		if(text) {
			ret = ifly_tts(text);
			free(text);
			text = NULL; 
			if(!IsIflytekCancled()) {
				if(ret == 0) {
					TuringPlayDoneEvent();
				}
			}
		}
		SetIflytekState(IFLYTEK_DONE);
		warning("IFLYTEK_DONE ...");
	}
}


void    CreateIflytekPthread(void)
{
	int iRet;
	pthread_attr_t a_thread_attr; 
	
	pthread_attr_init(&a_thread_attr);
	pthread_attr_setstacksize(&a_thread_attr, PTHREAD_STACK_MIN*2);

	iRet = pthread_create(&iflytekPthread, &a_thread_attr, IflytekPthread, NULL);
	
	pthread_attr_destroy(&a_thread_attr);
	if(iRet != 0)
	{
		error("pthread_create error:%s", strerror(iRet));
		exit(-1);
	}


}
int   DestoryIflytekPthread(void)
{
	if (iflytekPthread != 0)
	{
		pthread_join(iflytekPthread, NULL);
		info("iflytekPthread  end...");
	}	
}


