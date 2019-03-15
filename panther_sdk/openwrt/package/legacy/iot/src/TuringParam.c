
#include "TuringParam.h"
#include <assert.h>
#include <pthread.h>
#include <curl/curl.h>
#include <semaphore.h>
#include "uciConfig.h"


static TuringUser gTuringUser;
static pthread_mutex_t gTuringUserMtx = PTHREAD_MUTEX_INITIALIZER;  
static TuringMqtt gTuringMqtt;
static pthread_mutex_t gTuringMqttMtx = PTHREAD_MUTEX_INITIALIZER;  


static TuringUser* newTuringUsers(const char *userId,const char *aesKey,const char *apiKey)
{
	TuringUser *tulingUser = NULL;
	tulingUser = (TuringUser *)calloc(1,sizeof(TuringUser));
	if(tulingUser==NULL){
		return NULL;
	}
	memcpy(tulingUser->userId,userId,strlen(userId));
    memcpy(tulingUser->aesKey,aesKey,strlen(aesKey));
    memcpy(tulingUser->apiKey,apiKey,strlen(apiKey));
	return tulingUser;
}
static void setTuringUserToken(TuringUser*turingUser,  const char * token)
{
	memcpy(turingUser->token,token,strlen(token));
}
static void setTuringUserId(TuringUser*turingUser,  const char * userId)
{
	memcpy(turingUser->userId,userId,strlen(userId));
}
static void setTuringUserApiKey(TuringUser*turingUser,  const char * apiKey)
{
	memcpy(turingUser->apiKey,apiKey,strlen(apiKey));
}
static void setTuringUserAesKey(TuringUser*turingUser,  const char * aesKey)
{
	memcpy(turingUser->aesKey,aesKey,strlen(aesKey));
}
static void setTuringUserTone(TuringUser*turingUser, int tone)
{
	turingUser->tone = tone;
}

static void freeTulingUser(TuringUser *turingUser)
{
	if(turingUser){
		free(turingUser);
		turingUser=NULL;
	}
}


void     TuringUserInit(const char * apiKey , const char * aesKey ,const char *deviceId,  const char * userId, const char * token)
{
	pthread_mutex_lock(&gTuringUserMtx);
	bzero(&gTuringUser,sizeof(TuringUser));
	memcpy(gTuringUser.userId,userId,strlen(userId));
	memcpy(gTuringUser.aesKey,aesKey,strlen(aesKey));
    memcpy(gTuringUser.apiKey,apiKey,strlen(apiKey));
	memcpy(gTuringUser.deviceId,deviceId,strlen(deviceId));
	//memcpy(gTuringUser.deviceId,"aiAA8005dfc498ea",strlen("aiAA8005dfc498ea"));
	memcpy(gTuringUser.token,token,strlen(token));
	gTuringUser.tone = 0;
	gTuringUser.type = 0;
	pthread_mutex_unlock(&gTuringUserMtx);
}
void SetTuringDeviceId(const char * deviceId) 
{
	pthread_mutex_lock(&gTuringUserMtx);
	memcpy(gTuringUser.deviceId,deviceId,strlen(deviceId));
	pthread_mutex_unlock(&gTuringUserMtx);
}

char *GetTuringDeviceId() 
{
	char * deviceId = NULL;
	pthread_mutex_lock(&gTuringUserMtx);
	deviceId = gTuringUser.deviceId;
	pthread_mutex_unlock(&gTuringUserMtx);
	return deviceId;
}

char *GetTuringProductId()
{
    char * productId = NULL;
    pthread_mutex_lock(&gTuringUserMtx);
    productId = gTuringUser.productId;
    pthread_mutex_unlock(&gTuringUserMtx);
    return productId;
}
void SetTuringProductId(const char * productId)
{
    pthread_mutex_lock(&gTuringUserMtx);
    memcpy(gTuringUser.productId,productId,strlen(productId));
    pthread_mutex_unlock(&gTuringUserMtx);
}

void SetTuringToken(const char * token) 
{
	pthread_mutex_lock(&gTuringUserMtx);
	memcpy(gTuringUser.token,token,strlen(token));
	pthread_mutex_unlock(&gTuringUserMtx);
}


char *GetTuringToken() 
{
	char * token = NULL;
	pthread_mutex_lock(&gTuringUserMtx);
	token = gTuringUser.token;
	pthread_mutex_unlock(&gTuringUserMtx);
	return token;
}
void SetTuringUserId(const char * userId) 
{
	pthread_mutex_lock(&gTuringUserMtx);
	memcpy(gTuringUser.userId,userId,strlen(userId));
	pthread_mutex_unlock(&gTuringUserMtx);
}

