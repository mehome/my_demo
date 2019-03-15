#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <limits.h> 
#include <semaphore.h>

#include "slog.h"
#include "debug.h"
#include "fifobuffer.h"
#include "opus_audio_encode.h"
#include "alipal_conver_opus.h"
#include "wavRecorder.h"
#include "opus_audio_decode.h"

#define ENCODE_NONE				0
#define ENCODE_STARTED			1
#define ENCODE_ONGING			2
#define ENCODE_DONE				3
#define ENCODE_CLOSE			4
#define ENCODE_CANCLE			5

#undef 	FIFO_LENGTH
#define FIFO_LENGTH       		1024 * 16

#define OPUS_ENCODE_SIZE 		1024 * 2

static int encodeState = 0;
static int encodeWait = 0;

extern int g_alipal_record_flag;

int g_alipal_encode_flag = 0;


static pthread_t opusEncodePthread;

static pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;  
static pthread_cond_t cond = PTHREAD_COND_INITIALIZER; 
static pthread_mutex_t encodeMtx = PTHREAD_MUTEX_INITIALIZER;

/////////////////////////////////////////////////////////////////

FT_FIFO * g_EncodeFifo = NULL;


void EncodeFifoInit()
{
	if(!g_EncodeFifo)  {
		g_EncodeFifo = ft_fifo_alloc(FIFO_LENGTH);
		if(!g_EncodeFifo) {
			error("ft_fifo_alloc for g_DecodeFifo");
			exit(-1);
		}
	}
}
void EncodeFifoDeinit()
{
	if(g_EncodeFifo) {
		ft_fifo_free(g_EncodeFifo);
	}
}

void EncodeFifoClear()
{
	if(g_EncodeFifo) {
		ft_fifo_clear(g_EncodeFifo);
	}
}

unsigned int EncodeFifoPut(unsigned char *buffer, unsigned int len)
{
	unsigned int length = 0;
	if(g_EncodeFifo) {
		length = ft_fifo_put(g_EncodeFifo, buffer, len);
	}
	return length;
}

unsigned int EncodeFifoLength()
{
	unsigned int len = 0;
	if(g_EncodeFifo) {
		len = ft_fifo_getlenth(g_EncodeFifo);
	}
	return len;
}
unsigned int EncodeFifoSeek( unsigned char *buffer, unsigned int len)
{
	unsigned int length = 0;
	if(g_EncodeFifo) {
		ft_fifo_seek(g_EncodeFifo, buffer, 0, len);
		length = ft_fifo_setoffset(g_EncodeFifo, len);
	}
	return length;	
}

int IsEncodeCancled()
{
	int ret;
	pthread_mutex_lock(&mtx);
	ret = (encodeState == ENCODE_CANCLE);
	pthread_mutex_unlock(&mtx);
	return ret;
}

void SetEncodeState(int state)
{
	pthread_mutex_lock(&mtx);
	encodeState = state;
	pthread_mutex_unlock(&mtx);
}
int IsEncodeFinshed()
{
	int ret;
	pthread_mutex_lock(&mtx);
	ret = (encodeState == ENCODE_NONE);
	pthread_mutex_unlock(&mtx);
	return ret;
}
int IsEncodeDone()
{
	int ret;
	pthread_mutex_lock(&mtx);
	ret = (encodeState == ENCODE_DONE);
	pthread_mutex_unlock(&mtx);
	return ret;
}
void StartNewEncode()
{
	pthread_mutex_lock(&encodeMtx);
	if(encodeWait == 0) {
		info("start encode...");
		sem_post(&encodeSem);
	}
	pthread_mutex_unlock(&encodeMtx);

}

static void SetEncodeWait(int wait)
{
	pthread_mutex_lock(&encodeMtx);
	encodeWait = wait;
	pthread_mutex_unlock(&encodeMtx);
}

static void int_to_char(unsigned int i, unsigned char ch[4])
{
    ch[0] = i&0xFF;
    ch[1] = (i>>8)&0xFF;
    ch[2] = (i>>16)&0xFF;
    ch[3] = i>>24;
}

static void *OpusEncodePthread(void *arg)
{
	int mode = 8;
	int ch, dtx = 0;
	FILE *out, *in;
	void *wav, *amr;
	int channels = 1;
	int inputSize;
	unsigned int total = 0;
	unsigned char * inputBuf;

	char g_EncodeFlag = 1;
	unsigned char int_field[4];

	while(1) 
	{
		warning("^_^ENCODE_WAIT....");
		
		SetEncodeWait(0);

		sem_wait(&encodeSem);

		warning("^_^ENCODE_START....");
		
		SetEncodeWait(1);
		
		EncodeFifoClear();

		g_alipal_encode_flag = 0;

		Init_Opus_Encode(); //Opus编码
			
		inputSize = channels*2*320;
		inputBuf = (unsigned char*) malloc(inputSize);
		int iLength;
	
		while(g_alipal_record_flag == 1)
		{
			unsigned char outbuf[500];
			int read, i, n;
			
			iLength = CaptureFifoLength();
			
			if(iLength >= inputSize) 
			{
				iLength = inputSize ;
				
				read = CaptureFifoSeek(inputBuf, iLength);
				if(read == 0 || read != iLength) 
				{
					error("read == 0 || read != iLength ");
					break;
				}

				//开始Opus编码
				n = Start_Opus_Encode((short *)inputBuf, iLength, outbuf);
				
				int_to_char(n, int_field);
				
				if(EncodeFifoPut(int_field, sizeof(unsigned char)) < 1) 
				{
					error("1EncodeFifoPut(outbuf,n) != n");
					//break;
				}
				
				if(EncodeFifoPut(outbuf, n) < 1) 
				{
					error("2EncodeFifoPut(outbuf,n) != n");
					//break;
				}
				
				total = total + n;

				if(total >= inputSize)
				{
					StartNewDecode();				
				}

				///printf("[VOICE_DEBUG]Read Capture RB Len:%d, Opus Encode Len:%d, opus_total:%d\n", iLength, n, total);

			}
			else 
			{
				if(IsCaptureDone())
				{
					printf("[VOICE_DEBUG]++++++++++Get Capture FIFO Len:%d < 640\n", iLength);
					break;
				}
			
				usleep(500);
			}
			
			g_alipal_encode_flag = 1;
			
		}
		
		if(inputBuf != NULL)
			free(inputBuf);

		error("[VOICE_DEBUG]###########Encode end total:%d\n",total);
		
		total = 0;
	
		SetCaptureState(0);	

		//结束Opus编码
		End_Opus_Encdoe();

		g_alipal_encode_flag = 0;

		SetEncodeState(ENCODE_DONE);

	}
	
}


int CreateEncodePthread(void)
{
	int iRet;

	EncodeFifoInit();
	
	pthread_attr_t a_thread_attr; 
	pthread_attr_init(&a_thread_attr);
	pthread_attr_setstacksize(&a_thread_attr, PTHREAD_STACK_MIN * 2);

	iRet = pthread_create(&opusEncodePthread, &a_thread_attr, OpusEncodePthread, NULL);
	pthread_attr_destroy(&a_thread_attr);
	if(iRet != 0)
	{
		error("pthread_create error:%s", strerror(iRet));
		return -1;
	}
	
	return 0;
}

int  DestoryEncodePthread(void)
{
	EncodeFifoDeinit();
	
	if (opusEncodePthread != 0)
	{
		pthread_join(opusEncodePthread, NULL);
		info("opusEncodePthread end...");
	}	
}


