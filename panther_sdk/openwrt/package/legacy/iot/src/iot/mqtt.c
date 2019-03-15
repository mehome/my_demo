#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/time.h>
#include <errno.h>
#include <assert.h>
#include <limits.h>


#include <mosquitto.h>

#include "myutils/debug.h"
#include "control.h"
#include "event.h"
#include "DeviceStatus.h"
#include "common.h"
#ifdef ENABLE_HUABEI
#include "HuabeiUart.h"
#endif

#ifndef ENABLE_HUABEI
#define IOT_HOST "iot-ai.tuling123.com"
#else
#define IOT_HOST "api.drawbo.com"
#endif
#ifdef ENABLE_HUABEI

static  int g_mqtt_connnect = 1; 

int GetMqttConnect()
{
	int wanif_state = cdb_get_int("$wanif_state", 0);
	if(wanif_state != 2) 
		g_mqtt_connnect = 0;
	return g_mqtt_connnect;
}
#endif
enum {
	
	MQTT_MESSAGE_CHAT_TYPE,  //chat ,
	MQTT_MESSAGE_AUDIO_TYPE , //audio ,
	MQTT_MESSAGE_CONTROL_TYPE,
	MQTT_MESSAGE_NOTIFY_TYPE,
	MQTT_MESSAGE_NONE_TYPE ,
};


/* This struct is used to pass data to callbacks.
 * An instance "ud" is created in main() and populated, then passed to
 * mosquitto_new(). */
struct userdata {
	char **topics;
	int topic_count;
	int topic_qos;
	char **filter_outs;
	int filter_out_count;
	char *username;
	char *password;
	int verbose;
	bool quiet;
	bool no_retain;
	bool eol;
};
static bool run = true;

static unsigned char mqttThreadAlreadyLaunch = FALSE;

static pthread_t mqttThread;
static int      pleaseStop = 0;
#ifdef ENABLE_HUABEI
//static  char *host = "39.108.225.69"; 
static	char *MqttHost = "120.78.196.220";
static char *MqttUser= "admin";
static char *MqttPassword = "AsKdRaWbo20171117";
static	int MqttPort = 61613;
static  int g_mqtt_connnect = 1; 
#else
static char *MqttHost = "mqtt.ai.tuling123.com";
static char *MqttUser= "xinzhongxin";
static char *MqttPassword = "WokJx8RI";
static	int MqttPort = 10883;
#endif

#ifdef ENABLE_HUABEI
int GetMqttConnect()
{
	int wanif_state = cdb_get_int("$wanif_state", 0);
	if(wanif_state != 2) 
		g_mqtt_connnect = 0;
	return g_mqtt_connnect;
}
#endif
/* 处理图灵mqtt云端返回的数据 */
static int   ParseDataFromTuring(char *json)
{
	struct json_object *root = NULL, *type = NULL, *message = NULL,  *fromUser = NULL;
	char *pFromUSer = NULL; 
	warning("json:%s",json);
//json:{"type":1,"fromUser":"owWyV1JRXLXwNspfElM-eCADzdFY","message":{"mediaId":106129,"operate":1,"url":"http://appfile.tuling123.com/media/audio/20180524/80a0bd7c1e034833a14ebc6026525978.mp3","arg":"下雨天","tip":"http://turing-iot.oss-cn-beijing.aliyuncs.com/tts/tts-c49c9fd38a98426abd414bcc071019be.amr"}}
	root = json_tokener_parse(json);
	if (is_error(root)) 
	{
		error("json_tokener_parse root failed");
		return ;
	}
	fromUser =  json_object_object_get(root, "fromUser");
	if(fromUser) {
		char *tmp = NULL;
		tmp = json_object_get_string(fromUser);
		debug("pFromUser:%s",tmp);
		//pFromUser:owWyV1JRXLXwNspfElM-eCADzdFY
		SetTuringMqttFromUser(tmp);				 
	}
	
	type =  json_object_object_get(root, "type");
	if(type) {
	 	int iType;
		message =  json_object_object_get(root, "message");
	 	iType =  json_object_get_int(type);
		debug("iType:%d", iType);
		//iType:1
		if (message) {
			switch(iType) {
				case MQTT_MESSAGE_CHAT_TYPE:	//微信语音信息
					ParseChatData(message);
					break;
				case MQTT_MESSAGE_AUDIO_TYPE:	//播放/暂停，上下曲
					ParseAudioData(message);
					break;
				case MQTT_MESSAGE_CONTROL_TYPE:	//设备控制
					ParseControlData(message);
					break;	
				case MQTT_MESSAGE_NOTIFY_TYPE:	
					ParseNotifyData(message);	//绑定/解绑成功，
					break;
				default:
					error("unsupport message");
					break;		
			}
		}
	 }
	json_object_put(root);
	return 0;
}
/* mqtt数据回调     */
static void my_message_callback(struct mosquitto *mosq, void *obj, const struct mosquitto_message *message)
{
	struct userdata *ud;
	int i;
	bool res;
	int state ;
	assert(obj);
	ud = (struct userdata *)obj;
	state = cdb_get_int("$turing_power_state", 0);
#ifndef ENABLE_HUABEI
	if(state == 2)
		return;
#endif
	if(message->retain && ud->no_retain) return;
	if(ud->filter_outs){
		for(i=0; i<ud->filter_out_count; i++){
			mosquitto_topic_matches_sub(ud->filter_outs[i], message->topic, &res);
			if(res) return;
		}
	}
	if(ud->verbose){
		if(message->payloadlen){
			debug("%s ", message->topic);
		}else{
			debug("%s (null)", message->topic);
		}
	}
	if(message->payloadlen){
#ifdef ENABLE_HUABEI
		HuabeiJsonParser(message->payload);
#else
		ParseDataFromTuring(message->payload);
#endif
	}
}

