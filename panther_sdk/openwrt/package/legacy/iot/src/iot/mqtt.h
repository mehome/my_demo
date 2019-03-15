#ifndef __MQTT_H__
#define __MQTT_H__
extern void StartMqttThread(void *arg);
extern void StopMqttThread();
extern int MqttInit();

#endif