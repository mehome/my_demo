#include "amr_enc.h"

#include <stdio.h>
#include <stdint.h>
#include <limits.h>  
#include <semaphore.h>

#include <interf_enc.h>

#include "fifobuffer.h"
#include "common.h"

#include "myutils/debug.h"

#ifdef USE_UPLOAD_AMR

static pthread_t amrEncodePthread;
static pthread_mutex_t decodeMtx = PTHREAD_MUTEX_INITIALIZER; 
static int encodeState  = 0;
static pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;  
static pthread_cond_t cond = PTHREAD_COND_INITIALIZER; 
static int encodeWait = 0;

static pthread_mutex_t encodeMtx = PTHREAD_MUTEX_INITIALIZER;

static sem_t encodeSem;

static FT_FIFO * g_EncodeFifo = NULL;
#undef FIFO_LENGTH
#define FIFO_LENGTH       4096 *4


void AmrEncodeFifoInit()
{
	if(!g_EncodeFifo)  {
		g_EncodeFifo = ft_fifo_alloc(FIFO_LENGTH);
		if(!g_EncodeFifo) {
			error("ft_fifo_alloc for g_DecodeFifo");
			exit(-1);
		}
	}
}
void AmrEncodeFifoDeinit()
{
	if(g_EncodeFifo) {
		ft_fifo_free(g_EncodeFifo);
	}
}

void AmrEncodeFifoClear()
{
	if(g_EncodeFifo) {
		ft_fifo_clear(g_EncodeFifo);
	}
}

unsigned int AmrEncodeFifoPut(unsigned char *buffer, unsigned int len)
{
	unsigned int length = 0;
	if(g_EncodeFifo) {
		length = ft_fifo_put(g_EncodeFifo, buffer, len);
	}
	return length;
}

unsigned int AmrEncodeFifoLength()
{
	unsigned int len = 0;
	if(g_EncodeFifo) {
		len = ft_fifo_getlenth(g_EncodeFifo);
	}
	return len;
}
unsigned int AmrEncodeFifoSeek( unsigned char *buffer, unsigned int len)
{
	unsigned int length = 0;
	if(g_EncodeFifo) {
		ft_fifo_seek(g_EncodeFifo, buffer, 0, len);
		length = ft_fifo_setoffset(g_EncodeFifo, len);
	}
	return length;	
}

int AmrIsEncodeCancled()
{
	int ret;
	pthread_mutex_lock(&mtx);
	ret = (encodeState == ENCODE_CANCLE);
	pthread_mutex_unlock(&mtx);
	return ret;
}

void AmrSetEncodeState(int state)
{
	pthread_mutex_lock(&mtx);
	encodeState = state;
	pthread_mutex_unlock(&mtx);
}
int AmrIsEncodeFinshed()
{
	int ret;
	pthread_mutex_lock(&mtx);
	ret = (encodeState == ENCODE_NONE);
	pthread_mutex_unlock(&mtx);
	return ret;
}
int AmrIsEncodeDone()
{
	int ret;
	pthread_mutex_lock(&mtx);
	ret = (encodeState == ENCODE_DONE);
	pthread_mutex_unlock(&mtx);
	return ret;
}
void AmrStartNewEncode()
{
	pthread_mutex_lock(&encodeMtx);
	if(encodeWait == 0) {
		info("start encode...");
		sem_post(&encodeSem);
	}
	pthread_mutex_unlock(&encodeMtx);

}

static void AmrSetEncodeWait(int wait)
{
	pthread_mutex_lock(&encodeMtx);
	encodeWait = wait;
	pthread_mutex_unlock(&encodeMtx);
}



