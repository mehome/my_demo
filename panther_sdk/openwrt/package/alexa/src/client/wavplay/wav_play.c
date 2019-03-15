#include "wav_play.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <errno.h>
#include <pthread.h>

#include "../alert/AlertHead.h"
#include "../globalParam.h"

extern AlertManager *alertManager;

/**
 * @file mtdm_misc.c system help function
 *
 * @author  Frank Wang
 * @date    2010-10-06
 */

int snd_wav_get_fmt(wavcontainer_t *wav, snd_pcm_format_t *snd_format)
{
	if (LE_SHORT(wav->format.format) != WAV_FMT_PCM)
		return -1;

	switch (LE_SHORT(wav->format.sample_length)) {
	case 32:
		*snd_format = SND_PCM_FORMAT_S32_LE;
		break;
	case 24:
		*snd_format = SND_PCM_FORMAT_S24_LE;
		break;
	case 16:
		*snd_format = SND_PCM_FORMAT_S16_LE;
		break;
	case 8:
		*snd_format = SND_PCM_FORMAT_U8;
		break;
	default:
		*snd_format = SND_PCM_FORMAT_UNKNOWN;
		break;
	}

	return 0;
}

int wav_p_checkvalid(wavcontainer_t *container)
{
	if (container->header.magic != WAV_RIFF) {
		fprintf(stderr, "invalid wav magic.\n");
		return -1;
	}

	if (container->header.type != WAV_WAVE) {
		fprintf(stderr, "invalid wav type.\n");
		return -1;
	}
	if (container->format.magic != WAV_FMT) {
		fprintf(stderr, "invalid wav format magic.\n");
		return -1;
	}
	if ((container->format.fmt_size != LE_INT(16)) && (container->format.fmt_size != LE_INT(24)) && (container->format.fmt_size != LE_INT(32))) {
		fprintf(stderr, "invalid wav format size.\n");
		return -1;
	}
	if ((container->format.channels != LE_SHORT(1)) && (container->format.channels != LE_SHORT(2))) {
		fprintf(stderr, "invalid wav format channels.\n");
		return -1;
	}
	if (container->chunk.type != WAV_DATA) {
		fprintf(stderr, "invalid wav chunk.\n");
		return -1;
	}

	return 0;
}

