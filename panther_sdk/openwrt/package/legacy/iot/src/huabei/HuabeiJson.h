#ifndef __HUABEI_JSON_H__

#define __HUABEI_JSON_H__

char *HuabeiJsonUploadFile(char *key, char *deviceId, int type, char *speech);
char *HuabeiJsonGetTopic(char *key, char *deviceId);
char *HuabeiJsonNotifyStatus(char *key, char *deviceId, int type);
char *HuabeiJsonSendMessage(char *key, char *deviceId, void *msg);



#endif