#ifdef USE_AMR_8K_16BIT
static enum Mode findMode(const char* str) 
{
	struct {
		enum Mode mode;
		int rate;
	} modes[] = {
		{ MR475,  4750 },
		{ MR515,  5150 },
		{ MR59,   5900 },
		{ MR67,   6700 },
		{ MR74,   7400 },
		{ MR795,  7950 },
		{ MR102, 10200 },
		{ MR122, 12200 }
	};
	int rate = atoi(str);
	int closest = -1;
	int closestdiff = 0;
	unsigned int i;
	for (i = 0; i < sizeof(modes)/sizeof(modes[0]); i++) {
		if (modes[i].rate == rate)
			return modes[i].mode;
		if (closest < 0 || closestdiff > abs(modes[i].rate - rate)) {
			closest = i;
			closestdiff = abs(modes[i].rate - rate);
		}
	}
	fprintf(stderr, "Using bitrate %d\n", modes[closest].rate);
	return modes[closest].mode;
}
int amr_enc_file(const char *infile, const char *outfile)
{
	
	enum Mode mode = MR122;
	int ch, dtx = 0;
	//const char *infile, *outfile;
	FILE *out;
	void *wav, *amr;
	int format, sampleRate, channels, bitsPerSample;
	int inputSize;
	uint8_t* inputBuf;


	wav = wav_read_open(infile);
	if (!wav) {
		fprintf(stderr, "Unable to open wav file %s\n", infile);
		return 1;
	}
	if (!wav_get_header(wav, &format, &channels, &sampleRate, &bitsPerSample, NULL)) {
		fprintf(stderr, "Bad wav file %s\n", infile);
		return 1;
	}
	if (format != 1) {
		fprintf(stderr, "Unsupported WAV format %d\n", format);
		return 1;
	}
	if (bitsPerSample != 16) {
		fprintf(stderr, "Unsupported WAV sample depth %d\n", bitsPerSample);
		return 1;
	}
	if (channels != 1)
		fprintf(stderr, "Warning, only compressing one audio channel\n");
	if (sampleRate != 8000)
		fprintf(stderr, "Warning, AMR-NB uses 8000 Hz sample rate (WAV file has %d Hz)\n", sampleRate);
	inputSize = channels*2*160;
	inputBuf = (uint8_t*) malloc(inputSize*2);

	amr = Encoder_Interface_init(dtx);
	out = fopen(outfile, "wb");
	if (!out) {
		perror(outfile);
		return 1;
	}

	fwrite("#!AMR\n", 1, 6, out);
	info("Encoder_Interface_init");
	while (1) {
		if(IsHttpRequestCancled())
			break;
		short buf[160];
		uint8_t outbuf[500];
		int read, i, n;
		read = wav_read_data(wav, inputBuf, inputSize);
		read /= channels;
		read /= 2;
		if (read < 160)
			break;
		for (i = 0; i < 160; i++) {
			const uint8_t* in = &inputBuf[2*channels*i];
			buf[i] = in[0] | (in[1] << 8);
		}
		n = Encoder_Interface_Encode(amr, mode, buf, outbuf, 0);
	
		fwrite(outbuf, 1, n, out);
	}
	free(inputBuf);
	fclose(out);
	Encoder_Interface_exit(amr);
	
	info("Encoder_Interface_exit");
	wav_read_close(wav);

	return 0;
}
int amr_enc(const char *infile, const char *outfile)
{
	
	enum Mode mode = MR795;
	int ch, dtx = 0;
	//const char *infile, *outfile;
	FILE *out, *in;
	void *wav, *amr;
	int format, sampleRate, channels, bitsPerSample;
	int inputSize;
	uint8_t* inputBuf;

	format = 1;
	channels = 1;
	sampleRate = 8000;
	bitsPerSample = 16;
	
	in = fopen(infile, "rb");
	if (!in) {
		perror(infile);
		return 1;
	}
	if (format != 1) {
		fprintf(stderr, "Unsupported WAV format %d\n", format);
		return 1;
	}
	if (bitsPerSample != 16) {
		fprintf(stderr, "Unsupported WAV sample depth %d\n", bitsPerSample);
		return 1;
	}
	if (channels != 1)
		fprintf(stderr, "Warning, only compressing one audio channel\n");
	if (sampleRate != 8000)
		fprintf(stderr, "Warning, AMR-NB uses 8000 Hz sample rate (WAV file has %d Hz)\n", sampleRate);
	inputSize = channels*2*160;
	inputBuf = (uint8_t*) malloc(inputSize);

	amr = Encoder_Interface_init(dtx);
	out = fopen(outfile, "wb");
	if (!out) {
		perror(outfile);
		return 1;
	}

	fwrite("#!AMR\n", 1, 6, out);
	info("Encoder_Interface_init");
	while (1) {
		short buf[160];
		uint8_t outbuf[500];
		int read, i, n;
		//read = wav_read_data(wav, inputBuf, inputSize);
		read = fread(inputBuf, 1, inputSize, in);
		info("n:%d",read);
		if(read < 0)
			break;
		read /= channels;
		read /= 2;
		if (read < 160)
			break;
		for (i = 0; i < 160; i++) {
			const uint8_t* in = &inputBuf[2*channels*i];
			buf[i] = in[0] | (in[1] << 8);
		}
		n = Encoder_Interface_Encode(amr, mode, buf, outbuf, 0);
		fwrite(outbuf, 1, n, out);
	}
	free(inputBuf);
	fclose(out);
	fclose(in);
	Encoder_Interface_exit(amr);


	return 0;
}
static void *AmrEncodePthread(void *arg)
{
	//enum Mode mode = MR122;
	enum Mode mode = MR795;
	int ch, dtx = 0;
	FILE *out, *in;
	void *wav, *amr;
	int channels = 1;
	int inputSize;
	unsigned int total = 0;
	uint8_t* inputBuf;
	
	while(1) {
		
		AmrSetEncodeState(ENCODE_NONE);
		warning("ENCODE_NONE");
		AmrSetEncodeWait(0);
		sem_wait(&encodeSem);
		AmrSetEncodeWait(1);
	
		amr = Encoder_Interface_init(dtx);
		total= 0;
		AmrSetEncodeState(ENCODE_STARTED);
		out = fopen("/tmp/enc.amr", "wb");
		if (!out) {
			error("open /tmp/enc.amr failed");
			break;
		}

		fwrite("#!AMR\n", 1, 6, out);
		inputSize = channels*2*160;
		inputBuf = (uint8_t*) malloc(inputSize);
		int iLength;
		total = 0;
		while(1) {
				short buf[160];
				uint8_t outbuf[500];
				int read, i, n;
				
				AmrSetEncodeState(ENCODE_ONGING);
				
				iLength = CaptureFifoLength();
				if(iLength > inputSize) {
					iLength = inputSize ;
					
					read = CaptureFifoSeek(inputBuf, iLength);
					if(read == 0 || read != iLength) {
						error("read == 0 || read != iLength ");
						break;
					}
						
					read /= channels;
					read /= 2;
					if(read <= 0)
						break;
					for (i = 0; i < 160; i++) {
						const uint8_t* in = &inputBuf[2*channels*i];
						buf[i] = in[0] | (in[1] << 8);
					}
					n = Encoder_Interface_Encode(amr, mode, buf, outbuf, 0);

					fwrite(outbuf, 1, n, out);
					if(AmrEncodeFifoPut(outbuf,n) != n) {
						error("AmrEncodeFifoPut(outbuf,n) != n");
						break;
					}
					total += n;
					if(total >= UPLOAD_SIZE)  {
						NewHttpRequest();
					} 
					
				}else {
					if(IsCaptureFinshed())
						break;
				}	
		}
		error("Encode total:%d",total);
		PrintSysTime("Encode total");
		fclose(out);
		AmrSetEncodeState(ENCODE_DONE);
		Encoder_Interface_exit(amr);
	}
	
}
#endif