int snd_wav_init(snd_pcm_t *pcm, sndpcmcontainer_t *sndpcm, wavcontainer_t *wav, off64_t pcm_len)
{
	int rc;
	snd_pcm_hw_params_t *hwparams = NULL;
	snd_pcm_format_t format;
	uint32_t exact_rate;
	uint32_t buffer_time, period_time;
	int dir = 0;

	snd_pcm_hw_params_alloca(&hwparams);

	rc = snd_pcm_hw_params_any(pcm, hwparams);
	if (rc < 0)
	{
		fprintf(stderr, "Failed to snd_pcm_hw_params_any\n");
		return -1;
	}

	rc = snd_pcm_hw_params_set_access(pcm, hwparams, SND_PCM_ACCESS_RW_INTERLEAVED);
	if (rc < 0)
	{
		fprintf(stderr, "Failed to snd_pcm_hw_params_set_access.\n");
		return -1;
	}

	/* Set sample format */
	if (snd_wav_get_fmt(wav, &format) < 0) {
		fprintf(stderr, "failed to snd_wav_get_fmt\n");
		return -1;
	}

	snd_pcm_hw_params_set_format(pcm, hwparams, format);
	sndpcm->format = format;

	/* 1 for single channel, 2 for stereo */
	sndpcm->channels = LE_SHORT(wav->format.channels);
	rc = snd_pcm_hw_params_set_channels(pcm, hwparams, sndpcm->channels);
	if (rc < 0)
	{
		fprintf(stderr, "Failed to snd_pcm_hw_params_set_channels\n");
		return -1;
	}

	exact_rate = LE_INT(wav->format.sample_rate);

	rc = snd_pcm_hw_params_set_rate_near(pcm, hwparams, &exact_rate, &dir); //设置>频率
	if (rc < 0)
	{
		fprintf(stderr, "Failed to snd_pcm_hw_params_set_rate_near\n");
		return -1;
	}

	if (LE_INT(wav->format.sample_rate) != exact_rate) {
		fprintf(stderr, "The rate %d Hz is not supported , switch to %d Hz.\n",
			LE_INT(wav->format.sample_rate), exact_rate);
	}

	if (snd_pcm_hw_params_get_buffer_time_max(hwparams, &buffer_time, 0) < 0) {
		fprintf(stderr, "Error snd_pcm_hw_params_get_buffer_time_max\n");
		return -1;
	}
	if (buffer_time > 500000) buffer_time = 500000;
	period_time = buffer_time / 4;

	if (snd_pcm_hw_params_set_buffer_time_near(pcm, hwparams, &buffer_time, 0) < 0) {
		fprintf(stderr, "Error snd_pcm_hw_params_set_buffer_time_near\n");
		return -1;
	}

	if (snd_pcm_hw_params_set_period_time_near(pcm, hwparams, &period_time, 0) < 0) {
		fprintf(stderr, "Error snd_pcm_hw_params_set_period_time_near\n");
		return -1;
	}

	rc = snd_pcm_hw_params(pcm, hwparams);
	if (rc < 0)
	{
		fprintf(stderr, "Failed to snd_pcm_hw_params\n");
		return -1;
	}

	snd_pcm_hw_params_get_period_size(hwparams, &sndpcm->chunk_size, 0);
	snd_pcm_hw_params_get_buffer_size(hwparams, &sndpcm->buffer_size);
	if (sndpcm->chunk_size == sndpcm->buffer_size) {
		fprintf(stderr, ("Can't use period equal to buffer size (%lu == %lu)\n"), sndpcm->chunk_size, sndpcm->buffer_size);
		return -1;
	}

	sndpcm->bits_per_sample = snd_pcm_format_physical_width(format);
	sndpcm->bits_per_frame = sndpcm->bits_per_sample * LE_SHORT(wav->format.channels);

	sndpcm->chunk_bytes = sndpcm->chunk_size * sndpcm->bits_per_frame / 8;

	if( pcm_len <= 0 ) {
		 pcm_len = sndpcm->chunk_bytes;
	}

	/* Allocate audio data buffer */
	pcm_len = ROUNDUP(pcm_len, sndpcm->chunk_bytes);
	sndpcm->data_buf = (uint8_t *)malloc( pcm_len );
	
	if (!sndpcm->data_buf) {
		fprintf(stderr, "Error malloc: data_buf\n");
		return -1;
	}

	return 0;
}


int snd_wav_close(snd_pcm_t *pcm)
{
	if (!pcm)
	{
		return 0;
	}

	snd_pcm_drain(pcm);
	snd_pcm_close(pcm);
	return 0;
}


ssize_t snd_wav_read(int fd, void *buf, size_t count)
{
	ssize_t ret = 0, res;

	while (count > 0) {
		if ((res = read(fd, buf, count)) == 0)
			break;
		if (res < 0)
			return ret > 0 ? ret : res;
		count -= res;
		ret += res;
		buf = (char *)buf + res;
	}
	return ret;
}



int snd_wav_recover(snd_pcm_t *pcm)
{
	int err;
	snd_pcm_status_t* stat;

	snd_pcm_status_alloca(&stat);

	if ((err = snd_pcm_status(pcm, stat)) < 0) {
		fprintf(stderr, "alsa recover: pcm_status %s\n", snd_strerror(err));
		return -1;
	}
	if (snd_pcm_status_get_state(stat) == SND_PCM_STATE_XRUN) {
		struct timeval tnow, trig,diff;
		gettimeofday(&tnow, 0);
		snd_pcm_status_get_trigger_tstamp(stat, &trig);
		timersub(&tnow, &trig, &diff);
		fprintf(stderr, "alsa recover: xrun of at least %8.3lf ms\n",
			1e3 * tnow.tv_sec - 1e3 * trig.tv_sec + 1e-3 * tnow.tv_usec - 1e-3 * trig.tv_usec);
		if ( (err = snd_pcm_prepare(pcm) < 0) ) {
				fprintf(stderr, "alsa recover: pcm_prepare(play): %s\n", snd_strerror(err));
				return -1;
		}
	}

	if (snd_pcm_status_get_state(stat) == SND_PCM_STATE_DRAINING) {
			if (pcm && ((err = snd_pcm_prepare(pcm)) < 0)) {
				fprintf(stderr, "alsa recover: pcm_prepare(play): %s\n", snd_strerror(err));
				return -1;
			}
	}
	return 1;
}


