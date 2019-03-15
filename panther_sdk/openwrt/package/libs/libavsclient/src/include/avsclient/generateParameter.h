#ifndef __GENERATE_PARAMETER_H__
#define __GENERATE_PARAMETER_H__

#define LOCK_FILE_TRY_TIMES 1000
typedef struct LOGIN_INFO{
	char *pCodeVerifier; 
	char *pCodeChallenge;
	char *pCodeChallengeMethod;
	char *pProductId;
	char *pDeviceSerialNumber;
}LOGIN_INFO_STRUCT;

extern char* getConfigJsonData(char *pPath) ;
extern LOGIN_INFO_STRUCT* logInInfo_new();
extern void logInInfo_free(LOGIN_INFO_STRUCT *loginfo);

#endif
