

#include "amr_dec.h"

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <limits.h>  
#include <semaphore.h>

#include <interf_dec.h>

#include "myutils/debug.h"
#include "common.h"
#include "fifobuffer.h"

typedef unsigned char uint8_t;
#define AVCODEC_MAX_AUDIO_FRAME_SIZE 192000 // 1 second of 48khz 32bit audio

/* From WmfDecBytesPerFrame in dec_input_format_tab.cpp */
const int sizes[] = { 12, 13, 15, 17, 19, 20, 26, 31, 5, 6, 5, 5, 0, 0, 0, 0 };


#ifdef ENABLE_AMR_DECODE

static FT_FIFO * g_DecodeFifo = NULL;
#define FIFO_LENGTH     4096
static pthread_t amrDecodePthread;
static pthread_mutex_t decodeMtx = PTHREAD_MUTEX_INITIALIZER; 
static int decodeState  = 0;
static int decodeWait = 0;
static sem_t decodeSem;

static pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;  
static pthread_cond_t cond = PTHREAD_COND_INITIALIZER; 
#endif

int amr_dec(char *infile ,char *outfile)
{
	FILE* in, *out;
	char header[6];
	int n;
	void *wav, *amr;

	in = fopen(infile, "rb");
	if (!in) {
		perror(infile);
		return 1;
	}
	n = fread(header, 1, 6, in);
	if (n != 6 || memcmp(header, "#!AMR\n", 6)) {
		fprintf(stderr, "Bad header\n");
		return 1;
	}
	out = fopen(outfile, "wb");
	if (!out) {
		error("Unable to open %s /root/pcm.raw");
		return 1;
	}

	amr = Decoder_Interface_init();
	while (1) {
		uint8_t buffer[500], littleendian[320], *ptr;
		int size, i;
		int16_t outbuffer[160];
		/* Read the mode byte */
		n = fread(buffer, 1, 1, in);
		if (n <= 0)
			break;
		/* Find the packet size */
		size = sizes[(buffer[0] >> 3) & 0x0f];
		n = fread(buffer + 1, 1, size, in);
		if (n != size)
			break;

		/* Decode the packet */
		Decoder_Interface_Decode(amr, buffer, outbuffer, 0);

		/* Convert to little endian and write to wav */
		ptr = littleendian;
		for (i = 0; i < 160; i++) {
			*ptr++ = (outbuffer[i] >> 0) & 0xff;
			*ptr++ = (outbuffer[i] >> 8) & 0xff;
		}
		//wav_write_data(wav, littleendian, 320);
		size_t nWrite = fwrite(littleendian, 1 , 320, out);    
		if(nWrite != 320)  {
			error("fwrite :%d", nWrite);
			break;
			
		}
	}
	fclose(in);
	fclose(out);
	Decoder_Interface_exit(amr);
	//wav_write_close(wav);
	return 0;
}

int amr_dec_file(const char *infile, const char *outfile) 
{
	FILE* in;
	char header[9];
	int n;
	void *wav, *amr;
	
	in = fopen(infile, "rb");
	if (!in) {
		perror(infile);
		return 1;
	}
	n = fread(header, 1, 9, in);
	if (n != 9 || memcmp(header, "#!AMR\n", 6)) {
		error( "Bad header\n");
		return 1;
	}

	wav = wav_write_open(outfile, 8000, 16, 1);
	if (!wav) {
		error( "Unable to open %s\n", outfile);
		return 1;
	}
	
	amr = Decoder_Interface_init();
	info("Decoder_Interface_init");
	while (1) {
		uint8_t buffer[500], littleendian[320], *ptr;
		int size, i;
		int16_t outbuffer[160];
		/* Read the mode byte */
		n = fread(buffer, 1, 1, in);
		if (n <= 0)
			break;
		/* Find the packet size */
		size = sizes[(buffer[0] >> 3) & 0x0f];
		n = fread(buffer + 1, 1, size, in);
		if (n != size)
			break;

		/* Decode the packet */
		Decoder_Interface_Decode(amr, buffer, outbuffer, 0);

		/* Convert to little endian and write to wav */
		ptr = littleendian;
		for (i = 0; i < 160; i++) {
			*ptr++ = (outbuffer[i] >> 0) & 0xff;
			*ptr++ = (outbuffer[i] >> 8) & 0xff;
		}
		wav_write_data(wav, littleendian, 320);
	}
	fclose(in);
	Decoder_Interface_exit(amr);
	info("Decoder_Interface_exit");
	wav_write_close(wav);

	return 0;
}

#ifdef ENABLE_AMR_DECODE
void DecodeFifoInit()
{
	if(!g_DecodeFifo)  {
		g_DecodeFifo = ft_fifo_alloc(FIFO_LENGTH);
		if(!g_DecodeFifo) {
			error("ft_fifo_alloc for g_DecodeFifo");
			exit(-1);
		}
	}
}
void DecodeFifoDeinit()
{
	if(g_DecodeFifo) {
		ft_fifo_free(g_DecodeFifo);
	}
}

void DecodeFifoClear()
{
	if(g_DecodeFifo) {
		ft_fifo_clear(g_DecodeFifo);
	}
}

unsigned int DecodeFifoPut(unsigned char *buffer, unsigned int len)
{
	unsigned int length = 0;
	if(g_DecodeFifo) {
		length = ft_fifo_put(g_DecodeFifo, buffer, len);
	}
	return length;
}