char *GetTuringUserId() 
{
	char * userId = NULL;
	pthread_mutex_lock(&gTuringUserMtx);
	userId = gTuringUser.userId;
	pthread_mutex_unlock(&gTuringUserMtx);
	return userId;
}
void SetTuringApiKey(const char * apiKey) 
{
	pthread_mutex_lock(&gTuringUserMtx);
	memcpy(gTuringUser.apiKey,apiKey,strlen(apiKey));
	pthread_mutex_unlock(&gTuringUserMtx);
}

char *GetTuringApiKey() 
{
	char * aesKey = NULL;
	pthread_mutex_lock(&gTuringUserMtx);
	aesKey = gTuringUser.apiKey;
	pthread_mutex_unlock(&gTuringUserMtx);
	return aesKey;
}

char *GetTuringAesKey() 
{
	char * apiKey = NULL;
	pthread_mutex_lock(&gTuringUserMtx);
	apiKey = gTuringUser.aesKey;
	pthread_mutex_unlock(&gTuringUserMtx);
	return apiKey;
}

void SetTuringAesKey(const char * aesKey) 
{
	pthread_mutex_lock(&gTuringUserMtx);
	memcpy(gTuringUser.aesKey,aesKey,strlen(aesKey));
	pthread_mutex_unlock(&gTuringUserMtx);
}

//是否使用VAD，0：使用           2：不使用
void SetTuringType(int type)
{
	pthread_mutex_lock(&gTuringUserMtx);
	gTuringUser.type = type;
	pthread_mutex_unlock(&gTuringUserMtx);
}
int GetTuringType() 
{
	int tone = 0;
	pthread_mutex_lock(&gTuringUserMtx);
	tone = gTuringUser.type;
	pthread_mutex_unlock(&gTuringUserMtx);
	return tone;
}

int GetTuringTone() 
{
	int tone = 0;
	pthread_mutex_lock(&gTuringUserMtx);
	tone = gTuringUser.tone;
	pthread_mutex_unlock(&gTuringUserMtx);
	return tone;
}

void SetTuringTone(int tone)
{
	pthread_mutex_lock(&gTuringUserMtx);
	gTuringUser.tone = tone;
	pthread_mutex_unlock(&gTuringUserMtx);
}

int GetTuringSpeed() 
{
	int speed = 0;
	pthread_mutex_lock(&gTuringUserMtx);
	speed = gTuringUser.speed;
	pthread_mutex_unlock(&gTuringUserMtx);
	return speed;
}

void SetTuringSpeed(int speed)
{
	pthread_mutex_lock(&gTuringUserMtx);
	gTuringUser.speed = speed;
	pthread_mutex_unlock(&gTuringUserMtx);
}

int GetTuringPitch() 
{
	int pitch = 0;
	pthread_mutex_lock(&gTuringUserMtx);
	pitch = gTuringUser.pitch;
	pthread_mutex_unlock(&gTuringUserMtx);
	return pitch;
}

void SetTuringPitch(int pitch)
{
	pthread_mutex_lock(&gTuringUserMtx);
	gTuringUser.pitch = pitch;
	pthread_mutex_unlock(&gTuringUserMtx);
}

int GetTuringVolume() 
{
	int volume = 0;
	pthread_mutex_lock(&gTuringUserMtx);
	volume = gTuringUser.volume;
	pthread_mutex_unlock(&gTuringUserMtx);
	return volume;
}

void SetTuringVolume(int volume)
{
	pthread_mutex_lock(&gTuringUserMtx);
	gTuringUser.volume = volume;
	pthread_mutex_unlock(&gTuringUserMtx);
}

TuringMqtt *newTuringMqtt()
{
	TuringMqtt *turingMqtt = NULL;
	turingMqtt = (TuringMqtt *)calloc(1,sizeof(TuringMqtt));
	if(turingMqtt == NULL){
		return NULL;
	}

	return turingMqtt;
}

