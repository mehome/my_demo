#ifndef __AMR_DEC_H__
#define __AMR_DEC_H__

enum {
	DECODE_NONE= 0,
	DECODE_STARTED,
	DECODE_ONGING,
	DECODE_DONE,
	DECODE_CLOSE,
	DECODE_CANCLE,
};


enum decoder_command {
	DECODE_COMMAND_NONE = 0,
	DECODE_COMMAND_START,
	DECODE_COMMAND_STOP,
	DECODE_COMMAND_SEEK
};
enum decoder_state {
	DECODE_STATE_STOP = 0,
	DECODE_STATE_START,
	DECODE_STATE_DECODE,
	DECODE_STATE_ERROR,
};


int  DestoryDecodePthread(void);
int  CreateDecodePthread(void);





unsigned int DecodeFifoSeek( unsigned char *buffer, unsigned int len);
unsigned int DecodeFifoLength();
unsigned int DecodeFifoPut(unsigned char *buffer, unsigned int len);
void DecodeFifoClear();
void DecodeFifoDeinit();
void DecodeFifoInit();

void StartNewDecode();
int IsDecodeDone();
int IsDecodeFinshed();
void SetDecodeState(int state);
int IsDecodeCancled();


int amr_dec_file(const char *infile, const char *outfile) ;

int amr_dec(char *infile ,char *outfile);

#endif