static
ssize_t snd_wav_write_pcm(snd_pcm_t *pcm, sndpcmcontainer_t *sndpcm, size_t wcount, size_t load )
{
	ssize_t r;
	ssize_t ret = 0;
	uint8_t *data = sndpcm->data_buf + load;

	if (wcount < sndpcm->chunk_size) {
		snd_pcm_format_set_silence(sndpcm->format,
			data + (wcount * sndpcm->bits_per_frame) / 8,
			(sndpcm->chunk_size - wcount) * sndpcm->channels);
		wcount = sndpcm->chunk_size;
	}
	
	while (wcount > 0) {
	
			r = snd_pcm_writei(pcm, data, wcount);

			if (alerthandler_get_state(alertManager->handler) == ALERT_INTERRUPTED)
			{
				printf("alerthandler_get_state break\n");
				break;
			}
	
			if (r == -EAGAIN || (r >= 0 && (size_t)r < wcount)) {
				snd_pcm_wait(pcm, 500);
			}
			else if (r == -EPIPE) {
				snd_wav_recover(pcm);
				r = snd_pcm_recover(pcm, r, 1);
				fprintf(stderr, "EPIPE, Buffer Underrun.\n");
				if ( (r > 0) && (r <= wcount) ) {
					r = snd_pcm_writei(pcm, data, r );
				}
			}
			else if (r == -ESTRPIPE) {
				fprintf(stderr, "ESTRPIPE,Need suspend.\n");
				while ((r = snd_pcm_resume(pcm)) == -EAGAIN)
					usleep(100);	/* wait until the suspend flag is released */
				if (r < 0) {
					if ((r = snd_pcm_prepare(pcm)) < 0) {
						fprintf(stderr, "alsa resume: pcm_prepare %s\n", snd_strerror(r));
						return -1;
					}
					continue;
				}
			}
			else if (r < 0) {
				fprintf(stderr, "failed to snd_pcm_writei: %s", snd_strerror(r));
				break;
			}
			if (r > 0) {
				ret += r;
				wcount -= r;
				data += (r * sndpcm->bits_per_frame) / 8;
			}
	}
	return ret;
}

static
void snd_wav_play(snd_pcm_t *pcm, sndpcmcontainer_t *sndpcm, wavcontainer_t *wav, int fd)
{
	int load, ret;
	off64_t written = 0;
	off64_t c;
	off64_t count = LE_INT(wav->chunk.length);

	load = 0;
	while (written < count) {
		/* Must read [chunk_bytes] bytes data enough. */
		do {
			c = count - written;
			if (c > sndpcm->chunk_bytes)
				c = sndpcm->chunk_bytes;
			c -= load;

			if (c == 0)
				break;
			ret = snd_wav_read(fd, sndpcm->data_buf + load, c);
			if (alerthandler_get_state(alertManager->handler) == ALERT_INTERRUPTED)
			{
				printf("alerthandler_get_state break\n");
				break;
			}
			if (ret < 0) {
				fprintf(stderr, "failed to snd_wav_read\n");
				//exit(-1);
				break;
			}
			if (ret == 0)
				break;
			load += ret;
		} while ((size_t)load < sndpcm->chunk_bytes);

		/* Transfer to size frame */
		load = load * 8 / sndpcm->bits_per_frame;
		ret = snd_wav_write_pcm(pcm, sndpcm, load, 0 );
		if (ret != load)
			break;
		
		ret = ret * sndpcm->bits_per_frame / 8;
		written += ret;
		load = 0;
	}
}


static
void snd_wav_play2(snd_pcm_t *pcm, sndpcmcontainer_t *sndpcm, wavcontainer_t *wav, int fd, off64_t pcm_len)
{
	int ret = 0;
	off64_t load = 0, c = pcm_len;
	
	/* Must read [pcm_len] bytes data enough. */
	do {
		c -= load;

		if (c == 0)
			break;
		ret = snd_wav_read(fd, sndpcm->data_buf + load, c);
		if (ret < 0) {
			fprintf(stderr, "failed to snd_wav_read\n");
			break;
		}
		if (ret == 0)
			break;
		load += ret;
	} while ((size_t)load < pcm_len );
	
	load = c = 0;
	while (load < pcm_len) {
		/* Transfer to size frame */
		if ( ( pcm_len - load ) >= sndpcm->chunk_bytes)
				c = (sndpcm->chunk_bytes * 8) / sndpcm->bits_per_frame;
		else
				c = ((pcm_len - load) * 8) / sndpcm->bits_per_frame;
		
		ret = snd_wav_write_pcm(pcm, sndpcm, (size_t)c, (size_t)load );
		if (ret != c)
			break;
	
		load += ret * sndpcm->bits_per_frame / 8;
		
		c = 0;
	}

	return;
}

