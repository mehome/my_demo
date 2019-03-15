//File   : lrecord.c 
//Author : Loon <sepnic@gmail.com> 
 
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
#include <signal.h>

#include <time.h> 
#include <locale.h> 
#include <sys/unistd.h> 
#include <sys/stat.h> 
#include <sys/types.h> 
#include <alsa/asoundlib.h> 
#include <assert.h> 
#include "wav_parser.h" 
#include "sndwav_common.h" 
#include "slog.h"
#include "debug.h"
#include "debug.h"
#include "fifobuffer.h"

#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include <limits.h>

#include "opus_audio_encode.h"

#define DEFAULT_CHANNELS         (1) 
#define DEFAULT_SAMPLE_RATE      (44100) 
#define DEFAULT_SAMPLE_LENGTH    (16) 
#define DEFAULT_DURATION_TIME    (5) 
WAVContainer_t wav; 
SNDPCMContainer_t record; 
unsigned int CaptureFifoPut(unsigned char *buffer, unsigned int len);
unsigned int CaptureFifoLength();

int in_aborting = 0;

int g_alipal_record_flag = 0;


/********************************************************************************
*********************************************************************************
*********************************************************************************/
#undef  CAPTURE_FIFO_LENGTH
#define CAPTURE_FIFO_LENGTH         16384*5*4 ///327680B

#define CAPTURE_NONE		0
#define CAPTURE_STARTED		1
#define CAPTURE_ONGING		2
#define CAPTURE_DONE		3
#define CAPTURE_CLOSE		4
#define CAPTURE_CANCLE		5
	
#define CAPTURE_SIZE 		1024 * 8 * 2 //16384Byte


static int captureWait = 0;

static int captureState = 0;
static int captureEnd = 0;

static FT_FIFO * g_CaptureFifo = NULL;


static pthread_t capturePthread = 0;

sem_t captureSem;

static pthread_mutex_t captureMtx = PTHREAD_MUTEX_INITIALIZER;

void CaptureFifoInit()
{
	if(!g_CaptureFifo)  {
		g_CaptureFifo = ft_fifo_alloc(CAPTURE_FIFO_LENGTH);
		if(!g_CaptureFifo) {
			error("ft_fifo_alloc for g_CaptureFifo");
			exit(-1);
		}
		ft_fifo_clear(g_CaptureFifo);
	}
}
void CaptureFifoDeinit()
{
	if(g_CaptureFifo) {
		ft_fifo_free(g_CaptureFifo);
		g_CaptureFifo = NULL;
	}
}

void CaptureFifoClear()
{
	if(g_CaptureFifo) {
		ft_fifo_clear(g_CaptureFifo);
		//g_CaptureFifo = NULL;
	}
}

unsigned int CaptureFifoPut(unsigned char *buffer, unsigned int len)
{
	unsigned int length = 0;
	if(g_CaptureFifo) {
		length = ft_fifo_put(g_CaptureFifo, buffer, len);
	} else {
		error("NULL == g_CaptureFifo");
	}
	return length;
}

unsigned int CaptureFifoLength()
{
	unsigned int len = 0;
	if(g_CaptureFifo) {
		len = ft_fifo_getlenth(g_CaptureFifo);
	}
	return len;
}
unsigned int CaptureFifoSeek( unsigned char *buffer, unsigned int len)
{
	unsigned int length = 0;
	if(g_CaptureFifo) {
		ft_fifo_seek(g_CaptureFifo, buffer, 0, len);
		length = ft_fifo_setoffset(g_CaptureFifo, len);
	}
	return length;	
}

void SetCaptureState(int state)
{
	pthread_mutex_lock(&captureMtx);
	captureState = state;
	pthread_mutex_unlock(&captureMtx);
}

void SetCaptureWait(int wait)
{
	pthread_mutex_lock(&captureMtx);
	captureWait = wait;
	pthread_mutex_unlock(&captureMtx);
}

