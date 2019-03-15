#include "event.h"

#include <stdio.h>
#include <assert.h>

#include "myutils/debug.h"
#include "PlaylistStruct.h"

UploadFileEvent *newUploadFileEvent(char *body, char *file, int type)
{
	UploadFileEvent *event = NULL;
	
	event = calloc(1, sizeof(UploadFileEvent));
	if(event == NULL) {
		error("calloc for IotEvent failed");
		return NULL;
	}
	event->body = body;
	event->file = file;
	event->type = type;
	return event;
}
void freeUploadFileEvent(UploadFileEvent **event)
{
	if(*event) 
	{
		if((*event)->body) {
			free((*event)->body);
			(*event)->body = NULL;
		}
		if((*event)->file) {
			free((*event)->file);
			(*event)->file = NULL;
		}
		free((*event));
		*event = NULL;
	}
}


IotEvent *newIotEvent(char *body)
{
	IotEvent *event = NULL;
	
	event = calloc(1, sizeof(IotEvent));
	if(event == NULL) {
		error("calloc for IotEvent failed");
		return NULL;
	}
	event->body = body;
	return event;
}
void freeIotEvent(IotEvent **event)
{
	if(*event) 
	{
		if((*event)->body) {
			free((*event)->body);
			(*event)->body = NULL;
		}
		free((*event));
		*event = NULL;
	}
}

MqttEvent *newMqttEvent(char *message)
{
	MqttEvent *event = NULL;
	
	event = calloc(1, sizeof(MqttEvent));
	if(event == NULL) {
		error("calloc for IotEvent failed");
		return NULL;
	}
	event->message = message;
	return event;
}
void freeMqttEvent(MqttEvent **event)
{
	if(*event) 
	{
		if((*event)->message) {
			free((*event)->message);
			(*event)->message = NULL;
		}
		free(*event);
		*event = NULL;
		
	}
}
MediaIdEvent *newMediaIdEvent(char *mediaId)
{
	MediaIdEvent *event = NULL;
	
	event = calloc(1, sizeof(MediaIdEvent));
	if(event == NULL) {
		error("calloc for IotEvent failed");
		return NULL;
	}
	event->mediaId = mediaId;
	return event;
}
void freeMediaIdEvent(MediaIdEvent **event)
{
	if(*event) 
	{
		if((*event) ->mediaId) {
			free((*event) ->mediaId);
			(*event) ->mediaId = NULL;
		}
		free(*event);
		*event = NULL;
	}
}

FromUserEvent *newFromUserEvent(char *fromUser)
{
	FromUserEvent *event = NULL;
	
	event = calloc(1, sizeof(FromUserEvent));
	if(event == NULL) {
		error("calloc for FromUserEvent failed");
		return NULL;
	}
	event->fromUser= fromUser;
	return event;
}
void freeFromUserEvent(FromUserEvent **event)
{
	if(*event) 
	{
		if((*event)->fromUser) {
			free((*event)->fromUser);
			(*event)->fromUser = NULL;
		}
		free(*event);
		*event = NULL;
	}
}

TopicEvent *newTopicEvent(char *topic, char *clientId)
{
	TopicEvent *event = NULL;
	
	event = calloc(1, sizeof(TopicEvent));
	if(event == NULL) {
		error("calloc for TopicEvent failed");
		return NULL;
	}
	event->topic = topic;
	event->clientId = clientId;
	return event;
}
void freeTopicEvent(TopicEvent **event)
{
	if(*event) 
	{
		if((*event)->topic) {
			free((*event)->topic);
			(*event)->topic = NULL;
		}
		if((*event)->clientId) {
			free((*event)->clientId);
			(*event)->clientId = NULL;
		}
		free(*event);
		*event = NULL;
	}
}


TopicEvent *newAudioEvent(char *body)
{
	AudioEvent *event = NULL;
	
	event = calloc(1, sizeof(AudioEvent));
	if(event == NULL) {
		error("calloc for AudioEvent failed");
		return NULL;
	}
	event->body = body;

	return event;
}
void freeAudioEvent(AudioEvent *event)
{
	if(event) {
		free(event->body);
		free(event);
	}
}

AudioStatus * newAudioStatus(void)
{
	AudioStatus *status = NULL;
	
	status = calloc(1, sizeof(AudioStatus));
	if(status == NULL) {
		error("calloc for AudioEvent failed");
		return NULL;
	}

	return status;
}

void freeAudioStatus(AudioStatus **status)
{
	
	if(*status) {
		if((*status)->title) {
			free((*status)->title);
			(*status)->title = NULL;
		}
		if((*status)->url) {
			free((*status)->url);
			(*status)->url = NULL;
		}
		if((*status)->mediaId) {
			free((*status)->mediaId); 
			(*status)->mediaId = NULL;
		}
		if((*status)->tip) {
			free((*status)->tip);
			(*status)->tip = NULL;
		}
		free((*status));
		*status =  NULL;
	}
	
}

void deepFreeAudioStatus(AudioStatus **status)
{
	
	if(*status) {
		if((*status)->title) {
			free((*status)->title);
			(*status)->title = NULL;
		}
		if((*status)->url) {
			free((*status)->url);
			(*status)->url = NULL;
		}
		if((*status)->mediaId) {
			free((*status)->mediaId); 
			(*status)->mediaId = NULL;
		}
		if((*status)->tip) {
			free((*status)->tip);
			(*status)->tip = NULL;
		}
	}
	
}

int dupAudioStatus(AudioStatus *dest, AudioStatus *src)
{

	assert(dest != NULL);
	assert(src != NULL);
	
	if(src->url) {
		dest->url = strdup(src->url);
	}
	if(src->title) {
		dest->url = strdup(src->title);
	}
	if(src->artist) {
		dest->artist = strdup(src->artist);
	}
	if(src->mediaId) {
		dest->mediaId = strdup(src->mediaId);
	}
	if(src->tip) {
		dest->tip = strdup(src->tip);
	}

	dest->type = src->type; 		
	dest->play = src->play;
	dest->state = src->state;
	dest->progress = src->progress;
	dest->repeat = src->repeat;
	dest->random = src->random;
	dest->single = src->single;
	return 0;
}

int cpAudioStatusToMusicItem(MusicItem *dest, AudioStatus *src)
{

	assert(dest != NULL);
	assert(src != NULL);
	
	if(src->url) {
		dest->pContentURL = strdup(src->url);
	}
	if(src->title) {
		dest->pTitle = strdup(src->title);
	}
	if(src->mediaId) {
		dest->pId = strdup(src->mediaId);
	}
	if(src->artist) {
		dest->pArtist = strdup(src->artist);
	}
	if(src->tip) {
		dest->pTip = strdup(src->tip);
	}

	dest->type = src->type; 		

	return 0;
}



