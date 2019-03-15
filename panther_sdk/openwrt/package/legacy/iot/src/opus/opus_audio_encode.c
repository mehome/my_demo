#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <limits.h> 
#include <semaphore.h>

#include "myutils/debug.h"
#include "fifobuffer.h"
#include "opus_audio_encode.h"
//#include "alipal_conver_opus.h"
#include "opus.h"
#include "opus_encode_convert.h"



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

static pthread_t opusEncodePthread;

static pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;  
static pthread_cond_t cond = PTHREAD_COND_INITIALIZER; 
static pthread_mutex_t encodeMtx = PTHREAD_MUTEX_INITIALIZER;

/////////////////////////////////////////////////////////////////

static FT_FIFO * g_EncodeFifo = NULL;


void OpusEncodeFifoInit()
{
	if(!g_EncodeFifo)  {
		g_EncodeFifo = ft_fifo_alloc(FIFO_LENGTH);
		if(!g_EncodeFifo) {
			error("ft_fifo_alloc for g_DecodeFifo");
			exit(-1);
		}
	}
}
void OpusEncodeFifoDeinit()
{
	if(g_EncodeFifo) {
		ft_fifo_free(g_EncodeFifo);
	}
}

void OpusEncodeFifoClear()
{
	if(g_EncodeFifo) {
		ft_fifo_clear(g_EncodeFifo);
	}
}

unsigned int OpusEncodeFifoPut(unsigned char *buffer, unsigned int len)
{
	unsigned int length = 0;
	if(g_EncodeFifo) {
		length = ft_fifo_put(g_EncodeFifo, buffer, len);
	}
	return length;
}

unsigned int OpusEncodeFifoLength()
{
	unsigned int len = 0;
	if(g_EncodeFifo) {
		len = ft_fifo_getlenth(g_EncodeFifo);
	}
	return len;
}
unsigned int OpusEncodeFifoSeek( unsigned char *buffer, unsigned int len)
{
	unsigned int length = 0;
	if(g_EncodeFifo) {
		ft_fifo_seek(g_EncodeFifo, buffer, 0, len);
		length = ft_fifo_setoffset(g_EncodeFifo, len);
	}
	return length;	
}

int OpusIsEncodeCancled()
{
	int ret;
	pthread_mutex_lock(&mtx);
	ret = (encodeState == ENCODE_CANCLE);
	pthread_mutex_unlock(&mtx);
	return ret;
}

void SetOpusEncodeCancle()
{
	pthread_mutex_lock(&mtx);
	encodeState = ENCODE_CANCLE;
	pthread_mutex_unlock(&mtx);
}


void OpusSetEncodeState(int state)
{
	pthread_mutex_lock(&mtx);
	encodeState = state;
	pthread_mutex_unlock(&mtx);
}
int OpusIsEncodeFinshed()
{
	int ret;
	pthread_mutex_lock(&mtx);
	ret = (encodeState == ENCODE_NONE);
	pthread_mutex_unlock(&mtx);
	return ret;
}
int OpusIsEncodeDone()
{
	int ret;
	pthread_mutex_lock(&mtx);
	ret = (encodeState == ENCODE_DONE);
	pthread_mutex_unlock(&mtx);
	return ret;
}
void OpusStartNewEncode()
{
	pthread_mutex_lock(&encodeMtx);
	if(encodeWait == 0) {
		info("sem_post(&encodeSem) opus start");
		sem_post(&encodeSem);
	}
	pthread_mutex_unlock(&encodeMtx);

}

static void OpusSetEncodeWait(int wait)
{
	pthread_mutex_lock(&encodeMtx);
	encodeWait = wait;
	pthread_mutex_unlock(&encodeMtx);
}