/* mqtt连接成功回调       */  
static void my_connect_callback(struct mosquitto *mosq, void *obj, int result)
{
	int i;
	struct userdata *ud;

	assert(obj);
	ud = (struct userdata *)obj;
	debug("result:%d",result);
	if(!result){
		for(i=0; i<ud->topic_count; i++){
			debug("ud->topics[%d]:%s ud->topic_qos:%d",i, ud->topics[i],ud->topic_qos);
			mosquitto_subscribe(mosq, NULL, ud->topics[i], ud->topic_qos);
#ifdef ENABLE_HUABEI
			g_mqtt_connnect = 1;
#endif
			info("TuringPlayMusicEvent");
#ifdef ENABLE_YIYA
			int mqtt_connect = cdb_get_int("$mqtt_connect", 0);
			if( mqtt_connect == 0) {
				StartBootMusic();
				cdb_set_int("$mqtt_connect", 1);
			}	
#endif
			info("after TuringPlayMusicEvent");
#ifdef ENABLE_HUABEI
			HuabeiEventMqttConnected();
#endif
		}
	}else{
		if(result && !ud->quiet){
			//fprintf(stderr, "%s\n", mosquitto_connack_string(result));
			error("%s",mosquitto_connack_string(result));
			//exit(-1);
		}
	}
}

static void my_subscribe_callback(struct mosquitto *mosq, void *obj, int mid, int qos_count, const int *granted_qos)
{
	int i;
	struct userdata *ud;
	assert(obj);
	ud = (struct userdata *)obj;


	if(!ud->quiet) debug("Subscribed (mid: %d): %d", mid, granted_qos[0]);
	for(i=1; i<qos_count; i++){
		if(!ud->quiet) debug(", %d", granted_qos[i]);
	}
	if(!ud->quiet) printf("\n");
}

static void my_log_callback(struct mosquitto *mosq, void *obj, int level, const char *str)
{
	debug("%s", str);
}
/* mqtt断开网络回调       */
static void my_disconnect_callback(struct mosquitto *mosq, void *obj, int result)
{
	fatal("  mqtt disconnect ....");
#ifdef ENABLE_HUABEI
	g_mqtt_connnect = 0;
#endif
	run = false;
	pleaseStop = true;
}

