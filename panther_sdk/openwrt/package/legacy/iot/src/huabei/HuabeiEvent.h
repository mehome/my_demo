#ifndef __HUABEI_JSON_H__

#define __HUABEI_JSON_H__

int HuabeiEventUploadFile(int type, char *speech);
int HuabeiEventGetTopic();
int HuabeiEventNotifyStatus(int type);
int HuabeiEventSendMessage(void *msg);
#endif