static
int wav_read_header(int fd, wavcontainer_t *container)
{
	int hlen = 0;
	assert((fd >= 0) && container);

	if (read(fd, &container->header, sizeof(container->header)) != sizeof(container->header) ||
		read(fd, &container->format, sizeof(container->format)) != sizeof(container->format) ||
		read(fd, &container->chunk, sizeof(container->chunk)) != sizeof(container->chunk)) {

		fprintf(stderr, "failed to wav_read_header\n");
		return -1;
	}

	hlen = sizeof(wavcontainer_t);
	
	if (container->chunk.type == WAV_LIST) {
		lseek(fd, 0x46, SEEK_SET);
		if (read(fd, &container->chunk, sizeof(container->chunk)) != sizeof(container->chunk))
		{
			fprintf(stderr, "failed to wav_read_header\n");
			return -1;
		}
		hlen += 34;
	}

	if (wav_p_checkvalid(container) < 0) {
		fprintf(stderr, "invalid wav\n");
		return -1;
	}
	
	return hlen;
}


int wav_play(const char *filename)
{
	int fd = -1, ret = -1;
	off64_t pcm_len = 0;
	snd_pcm_t *pcm = NULL;
	wavcontainer_t wav;
	sndpcmcontainer_t pb;

	memset(&pb, 0x0, sizeof(pb));

	fd = open(filename, O_RDONLY);
	if (fd < 0) {
		fprintf(stderr, "failed to open %s\n", filename);
		return -1;
	}

	if ((ret = snd_pcm_open(&pcm, "default", SND_PCM_STREAM_PLAYBACK, 0)) < 0) {
		fprintf(stderr, "Failed to open PCM device.\n");
		goto pfail;
	}

	if ((ret = wav_read_header(fd, &wav)) < 0) {
		fprintf(stderr, "failed to wav_read_header %s\n", filename);
		goto pfail;
	}

	if ((ret = snd_wav_init(pcm, &pb, &wav, pcm_len)) < 0) {
		fprintf(stderr, "failed to snd_wav_init\n");
		goto pfail;
	}

	snd_wav_play(pcm, &pb, &wav, fd);

	ret = 0;

pfail:
	if (fd)
		close(fd);
	if (pb.data_buf)
		free(pb.data_buf);
	if (pcm)
		snd_wav_close(pcm);

	return ret;
}


/* use wav_play2 to avoid underrun, but waste memory */
int wav_play2(const char *filename)
{
	int fd = -1, ret = -1;
	off64_t pcm_len = 0;
	snd_pcm_t *pcm = NULL;
	wavcontainer_t wav;
	sndpcmcontainer_t pb;
	struct stat st;

	memset(&pb, 0x0, sizeof(pb));
	memset(&st, 0 , sizeof(st));
  
  if (stat(filename, &st) == 0) 
  	pcm_len = st.st_size;

	fd = open(filename, O_RDONLY);
	if (fd < 0) {
		fprintf(stderr, "failed to open %s\n", filename);
		return -1;
	}

	if ((ret = snd_pcm_open(&pcm, "default", SND_PCM_STREAM_PLAYBACK, 0)) < 0) {
		fprintf(stderr, "Failed to open PCM device.\n");
		goto pfail;
	}

	if ((ret = wav_read_header(fd, &wav)) < 0) {
		fprintf(stderr, "failed to wav_read_header %s\n", filename);
		goto pfail;
	}

	if( pcm_len <= 0 )
		pcm_len = LE_INT(wav.header.length) + 8;
		
	pcm_len -= ret;

	if ((ret = snd_wav_init(pcm, &pb, &wav, pcm_len)) < 0) {
		fprintf(stderr, "failed to snd_wav_init\n");
		goto pfail;
	}

	snd_wav_play2(pcm, &pb, &wav, fd, pcm_len);

	ret = 0;

pfail:
	if (fd)
		close(fd);
	if (pb.data_buf)
		free(pb.data_buf);
	if (pcm)
		snd_wav_close(pcm);

	return ret;
}
/*int main(int argc, char *argv[]){
	if(!argv[1])return -1;
	if(argv[1][0]=='1'){
		if(argv[2]){
			return wav_play(argv[2]);
		}
	}
	return wav_play2(argv[1]);
}*/

