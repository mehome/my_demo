#ifndef __TURING_PARAM_H__
#define  __TURING_PARAM_H__

/* 图灵TTS参数. */
typedef      struct TuringTTsParam 
{
	int tts;
	int tts_lan;
	int speed;	// 0-9  
	int pitch;  // 0-9 
	int tone;   // 0,1,2
	int volume; // 0-9 
}TuringTTsParam;

/* 图灵用户参数. */
typedef struct turing_user{
	char userId[33];
	char deviceId[17];
	char aesKey[17];
	char apiKey[33];
	char token[33];
	int type;
	int tone;
	int volume;
	int speed;	
	int pitch;
    char productId[32];
}TuringUser;


/* mqtt相关的参数. */
typedef struct turing_mqtt {
	char *topic;
	char *clientId;
	char *mediaId;
	char *fromUser;
	char *iotHost;
	char *mqttUser;
	char *mqttPassword;
	char *mqttHost;
	char *mqttPort;
}TuringMqtt;

void SetTuringVolume(int volume);
int GetTuringVolume() ;
void SetTuringPitch(int pitch);
int GetTuringPitch() ;
void SetTuringSpeed(int speed);
int GetTuringSpeed() ;
void SetTuringTone(int tone);
int GetTuringTone() ;
void SetTuringType(int type);
int GetTuringType() ;
void SetTuringAesKey(const char * aesKey) ;
char *GetTuringAesKey() ;
char *GetTuringApiKey() ;
void SetTuringApiKey(const char * apiKey) ;
char *GetTuringUserId() ;
void SetTuringUserId(const char * userId) ;
char *GetTuringToken() ;
void SetTuringToken(const char * token) ;
char *GetTuringDeviceId() ;
void SetTuringDeviceId(const char * deviceId) ;
/* BEGIN: Added by Frog, 2018/4/28 */
char *GetTuringProductId() ;
void SetTuringProductId(const char * productId) ;
/* END:   Added by Frog, 2018/4/28 */
void     TuringUserInit(const char * apiKey , const char * aesKey ,const char *deviceId,  const char * userId, const char * token);
TuringMqtt *newTuringMqtt();
void freeTuringMqtt(TuringMqtt **mqtt);
int TuringMqttInit();
void TuringMqttDeinit();
void SetTuringMqttClientId(char * clinetId) ;
void SetTuringMqttTopic(char * topic) ;
void SetTuringMqttMediaId(char * mediaId) ;
void SetTuringMqttFromUser(char * fromUser) ;
void SetTuringMqttIotHost(char * iotHost) ;
void SetTuringMqttMqttUser(char * mqttUser) ;
void SetTuringMqttMqttPassword(char * mqttPassword) ;
void SetTuringMqttMqttHost(char * mqttHost) ;
void SetTuringMqttMqttPort(char * mqttPort) ;
char *GetTuringMqttFromUser(void) ;
char *GetTuringMqttTopic(void) ;
char *GetTuringMqttClientId(void) ;
char *GetTuringMqttMediaId(void) ;
char *GetTuringMqttIotHost(void) ;
char *GetTuringMqttMqttUser(void) ;
char *GetTuringMqttMqttPassword(void) ;
char *GetTuringMqttMqttHost(void) ;
char *GetTuringMqttMqttPort(void) ;



































#endif
