static void int_to_char(unsigned int i, unsigned char ch[4])
{
    ch[0] = i>>24;
    ch[1] = (i>>16)&0xFF;
    ch[2] = (i>>8)&0xFF;
    ch[3] = i&0xFF;
}
//#define SAVE_OPUS_FILE 1
#ifdef SAVE_OPUS_FILE
#define OPUS_FILE "/tmp/sample.opus"
#endif
static void *OpusEncodePthread(void *arg)
{
	int mode = 8;
	int ch, dtx = 0;
	FILE *out = NULL;
	void *wav, *amr;
	int channels = 1;
	int inputSize;
	unsigned int total = 0;
	unsigned char * inputBuf;
	char g_EncodeFlag = 1;
	unsigned char int_field[4];
	int http_flag = 0;
	warning("^_^OpusEncodePthread start....");
	int *encode_length;
	opus_uint32 *enc_final_range_length;
	while(1) 
	{
		warning("^_^ENCODE_WAIT....");
		OpusSetEncodeState(ENCODE_NONE);
		OpusSetEncodeWait(0);

		sem_wait(&encodeSem);
        http_flag = 0;
		warning("^_^ENCODE_START....");
		http_flag = 0;
		OpusSetEncodeWait(1);
		OpusSetEncodeState(ENCODE_ONGING);
		OpusEncodeFifoClear();
#ifdef SAVE_OPUS_FILE
		out = fopen(OPUS_FILE, "wb+");
		if (!out) {
			error("open %s failed", OPUS_FILE);
			break;
		}
#endif
		//fwrite("#!AMR\n", 1, 6, out);
	//	Init_Opus_Encode(); //Opus编码
		opus_encode_buffer_init();	
		inputSize = channels*2*320;
		//inputSize = channels*320;
		inputBuf = (unsigned char*) malloc(inputSize);
		int iLength;
	
		while(1)
		{
            if(OpusIsEncodeCancled())
            {
                info("opus encode thread cancled");
                break; 
            }
			unsigned char outbuf[1500];
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
				//n = Start_Opus_Encode((short *)inputBuf, iLength, outbuf,encode_length,enc_final_range);

                opus_encode_buffer_convert((short *)inputBuf, iLength, outbuf,&encode_length,&enc_final_range_length);
                trace("encode_length =%d,enc_final_range_length =%d,outbuf =%d\n",encode_length,enc_final_range_length,strlen(outbuf));
				int_to_char(encode_length, int_field);
				
				//if(EncodeFifoPut(int_field, sizeof(unsigned char)) !=   sizeof(unsigned char)) 

				if(OpusEncodeFifoPut(int_field, 4) !=   4) 

				{
					error("1EncodeFifoPut(outbuf,n) != n");
					break;
				}
				total = total + sizeof(int);
				//fwrite(int_field, 1, sizeof(unsigned char), out);
#ifdef SAVE_OPUS_FILE
				fwrite(int_field, 1, 4, out);
#endif				
				int_to_char(enc_final_range_length, int_field);
				if(OpusEncodeFifoPut(int_field, 4) !=   4) 

				{
					error("1EncodeFifoPut(outbuf,n) != n");
					break;
				}
				//fwrite(int_field, 1, sizeof(unsigned char), out);
#ifdef SAVE_OPUS_FILE				
				fwrite(int_field, 1, 4, out);
#endif				
				total = total + sizeof(int);
				//if(EncodeFifoPut(outbuf, n) != n) 
				if(OpusEncodeFifoPut(outbuf, encode_length) != encode_length) 
				{
					error("2EncodeFifoPut(outbuf,n) != n");
					break;
				}
#ifdef SAVE_OPUS_FILE				
				fwrite(outbuf, 1, encode_length, out);
#endif
				total = total + encode_length;
                trace("total =%d,inputSize =%d",total,inputSize);
                if(OpusIsEncodeCancled())
                {
                    info("opus encode thread cancled");
                    break; 
                }
				if(total >= inputSize)
				{		
				 	if(http_flag == 0)
                    {
						http_flag = 1;
                        NewHttpRequest();	//通知http上报录音的数据
					}	
				    total = 0;	
				}
			}
			else 
			{
                if(OpusIsEncodeCancled())
                {
                    info("opus encode thread cancled");
                    break; 
                }
				if(!get_mic_record_status())
				{
				    int encode_length= OpusEncodeFifoLength();
					printf("[VOICE_DEBUG]++++++++++Get Capture FIFO Len:%d  < 640 encode_length =%d\n", iLength,encode_length);
				//	NewHttpRequest();
					break;
				}
				usleep(500);
			}
			
			
	}
	if(out)
		fclose(out);
		if(inputBuf)
		free(inputBuf);
	error("[VOICE_DEBUG]###########Encode end total:%d\n",total);
		
	total = 0;
	
	//SetCaptureState(0);	

	//结束Opus编码
	End_Opus_Encdoe();

	//OpusSetEncodeState(ENCODE_DONE);

	}
	
}


int CreateOpusEncodePthread(void)
{
	int iRet;

	OpusEncodeFifoInit();
	
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

int  DestoryOpusEncodePthread(void)
{
	OpusEncodeFifoDeinit();
	
	if (opusEncodePthread != 0)
	{
		pthread_join(opusEncodePthread, NULL);
		info("opusEncodePthread end...");
	}	
}


