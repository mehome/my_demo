#ifndef __EVENT_H__
#define __EVENT_H__

#include <stdio.h>

#include "DeviceStatus.h"



IotEvent *newIotEvent(char *body);
void freeIotEvent(IotEvent **event);

MqttEvent *newMqttEvent(char *message);
void freeMqttEvent(MqttEvent **event);

MediaIdEvent *newMediaIdEvent(char *mediaId);
void freeMediaIdEvent(MediaIdEvent **event);


TopicEvent *newTopicEvent(char *topic, char *clientId);
void freeTopicEvent(TopicEvent **event);

UploadFileEvent *newUploadFileEvent(char *body, char *file, int type);
void freeUploadFileEvent(UploadFileEvent **event);



#endif
