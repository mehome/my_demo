#ifndef __CAPTURE_H__
#define __CAPTURE_H__

//#define _AUDIO_TEST_

#define CAPTURE_VAD_COUNT 28

#define POST_DATA_LENGTH 1024 * 16

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


#endif /* __CAPTIRE_H__ */
