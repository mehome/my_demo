#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <pthread.h>
#include <mad.h>
#include <semaphore.h>
#include <fcntl.h>
#include "../debug.h"
#include "../fifobuffer.h"

//extern FT_FIFO * g_DecodeAudioFifo;
extern FT_FIFO * g_PlayAudioFifo;

struct buffer {
	unsigned char const *start;
	unsigned long length;
};

int fd;

unsigned long get_file_size(const char *path)
{
	int iRet = -1;
	struct stat buf;  

	if (0 == stat(path, &buf))
	{
		return buf.st_size;
	}

	return iRet;
}

int safe_open(const char *name)
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

static enum mad_flow input(void *data,
		    struct mad_stream *stream)
{
	struct buffer *buffer = data;

	DEBUG_PRINTF("this is input..");

	if (!buffer->length)
		return MAD_FLOW_STOP;

	mad_stream_buffer(stream, buffer->start, buffer->length);

	buffer->length = 0;

	return MAD_FLOW_CONTINUE;
}

static inline signed int scale(mad_fixed_t sample)
{
	/* round */
	sample += (1L << (MAD_F_FRACBITS - 16));

	/* clip */
	if (sample >= MAD_F_ONE)
	sample = MAD_F_ONE - 1;
	else if (sample < -MAD_F_ONE)
	sample = -MAD_F_ONE;

	/* quantize */
	return sample >> (MAD_F_FRACBITS + 1 - 16);
}

//static int iCount = 0;
extern size_t g_playttsaudio_chunk_bytes;


static enum mad_flow output(void *data,
		     struct mad_header const *header,
		     struct mad_pcm *pcm)
{
	int iLength = 0;
	int iIndex = 0;
	int iRet = -1;
    unsigned int nchannels, nsamples;
    mad_fixed_t const *left_ch, *right_ch;
    // pcm->samplerate contains the sampling frequency
    nchannels = pcm->channels;
    nsamples = pcm->length;
    left_ch = pcm->samples[0];
    right_ch = pcm->samples[1];
    short buf[nsamples];
	char *pPlayBuff = NULL;
    int i = 0;

	char *pTemp = NULL;

	while (nsamples--) {
        signed int sample;
		
		sample = scale(*left_ch++);
		buf[i++] = sample;
    }

	//iCount++;
	//DEBUG_PRINTF("this is output ... i*2:%d", i*2);

	ft_fifo_put(g_PlayAudioFifo, (unsigned char *)buf, i*2);

	if (NULL == pTemp)
		pTemp = malloc(g_playttsaudio_chunk_bytes);
	if (NULL == pTemp)
	{
		DEBUG_PRINTF("malloc failed, break;");
		return MAD_FLOW_CONTINUE;
	}

	while (1)
	{
		iLength = ft_fifo_getlenth(g_PlayAudioFifo);
		if (iLength >= g_playttsaudio_chunk_bytes)
		{
			iLength = g_playttsaudio_chunk_bytes;

			ft_fifo_seek(g_PlayAudioFifo, pTemp, 0, iLength);
			ft_fifo_setoffset(g_PlayAudioFifo, iLength);

			PlayTTSAudio_Buffer(pTemp, iLength);

			continue;
		}
		break;
	}

	free(pTemp);
	pTemp = NULL;

	return MAD_FLOW_CONTINUE;
}

static enum mad_flow error(void *data,
		    struct mad_stream *stream,
		    struct mad_frame *frame)
{
	struct buffer *buffer = data;

	fprintf(stderr, "decoding error 0x%04x (%s) at byte offset %u\n",
	  stream->error, mad_stream_errorstr(stream),
	  stream->this_frame - buffer->start);

	/* return MAD_FLOW_BREAK here to stop decoding (and propagate an error) */

	return MAD_FLOW_CONTINUE;
}

int decode(unsigned char const *start, unsigned long length)
{
	struct buffer buffer;
	struct mad_decoder decoder;
	int result;

	/* initialize our private message structure */

	buffer.start  = start;
	buffer.length = length;

	/* configure input, output, and error functions */

	mad_decoder_init(&decoder, &buffer,
		   input, 0 /* header */, 0 /* filter */, output,
		   NULL, 0 /* message */);

	/* start decoding */

	result = mad_decoder_run(&decoder, MAD_DECODER_MODE_SYNC);

	/* release the decoder */

	mad_decoder_finish(&decoder);

	//DEBUG_PRINTF("release the decoder");

	return result;
}

#if 0
void *Decode_pthread(void)
{
	char *pData = NULL;

	int from_fd = 0;

	unsigned long wFilesize = 0;

	g_DecodeAudioFifo = ft_fifo_alloc(DECODE_FIFO_BUFFLTH);

	from_fd = open("test.mp3", O_RDONLY);

	wFilesize = get_file_size("test.mp3");

	pData = malloc(wFilesize);

	read(from_fd, pData, wFilesize);

	close(from_fd);

	fd = safe_open("decoder.wav");

	decode(pData, wFilesize);

	close(fd);
}

#if 1
int main(int argc, char *argv[])
{
	char *pData = NULL;

	int from_fd = 0;

	unsigned long wFilesize = 0;
	g_DecodeAudioFifo = ft_fifo_alloc(DECODE_FIFO_BUFFLTH);

	from_fd = open("test.mp3", O_RDONLY);

	wFilesize = get_file_size("test.mp3");

	pData = malloc(wFilesize);

	read(from_fd, pData, wFilesize);

	close(from_fd);

	fd = safe_open("decoder.wav");
	alsa_init();
	decode(pData, wFilesize);
	alsa_close();
	close(fd);


	return 0;
}
#endif
#endif

