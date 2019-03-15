#ifndef __CAPTURE_H__
#define __CAPTURE_H__

//#define _AUDIO_TEST_

#define CAPTURE_VAD_COUNT 28

#define POST_DATA_LENGTH 1024 * 16

#define RECORD_VAD_START 	0
#define RECORD_VAD_END    	1

enum {
	CAPTURE_NONE= 0,
	CAPTURE_STARTED,
	CAPTURE_ONGING,
	CAPTURE_DONE,
	CAPTURE_CLOSE,
	CAPTURE_CANCLE,
};

enum {
	CHAT_NONE= 0,
	CHAT_STARTED,
	CHAT_ONGING,
	CHAT_DONE,
	CHAT_CLOSE,
	CHAT_CANCLE,
};



typedef struct Voice
{
	char pData[POST_DATA_LENGTH];  
	int iLength;
	struct Voice *next;  
}VoiceData;

extern int capture_voice(void);

extern void CreateCaptureVoicePthread(void);
extern void SetVADFlag(char byFlag);
extern char GetVADFlag(void);

int IsCaptureCancled();
void SetCaptureState(int state);
int IsCaptureFinshed();
int IsCaptureDone();


int IsChatCancled();
void SetChatState(int state);
int IsChatFinshed();
int IsChatDone();

int capture_turing_chat(char *name);




#endif /* __CAPTIRE_H__ */