int IsCaptureFinshed()
{
	int ret;
	pthread_mutex_lock(&captureMtx);
	ret = (captureState == CAPTURE_NONE);
	pthread_mutex_unlock(&captureMtx);
	return ret;
}

int IsCaptureDone()
{
	int ret;
	pthread_mutex_lock(&captureMtx);
	ret = (captureState == CAPTURE_DONE);
	pthread_mutex_unlock(&captureMtx);
	return ret;
}


int IsCaptureCancled()
{
	int ret;
	pthread_mutex_lock(&captureMtx);
	ret = (captureState == CAPTURE_CANCLE);
	pthread_mutex_unlock(&captureMtx);
	return ret;
}


int SNDWAV_PrepareWAVParams(WAVContainer_t *wav) 
{ 
    assert(wav); 
 
    uint16_t channels = DEFAULT_CHANNELS; 
    uint16_t sample_rate = DEFAULT_SAMPLE_RATE; 
    uint16_t sample_length = DEFAULT_SAMPLE_LENGTH; 
    uint32_t duration_time = DEFAULT_DURATION_TIME; 
 
    /* Const */ 
    wav->header.magic = WAV_RIFF; 
    wav->header.type = WAV_WAVE; 
    wav->format.magic = WAV_FMT; 
    wav->format.fmt_size = LE_INT(16); 
    wav->format.format = LE_SHORT(WAV_FMT_PCM); 
    wav->chunk.type = WAV_DATA; 
 
    /* User definition */ 
    wav->format.channels = LE_SHORT(channels); 
    wav->format.sample_rate = LE_INT(sample_rate); 
    wav->format.sample_length = LE_SHORT(sample_length); 
 
    /* See format of wav file */ 
	wav->format.blocks_align = LE_SHORT(channels * sample_length / 8); 
	wav->format.bytes_p_second = LE_INT((uint16_t)(wav->format.blocks_align) * sample_rate); 
	     
	wav->chunk.length = LE_INT(duration_time * (uint32_t)(wav->format.bytes_p_second)); 
	wav->header.length = LE_INT((uint32_t)(wav->chunk.length) +
	        sizeof(wav->chunk) + sizeof(wav->format) + sizeof(wav->header) - 8); 
 
    return 0; 
} 
 
void SNDWAV_Record(SNDPCMContainer_t *sndpcm, WAVContainer_t *wav, int fd) 
{ 
    off64_t rest; 
    size_t c, frame_size; 
     
    if (WAV_WriteHeader(fd, wav) < 0) { 
        exit(-1); 
    } 
 
    rest = wav->chunk.length; 
    while (!in_aborting) { 
       // c = (rest <= (off64_t)sndpcm->chunk_bytes) ? (size_t)rest : sndpcm->chunk_bytes; 
        c = sndpcm->chunk_bytes;
        frame_size = c * 8 / sndpcm->bits_per_frame; 
        if (SNDWAV_ReadPcm(sndpcm, frame_size) != frame_size) 
            break; 
		
        if (write(fd, sndpcm->data_buf, c) != c) { 
             error( "Error SNDWAV_Record [write]"); 
            exit(-1); 
        } 
 		//VoiceDecoderPut(sndpcm->data_buf, c);
        rest -= c; 
    } 
} 

#define BIG_ENDIAN 1
static int vad(short samples[], int len)
{
  int i;
#if BIG_ENDIAN
  unsigned char *p;
  unsigned char temp;
#endif
  long long sum = 0, sum2 = 0;
  for(i = 0; i < len; i++) {
#if BIG_ENDIAN
	p = &samples[i];
	temp = p[0];
	p[0] = p[1];
	p[1] = temp;
#endif
	sum += (samples[i] * samples[i]);
	sum2 += (1000000);
	//sum2 += g.m_iVadThreshold;
  }
  //printf("sum=%lld, sum2=%lld\n", sum, sum2);
  if (sum > sum2)
	  return 1;
  else
	  return 0;
}

static char byVADCont = 32;


