#include "HuabeiStruct.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <semaphore.h>

static HuabeiData *gHuabeiData = NULL;
static pthread_mutex_t gHuabeiDataMtx = PTHREAD_MUTEX_INITIALIZER;  

static HuabeiUser gHuabeiUser;
static pthread_mutex_t gHuabeiUserMtx = PTHREAD_MUTEX_INITIALIZER; 


HuabeiData *newHuabeiData()
{
	HuabeiData *data = NULL;
	
	data = calloc(1, sizeof(HuabeiData));
	if (data == NULL) {
		error("calloc for AudioEvent failed");
		return NULL;
	}

	return data;
}

void freeHuabeiData(HuabeiData **data)
{
	
	if(*data) {
		if((*data)->dirName) {
			free((*data)->dirName);
			(*data)->dirName = NULL;
		}
		if((*data)->url) {
			free((*data)->url);
			(*data)->url = NULL;
		}
		free((*data));
		*data =  NULL;
	}
	
}
int HuabeiDataInit()
{	int ret = 0;
	if(gHuabeiData == NULL) {
		gHuabeiData = newHuabeiData();
		if(!gHuabeiData)
			ret = -1;
		gHuabeiData->res = -1;
	}
	return ret;
}

int HuabeiDataDeinit()
{	int ret = 0;
	pthread_mutex_lock(&gHuabeiDataMtx);
	freeHuabeiData(&gHuabeiData);
	pthread_mutex_unlock(&gHuabeiDataMtx);

	return ret;
}

void SetHuabeiDataDirName(char * dirName) 
{
	assert(gHuabeiData != NULL);
	pthread_mutex_lock(&gHuabeiDataMtx);
	if(gHuabeiData)
	{
		if(gHuabeiData->dirName) {
			free(gHuabeiData->dirName);
			gHuabeiData->dirName = NULL;
		}
		gHuabeiData->dirName = strdup(dirName);
	}
	pthread_mutex_unlock(&gHuabeiDataMtx);
}
void SetHuabeiDataUrl(char * url) 
{
	assert(gHuabeiData != NULL);
	pthread_mutex_lock(&gHuabeiDataMtx);
	{
		if(gHuabeiData->url) {
			free(gHuabeiData->url);
			gHuabeiData->url = NULL;
		}
		gHuabeiData->url = strdup(url);
	}
	pthread_mutex_unlock(&gHuabeiDataMtx);
}
void SetHuabeiDataSerialNo( int serialNo) 
{
	assert(gHuabeiData != NULL);
	pthread_mutex_lock(&gHuabeiDataMtx);
	gHuabeiData->serialNo =serialNo;
	pthread_mutex_unlock(&gHuabeiDataMtx);
}
void SetHuabeiDataRes(int res) 
{
	assert(gHuabeiData != NULL);
	pthread_mutex_lock(&gHuabeiDataMtx);
	gHuabeiData->res =res;
	pthread_mutex_unlock(&gHuabeiDataMtx);
}
void SetHuabeiDataUserkeys( int userkeys) 
{
	assert(gHuabeiData != NULL);
	pthread_mutex_lock(&gHuabeiDataMtx);
	gHuabeiData->userkeys =userkeys;
	pthread_mutex_unlock(&gHuabeiDataMtx);
}

int GetHuabeiDataDirName(void) 
{
	assert(gHuabeiData != NULL);
	char *dirName = NULL;
	pthread_mutex_lock(&gHuabeiDataMtx);
	if(gHuabeiData)
	{
		dirName = gHuabeiData->dirName;
	} 
	pthread_mutex_unlock(&gHuabeiDataMtx);
	return dirName;
}

int GetHuabeiDataUrl(void) 
{
	assert(gHuabeiData != NULL);
	char *url = NULL;
	pthread_mutex_lock(&gHuabeiDataMtx);
	if(gHuabeiData)
	{
		url = gHuabeiData->url;
	} 
	pthread_mutex_unlock(&gHuabeiDataMtx);
	return url;
}

int GetHuabeiDataRes(void) 
{
	int res = -1;
	assert(gHuabeiData != NULL);
	pthread_mutex_lock(&gHuabeiDataMtx);
	if(gHuabeiData)
	{
		res = gHuabeiData->res;
	} 
	pthread_mutex_unlock(&gHuabeiDataMtx);
	return res;
}
int GetHuabeiDataUserkeys(void) 
{
	int res = -1;
	assert(gHuabeiData != NULL);
	pthread_mutex_lock(&gHuabeiDataMtx);
	if(gHuabeiData)
	{
		res = gHuabeiData->userkeys;
	} 
	pthread_mutex_unlock(&gHuabeiDataMtx);
	return res;
}

