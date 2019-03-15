#ifndef __SPEECH_HEADER__
#define __SPEECH_HEADER__


enum {
	IFLYTEK_NONE= 0,
	IFLYTEK_STARTED,
	IFLYTEK_ONGING,
	IFLYTEK_DONE,
	IFLYTEK_CLOSE,
	IFLYTEK_CANCLE,
};

extern void WaitForIflytekFinshed();
extern void StartIflytek(char *value);

extern     int ifly_tts(char *word);

#endif  /* __SPEECH_HEADER__ */