void Record(SNDPCMContainer_t *sndpcm, void *wav, unsigned long rest) 
{ 
    size_t c, frame_size; 
	unsigned long len;
	char *tmp = NULL; 
    char *pAudioBuffer;
	unsigned wAudioLen;
	int iCount = 0;
	int iVoiceCount = 0;
	int isVoice = 0;
	char byFlag = 0;
	char byVadFlag = 0;
	char byStart = 0;
	char byVadEndFlag = 0;
	char byVAD_StartFlag = 0;
	unsigned long pcm_count = 0;
	unsigned int total_len = 0; 

	char g_CaptureFlag = 1;

	g_alipal_record_flag = 0;

#if 0	
	tmp = malloc(sndpcm->chunk_bytes*160/441/2); 
#else
	tmp = malloc(sndpcm->chunk_bytes); 
#endif
	if (tmp == NULL)
		return;
	
	while (rest > 0) 
	{
        c = (rest <= (off64_t)sndpcm->chunk_bytes) ? (size_t)rest : sndpcm->chunk_bytes; 
       // c = sndpcm->chunk_bytes;
        frame_size = c * 8 / sndpcm->bits_per_frame; 
	 	if (SNDWAV_ReadPcm(sndpcm, frame_size) != frame_size)  {
			error( "SNDWAV_ReadPcm failed"); 
            break; 
	 	}
       	//wav_write_data(wav, sndpcm->data_buf, c);
 		//VoiceDecoderPut(sndpcm->data_buf, c);
        rest -= c; 
#if 0
		len = 0;
		resample_buf_441_to_16(sndpcm->data_buf, c, tmp, &len);
#else
		len = c;
		memcpy(tmp, sndpcm->data_buf, len);
#endif
		//printf("c=%u,len=%u\n", c, len);
		
		//------------------------------PCM数据写入RB--------------------------
		unsigned int fifoPutLen = 0;
		///unsigned int maxwritelen = CaptureFifoLength(); //debug
		if((fifoPutLen = CaptureFifoPut(tmp, len) ) != len) 
		{
			error("[VOICE_DEBUG]CaptureFifoPut faile fifoPutLen:%d len:%d\n",fifoPutLen, len);
			//break;
		}
		///printf("[VOICE_DEBUG]CaptureFifoPut maxwritelen:%d fifoPutLen:%d len:%d\n",maxwritelen, fifoPutLen, len);
	
		//---------------------------------End-----------------------------------
		
       	wav_write_data(wav, tmp, len);
		pAudioBuffer = tmp;
		wAudioLen = len;
		total_len += c;
		if ((0 == byVadEndFlag) && (1 == byVAD_StartFlag))
		//if (0 == byVadEndFlag)
		{
			if (0 == byStart)
			{
				// vad返回值为1时为is voice
				if (1 == vad(pAudioBuffer, wAudioLen/2))
				{
					///printf("Vad ================================== 0");
					byStart = 1;
					isVoice = 1;
					iVoiceCount = 0;
				} 
				else 
				{
					isVoice =  0;
					iVoiceCount++;
				}
			}
			else 
			{
				if (0 == vad(pAudioBuffer, wAudioLen/2)) 
				{
					isVoice =  0;
					iCount++;
					iVoiceCount++;
					////printf("Vad iCount:%d==================================\n", iCount);
					//error("isVoice:%d iVoiceCount:%d", isVoice, iVoiceCount);
					if (byVADCont <= iCount)
					{
						if (0 == byVadFlag)
						{
							error("1total_len:%ld",total_len);
							byVadFlag = 1;
							iCount = 0;
							isVoice = 0;
							byVadEndFlag = 1; // 此次本地VAD结束，任务完成
							byVAD_StartFlag = 0;
							//SetVADFlag(RECORD_VAD_END);
							
							///StartNewEncode();

							

							break;
						}
					}
				}
				else
				{
					error("2total_len:%ld",total_len);
					iCount  = 0;
					isVoice = 1;
					iVoiceCount = 0;
				}
				
			}
			if(isVoice == 0 &&  iVoiceCount > 160) {
				//PrintSysTime("iVoiceCount > 140");
				error("3total_len:%ld",total_len);
				iVoiceCount = 0;
				
				break;
			}

			g_alipal_record_flag = 1;
			
		}
		
		pcm_count += len;

		if (byVAD_StartFlag == 0 && pcm_count > 9280)
		{
			byVAD_StartFlag = 1;
			
		}
		
		//写入RB PCM数据后通知OPUS编码线程开始
		if(pcm_count > CAPTURE_SIZE)
		{
			StartNewEncode();
		}
		
    } 
	
	printf("[VOICE_DEBUG]++++++++++++Capture End Len:%d\n", total_len);
	g_alipal_record_flag = 0;
	
	//捕获PCM录音状态结束
	SetCaptureState(CAPTURE_DONE);
	
	if (tmp != NULL)
		free(tmp);
	
} 