pthread_t WavplayAudio_Pthread;




static
ssize_t tonesnd_wav_write_pcm(snd_pcm_t *pcm, sndpcmcontainer_t *sndpcm, size_t wcount, size_t load )
{
	ssize_t r;
	ssize_t ret = 0;
	uint8_t *data = sndpcm->data_buf + load;

	if (wcount < sndpcm->chunk_size) {
		snd_pcm_format_set_silence(sndpcm->format,
			data + (wcount * sndpcm->bits_per_frame) / 8,
			(sndpcm->chunk_size - wcount) * sndpcm->channels);
		wcount = sndpcm->chunk_size;
	}
	
	while (wcount > 0) {
	
			//r = tone_snd_pcm_writei(pcm, data, wcount);
			r = snd_pcm_writei(pcm, data, wcount);

			/*if (alerthandler_get_state(alertManager->handler) != ALERT_PLAYING)
			{
				break;
			}*/
	
			if (r == -EAGAIN || (r >= 0 && (size_t)r < wcount)) {
				snd_pcm_wait(pcm, 500);
			}
			else if (r == -EPIPE) {
				snd_wav_recover(pcm);
				r = snd_pcm_recover(pcm, r, 1);
				fprintf(stderr, "EPIPE, Buffer Underrun.\n");
				if ( (r > 0) && (r <= wcount) ) {
					//r = tone_snd_pcm_writei(pcm, data, r );
					r = snd_pcm_writei(pcm, data, r );
				}
			}
			else if (r == -ESTRPIPE) {
				fprintf(stderr, "ESTRPIPE,Need suspend.\n");
				while ((r = snd_pcm_resume(pcm)) == -EAGAIN)
					usleep(100);	/* wait until the suspend flag is released */
				if (r < 0) {
					if ((r = snd_pcm_prepare(pcm)) < 0) {
						fprintf(stderr, "alsa resume: pcm_prepare %s\n", snd_strerror(r));
						return -1;
					}
					continue;
				}
			}
			else if (r < 0) {
				fprintf(stderr, "failed to snd_pcm_writei: %s", snd_strerror(r));
				break;
			}
			if (r > 0) {
				ret += r;
				wcount -= r;
				data += (r * sndpcm->bits_per_frame) / 8;
			}
	}
	return ret;
}


static
void tone_snd_wav_play(snd_pcm_t *pcm, sndpcmcontainer_t *sndpcm, wavcontainer_t *wav, int fd)
{
	int load, ret;
	off64_t written = 0;
	off64_t c;
	off64_t count = LE_INT(wav->chunk.length);

	load = 0;
	while (written < count) {
		/* Must read [chunk_bytes] bytes data enough. */
		do {
			c = count - written;
			if (c > sndpcm->chunk_bytes)
				c = sndpcm->chunk_bytes;
			c -= load;

			if (c == 0)
				break;
			ret = snd_wav_read(fd, sndpcm->data_buf + load, c);
			/*if (alerthandler_get_state(alertManager->handler) != ALERT_PLAYING)
			{
				break;
			}*/
			
			if (ret < 0) {
				fprintf(stderr, "failed to snd_wav_read\n");
				//exit(-1);
				break;
			}
			if (ret == 0)
				break;
			load += ret;
		} while ((size_t)load < sndpcm->chunk_bytes);

		/* Transfer to size frame */
		load = load * 8 / sndpcm->bits_per_frame;
		ret = tonesnd_wav_write_pcm(pcm, sndpcm, load, 0 );
		if (ret != load)
			break;
		
		ret = ret * sndpcm->bits_per_frame / 8;
		written += ret;
		load = 0;
	}
}



