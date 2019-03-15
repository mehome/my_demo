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
#include "opus_audio_decode.h"
#include "opus_audio_encode.h"
#include "alipal_send_opus.h"
#include "pal.h"

#define DECODE_NONE				0
#define DECODE_STARTED			1
#define DECODE_ONGING			2
#define DECODE_DONE				3
#define DECODE_CLOSE			4
#define DECODE_CANCLE			5

#undef 	FIFO_LENGTH
#define FIFO_LENGTH       		1024 * 2

#define OPUS_DECODE_SIZE 		1024

static int decodeState = 0;
static int decodeWait = 0;

extern int g_alipal_encode_flag;

static pthread_t opusDecodePthread;

static pthread_mutex_t decodeMtx = PTHREAD_MUTEX_INITIALIZER; 
static pthread_mutex_t decodemtx = PTHREAD_MUTEX_INITIALIZER;  
static pthread_cond_t decodecond = PTHREAD_COND_INITIALIZER; 

/////////////////////////////////////////////////////////////////
int IsDecodeCancled()
{
	int ret;
	pthread_mutex_lock(&decodemtx);
	ret = (decodeState == DECODE_CANCLE);
	pthread_mutex_unlock(&decodemtx);
	return ret;
}

void SetDecodeState(int state)
{
	pthread_mutex_lock(&decodemtx);
	decodeState = state;
	pthread_mutex_unlock(&decodemtx);
}
int IsDecodeFinshed()
{
	int ret;
	pthread_mutex_lock(&decodemtx);
	ret = (decodeState == DECODE_NONE);
	pthread_mutex_unlock(&decodemtx);
	return ret;
}
int IsDecodeDone()
{
	int ret;
	pthread_mutex_lock(&decodemtx);
	ret = (decodeState == DECODE_DONE);
	pthread_mutex_unlock(&decodemtx);
	return ret;
}

/********************decodeMtx***************************/
void StartNewDecode()
{
	pthread_mutex_lock(&decodeMtx);
	if(decodeWait == 0) {
		info("start decode...");
		sem_post(&decodeSem);
	}
	pthread_mutex_unlock(&decodeMtx);
}

static void SetDecodeWait(int wait)
{
	pthread_mutex_lock(&decodeMtx);
	decodeWait = wait;
	pthread_mutex_unlock(&decodeMtx);
}

/*************************************************/
static void *OpusDecodePthread(void *arg)
{
	int mode = 8;
	int ch, dtx = 0;
	FILE *out, *in;
	void *wav, *amr;
	int channels = 1;
	int inputSize;
	unsigned int total = 0;
	unsigned char * inputBuf;

	while(1) 
	{
		SetDecodeWait(0);
		
		warning("^_^DECODE_WAIT....\n");

		sem_wait(&decodeSem);
		
		warning("^_^DECODE_START....\n");
		
		SetDecodeWait(1);
		
		Init_Opus_Decode(); //Opus解码
	
		total= 0;
		
		inputSize = channels*2*320;
		inputBuf = (unsigned char*) malloc(inputSize);
		int iLength;
		
		while(g_alipal_encode_flag == 1) 
		{
			int read, iRet;
			
			iLength = EncodeFifoLength();
			
			if(iLength >= inputSize) 
			{
				iLength = inputSize ;
				
				read = EncodeFifoSeek(inputBuf, iLength);
				if(read == 0 || read != iLength) {
					error("read == 0 || read != iLength ");
					break;
				}
				//开始Opus解码
				iRet = Start_Opus_Decode(inputBuf, iLength);
				if(iRet == PAL_VAD_STATUS_STOP)
				{
					break;
				}
			}
			else 
			{
				if(IsEncodeDone())
				{
					break;
				}
				
				///printf("decode delay--------------------------\n");
				usleep(1000);
			
			}	
			total += read;
			
		}

		if(inputBuf != NULL)
			free(inputBuf);
		
		g_alipal_encode_flag = 0;

		SetEncodeState(0);
		
		printf("^_^==========================Opus ASR Decode Success total:%d!\n", total);
			
		//结束Opus解码成功
		End_Opus_Decdoe();
		
	}
	
}


int CreateDecodePthread(void)
{
	int iRet;
	
	pthread_attr_t a_thread_attr; 
	pthread_attr_init(&a_thread_attr);
	pthread_attr_setstacksize(&a_thread_attr, PTHREAD_STACK_MIN * 6);
	
	iRet = pthread_create(&opusDecodePthread, &a_thread_attr, OpusDecodePthread, NULL);
	pthread_attr_destroy(&a_thread_attr);
	if(iRet != 0)
	{
		error("pthread_create error:%s", strerror(iRet));
		return -1;
	}
	return 0;
}

int DestoryDecodePthread(void)
{
	if (opusDecodePthread != 0)
	{
		pthread_join(opusDecodePthread, NULL);
		info("opusDecodePthread end...");
	}	
}


