#ifndef __AMR_ENC_H__
#define __AMR_ENC_H__

//#include "common.h"

enum {
	ENCODE_NONE= 0,
	ENCODE_STARTED,
	ENCODE_ONGING,
	ENCODE_DONE,
	ENCODE_CLOSE,
	ENCODE_CANCLE,
};



int  DestoryAmrEncodePthread(void);
int  CreateAmrEncodePthread(void);


unsigned int AmrEncodeFifoSeek( unsigned char *buffer, unsigned int len);
unsigned int AmrEncodeFifoLength();
unsigned int AmrEncodeFifoPut(unsigned char *buffer, unsigned int len);
void AmrEncodeFifoClear();
void AmrEncodeFifoDeinit();
void AmrEncodeFifoInit();

void AmrStartNewEncode();
int AmrIsEncodeDone();
int AmrIsEncodeFinshed();
void AmrSetEncodeState(int state);
int AmrIsEncodeCancled();



int amr_enc_file(const char *infile, const char *outfile);
int amr_enc(const char *infile, const char *outfile);



//

#endif