void freeTuringMqtt(TuringMqtt **mqtt)
{

	if(*mqtt) {
		if((*mqtt)->topic){
			free((*mqtt)->topic);
			(*mqtt)->topic =NULL;
		}
		if((*mqtt)->clientId){
			free((*mqtt)->clientId);
			(*mqtt)->clientId =NULL;
		}
		if((*mqtt)->topic){
			free((*mqtt)->topic);
			(*mqtt)->topic =NULL;
		}
		if((*mqtt)->mediaId){
			free((*mqtt)->mediaId);
			(*mqtt)->mediaId =NULL;
		}
		if((*mqtt)->fromUser){
			free((*mqtt)->fromUser);
			(*mqtt)->fromUser =NULL;
		}
		if((*mqtt)->iotHost){
			free((*mqtt)->iotHost);
			(*mqtt)->iotHost =NULL;
		}
		if((*mqtt)->mqttUser){
			free((*mqtt)->mqttUser);
			(*mqtt)->mqttUser =NULL;
		}
		if((*mqtt)->mqttPassword){
			free((*mqtt)->mqttPassword);
			(*mqtt)->mqttPassword =NULL;
		}
		if((*mqtt)->mqttHost){
			free((*mqtt)->mqttHost);
			(*mqtt)->mqttHost =NULL;
		}
		if((*mqtt)->mqttPort){
			free((*mqtt)->mqttPort);
			(*mqtt)->mqttPort =NULL;
		}
		free(*mqtt);
		*mqtt= NULL;
	}
}

int TuringMqttInit()
{	int ret = 0;
	pthread_mutex_lock(&gTuringMqttMtx);
	bzero(&gTuringMqtt, sizeof(TuringMqtt));
	pthread_mutex_unlock(&gTuringMqttMtx);

	return ret;
}
void TuringMqttDeinit()
{
	TuringMqtt *info = &gTuringMqtt;
	pthread_mutex_lock(&gTuringMqttMtx);
	freeTuringMqtt(&info);
	pthread_mutex_unlock(&gTuringMqttMtx);
}

void SetTuringMqttClientId(char * clinetId) 
{
	pthread_mutex_lock(&gTuringMqttMtx);
	{
		if(gTuringMqtt.clientId) {
			free(gTuringMqtt.clientId);
			gTuringMqtt.clientId = NULL;
		}
		gTuringMqtt.clientId = strdup(clinetId);
	}
	pthread_mutex_unlock(&gTuringMqttMtx);
}
void SetTuringMqttTopic(char * topic) 
{
	pthread_mutex_lock(&gTuringMqttMtx);
	if(gTuringMqtt.topic) {
		free(gTuringMqtt.topic);
		gTuringMqtt.topic = NULL;
	}
	gTuringMqtt.topic = strdup(topic);
	pthread_mutex_unlock(&gTuringMqttMtx);
}
void SetTuringMqttMediaId(char * mediaId) 
{
	pthread_mutex_lock(&gTuringMqttMtx);
	//if(gTuringMqtt)
	{
	
		if(gTuringMqtt.mediaId) {
			free(gTuringMqtt.mediaId);
			gTuringMqtt.mediaId = NULL;
		}
		gTuringMqtt.mediaId = strdup(mediaId);
	}
	pthread_mutex_unlock(&gTuringMqttMtx);
}

void SetTuringMqttFromUser(char * fromUser) 
{
	pthread_mutex_lock(&gTuringMqttMtx);
	//if(gTuringMqtt)
	{
		if(gTuringMqtt.fromUser) {
			free(gTuringMqtt.fromUser);
			gTuringMqtt.fromUser = NULL;
		}
		gTuringMqtt.fromUser = strdup(fromUser);
	}
	pthread_mutex_unlock(&gTuringMqttMtx);
}

void SetTuringMqttIotHost(char * iotHost) 
{
	pthread_mutex_lock(&gTuringMqttMtx);
	//if(gTuringMqtt)
	{
		if(gTuringMqtt.iotHost) {
			free(gTuringMqtt.iotHost);
			gTuringMqtt.iotHost = NULL;
		}
		gTuringMqtt.iotHost = strdup(iotHost);
	}
	pthread_mutex_unlock(&gTuringMqttMtx);
}

void SetTuringMqttMqttUser(char * mqttUser) 
{
	pthread_mutex_lock(&gTuringMqttMtx);
	//if(gTuringMqtt)
	{
	
		if(gTuringMqtt.mqttUser) {
			free(gTuringMqtt.mqttUser);
			gTuringMqtt.mqttUser = NULL;
		}
		gTuringMqtt.mqttUser = strdup(mqttUser);
	}
	pthread_mutex_unlock(&gTuringMqttMtx);
}
void SetTuringMqttMqttPassword(char * mqttPassword) 
{
	pthread_mutex_lock(&gTuringMqttMtx);
	//if(gTuringMqtt)
	{
	
		if(gTuringMqtt.mqttPassword) {
			free(gTuringMqtt.mqttPassword);
			gTuringMqtt.mqttPassword = NULL;
		}
		gTuringMqtt.mqttPassword = strdup(mqttPassword);
	}
	pthread_mutex_unlock(&gTuringMqttMtx);
}
void SetTuringMqttMqttHost(char * mqttHost) 
{
	pthread_mutex_lock(&gTuringMqttMtx);
	//if(gTuringMqtt)
	{
	
		if(gTuringMqtt.mqttHost) {
			free(gTuringMqtt.mqttHost);
			gTuringMqtt.mqttHost = NULL;
		}
		gTuringMqtt.mqttHost = strdup(mqttHost);
	}
	pthread_mutex_unlock(&gTuringMqttMtx);
}
void SetTuringMqttMqttPort(char * mqttPort) 
{
	pthread_mutex_lock(&gTuringMqttMtx);
	//if(gTuringMqtt)
	{
	
		if(gTuringMqtt.mqttPort) {
			free(gTuringMqtt.mqttPort);
			gTuringMqtt.mqttPort = NULL;
		}
		gTuringMqtt.mqttPort = strdup(mqttPort);
	}
	pthread_mutex_unlock(&gTuringMqttMtx);
}