#ifdef USE_AMR_16K_16BIT
int findMode(const char* str) 
{
	struct {
		int mode;
		int rate;
	} modes[] = {
		{ 0,  6600 },
		{ 1,  8850 },
		{ 2, 12650 },
		{ 3, 14250 },
		{ 4, 15850 },
		{ 5, 18250 },
		{ 6, 19850 },
		{ 7, 23050 },
		{ 8, 23850 }
	};
	int rate = atoi(str);
	int closest = -1;
	int closestdiff = 0;
	unsigned int i;
	for (i = 0; i < sizeof(modes)/sizeof(modes[0]); i++) {
		if (modes[i].rate == rate)
			return modes[i].mode;
		if (closest < 0 || closestdiff > abs(modes[i].rate - rate)) {
			closest = i;
			closestdiff = abs(modes[i].rate - rate);
		}
	}
	fprintf(stderr, "Using bitrate %d\n", modes[closest].rate);
	return modes[closest].mode;
}
int amr_enc_file(const char *infile, const char *outfile)
{
	int mode = 8;
	int ch, dtx = 0;

	FILE* out, *in;
	void *wav, *amr;
	int format, sampleRate, channels, bitsPerSample;
	int inputSize;
	uint8_t* inputBuf;
	wav = wav_read_open(infile);
	if (!wav) {
		fprintf(stderr, "Unable to open wav file %s\n", infile);
		return 1;
	}
	if (!wav_get_header(wav, &format, &channels, &sampleRate, &bitsPerSample, NULL)) {
		fprintf(stderr, "Bad wav file %s\n", infile);
		return 1;
	}	
	
	if (format != 1) {
		fprintf(stderr, "Unsupported WAV format %d\n", format);
		return 1;
	}
	if (bitsPerSample != 16) {
		fprintf(stderr, "Unsupported WAV sample depth %d\n", bitsPerSample);
		return 1;
	}
	if (channels != 1)
		fprintf(stderr, "Warning, only compressing one audio channel\n");
	if (sampleRate != 16000)
		fprintf(stderr, "Warning, AMR-WB uses 16000 Hz sample rate (WAV file has %d Hz)\n", sampleRate);
	inputSize = channels*2*320;
	inputBuf = (uint8_t*) malloc(inputSize);

	amr = E_IF_init();
	out = fopen(outfile, "wb");
	if (!out) {
		perror(outfile);
		return 1;
	}

	fwrite("#!AMR-WB\n", 1, 9, out);
	while (1) {
		int read, i, n;
		short buf[320];
		uint8_t outbuf[500];

		read = wav_read_data(wav, inputBuf, inputSize);
		read /= channels;
		read /= 2;
		if (read < 320)
			break;
		for (i = 0; i < 320; i++) {
			const uint8_t* in = &inputBuf[2*channels*i];
			buf[i] = in[0] | (in[1] << 8);
		}
		n = E_IF_encode(amr, mode, buf, outbuf, dtx);
		fwrite(outbuf, 1, n, out);
	}
	free(inputBuf);
	fclose(out);
	E_IF_exit(amr);
	wav_read_close(wav);

	return 0;
}
int amr_enc(const char *infile, const char *outfile)
{
	int mode = 8;
	int ch, dtx = 0;

	FILE* out, *in;
	void *wav, *amr;
	int format, sampleRate, channels, bitsPerSample;
	int inputSize;
	uint8_t* inputBuf;
	format = 1;
	channels = 1;
	sampleRate = 16000;
	bitsPerSample = 16;

	in = fopen(infile, "rb");
	if (!in) {
		perror(infile);
		return 1;
	}


	if (format != 1) {
		fprintf(stderr, "Unsupported WAV format %d\n", format);
		return 1;
	}
	if (bitsPerSample != 16) {
		fprintf(stderr, "Unsupported WAV sample depth %d\n", bitsPerSample);
		return 1;
	}
	if (channels != 1)
		fprintf(stderr, "Warning, only compressing one audio channel\n");
	if (sampleRate != 16000)
		fprintf(stderr, "Warning, AMR-WB uses 16000 Hz sample rate (WAV file has %d Hz)\n", sampleRate);
	inputSize = channels*2*320;
	inputBuf = (uint8_t*) malloc(inputSize);

	amr = E_IF_init();
	out = fopen(outfile, "wb");
	if (!out) {
		perror(outfile);
		return 1;
	}

	fwrite("#!AMR-WB\n", 1, 9, out);
	while (1) {
		int read, i, n;
		short buf[320];
		uint8_t outbuf[500];

		read = fread(inputBuf, 1, inputSize, in);
		read /= channels;
		read /= 2;
		if (read < 320)
			break;
		for (i = 0; i < 320; i++) {
			const uint8_t* in = &inputBuf[2*channels*i];
			buf[i] = in[0] | (in[1] << 8);
		}
		n = E_IF_encode(amr, mode, buf, outbuf, dtx);
		fwrite(outbuf, 1, n, out);
	}
	free(inputBuf);
	fclose(out);
	E_IF_exit(amr);
	wav_read_close(wav);

	return 0;
}
static void *AmrEncodePthread(void *arg)
{
	//int mode = 4;
	int mode = 8;
	int ch, dtx = 0;
	FILE *out, *in;
	void *wav, *amr;
	int channels = 1;
	int inputSize;
	unsigned int total = 0;
	uint8_t* inputBuf;
	
	while(1) {
		
		AmrSetEncodeState(ENCODE_NONE);
		warning("ENCODE_NONE");
		AmrSetEncodeWait(0);
		sem_wait(&encodeSem);
		AmrSetEncodeWait(1);
	
		amr = E_IF_init();
		total= 0;
		AmrSetEncodeState(ENCODE_STARTED);
		out = fopen("/tmp/encwb.amr", "wb");
		if (!out) {
			error("open /tmp/encwb.amr failed");
			break;
		}

		fwrite("#!AMR-WB\n", 1, 9, out) ;
		inputSize = channels*2*320;
		inputBuf = (uint8_t*) malloc(inputSize);
		int iLength;
		
		while(1) {
				short buf[320];
				uint8_t outbuf[500];
				int read, i, n;
				
				AmrSetEncodeState(ENCODE_ONGING);
				
				iLength = CaptureFifoLength();
				if(iLength > inputSize) {
					iLength = inputSize ;
					
					read = CaptureFifoSeek(inputBuf, iLength);
					if(read == 0 || read != iLength) {
						error("read == 0 || read != iLength ");
						break;
					}
					
					read /= channels;
					read /= 2;
					if(read < 320)
						break;
					for (i = 0; i < 320; i++) {
						const uint8_t* in = &inputBuf[2*channels*i];
						buf[i] = in[0] | (in[1] << 8);
					}

					n = E_IF_encode(amr, mode, buf, outbuf, 1);

					fwrite(outbuf, 1, n, out);
					if(AmrEncodeFifoPut(outbuf,n) != n) {
						error("AmrEncodeFifoPut(outbuf,n) != n");
						break;
					}
					total += n;
					if(total >= UPLOAD_SIZE)  {
						NewHttpRequest();
					} 
					
				}else {
					if(IsCaptureFinshed())
						break;
				}	
		}
		error("Encode total:%d",total);
		//error("Encode total:%d",total);
			PrintSysTime("Encode total");
		fclose(out);
		AmrSetEncodeState(ENCODE_DONE);
		E_IF_exit(amr);
	}
	
}
#endif


int   CreateAmrEncodePthread(void)
{
	int iRet;

	AmrEncodeFifoInit();
	
	pthread_attr_t a_thread_attr; 
	pthread_attr_init(&a_thread_attr);
	pthread_attr_setstacksize(&a_thread_attr, PTHREAD_STACK_MIN * 4);

	iRet = pthread_create(&amrEncodePthread, &a_thread_attr, AmrEncodePthread, NULL);
	pthread_attr_destroy(&a_thread_attr);
	if(iRet != 0)
	{
		error("pthread_create error:%s", strerror(iRet));
		return -1;
	}
	return 0;
}
int  DestoryAmrEncodePthread(void)
{
	AmrEncodeFifoDeinit();
	if (amrEncodePthread != 0)
	{
		pthread_join(amrEncodePthread, NULL);
		info("amrEncodePthread end...");
	}	
}


#endif