/* mqtt线程 */  
static void *mqtt(void *arg)
{
	char *id = NULL;
	char *id_prefix = NULL;
	int i;

	char *topic = NULL;
  	char *clientId = NULL;
	
	char *host = MqttHost;
	int port = MqttPort;
	
	char *bind_address = NULL;
	int keepalive = 60;
	bool clean_session = true;
	bool debug = false;
	struct mosquitto *mosq = NULL;
	int rc;
	char hostname[256];
	char err[1024];
	struct userdata ud;

	char *psk = NULL;
	char *psk_identity = NULL;

	char *ciphers = NULL;

	bool use_srv = false;

	char *will_payload = NULL;
	long will_payloadlen = 0;
	int will_qos = 0;
	bool will_retain = false;
	char *will_topic = NULL;

	bool insecure = false;
	char *cafile = NULL;
	char *capath = NULL;
	char *certfile = NULL;
	char *keyfile = NULL;
	char *tls_version = NULL;
	debug("start....");

	topic = GetTuringMqttTopic();
	clientId = GetTuringMqttClientId();

	memset(&ud, 0, sizeof(struct userdata));
	ud.eol = true;
	
	char *mqttUser = GetTuringMqttMqttUser();
	if(mqttUser == NULL) {
		info("mqttUser == NULL");
		mqttUser =  MqttUser;
	}
	char *mqttPassword = GetTuringMqttMqttPassword();
	if(mqttPassword == NULL) {
		info("mqttPassword == NULL");
		mqttPassword = MqttPassword;
	}
	char *mqttHost = GetTuringMqttMqttHost();
	if(mqttHost == NULL) {
		info("mqttHost == NULL");
		mqttHost = MqttHost;
	}
	char *mqttPort = GetTuringMqttMqttPort();
	if(mqttPort != NULL) {
		info("mqttPort != NULL");
		port = atoi(mqttPort);
	} else {
		info("mqttPort == NULL");	
		port = MqttPort;
	}
	ud.username= mqttUser;
	ud.password = mqttPassword;

	info("mqttHost:%s", mqttHost);
	info("mqttUser:%s, mqttPassword:%s", ud.username, ud.password);
	info("mqttHost:%s, mqttPort:%d", host, port);
	info("topic:%s, clientId:%s", topic, clientId);
	
	mosquitto_lib_init();
	if(topic) {
		ud.topic_count++;
		ud.topics = realloc(ud.topics, ud.topic_count*sizeof(char *));
		ud.topics[ud.topic_count-1] = topic;
	}

	mosq = mosquitto_new(clientId, clean_session, &ud);
	debug("mosquitto_new success");
	if(!mosq){
		switch(errno){
			case ENOMEM:
				if(!ud.quiet) error("Error: Out of memory.\n");
				break;
			case EINVAL:
				if(!ud.quiet) error("Error: Invalid id and/or clean_session.\n");
				break;
		}
		mosquitto_lib_cleanup();
		return 1;
	}
	debug("mosquitto_username_pw_set");
	mosquitto_username_pw_set(mosq, ud.username, ud.password);
	
	if(debug){
		mosquitto_log_callback_set(mosq, my_log_callback);
	}
	if(will_topic && mosquitto_will_set(mosq, will_topic, will_payloadlen, will_payload, will_qos, will_retain)){
		if(!ud.quiet) fprintf(stderr, "Error: Problem setting will.\n");
		mosquitto_lib_cleanup();
		return 1;
	}
	if(ud.username && mosquitto_username_pw_set(mosq, ud.username, ud.password)){
		if(!ud.quiet) fprintf(stderr, "Error: Problem setting username and password.\n");
		mosquitto_lib_cleanup();
		return 1;
	}
	if((cafile || capath) && mosquitto_tls_set(mosq, cafile, capath, certfile, keyfile, NULL)){
		if(!ud.quiet) fprintf(stderr, "Error: Problem setting TLS options.\n");
		mosquitto_lib_cleanup();
		return 1;
	}
	if(insecure && mosquitto_tls_insecure_set(mosq, true)){
		if(!ud.quiet) fprintf(stderr, "Error: Problem setting TLS insecure option.\n");
		mosquitto_lib_cleanup();
		return 1;
	}
	if(psk && mosquitto_tls_psk_set(mosq, psk, psk_identity, NULL)){
		if(!ud.quiet) fprintf(stderr, "Error: Problem setting TLS-PSK options.\n");
		mosquitto_lib_cleanup();
		return 1;
	}
	if(tls_version && mosquitto_tls_opts_set(mosq, 1, tls_version, ciphers)){
		if(!ud.quiet) fprintf(stderr, "Error: Problem setting TLS options.\n");
		mosquitto_lib_cleanup();
		return 1;
	}

	
	mosquitto_connect_callback_set(mosq, my_connect_callback);
	mosquitto_message_callback_set(mosq, my_message_callback);
	mosquitto_disconnect_callback_set(mosq, my_disconnect_callback);
	debug("mosquitto_connect_callback_set success");
	if(debug){
		mosquitto_subscribe_callback_set(mosq, my_subscribe_callback);
	}
	if(use_srv){
		rc = mosquitto_connect_srv(mosq, mqttHost, keepalive, bind_address);
	}else{
		rc = mosquitto_connect_bind(mosq, mqttHost, port, keepalive, bind_address);
	}

	if(rc){
		if(!ud.quiet){
			if(rc == MOSQ_ERR_ERRNO){
#ifndef WIN32
				strerror_r(errno, err, 1024);
#else
				FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, errno, 0, (LPTSTR)&err, 1024, NULL);
#endif
				error("Error: %s\n", err);
				exit(-1);

			}else{
				error("Unable to connect (%d).\n", rc);
				exit(-1);
			}
		}
		mosquitto_lib_cleanup();
		return rc;
	}