int tone_wav_play(const char *filename)
{
	int fd = -1, ret = -1;
	off64_t pcm_len = 0;
	snd_pcm_t *pcm = NULL;
	wavcontainer_t wav;
	sndpcmcontainer_t pb;

	memset(&pb, 0x0, sizeof(pb));

	fd = open(filename, O_RDONLY);
	if (fd < 0) {
		fprintf(stderr, "failed to open %s\n", filename);
		return -1;
	}

	if ((ret = snd_pcm_open(&pcm, "wavplay", SND_PCM_STREAM_PLAYBACK, 0)) < 0) {
		fprintf(stderr, "Failed to open PCM device.\n");
		goto pfail;
	}

	if ((ret = wav_read_header(fd, &wav)) < 0) {
		fprintf(stderr, "failed to wav_read_header %s\n", filename);
		goto pfail;
	}

	if ((ret = snd_wav_init(pcm, &pb, &wav, pcm_len)) < 0) {
		fprintf(stderr, "failed to snd_wav_init\n");
		goto pfail;
	}

	tone_snd_wav_play(pcm, &pb, &wav, fd);

	ret = 0;

pfail:
	if (fd)
		close(fd);
	if (pb.data_buf)
		free(pb.data_buf);
	if (pcm)
		snd_wav_close(pcm);

	return ret;
}



void *WavPlayAudioPthread(void)
{
	printf("wav_play /root/res/med_ui_wakesound._TTH_.wav\n");

	tone_wav_play("/root/res/med_ui_wakesound._TTH_.wav");
	pthread_exit(NULL);
}

void CreateWavPlayPthread(void)
{
	pthread_attr_t a_thread_attr; 
	pthread_attr_init(&a_thread_attr);
	pthread_attr_setstacksize(&a_thread_attr, 16384);

	pthread_create(&WavplayAudio_Pthread, &a_thread_attr, WavPlayAudioPthread, NULL);
	
	pthread_detach(WavplayAudio_Pthread);
}


static snd_pcm_uframes_t g_playttsaudio_chunk_size;
size_t g_playttsaudio_chunk_bytes;
static snd_pcm_t *g_playttsaudio_handle;
static size_t g_playttsaudio_bits_per_sample, g_playttsaudio_bits_per_frame;

int PlayTTSAudio_Init()
{
	int iRet = -1;

	int rc;
	snd_pcm_hw_params_t *hwparams = NULL;
	snd_pcm_format_t format;
	uint32_t exact_rate = ASR_TTS_SPEED;
	uint32_t buffer_time, period_time;
	snd_pcm_uframes_t buffer_size;
	int dir = 0;

	DEBUG_PRINTF("PlayTTSAudio_Init");

	if ((iRet = snd_pcm_open(&g_playttsaudio_handle, "media", SND_PCM_STREAM_PLAYBACK, 0)) < 0) {
		DEBUG_ERROR("Failed to open PCM device.");
		return -1;
	}

	snd_pcm_hw_params_alloca(&hwparams);

	rc = snd_pcm_hw_params_any(g_playttsaudio_handle, hwparams);
	if (rc < 0)
	{
		DEBUG_ERROR("Failed to snd_pcm_hw_params_any");
		return -1;
	}

	// 交错访问
	rc = snd_pcm_hw_params_set_access(g_playttsaudio_handle, hwparams, SND_PCM_ACCESS_RW_INTERLEAVED);
	if (rc < 0)
	{
		DEBUG_ERROR("Failed to snd_pcm_hw_params_set_access.");
		return -1;
	}

	// SND_PCM_FORMAT_S16_LE
	snd_pcm_hw_params_set_format(g_playttsaudio_handle, hwparams, ASR_TTS_FORMAT);

	/* 1 for single channel, 2 for stereo */
	rc = snd_pcm_hw_params_set_channels(g_playttsaudio_handle, hwparams, ASR_TTS_CHANNELS);
	if (rc < 0)
	{
		DEBUG_ERROR("Failed to snd_pcm_hw_params_set_channels");
		return -1;
	}

	//设置>频率 24000
	rc = snd_pcm_hw_params_set_rate_near(g_playttsaudio_handle, hwparams, &exact_rate, &dir); 
	if (rc < 0)
	{
		DEBUG_ERROR("Failed to snd_pcm_hw_params_set_rate_near");
		return -1;
	}

	if (snd_pcm_hw_params_get_buffer_time_max(hwparams, &buffer_time, 0) < 0) {
		DEBUG_ERROR("Error snd_pcm_hw_params_get_buffer_time_max");
		return -1;
	}
	if (buffer_time > 500000) buffer_time = 500000;
	period_time = buffer_time / 4;

	if (snd_pcm_hw_params_set_buffer_time_near(g_playttsaudio_handle, hwparams, &buffer_time, 0) < 0) {
		DEBUG_ERROR("Error snd_pcm_hw_params_set_buffer_time_near");
		return -1;
	}

	if (snd_pcm_hw_params_set_period_time_near(g_playttsaudio_handle, hwparams, &period_time, 0) < 0) {
		DEBUG_ERROR("Error snd_pcm_hw_params_set_period_time_near");
		return -1;
	}

	rc = snd_pcm_hw_params(g_playttsaudio_handle, hwparams);
	if (rc < 0)
	{
		DEBUG_ERROR("Failed to snd_pcm_hw_params");
		return -1;
	}

	snd_pcm_hw_params_get_period_size(hwparams, &g_playttsaudio_chunk_size, 0);
	snd_pcm_hw_params_get_buffer_size(hwparams, &buffer_size);
	if (g_playttsaudio_chunk_size == buffer_size) {
		DEBUG_ERROR(("Can't use period equal to buffer size (%lu == %lu)"), g_playttsaudio_chunk_size, buffer_size);
		return -1;
	}

	g_playttsaudio_bits_per_sample = snd_pcm_format_physical_width(ASR_TTS_FORMAT);
	g_playttsaudio_bits_per_frame = g_playttsaudio_bits_per_sample * ASR_TTS_CHANNELS;

	g_playttsaudio_chunk_bytes = g_playttsaudio_chunk_size * g_playttsaudio_bits_per_frame / 8;

	DEBUG_PRINTF("g_playttsaudio_chunk_bytes:%d, g_playttsaudio_chunk_size:%d", g_playttsaudio_chunk_bytes, g_playttsaudio_chunk_size);

	return 0;
}

