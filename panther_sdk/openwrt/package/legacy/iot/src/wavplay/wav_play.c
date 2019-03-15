#include "wav_play.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <errno.h>
#include "common.h"

static ssize_t safe_read(int fd, void *buf, size_t count)
{
	ssize_t result = 0, res;
	while (count > 0) {
		if ((res = read(fd, buf, count)) == 0)
			break;
		if (res < 0)
			return result > 0 ? result : res;
		count -= res;
		result += res;
		buf = (char *)buf + res;
	}
	return result;
}

/*
 * helper for parse_wavefile
 */

static size_t parse_wavefile_read_err(int fd, u_char *buffer, size_t *size, size_t reqsize, int line)
{
	if (*size >= reqsize)
		return *size;
	if ((size_t)safe_read(fd, buffer + *size, reqsize - *size) != reqsize - *size) {
		fprintf(stderr,"read error (called from line %i)", line);
		return -1;
	}
	return *size = reqsize;
}

#define check_wavefile_space_err(buffer, len, blimit) \
	if (len > blimit) { \
		blimit = len; \
		if ((buffer = realloc(buffer, blimit)) == NULL) { \
			fprintf(stderr,"not enough memory");		  \
			goto err;                             \
		} \
	}


static ssize_t parse_wavefile(int fd, u_char *_buffer, size_t size, wavcontainer_t *wav)
{
	wavheader_t *h = (wavheader_t *)_buffer;
	u_char *buffer = NULL;
	size_t blimit = 0;
	wavfmt_body_t *f;
	wavchunkheader_t *c;
	u_int type, len;
	unsigned short format, channels;
	int big_endian, native_format;

	if (size < sizeof(wavheader_t)) {
		return -1;
	}
	
	if (h->magic == WAV_RIFF) {
		wav->header.magic = WAV_RIFF;
		big_endian = 0;
	}
	else if (h->magic == WAV_RIFX) {
		wav->header.magic = WAV_RIFX;
		big_endian = 1;
	}
	else {
		return -1;
	}
	if (h->type != WAV_WAVE) {
		return -1;
	}
		
	wav->header.type = WAV_WAVE;
	if (size > sizeof(wavheader_t)) {
		check_wavefile_space_err(buffer, size - sizeof(wavheader_t), blimit);
		memcpy(buffer, _buffer + sizeof(wavheader_t), size - sizeof(wavheader_t));
	}
	size -= sizeof(wavheader_t);
	while (1) {
		check_wavefile_space_err(buffer, sizeof(wavchunkheader_t), blimit);
		if(parse_wavefile_read_err(fd, buffer, &size, sizeof(wavchunkheader_t), __LINE__)  < 0)
			goto err;
		c = (wavchunkheader_t*)buffer;
		type = c->type;
		len = TO_CPU_INT(c->length, big_endian);
		len += len % 2;
		if (size > sizeof(wavchunkheader_t))
			memmove(buffer, buffer + sizeof(wavchunkheader_t), size - sizeof(wavchunkheader_t));
		size -= sizeof(wavchunkheader_t);
		if (type == WAV_FMT)
			break;
		check_wavefile_space_err(buffer, len, blimit);
		if(parse_wavefile_read_err(fd, buffer, &size, len, __LINE__)  < 0)
			goto err;
		if (size > len)
			memmove(buffer, buffer + len, size - len);
		size -= len;
	}

	if (len < sizeof(wavfmt_body_t)) {
		fprintf(stderr,"unknown length of 'fmt ' chunk (read %u, should be %u at least)\n",
		      len, (unsigned )sizeof(wavfmt_body_t));
		goto err;
	}
	check_wavefile_space_err(buffer, len, blimit);
	if(parse_wavefile_read_err(fd, buffer, &size, len, __LINE__) < 0)
		goto err;
	f = (wavfmt_body_t*) buffer;
	format = TO_CPU_SHORT(f->format, big_endian);
	if (format == WAV_FMT_EXTENSIBLE) {
		wavfmt_body_ext_t *fe = (wavfmt_body_ext_t*)buffer;
		if (len < sizeof(wavfmt_body_ext_t)) {
			fprintf(stderr, "unknown length of extensible 'fmt ' chunk (read %u, should be %u at least)\n",
					len, (u_int)sizeof(wavfmt_body_ext_t));
			goto err;
		}
		if (memcmp(fe->guid_tag, WAV_GUID_TAG, 14) != 0) {
			fprintf(stderr,"wrong format tag in extensible 'fmt ' chunk\n");
			goto err;
		}
		format = TO_CPU_SHORT(fe->guid_format, big_endian);
	}
	if (format != WAV_FMT_PCM &&
	    format != WAV_FMT_IEEE_FLOAT) {
       	fprintf(stderr, "can't play WAVE-file format 0x%04x which is not PCM or FLOAT encoded\n", format);
		goto err;
	}
	channels = TO_CPU_SHORT(f->channels, big_endian);
	if (channels < 1) {
		fprintf(stderr, "can't play WAVE-files with %d tracks\n", channels);
		goto err;
	}

	wav->format.channels = channels;
	switch (TO_CPU_SHORT(f->bit_p_spl, big_endian)) {
	case 8:
		if (wav->format.format != DEFAULT_FORMAT &&
		    wav->format.format != SND_PCM_FORMAT_U8)
			fprintf(stderr, "Warning: format is changed to U8\n");
		
		wav->format.format = SND_PCM_FORMAT_U8;
		break;
	case 16:
		if (big_endian)
			native_format = SND_PCM_FORMAT_S16_BE;
		else
			native_format = SND_PCM_FORMAT_S16_LE;
		if (wav->format.format != DEFAULT_FORMAT &&
		    wav->format.format != native_format)
			fprintf(stderr, "Warning: format is changed to %s\n",snd_pcm_format_name(native_format));
		wav->format.format = native_format;
		break;
	case 24:
		switch (TO_CPU_SHORT(f->byte_p_spl, big_endian) / wav->format.channels) {
		case 3:
			if (big_endian) 
				native_format = SND_PCM_FORMAT_S24_3BE;
			else
				native_format = SND_PCM_FORMAT_S24_3LE;
			if (wav->format.format != DEFAULT_FORMAT &&
			    wav->format.format != native_format)
				fprintf(stderr, "Warning: format is changed to %s\n", snd_pcm_format_name(native_format));
			wav->format.format = native_format;
			break;
		case 4:
			if (big_endian)
				native_format = SND_PCM_FORMAT_S24_BE;
			else
				native_format = SND_PCM_FORMAT_S24_LE;
			if (wav->format.format != DEFAULT_FORMAT &&
			   	wav->format.format!= native_format)
				fprintf(stderr, "Warning: format is changed to %s\n", snd_pcm_format_name(native_format));
			wav->format.format = native_format;
			break;
		default:
			fprintf(stderr, " can't play WAVE-files with sample %d bits in %d bytes wide (%d channels)\n",
			      TO_CPU_SHORT(f->bit_p_spl, big_endian),
			      TO_CPU_SHORT(f->byte_p_spl, big_endian),
			      wav->format.format);
			goto err;
		}
		break;
	case 32:
		if (format == WAV_FMT_PCM) {
			if (big_endian)
				native_format = SND_PCM_FORMAT_S32_BE;
			else
				native_format = SND_PCM_FORMAT_S32_LE;
			wav->format.format = native_format;
		} else if (format == WAV_FMT_IEEE_FLOAT) {
			if (big_endian)
				native_format = SND_PCM_FORMAT_FLOAT_BE;
			else
				native_format = SND_PCM_FORMAT_FLOAT_LE;
			wav->format.format = native_format;
		}
		break;
	default:
		fprintf(stderr, " can't play WAVE-files with sample %d bits wide\n", TO_CPU_SHORT(f->bit_p_spl, big_endian));
		goto err;
	}
	//hwparams.rate = TO_CPU_INT(f->sample_fq, big_endian);
	wav->format.sample_rate = TO_CPU_INT(f->sample_fq, big_endian);
	if (size > len)
		memmove(buffer, buffer + len, size - len);
	size -= len;
	
	while (1) {
		u_int type, len;

		check_wavefile_space_err(buffer, sizeof(wavchunkheader_t), blimit);
		if(parse_wavefile_read_err(fd, buffer, &size, sizeof(wavchunkheader_t), __LINE__) <0)
			goto err;
		c = (wavchunkheader_t*)buffer;
		type = c->type;
		len = TO_CPU_INT(c->length, big_endian);
		if (size > sizeof(wavchunkheader_t))
			memmove(buffer, buffer + sizeof(wavchunkheader_t), size - sizeof(wavchunkheader_t));
		size -= sizeof(wavchunkheader_t);
		if (type == WAV_DATA) {
			if (len < wav->chunk.length && len < 0x7ffffffe) {
				wav->chunk.length = len;
			}
			if (size > 0)
				memcpy(_buffer, buffer, size);
			free(buffer);
			return size;
		}
		len += len % 2;
		check_wavefile_space_err(buffer, len, blimit);
		if(parse_wavefile_read_err(fd, buffer, &size, len, __LINE__) < 0)
			goto err;
		if (size > len)
			memmove(buffer, buffer + len, size - len);
		size -= len;
	}
err:
	/* shouldn't be reached */
	if(buffer)
		free(buffer);
	return -1;
}

