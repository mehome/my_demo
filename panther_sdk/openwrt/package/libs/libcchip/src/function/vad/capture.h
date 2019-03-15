#ifndef __CAPTURE_H__
#define __CAPTURE_H__

//#define _AUDIO_TEST_

#define CAPTURE_VAD_COUNT 28

#define POST_DATA_LENGTH 1024 * 16

#define DEBUG_ON         

#ifdef DEBUG_ON
#define DEBUG_PRINTF(fmt, args...) do { \
	{printf("[%s:%d]"#fmt"\r\n", __func__, __LINE__, ##args); }} while(0) 
#else
	DEBUG_PRINTF(fmt, args...)
#endif

#define DEBUG_ERROR(fmt, args...) do { printf("\033[1;31m""[%s:%d]"#fmt"\r\n""\033[0m", __func__, __LINE__, ##args); } while(0) 

extern int capture_voice(char *pVADValue);


#endif /* __CAPTIRE_H__ */