unsigned int DecodeFifoLength()
{
	unsigned int len = 0;
	if(g_DecodeFifo) {
		len = ft_fifo_getlenth(g_DecodeFifo);
	}
	return len;
}
unsigned int DecodeFifoSeek( unsigned char *buffer, unsigned int len)
{
	unsigned int length = 0;
	if(g_DecodeFifo) {
		ft_fifo_seek(g_DecodeFifo, buffer, 0, len);
		length = ft_fifo_setoffset(g_DecodeFifo, len);
	}
	return length;	
}

int IsDecodeCancled()
{
	int ret;
	pthread_mutex_lock(&mtx);
	ret = (decodeState == DECODE_CANCLE);
	pthread_mutex_unlock(&mtx);
	return ret;
}

void SetDecodeState(int state)
{
	pthread_mutex_lock(&mtx);
	decodeState = state;
	pthread_mutex_unlock(&mtx);
}
int IsDecodeFinshed()
{
	int ret;
	pthread_mutex_lock(&mtx);
	ret = (decodeState == DECODE_NONE);
	pthread_mutex_unlock(&mtx);
	return ret;
}
int IsDecodeDone()
{
	int ret;
	pthread_mutex_lock(&mtx);
	ret = (decodeState == DECODE_DONE);
	pthread_mutex_unlock(&mtx);
	return ret;
}
void StartNewDecode()
{
	pthread_mutex_lock(&decodeMtx);
	if(decodeWait == 0) {
		//sem_post(&decodeSem);
	}
	pthread_mutex_unlock(&decodeMtx);

}

static void SetEncodeWait(int wait)
{
	pthread_mutex_lock(&decodeMtx);
	decodeWait = wait;
	pthread_mutex_unlock(&decodeMtx);
}



static void *AmrDecodePthread(void *arg)
{
	char header[6];
	int n, iLength;
	void *wav, *amr;
	FILE* in, *out;
	info("start AmrDecodePthread");
	int max = 0;

	
	out = fopen("/tmp/test.pcm", "wb");
	if (!out) {
		error("Unable to open /tmp/test.pcm");
		return ;
	}
	amr = Decoder_Interface_init();
	while(1) {
		uint8_t buffer[500], littleendian[320], *ptr;
	
		int size, i;
		int16_t outbuffer[160];

		pthread_mutex_lock(&mtx);    
		iLength = ft_fifo_getlenth(g_DecodeFifo);

		while(ft_fifo_getlenth(g_DecodeFifo) < 6) {
			 info("pthread_cond_wait...");
			 pthread_cond_wait(&cond, &mtx);     
		}

		char *pheader = calloc(1, 7);
		if (NULL == pheader)
		{
			error("malloc failed.\n");
			break;
		}
		ft_fifo_seek(g_DecodeFifo, pheader, 0, 6);
		ft_fifo_setoffset(g_DecodeFifo, 6);
	
		pheader[6]='\0';
		if(memcmp(pheader, "#!AMR\n", 6)) {
			free(pheader);
			pthread_mutex_unlock(&mtx);    
			break;
		}
  		free(pheader);
		int iLen;
restart: 
		bzero(buffer, 500);
		bzero(littleendian, 320);
		while(1) {
			
			iLen = ft_fifo_getlenth(g_DecodeFifo);
			if(iLen <= 0) 
				goto exit;
			if(iLen > 0) 
				break;		
				//pthread_cond_wait(&cond, &mtx);     
		}
		iLen = ft_fifo_seek(g_DecodeFifo, buffer, 0, 1);
		max += iLen;

		ft_fifo_setoffset(g_DecodeFifo, 1);
		size = sizes[(buffer[0] >> 3) & 0x0f];
		info("size:%d",size) ;
		while(ft_fifo_getlenth(g_DecodeFifo) < size)	{
			pthread_cond_wait(&cond, &mtx);     
		}
		info("ft_fifo_seek");
		iLen = ft_fifo_seek(g_DecodeFifo, buffer+1, 0,size);
		max += iLen;
		info("iLen:%d",iLen);
		ft_fifo_setoffset(g_DecodeFifo, iLen);
		info("Decoder_Interface_Decode");
		/* Decode the packet */
		Decoder_Interface_Decode(amr, buffer, outbuffer, 0);
		/* Convert to little endian and write to wav */
		ptr = littleendian;
		for (i = 0; i < 160; i++) {
			*ptr++ = (outbuffer[i] >> 0) & 0xff;
			*ptr++ = (outbuffer[i] >> 8) & 0xff;
		}
		//AplayFifoPut(littleendian, 320);
		info("fwrite");
		size_t nWrite = fwrite(littleendian, 1 , 320, out);  
		if(nWrite != 320)  {
			error("fwrite :%d", nWrite);
			break;
			
		}
		goto restart;
	}
exit:
	info("max:%d",max);
	decodeState = 1;
	fclose(out);
	Decoder_Interface_exit(amr);
	eval("aplay", "-r", "8000", "-f" , "s16_le", "-c" ,"1", "/tmp/test.pcm");

}

int  CreateDecodePthread(void)
{
	int iRet;
	pthread_attr_t a_thread_attr; 
	
	DecodeFifoInit();
		
	pthread_attr_init(&a_thread_attr);
	pthread_attr_setstacksize(&a_thread_attr, PTHREAD_STACK_MIN*4);

	iRet = pthread_create(&amrDecodePthread, &a_thread_attr, AmrDecodePthread, NULL);
	pthread_attr_destroy(&a_thread_attr);
	if(iRet != 0)
	{
		error("pthread_create error:%s", strerror(iRet));
		return -1;
	}
	return 0;
}
int  DestoryDecodePthread(void)
{

	if (amrDecodePthread != 0)
	{
		pthread_join(amrDecodePthread, NULL);
		info("amrDecodePthread end...");
	}	

	DecodeFifoDeinit();
}

#endif