char *GetTuringMqttFromUser(void) 
{
	char *fromUser = NULL;
	pthread_mutex_lock(&gTuringMqttMtx);
	//if(gTuringMqtt)
	{
		if(gTuringMqtt.fromUser) {
			fromUser = gTuringMqtt.fromUser;
		}
	}
	pthread_mutex_unlock(&gTuringMqttMtx);
	return fromUser;
}

char *GetTuringMqttTopic(void) 
{
	char *topic = NULL;
	//pthread_mutex_lock(&gTuringMqttMtx);
	//if(gTuringMqtt)
	{
		if(gTuringMqtt.topic) {
			topic = gTuringMqtt.topic;
		}
	} 
	//pthread_mutex_unlock(&gTuringMqttMtx);
	return topic;
}

char *GetTuringMqttClientId(void) 
{
	char *clientId = NULL;
	pthread_mutex_lock(&gTuringMqttMtx);
	//if(gTuringMqtt)
	{
		if(gTuringMqtt.clientId) {
			clientId = gTuringMqtt.clientId;
		}
	} 
	pthread_mutex_unlock(&gTuringMqttMtx);
	return clientId;
}

char *GetTuringMqttMediaId(void) 
{
	char *mediaId = NULL;
	pthread_mutex_lock(&gTuringMqttMtx);
	//if(gTuringMqtt)
	{
		if(gTuringMqtt.mediaId) {
			mediaId = gTuringMqtt.mediaId;
		}
	}
	pthread_mutex_unlock(&gTuringMqttMtx);
	return mediaId;
}
char *GetTuringMqttIotHost(void) 
{
	char *iotHost = NULL;
	pthread_mutex_lock(&gTuringMqttMtx);
	//if(gTuringMqtt)
	{
		if(gTuringMqtt.iotHost) {
			iotHost = gTuringMqtt.iotHost;
		}
	}
	pthread_mutex_unlock(&gTuringMqttMtx);
	return iotHost;
}
char *GetTuringMqttMqttUser(void) 
{
	char *mqttUser = NULL;
	pthread_mutex_lock(&gTuringMqttMtx);
	//if(gTuringMqtt)
	{
		if(gTuringMqtt.mqttUser) {
			mqttUser = gTuringMqtt.mqttUser;
		}
	}
	pthread_mutex_unlock(&gTuringMqttMtx);
	return mqttUser;
}
char *GetTuringMqttMqttPassword(void) 
{
	char *mqttPassword = NULL;
	pthread_mutex_lock(&gTuringMqttMtx);
	//if(gTuringMqtt)
	{
		if(gTuringMqtt.mqttPassword) {
			mqttPassword = gTuringMqtt.mqttPassword;
		}
	}
	pthread_mutex_unlock(&gTuringMqttMtx);
	return mqttPassword;
}

char *GetTuringMqttMqttHost(void) 
{
	char *mqttHost = NULL;
	pthread_mutex_lock(&gTuringMqttMtx);
	//if(gTuringMqtt)
	{
		if(gTuringMqtt.mqttHost) {
			mqttHost = gTuringMqtt.mqttHost;
		}
	}
	pthread_mutex_unlock(&gTuringMqttMtx);
	return mqttHost;
}

char *GetTuringMqttMqttPort(void) 
{
	char *mqttPort = NULL;
	pthread_mutex_lock(&gTuringMqttMtx);
	//if(gTuringMqtt)
	{
		if(gTuringMqtt.mqttPort) {
			mqttPort = gTuringMqtt.mqttPort;
		}
	}
	pthread_mutex_unlock(&gTuringMqttMtx);
	return mqttPort;
}

void TuringTTsInit(TuringTTsParam *param, int tone, int speed, int pitch, int volume)
{
	assert(param != NULL);
	
	param->tone = tone;
	param->speed = speed;
	param->pitch = pitch;
	param->volume = volume;
	
}
