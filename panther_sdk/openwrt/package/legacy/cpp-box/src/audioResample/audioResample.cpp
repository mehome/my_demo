#include "audioResample.h"

int audioResample::init(
	AVSampleFormat out_sample_fmt,
	int out_sample_rate,
	int out_channels,
	AVSampleFormat in_sample_fmt,
	int in_sample_rate,
	int in_channels,
	int outBufbytes
){
	this->out_sample_fmt=out_sample_fmt;
	this->out_sample_rate=out_sample_rate;
	this->out_channels=out_channels;
	this->in_sample_fmt=in_sample_fmt;
	this->in_sample_rate=in_sample_rate;
	this->in_channels=in_channels;
	this->outBufbytes=outBufbytes;

	int in_channel_layout = av_get_default_channel_layout(in_channels);
	int out_channel_layout = av_get_default_channel_layout(out_channels);
	outBuffer=(uint8_t *)av_malloc(outBufbytes);
	if(!outBuffer){
		err("av_malloc failure!");
		return -1;		
	}
	convertCtx = swr_alloc();
	if(!outBuffer){
		err("swr_alloc failure!");
		deInit();
		return -2;
	}
	convertCtx = swr_alloc_set_opts(NULL,
								  out_channel_layout,
								  out_sample_fmt,
								  out_sample_rate,
								  in_channel_layout,
								  in_sample_fmt,
								  in_sample_rate,
								  0,
								  NULL);
	if(!convertCtx){
		err("swr_alloc_set_opts failure!");
		deInit();
		return -3;
	}
	int err=swr_init(convertCtx);
	if(err){
		err("swr_init failure!");
		deInit();
		return -4;	
	}
	return 0;	
};

int audioResample::convert(
								uint8_t **pOutBuffer,
								int *pOutRealSize,
								const uint8_t **pInBuffer,
								int inDataBytes			
){	

	int inDataSamples = inDataBytes / in_channels / av_get_bytes_per_sample(in_sample_fmt);
	int outBufferSamples = outBufbytes / out_channels / av_get_bytes_per_sample(out_sample_fmt);

	memset(outBuffer,0,outBufbytes);
	int frame_size = swr_convert(convertCtx,&outBuffer,outBufferSamples,pInBuffer,inDataSamples);
	if(frame_size <= 0){
		err("swr_convert failure,err=%d",frame_size);
		return -1;
	}
	*pOutRealSize = frame_size * out_channels * av_get_bytes_per_sample(out_sample_fmt);
	*pOutBuffer = outBuffer;
	return 0;
}

int audioResample::deInit(){
    if (convertCtx) {
        swr_free(&convertCtx);
        convertCtx = NULL;
    }
	if(outBuffer){
		free(outBuffer);
		outBuffer=NULL;
	}
	return 0;
}