static void prg_exit(int code) 
{

	if (record.handle)
		snd_pcm_close(record.handle);
	exit(code);
}
void signal_handler(int sig)
{
	if (in_aborting)
		return;

	in_aborting = 1;
	if (record.handle)
		snd_pcm_abort(record.handle);
	if (sig == SIGABRT) {
		/* do not call snd_pcm_close() and abort immediately */
		record.handle = NULL;
		prg_exit(-1);
	}
	signal(sig, signal_handler);
}

//int main(int argc, char *argv[]) 
int wavRecord( char *filename)
{ 
   // char *filename; 
    char *devicename = "default"; 
    int fd; 

    memset(&record, 0x0, sizeof(record)); 

    remove(filename); 
    if ((fd = open(filename, O_WRONLY | O_CREAT, 0644)) == -1) { 
        fprintf(stderr, "Error open: [%s]/n", filename); 
        return -1; 
    } 

    if (snd_pcm_open(&record.handle, devicename, SND_PCM_STREAM_CAPTURE, 0) < 0) { 
         error( "Error snd_pcm_open [ %s]", devicename);  
        goto Err; 
    } 
 	error( "snd_pcm_open");  
    if (SNDWAV_PrepareWAVParams(&wav) < 0) { 
       error( "Error SNDWAV_PrepareWAVParams");  
        goto Err; 
    } 
 	error( "SNDWAV_PrepareWAVParams");  
    if (SNDWAV_SetParams(&record, &wav) < 0) { 
        error( "Error set_snd_pcm_params");  
        goto Err; 
    } 
	error( "SNDWAV_SetParams");  
 
    SNDWAV_Record(&record, &wav, fd); 
	//new_doDecoder(&record, &wav, fd); 
	//error( "new_doDecoders");  
    snd_pcm_drain(record.handle); 
 
    close(fd); 
    free(record.data_buf); 
    snd_pcm_close(record.handle); 
    return 0; 
 
Err: 
    close(fd); 
  //  remove(filename); 
    if (record.data_buf) 
		free(record.data_buf); 
    if (record.handle) 
		snd_pcm_close(record.handle); 
    return -1; 
}