int snd_wav_init(snd_pcm_t *pcm, sndpcmcontainer_t *sndpcm, wavcontainer_t *wav, unsigned int pcm_len)
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

	format = wav->format.format;
	snd_pcm_hw_params_set_format(pcm, hwparams, format);
	sndpcm->format = format;

	/* 1 for single channel, 2 for stereo */
	sndpcm->channels = wav->format.channels;
	rc = snd_pcm_hw_params_set_channels(pcm, hwparams, sndpcm->channels);
	if (rc < 0)
	{
		fprintf(stderr, "Failed to snd_pcm_hw_params_set_channels\n");
		return -1;
	}

	exact_rate = wav->format.sample_rate;

	rc = snd_pcm_hw_params_set_rate_near(pcm, hwparams, &exact_rate, &dir); //设置>频率
	if (rc < 0)
	{
		fprintf(stderr, "Failed to snd_pcm_hw_params_set_rate_near\n");
		return -1;
	}

	if ( wav->format.sample_rate != exact_rate) {
		fprintf(stderr, "The rate %d Hz is not supported , switch to %d Hz.\n", wav->format.sample_rate, exact_rate);
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
	sndpcm->bits_per_frame = sndpcm->bits_per_sample * wav->format.channels;

	sndpcm->chunk_bytes = sndpcm->chunk_size * sndpcm->bits_per_frame / 8;

	if( pcm_len <= 0 ) {
		 pcm_len = sndpcm->chunk_bytes;
	}
	/* Allocate audio data buffer */
	pcm_len = ROUNDUP(pcm_len, sndpcm->chunk_bytes);
	sndpcm->data_buf = (uint8_t *)malloc( pcm_len);
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
	unsigned written = 0;
	unsigned c;
	unsigned count = wav->chunk.length;

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
			if (ret < 0) {
				fprintf(stderr, "failed to snd_wav_read\n");
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
void snd_wav_play2(snd_pcm_t *pcm, sndpcmcontainer_t *sndpcm, wavcontainer_t *wav, int fd, unsigned int pcm_len)
{
	int ret = 0;
	unsigned load = 0, c = pcm_len;
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
		if (ret == 0) {
			break;
		}
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
void snd_wav_play3(snd_pcm_t *pcm, sndpcmcontainer_t *sndpcm, wavcontainer_t *wav, int fd, unsigned int pcm_len,  QuitCB quit)
{
	int ret = 0;
	unsigned load = 0, c = pcm_len;
	/* Must read [pcm_len] bytes data enough. */
	do {
		c -= load;
		if(quit && quit()) {
			break;
		}
		if (c == 0)
			break;
		ret = snd_wav_read(fd, sndpcm->data_buf + load, c);
		if (ret < 0) {
			fprintf(stderr, "failed to snd_wav_read\n");
			break;
		}
		if (ret == 0) {
			break;
		}
		load += ret;
	} while ((size_t)load < pcm_len );
	
	load = c = 0;
	while (load < pcm_len) {
		
		if(quit && quit()) {
			break;
		}
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

int wav_play(const char *filename)
{
	
	int ret = -1, fd = -1;
	unsigned int pcm_len = 0;
	snd_pcm_t *pcm = NULL;
	sndpcmcontainer_t pb;
	wavcontainer_t wav;
	struct stat st;

	memset(&pb, 0x0, sizeof(pb));
	memset(&st, 0 , sizeof(st));
	memset(&wav, 0 , sizeof(wav));
	
	fd = open(filename, O_RDONLY);
	if (fd < 0) {
		fprintf(stderr, "failed to open %s\n", filename);
		return -1;
	}

	if ((ret = snd_pcm_open(&pcm, "default", SND_PCM_STREAM_PLAYBACK, 0)) < 0) {
		fprintf(stderr, "Failed to open PCM device.\n");
		goto pfail;
	}
	unsigned char *buf = calloc(1, 64);
	if ((size_t)safe_read(fd, buf ,26)!= 26) {
		fprintf(stderr, "Failed to safe_read\n");
		goto pfail;
	}

	wav.chunk.length = UNSIGNED_LONG;
	wav.format.format = 2;
	wav.format.sample_rate = 16000;
	wav.format.channels = 1;
	
	if(parse_wavefile(fd, buf, 26, &wav)< 0) {
		fprintf(stderr, "not wav file\n");
		goto pfail;
	}

	if ((ret = snd_wav_init(pcm, &pb, &wav, pcm_len)) < 0) {
		fprintf(stderr, "failed to snd_wav_init\n");
		goto pfail;
	}
	
	snd_wav_play(pcm, &pb, &wav, fd);
	
	
	ret = 0;

pfail:
	if(fd > 0)
		close(fd);
	if(buf)
		free(buf);
	if (pb.data_buf)
		free(pb.data_buf);
	if (pcm)
		snd_wav_close(pcm);

	return ret;
}

/* use wav_preload to avoid underrun, but waste memory */
int wav_preload(const char *filename)
{
	
	int ret = -1, fd = -1;
	unsigned int pcm_len = 0;
	snd_pcm_t *pcm = NULL;
	sndpcmcontainer_t pb;
	wavcontainer_t wav;
	struct stat st;

	memset(&pb, 0x0, sizeof(pb));
	memset(&st, 0 , sizeof(st));
  	memset(&wav, 0 , sizeof(wav));
	
	fd = open(filename, O_RDONLY);
	if (fd < 0) {
		fprintf(stderr, "failed to open %s\n", filename);
		return -1;
	}

	if ((ret = snd_pcm_open(&pcm, "common", SND_PCM_STREAM_PLAYBACK, 0)) < 0) {
		fprintf(stderr, "Failed to open PCM device.\n");
		goto pfail;
	}
	
	unsigned char *buf = calloc(1, 64);
	if ((size_t)safe_read(fd, buf ,26)!= 26) {
		fprintf(stderr, "Failed to safe_read\n");
		goto pfail;
	}

	wav.chunk.length = UNSIGNED_LONG;
	wav.format.format = 2;
	wav.format.sample_rate = 16000;
	wav.format.channels = 1;
	
	if(parse_wavefile(fd, buf, 26, &wav)< 0) {
		fprintf(stderr, "not wav file\n");
		goto pfail;
	}
	pcm_len = wav.chunk.length;
	if ((ret = snd_wav_init(pcm, &pb, &wav, pcm_len)) < 0) {
		fprintf(stderr, "failed to snd_wav_init\n");
		goto pfail;
	}
	
	snd_wav_play2(pcm, &pb, &wav, fd,  pcm_len);

	ret = 0;

pfail:
	if(fd > 0)
		close(fd);
	if(buf)
		free(buf);
	if (pb.data_buf)
		free(pb.data_buf);
	if (pcm)
		snd_wav_close(pcm);

	return ret;
}

/* use wav_preload to avoid underrun, but waste memory */
int wav_unblock(const char *filename, QuitCB quit)
{
	
	int ret = -1, fd = -1;
	unsigned int pcm_len = 0;
	snd_pcm_t *pcm = NULL;
	sndpcmcontainer_t pb;
	wavcontainer_t wav;
	struct stat st;

	memset(&pb, 0x0, sizeof(pb));
	memset(&st, 0 , sizeof(st));
  	memset(&wav, 0 , sizeof(wav));
	
	fd = open(filename, O_RDONLY);
	if (fd < 0) {
		fprintf(stderr, "failed to open %s\n", filename);
		return -1;
	}

	if ((ret = snd_pcm_open(&pcm, "default", SND_PCM_STREAM_PLAYBACK, 0)) < 0) {
		fprintf(stderr, "Failed to open PCM device.\n");
		goto pfail;
	}
	
	unsigned char *buf = calloc(1, 64);
	if ((size_t)safe_read(fd, buf ,26)!= 26) {
		fprintf(stderr, "Failed to safe_read\n");
		goto pfail;
	}

	wav.chunk.length = UNSIGNED_LONG;
	wav.format.format = 2;
	wav.format.sample_rate = 16000;
	wav.format.channels = 1;
	
	if(parse_wavefile(fd, buf, 26, &wav)< 0) {
		fprintf(stderr, "not wav file\n");
		goto pfail;
	}
	pcm_len = wav.chunk.length;
	if ((ret = snd_wav_init(pcm, &pb, &wav, pcm_len)) < 0) {
		fprintf(stderr, "failed to snd_wav_init\n");
		goto pfail;
	}
	
	snd_wav_play3(pcm, &pb, &wav, fd,  pcm_len, quit);

	ret = 0;

pfail:
	if(fd > 0)
		close(fd);
	if(buf)
		free(buf);
	if (pb.data_buf)
		free(pb.data_buf);
	if (pcm)
		snd_wav_close(pcm);

	return ret;
}

