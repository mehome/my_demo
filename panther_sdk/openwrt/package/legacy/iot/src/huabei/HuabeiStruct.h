#ifndef __HUABEI_STRUCT_H__

#define      __HUABEI_STRUCT_H__

typedef struct HuabeiData {
	char *dirName;
	char *url;
	int serialNo;
	int res;  //0 : net 1:
	int userkeys;
	int index; 
	int type;
}HuabeiData;


typedef struct HuabeiSendMsg {
	int type;
	int userkeys;
	char *mediaid;
	char *tousers;
}HuabeiSendMsg;

typedef struct huabei_user {
	char deviceId[17];
	char aesKey[17];
	char apiKey[33];
} HuabeiUser;

HuabeiSendMsg *newHuabeiSendMsg();
void freeHuabeiSendMsg(HuabeiSendMsg **status);
void setHuabeiSendMsgMediaid(HuabeiSendMsg *msg ,char *mediaid);
HuabeiSendMsg* dupHuabeiSendMsg(HuabeiSendMsg *status);

char *GetHuabeiApiKey() ;
char *GetHuabeiAesKey() ;
char *GetHuabeiDeviceId() ;
void SetHuabeiAesKey(const char * aesKey) ;
void SetHuabeiApiKey(const char * apiKey) ;
void SetHuabeiDeviceId(const char * deviceId) ;
void HuabeiUserInit(const char * apiKey , const char * aesKey ,const char *deviceId);










#endif