#if 1
	rc = mosquitto_loop_forever(mosq, -1, 1);
#else
	do{
		rc = mosquitto_loop(mosq, 1, 10);
	}while(rc == MOSQ_ERR_SUCCESS && run);
#endif
	mosquitto_destroy(mosq);
	mosquitto_lib_cleanup();

	if(rc){
		error("Error: %s\n", mosquitto_strerror(rc));
	}
	return rc;
	pthread_exit(0);
}	

/* 启动mqtt线程      */  
void StartMqttThread(void *arg)
{
	pleaseStop = 0;
	if (mqttThreadAlreadyLaunch)   
        return ;
	pthread_t a_thread;
    pthread_attr_t a_thread_attr;
    PTHREAD_SET_UNATTACH_ATTR(a_thread_attr);
	pthread_attr_setstacksize(&a_thread_attr, PTHREAD_STACK_MIN*8);
	pthread_create(&mqttThread, &a_thread_attr, mqtt, arg);
	pthread_attr_destroy(&a_thread_attr);
	mqttThreadAlreadyLaunch = true;
}
/* 结束mqtt线程      */  
void StopMqttThread()
{
	if(pleaseStop)
		return ;
	pleaseStop = 1;
	
	pthread_join(mqttThread, NULL);
}

/*
 * mqtt参数初始化 
 * 从/etc/config/iot.json中获取
 */  
int MqttInit()
{

	TuringMqttInit();
	char *iotHost = ConfigParserGetValue("iotHost");
	if(iotHost) 
	{
		info("iotHost:%s",iotHost);
		SetTuringMqttIotHost(iotHost);
		free(iotHost);
	} 
	else 
	{
		SetTuringMqttIotHost(IOT_HOST);
	}

	char *mqttHost = ConfigParserGetValue("mqttHost");
	if(mqttHost) 
	{
		info("mqttHost:%s",mqttHost);
		SetTuringMqttMqttHost(mqttHost);
		free(mqttHost);
		
	}
	char *mqttPort = ConfigParserGetValue("mqttPort");
	if(mqttPort) 
	{
		SetTuringMqttMqttPort(mqttPort);
		info("mqttHost:%s",mqttPort);
		free(mqttPort);
	}
	char *mqttUser = ConfigParserGetValue("mqttUser");
	if(mqttUser) 
	{
		SetTuringMqttMqttUser(mqttUser);
		info("mqttHost:%s",mqttUser);
		free(mqttUser);
	}
	char *mqttPassword = ConfigParserGetValue("mqttPassword");
	if(mqttPassword) 
	{
		SetTuringMqttMqttPassword(mqttPassword);
		info("mqttHost:%s",mqttPassword);
		free(mqttPassword);
	}
}

