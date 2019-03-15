#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <math.h>
#include <math.h>
#include <stdarg.h>
#include <alsa/asoundlib.h>

//typedef char int8_t;
typedef unsigned char uint8_t;
typedef short int16_t;
typedef unsigned short uint16_t;
typedef int int32_t;
typedef unsigned int uint32_t;
double volume = 0.5;
double volume_;
double volCorrection_ = 1.0;

uint16_t samplebits = 16;
uint16_t sample_channels = 2;
int little_big_endian;
double PCM_VOLUME_1 = 1024;

#	define SWAP_16(x) (__builtin_bswap16(x))
#	define SWAP_32(x) (__builtin_bswap32(x))
#	define SWAP_64(x) (__builtin_bswap64(x))

int8_t swap_8(const int8_t val)
{
	return val;
}

int16_t swap_16(const int16_t val)
{
	return SWAP_16(val);
}

int32_t swap_32(const int32_t val)
{
	return SWAP_32(val);
} 

#define cprintf(fmt, args...) do { \
        FILE *fp = fopen("/dev/console", "w"); \
        if (fp) { \
                fprintf(fp, fmt , ## args); \
                fclose(fp); \
        } \
} while (0)

//http://stackoverflow.com/questions/1165026/what-algorithms-could-i-use-for-audio-volume-level
static void setVolume(double volume)
{
	double base = M_E;
//	double base = 10.;
	volume_ = (pow(base, volume)-1) / (base-1);
//	cprintf("setVolume volume=%lf\n",volume);
//	cprintf("setVolume volume_=%lf\n",volume_);
}
static int     pcm_float_to_volume_mpd(double volume)
{
	return volume * PCM_VOLUME_1 + 0.5;
}	 
void setVolumeMpd(double volume)
{
       volume_ = pcm_float_to_volume_mpd((exp(volume / 25.0) - 1) /
			     (54.5981500331F - 1));
}	

    
static void adjustVolumeValue_8(char *buffer, size_t count, double volume)
{
	size_t n;
	int8_t* bufferT = (int8_t *)buffer;
	for (n=0; n<count; ++n) {
		//bufferT[n] = endian::swap<T>(endian::swap<T>(bufferT[n]) * volume);
	    bufferT[n] =swap_8((swap_8(bufferT[n]) * volume));
	}
}
	
static void adjustVolumeValue_16(char *buffer, size_t count, double volume)
{
	size_t n;
	int16_t* bufferT = (int16_t *)buffer;
	if (little_big_endian ==1){
		for (n=0; n<count; ++n){
		//	bufferT[n] = endian::swap<T>(endian::swap<T>(bufferT[n]) * volume);
			short val = (swap_16(bufferT[n]) * volume);
			if(val > 32767)
       			 val = 32767;
   			 else if(val < -32768)
        		val = -32768;
		    bufferT[n] =swap_16((swap_16(bufferT[n]) * volume));
		}
	}else{
		for (n=0; n<count; ++n){
			short val = (bufferT[n] * volume);
			if(val > 32767)
       			val = 32767;
   			else if(val < -32768)
        		val = -32768;
	 	   bufferT[n] =((bufferT[n]) * volume);
	 	}
	}
}
	
static void adjustVolumeValue_32(char *buffer, size_t count, double volume)
{
	size_t n;
	int32_t* bufferT = (int32_t *)buffer;		
	if (little_big_endian ==1) {
		for (n=0; n<count; ++n) {
			//	bufferT[n] = endian::swap<T>(endian::swap<T>(bufferT[n]) * volume);
			bufferT[n] =swap_32((swap_32(bufferT[n]) * volume));
		}
	} else {
		for (n=0; n<count; ++n) {
			bufferT[n] =((bufferT[n]) * volume);
		}
	}
}

static void adjustVolume(char* buffer, size_t frames)
{
	double volume = volume_;
//	const SampleFormat& sampleFormat = stream_->getFormat();
//  cprintf("adjustVolume volume=%lf\n",volume_);
	//if ((volume < 1.0) || (volCorrection_ != 1.))
	//if ((volCorrection_ != 1.))
	{
		volume *= volCorrection_;
		if (samplebits == 8)
			adjustVolumeValue_8(buffer, frames*sample_channels, volume);
		else if (samplebits == 16)
			adjustVolumeValue_16(buffer, frames*sample_channels, volume);
		else if (samplebits == 24 || samplebits == 32)
			adjustVolumeValue_32(buffer, frames*sample_channels, volume);
	}
}
int pcm_volume_filter(const void *buffer, int size, int channels, int sample_bits, snd_pcm_format_t format)
{
	//double tmp_vol = cdb_get_int("$ra_vol", 80);
	volume = (150/100)*0.85;
	setVolume(volume);
	//setVolumeMpd(volume);
	sample_channels = channels;
	samplebits = sample_bits;
	little_big_endian = snd_pcm_format_little_endian(format);
	adjustVolume(buffer, size);
	assert( size == 0 || buffer);

	return size;
}