#if 0
int wav_record(char *filename)
{ 
	char *devicename = "default"; 
	void *wav = NULL;
	
	memset(&record, 0x0, sizeof(record)); 
	remove(filename); 
	
	if (snd_pcm_open(&record.handle, devicename, SND_PCM_STREAM_CAPTURE, 0) < 0) { 
		error("Error snd_pcm_open [ %s]", devicename);  
		goto err ;;
	} 
	wav = wav_write_open(filename, 16000, 16, 1);
	if(wav == NULL) {
		error("Error SNDWAV_PrepareWAVParams");
		goto err; 
	}
	
	error("SND_PCM_FORMAT_S16_BE:%d",SND_PCM_FORMAT_S16_BE);
	error("SND_PCM_FORMAT_S16_LE:%d",SND_PCM_FORMAT_S16_LE); 
	
	if(SetParams(&record,  44100 , SND_PCM_FORMAT_S16_LE, 2) < 0) { 
		error("Error set_snd_pcm_params");  
		goto err; 
	} 
	
	error( "SNDWAV_SetParams");  
	
	unsigned long rest =  10 * 44100 * 2 * 2;

	CaptureFifoClear(); //RB清0
	
	Record(&record, wav, rest); 

	if(wav)
		wav_write_close(wav);
	sync();
	snd_pcm_drain(record.handle); 

	if(record.data_buf)
		free(record.data_buf); 

	if(record.handle)
		snd_pcm_close(record.handle); 

	return 0; 

err: 
	
	if (record.data_buf) 
		free(record.data_buf); 

	if (record.handle) 
		snd_pcm_close(record.handle); 

	return -1; 
} 
#else
int wav_record(char *filename)
{ 
	char *devicename = "plughw:1,0"; 
	void *wav = NULL;
	
	memset(&record, 0x0, sizeof(record)); 
	remove(filename); 
	
	if (snd_pcm_open(&record.handle, devicename, SND_PCM_STREAM_CAPTURE, 0) < 0) { 
		error("Error snd_pcm_open [ %s]", devicename);  
		goto err ;;
	} 
	wav = wav_write_open(filename, 16000, 16, 1);
	if(wav == NULL) {
		error("Error SNDWAV_PrepareWAVParams");
		goto err; 
	}
	
	error("SND_PCM_FORMAT_S16_BE:%d",SND_PCM_FORMAT_S16_BE);
	error("SND_PCM_FORMAT_S16_LE:%d",SND_PCM_FORMAT_S16_LE); 
	
	if(SetParams(&record,  16000 , SND_PCM_FORMAT_S16_LE, 1) < 0) { 
		error("Error set_snd_pcm_params");  
		goto err; 
	} 
	
	error( "SNDWAV_SetParams");  
	
	unsigned long rest =  10 * 16000 * 1 * 2;

	CaptureFifoClear(); //RB清0
	
	Record(&record, wav, rest); 

	sync();
	snd_pcm_drain(record.handle);

	if(wav)
		wav_write_close(wav);

	if(record.data_buf)
		free(record.data_buf); 

	if(record.handle)
		snd_pcm_close(record.handle); 

	return 0; 

err: 
	
	if (record.data_buf) 
		free(record.data_buf); 

	if (record.handle) 
		snd_pcm_close(record.handle); 

	return -1; 
} 
#endif

void *CaptureAudioPthread(void *arg)
{
	int iRet;
	iRet = sem_init(&captureSem, 0, 0);
	if(iRet != 0)
	{
		DEBUG_PRINTF("sem_init error:%s\n", strerror(iRet));
		exit(-1);
	}
	
	warning("...start CaptureAudioPthread...");
	
	while(1)
	{
		warning("CAPTURE_NONE ...");
		
		sem_wait(&captureSem);

		tone_wav_play("/root/res/med_ui_wakesound_touch._TTH_.wav");
#if 1
		system("uartdfifo.sh CaptureStart");
#endif
		
		wav_record("/tmp/1.wav");

#if 0
		system("uartdfifo.sh tlkoff");
#else
		OnWriteMsgToWakeup("start");
		system("uartdfifo.sh Identify_OK");
#endif

		warning("CAPTURE_DONE ...");
	}
	
}


void CreateCapturePthread(void)
{
	int iRet;

	CaptureFifoInit();
	
	pthread_attr_t a_thread_attr; 
	pthread_attr_init(&a_thread_attr);
	pthread_attr_setstacksize(&a_thread_attr, PTHREAD_STACK_MIN*4);

	iRet = pthread_create(&capturePthread, &a_thread_attr, CaptureAudioPthread, NULL);
	pthread_attr_destroy(&a_thread_attr);
	warning("create capturePthread ...");
	if(iRet != 0)
	{
		error("pthread_create error:%s", strerror(iRet));
		exit(-1);
	}

}

void DestoryCapturePthread(void)
{

	if (capturePthread != 0)
	{
		pthread_join(capturePthread, NULL);
	}
	CaptureFifoDeinit();

}




