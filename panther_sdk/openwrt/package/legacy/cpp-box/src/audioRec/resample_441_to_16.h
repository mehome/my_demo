#ifndef RESAMPLING_H
#define RESAMPLING_H

//#include <sndfile.h>  
#include <string.h>
#include <math.h>

typedef struct rsp_para {
	int table_index; //系数表的index
	int run_num; //记录当前转换的数据的流水号
	short * data_ptr; // 最初BUFFER_HEADER 个点为历史数据.之后为新数据
} rsp_para_t ;

typedef struct {
  unsigned long chunkID,        //ASCII "RIFF" ;
                chunkSize,      // 4 + (8 + subChunk1size) + ( 8 + subChunk2Size) = 36 + subChunk2Size;
                format,         //ascii "WAVE"
                subChunkID1,    //ascii "fmt "
                subChunk1Size;  //16, for PCM
  unsigned short audioFormat,   // 1, for PCM.
                 numChannels;   // 1 --> mono, 2 --> stereo;
  unsigned long sampleRate,     //for example: 16000 --> 16k, 44100 --> 44.1k
                byteRate;       // sampleRate * numChannels * bitsPerSample/8.
  unsigned short blockAlign,    // numChannels * bitsPerSample/8.
                 bitsPerSample; // 8 bit = 8, 16 bit = 16, ...
  unsigned long subChunk2ID,    //ascii "data"
                subChunk2Size;  //num of bytes in the following data segment.
// The following is the data segment. in case of stereo sound, left channel first.
}WAVE_FILE_HEADER;

class Resample{
public:
	void resample_441_to_16(char * infile,char * outfile);
	void resample_buf_441_to_16(char * in,unsigned long len,char * out, unsigned long  *length);
	//void resampling(rsp_para_t *rsp, short *in_ptr,unsigned long in_len, short *out_ptr);
	//void resampling(rsp_para_t rsp, short *in_ptr,unsigned long in_len, short *out_ptr);
private:
	void ConvertFreq(short *source, long srcIdx, short *target, long tgtIdx,long coefIdx);
};



#endif


