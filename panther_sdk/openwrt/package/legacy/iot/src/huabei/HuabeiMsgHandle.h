#ifndef __HUABEI_MSG_HANDLE_H__

#define __HUABEI_MSG_HANDLE_H__


typedef int (*handler)(void *arg);


typedef struct Huabei_msg_handler{
    int type;
    handler handle;
}HuabeiMsgHanlder;


enum {
	HUABEI_MQTT_TYPE_VOICE_INTERCOM  = 0,
	HUABEI_MQTT_TYPE_MUSIC_STORY,
	HUABEI_MQTT_TYPE_DATA_FILE,
	HUABEI_MQTT_TYPE_OPERATION_CONTROL,
	HUABEI_MQTT_TYPE_USERINTERFACE_CONTROL,
	HUABEI_MQTT_TYPE_STABLEDISEASE_CONTROL,
};

int HuabeiMsgHanlde(int type ,void *arg);

#endif