int GetHuabeiDataSerialNo(void) 
{
	int serialNo = -1;
	assert(gHuabeiData != NULL);
	pthread_mutex_lock(&gHuabeiDataMtx);
	if(gHuabeiData)
	{
		serialNo = gHuabeiData->serialNo;
	} 
	pthread_mutex_unlock(&gHuabeiDataMtx);
	return serialNo;
}

HuabeiSendMsg *newHuabeiSendMsg()
{
	HuabeiSendMsg *msg = NULL;
	
	msg = calloc(1, sizeof(HuabeiSendMsg));
	if (msg == NULL) {
		error("calloc for AudioEvent failed");
		return NULL;
	}

	return msg;
}

void freeHuabeiSendMsg(HuabeiSendMsg **status)
{
	
	if(*status) {
		if((*status)->mediaid) {
			free((*status)->mediaid);
			(*status)->mediaid = NULL;
		}
		if((*status)->tousers) {
			free((*status)->tousers);
			(*status)->tousers = NULL;
		}
		free((*status));
		*status =  NULL;
	}
	
}
void setHuabeiSendMsgMediaid(HuabeiSendMsg *msg ,char *mediaid)
{
	assert(msg != NULL);
	if(msg->mediaid)
		free(msg->mediaid);
	msg->mediaid = mediaid;
}

HuabeiSendMsg* dupHuabeiSendMsg(HuabeiSendMsg *status)
{

	HuabeiSendMsg *msg = NULL;

	if(status == NULL) 
		return NULL;
	
	msg = calloc(1, sizeof(HuabeiSendMsg));
	if(msg) {
		//memcpy(msg, status, sizeof(HuabeiSendMsg));
		msg->type = status->type;
		msg->userkeys = status->userkeys;
		if(status->mediaid)
			msg->mediaid = strdup(status->mediaid);
		if(status->tousers)
			msg->tousers = strdup(status->tousers);
	}
	
	return msg;
}

void HuabeiUserInit(const char * apiKey , const char * aesKey ,const char *deviceId)
{
	pthread_mutex_lock(&gHuabeiUserMtx);
	bzero(&gHuabeiUser,sizeof(HuabeiUser));
	memcpy(gHuabeiUser.aesKey,aesKey,strlen(aesKey));
    memcpy(gHuabeiUser.apiKey,apiKey,strlen(apiKey));
	memcpy(gHuabeiUser.deviceId,deviceId,strlen(deviceId));
	pthread_mutex_unlock(&gHuabeiUserMtx);
}
void SetHuabeiDeviceId(const char * deviceId) 
{
	pthread_mutex_lock(&gHuabeiUserMtx);
	memcpy(gHuabeiUser.deviceId,deviceId,strlen(deviceId));
	pthread_mutex_unlock(&gHuabeiUserMtx);
}
void SetHuabeiApiKey(const char * apiKey) 
{
	pthread_mutex_lock(&gHuabeiUserMtx);
	memcpy(gHuabeiUser.apiKey,apiKey,strlen(apiKey));
	pthread_mutex_unlock(&gHuabeiUserMtx);
}


void SetHuabeiAesKey(const char * aesKey) 
{
	pthread_mutex_lock(&gHuabeiUserMtx);
	memcpy(gHuabeiUser.aesKey,aesKey,strlen(aesKey));
	pthread_mutex_unlock(&gHuabeiUserMtx);
}

char *GetHuabeiDeviceId() 
{
	char * deviceId = NULL;
	pthread_mutex_lock(&gHuabeiUserMtx);
	deviceId = gHuabeiUser.deviceId;
	pthread_mutex_unlock(&gHuabeiUserMtx);
	return deviceId;
}
char *GetHuabeiApiKey() 
{
	char * aesKey = NULL;
	pthread_mutex_lock(&gHuabeiUserMtx);
	aesKey = gHuabeiUser.apiKey;
	pthread_mutex_unlock(&gHuabeiUserMtx);
	return aesKey;
}

char *GetHuabeiAesKey() 
{
	char * apiKey = NULL;
	pthread_mutex_lock(&gHuabeiUserMtx);
	apiKey = gHuabeiUser.aesKey;
	pthread_mutex_unlock(&gHuabeiUserMtx);
	return apiKey;
}

