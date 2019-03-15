#ifndef ____MYUTILS_CONFIG_PARSER_H__
#define ____MYUTILS_CONFIG_PARSER_H__

#define IOT_CONFIG_FILE "/etc/config/iot.json"

typedef struct IotConfig {
	char *appID;
	char *apiKeyTuring;
	char *aesKeyTuring;
	char *toneTuring;
	char *speedTuring;
	char *pitchTuring;
	char *volumeTuring;
	char *apiKeyHuabei;
	char *aesKeyHuabei;
	char *aiWiFiUrl;
	char *apiUrl;
	char *iotHost;	
	char *discoverType;
	char *mqttUser;
	char *mqttPassword;
	char *mqttPort;
	char *mqttHost;
	char *productId;
	char *showLog;
	char *logLevel;
	char *vadCount;
    char *vad_Threshold;
}IotConfig;

void ShowIotConfig(IotConfig *config);
void FreeIotConfig();
int ConfigParserReader(IotConfig* config);
int ConfigParserWriter(IotConfig* config);
int ConfigParserSetValue(char *key ,char *value);
char *ConfigParserGetValue(char *key);
void ConfigParserFreeValue(char *key);



#endif











