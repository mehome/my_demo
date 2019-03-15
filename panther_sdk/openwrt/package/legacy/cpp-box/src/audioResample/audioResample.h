#ifndef AUDIORESAMPLE_H_
#define AUDIORESAMPLE_H_
#include <string>
#ifdef __cplusplus
extern "C"
{
#endif
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswresample/swresample.h>
#include <libcchip/platform.h>
#ifdef __cplusplus
};
#endif

class audioResample{
public:
	int init(
		AVSampleFormat out_sample_fmt,
		int out_sample_rate,
		int out_channels,
		AVSampleFormat in_sample_fmt,
		int in_sample_rate,
		int in_channels,
		int outBufbytes
);
	int convert(
						uint8_t **pOutBuffer,
						int *pOutRealSize,
						const uint8_t ** pInBuffer,
						int inDataSamples			
	);	
	int deInit();
private:
	AVSampleFormat out_sample_fmt;
	int out_sample_rate;
	int out_channels;
	AVSampleFormat in_sample_fmt;
	int in_sample_rate;
	int in_channels;
	uint8_t *outBuffer;
	int outBufbytes;	
	struct SwrContext *convertCtx;
};

#endif