ssize_t PlayTTSAudio_WritePcm(unsigned char *data_buf, size_t wcount)  
{  
    ssize_t r;  
    ssize_t result = 0;  
    unsigned char *data = data_buf;  
  
    if (wcount < g_playttsaudio_chunk_size) {
        snd_pcm_format_set_silence(ASR_TTS_FORMAT,   
            data + wcount * g_playttsaudio_bits_per_frame / 8,   
            (g_playttsaudio_chunk_size - wcount) * ASR_TTS_CHANNELS);  
        wcount = g_playttsaudio_chunk_size;  
    }  
    while (wcount > 0) {  
        r = snd_pcm_writei(g_playttsaudio_handle, data, wcount);  
        if (r == -EAGAIN || (r >= 0 && (size_t)r < wcount)) {  
            snd_pcm_wait(g_playttsaudio_handle, 1000);  
        } else if (r == -EPIPE) {  
            snd_pcm_prepare(g_playttsaudio_handle);  
            //printf( " Buffer Underrun ");  
        } else if (r == -ESTRPIPE) {              
            //printf( " Need suspend");          
        } else if (r < 0) {  
            printf( "Error snd_pcm_writei: [%s]", snd_strerror(r));  
            exit(-1);  
        }  
        if (r > 0) {  
            result += r;  
            wcount -= r;  
            data += r * g_playttsaudio_bits_per_frame / 8;  
        }  
    }  
    return result;  
}  


void PlayTTSAudio_Buffer(char *buf, unsigned int count)  
{  
	int ret;
    unsigned int  load;  
    unsigned int  written = 0;  
    unsigned int  c;  
  
    load = 0;

    while (written < count) {  
		
	  	c = count - written;	 
		if(c > g_playttsaudio_chunk_bytes)
			load = g_playttsaudio_chunk_bytes;
		else
			load = c; 

        load = load * 8 / g_playttsaudio_bits_per_frame; 
        ret = PlayTTSAudio_WritePcm(buf, load);  
        if (ret != load)  
            break;  
        
        ret = ret * g_playttsaudio_bits_per_frame / 8;  
        written += ret;  
        load = 0;  
    }  
}   

void PlayTTSAudio_Close(void)
{
	if (g_playttsaudio_handle != NULL)
	{		
		snd_pcm_drain(g_playttsaudio_handle);
		snd_pcm_close(g_playttsaudio_handle);
		g_playttsaudio_handle = NULL;	
	}	
	
	DEBUG_PRINTF("PlayTTSAudio_Close\n");
}

