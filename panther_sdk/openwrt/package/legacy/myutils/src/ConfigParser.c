#include "ConfigParser.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>

#include <json-c/json.h>

#include "debug.h"





#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
#endif


typedef int (*writerPorc)(void *arg, char *value);
typedef  char * (*readerPorc)(void *arg);

typedef struct      KeyWriter     {
	//char key[16];
	char *key;
	writerPorc exec;
}KeyWriter;

typedef struct      KeyReader   {
	//char key[16];
	char *key;
	readerPorc exec;
}KeyReader;


IotConfig *conf = NULL;


static void     appid_writer(void *arg, char *val)
{
	debug("....");
	IotConfig* config = (IotConfig* )arg;
	if(config->appID) {
		free(config->appID);
		config->appID = NULL;
	}
	config->appID = strdup(val);
}
static void 	apikeyturing_writer(void *arg, char *val)
{
	debug("....");
	IotConfig* config = (IotConfig* )arg;
	if(config->apiKeyTuring) {
		free(config->apiKeyTuring);
		config->apiKeyTuring = NULL;
	}
	config->apiKeyTuring = strdup(val);
}
static void     aeskeyturing_writer(void *arg, char *val)
{
	debug("....");
	IotConfig* config = (IotConfig* )arg;
	if(config->aesKeyTuring) {
		free(config->aesKeyTuring);
		config->aesKeyTuring = NULL;
	}
	config->aesKeyTuring = strdup(val);
}

static void 	toneturing_writer(void *arg, char *val)
{
	debug("....");
	IotConfig* config = (IotConfig* )arg;
	if(config->toneTuring) {
		free(config->toneTuring);
		config->toneTuring = NULL;
	}
	config->toneTuring = strdup(val);
}
static void  	speedturing_writer(void *arg, char *val)
{
	debug("....");
	IotConfig* config = (IotConfig* )arg;
	if(config->speedTuring) {
		free(config->speedTuring);
		config->speedTuring = NULL;
	}
	config->speedTuring = strdup(val);
}
static void  	pitchturing_writer(void *arg, char *val)
{
	debug("....");
	IotConfig* config = (IotConfig* )arg;
	if(config->pitchTuring) {
		free(config->pitchTuring);
		config->pitchTuring = NULL;
	}
	config->pitchTuring = strdup(val);
}
static void volumeturing_writer(void *arg, char *val)
{
	debug("....");
	IotConfig* config = (IotConfig* )arg;
	if(config->volumeTuring) {
		free(config->volumeTuring);
		config->volumeTuring = NULL;
	}
	config->volumeTuring = strdup(val);
}

static void apikeyhuabei_writer(void *arg, char *val)
{
	debug("....");
	IotConfig* config = (IotConfig* )arg;
	if(config->apiKeyHuabei) {
		free(config->apiKeyHuabei);
		config->apiKeyHuabei = NULL;
	}
	config->apiKeyHuabei = strdup(val);
}
static void     aeskeyhuabei_writer(void *arg, char *val)
{
	debug("....");
	IotConfig* config = (IotConfig* )arg;
	if(config->aesKeyHuabei) {
		free(config->aesKeyHuabei);
		config->aesKeyHuabei = NULL;
	}
	config->aesKeyHuabei = strdup(val);
}

static void aiwifiurl_writer(void *arg, char *val)
{
	debug("....");
	IotConfig* config = (IotConfig* )arg;
	if(config->aiWiFiUrl) {
		free(config->aiWiFiUrl);
		config->aiWiFiUrl = NULL;
	}
	config->aiWiFiUrl = strdup(val);
}


static void 	apiurl_writer(void *arg, char *val)
{
	debug("....");
	IotConfig* config = (IotConfig* )arg;
	if(config->apiUrl) {
		free(config->apiUrl);
		config->apiUrl = NULL;
	}
	config->apiUrl = strdup(val);
}

static void iothost_writer(void *arg, char *val)
{
	debug("....");
	IotConfig* config = (IotConfig* )arg;
	if(config->iotHost) {
		free(config->iotHost);
		config->iotHost = NULL;
	}
	config->iotHost = strdup(val);
}	
static void discovertype_writer(void *arg, char *val)
{
	debug("....");
	IotConfig* config = (IotConfig* )arg;
	if(config->discoverType) {
		free(config->discoverType);
		config->discoverType = NULL;
	}
	config->discoverType = strdup(val);
}
static void mqttuser_writer(void *arg, char *val)
{
	debug("....");
	IotConfig* config = (IotConfig* )arg;
	if(config->mqttUser) {
		free(config->mqttUser);
		config->mqttUser = NULL;
	}
	config->mqttUser = strdup(val);
}

static void mqttpassword_writer(void *arg, char *val)
{
	debug("....");
	IotConfig* config = (IotConfig* )arg;
	if(config->mqttPassword) {
		free(config->mqttPassword);
		config->mqttPassword = NULL;
	}
	config->mqttPassword = strdup(val);
}
static void mqttport_writer(void *arg, char *val)
{
	debug("....");
	IotConfig* config = (IotConfig* )arg;
	if(config->mqttPort) {
		free(config->mqttPort);
		config->mqttPort = NULL;
	}
	config->mqttPort = strdup(val);
}

static void mqtthost_writer(void *arg, char *val)
{
	debug("....");
	IotConfig* config = (IotConfig* )arg;
	if(config->mqttHost) {
		free(config->mqttHost);
		config->mqttHost = NULL;
	}
	config->mqttHost = strdup(val);
}

static void productid_writer(void *arg, char *val)
{
	debug("....");
	IotConfig* config = (IotConfig* )arg;
	if(config->productId) {
		free(config->productId);
		config->productId = NULL;
	}
	config->productId = strdup(val);
}
static void showlog_writer(void *arg, char *val)
{
	debug("....");
	IotConfig* config = (IotConfig* )arg;
	if(config->showLog) {
		free(config->showLog);
		config->showLog = NULL;
	}
	config->showLog = strdup(val);
}
static void loglevel_writer(void *arg, char *val)
{
	debug("....");
	IotConfig* config = (IotConfig* )arg;
	if(config->logLevel) {
		free(config->logLevel);
		config->logLevel = NULL;
	}
	config->logLevel = strdup(val);
}

static void vadcount_writer(void *arg, char *val)
{
	debug("....");
	IotConfig* config = (IotConfig* )arg;
	if(config->vadCount) {
		free(config->vadCount);
		config->vadCount = NULL;
	}
	config->vadCount = strdup(val);
}
static void vad_Threshold_writer(void *arg, char *val)
{
	debug("....");
	IotConfig* config = (IotConfig* )arg;
	if(config->vad_Threshold) {
		free(config->vad_Threshold);
		config->vad_Threshold = NULL;
	}
	config->vad_Threshold = strdup(val);
}




static KeyWriter configWriter[] = {
	{"appID", 	 appid_writer},
	{"apiKeyTuring",   apikeyturing_writer},
	{"aesKeyTuring",   aeskeyturing_writer},
	{"toneTuring",	 toneturing_writer},
	{"speedTuring",	 speedturing_writer},
	{"pitchTuring",	 pitchturing_writer},
	{"volumeTuring",	 volumeturing_writer},
	{"apiKeyHuabei",   apikeyhuabei_writer},
	{"aesKeyHuabei",   aeskeyhuabei_writer},
	{"aiWiFiUrl",aiwifiurl_writer},
	{"apiUrl",	 	apiurl_writer},
	{"iotHost",  	iothost_writer},
	{"discoverType", discovertype_writer},
	{"mqttUser",  	 mqttuser_writer},
	{"mqttPassword", mqttpassword_writer},
	{"mqttPort",  mqttport_writer},
	{"mqttHost",  mqtthost_writer},
	{"productId", productid_writer},
	{"showLog",  showlog_writer},
	{"logLevel",  loglevel_writer},
	{"vadCount",  vadcount_writer},
	{"vad_Threshold",  vad_Threshold_writer},
	{NULL, NULL}
};

static char *appid_reader(void *arg)
{
	debug("....");
	IotConfig* config = (IotConfig* )arg;
	char *val = NULL;
	if(config->appID) {
		val = strdup(config->appID);
	}
	return val;

}

static char *apikeyturing_reader(void *arg)
{
	debug("....");
	IotConfig* config = (IotConfig* )arg;
	char *val = NULL;
	if(config->apiKeyTuring) {
		val = strdup(config->apiKeyTuring);
	}
	return val;
}

static char *aeskeyturing_reader(void *arg)
{
	debug("....");
	IotConfig* config = (IotConfig* )arg;
	char *val = NULL;
	if(config->aesKeyTuring) {
		val = strdup(config->aesKeyTuring);
	}
	return val;
}

static char *toneturing_reader(void *arg)
{
	debug("....");
	IotConfig* config = (IotConfig* )arg;
	char *val = NULL;
	if(config->toneTuring) {
		val = strdup(config->toneTuring);
	}
	return val;
}

static char *speedturing_reader(void *arg)
{
	debug("....");
	IotConfig* config = (IotConfig* )arg;
	char *val = NULL;
	if(config->speedTuring) {
		val = strdup(config->speedTuring);
	}
	return val;
}

static char *pitchturing_reader(void *arg)
{
	debug("....");
	IotConfig* config = (IotConfig* )arg;
	char *val = NULL;
	if(config->pitchTuring) {
		val = strdup(config->pitchTuring);
	}
	return val;
}

static char *volumeturing_reader(void *arg)
{
	debug("....");
	IotConfig* config = (IotConfig* )arg;
	char *val = NULL;
	if(config->volumeTuring) {
		val = strdup(config->volumeTuring);
	}
	return val;
}

static char *apikeyhuabei_reader(void *arg)
{
	debug("....");
	IotConfig* config = (IotConfig* )arg;
	char *val = NULL;
	if(config->apiKeyHuabei) {
		val = strdup(config->apiKeyHuabei);
	}
	return val;
}
static  char *  aeskeyhuabei_reader(void *arg)
{
	IotConfig* config = (IotConfig* )arg;
	char *val = NULL;
	if(config->aesKeyHuabei) {
		val = strdup(config->aesKeyHuabei);
	}
	return val;
}

static char *aiwifiurl_reader(void *arg)
{
	
	debug("....");
	IotConfig* config = (IotConfig* )arg;
	char *val = NULL;
	if(config->aiWiFiUrl) {
		val = strdup(config->aiWiFiUrl);
	}
	return val;
}


static char *apiurl_reader(void *arg)
{

	debug("....");
	IotConfig* config = (IotConfig* )arg;
	char *val = NULL;
	if(config->apiUrl) {
		val = strdup(config->apiUrl);
	}
	return val;
}

static char *iothost_reader(void *arg)
{	
	debug("....");
	IotConfig* config = (IotConfig* )arg;
	char *val = NULL;
	if(config->iotHost) {
		val = strdup(config->iotHost);
	}
	return val;
}
static char *discovertype_reader(void *arg)
{	
	debug("....");
	IotConfig* config = (IotConfig* )arg;
	char *val = NULL;
	if(config->discoverType) {
		val = strdup(config->discoverType);
	}
	return val;
}
static char *mqttuser_reader(void *arg)
{	
	debug("....");
	IotConfig* config = (IotConfig* )arg;
	char *val = NULL;
	if(config->mqttUser) {
		val = strdup(config->mqttUser);
	}
	return val;
}
static char *mqttpassword_reader(void *arg)
{	
	debug("....");
	IotConfig* config = (IotConfig* )arg;
	char *val = NULL;
	if(config->mqttPassword) {
		val = strdup(config->mqttPassword);
	}
	return val;
}
static char *mqttport_reader(void *arg)
{	
	debug("....");
	IotConfig* config = (IotConfig* )arg;
	char *val = NULL;
	if(config->mqttPort) {
		val = strdup(config->mqttPort);
	}
	return val;
}
static char     *mqtthost_reader(void *arg)
{	
	debug("....");
	IotConfig* config = (IotConfig* )arg;
	char *val = NULL;
	if(config->mqttHost) {
		val = strdup(config->mqttHost);
	}
	return val;
}

static char *productid_reader(void *arg)
{	
	debug("....");
	IotConfig* config = (IotConfig* )arg;
	char *val = NULL;
	if(config->productId) {
		val = strdup(config->productId);
	}
	return val;
}
static char *showlog_reader(void *arg)
{	
	debug("....");
	IotConfig* config = (IotConfig* )arg;
	char *val = NULL;
	if(config->showLog) {
		val = strdup(config->showLog);
	}
	return val;
}
static char *loglevel_reader(void *arg)
{	
	debug("....");
	IotConfig* config = (IotConfig* )arg;
	char *val = NULL;
	if(config->logLevel) {
		val = strdup(config->logLevel);
	}
	return val;
}

static char *vadcount_reader(void *arg)
{	
	debug("....");
	IotConfig* config = (IotConfig* )arg;
	char *val = NULL;
	if(config->vadCount) {
		val = strdup(config->vadCount);
	}
	return val;
}

static char *vad_Threshold_reader(void *arg)
{
	debug("....");
	IotConfig* config = (IotConfig* )arg;
	char *val = NULL;
	if(config->vad_Threshold) {
		val = strdup(config->vad_Threshold);
	}
	return val;
}


static KeyReader configReader[] = {
	{"appID", 	 appid_reader},
	{"apiKeyTuring",   apikeyturing_reader},
	{"aesKeyTuring",	 aeskeyturing_reader},
	{"toneTuring",	 toneturing_reader},
	{"speedTuring",	 speedturing_reader},
	{"pitchTuring",	 pitchturing_reader},
	{"volumeTuring", volumeturing_reader},
	{"apiKeyHuabei",   apikeyhuabei_reader},
	{"aesKeyHuabei",	 aeskeyturing_reader},
	{"aiWiFiUrl",aiwifiurl_reader},
	{"apiUrl",	 apiurl_reader},
	{"iotHost",  iothost_reader},
	{"discoverType",  discovertype_reader},
	{"mqttUser",  	  mqttuser_reader},
	{"mqttPassword",  mqttpassword_reader},
	{"mqttPort",  mqttport_reader},
	{"mqttHost",  mqtthost_reader},
	{"productId", productid_reader},
	{"showLog",  showlog_reader},
	{"logLevel",  loglevel_reader},
	{"vadCount",  vadcount_reader},
	{"vad_Threshold",  vad_Threshold_reader},	
	{NULL, NULL}
};


static void ExecWriterProc(void *arg, char *key,char *val)
{
	int i = 0;
	int len = ARRAY_SIZE(configWriter) ;

	for(i = 0; i < len; i++) 
	{
		debug("configKey[%d].key:%s", i, configWriter[i].key);
		if(configWriter[i].key) {
			if(strncmp(configWriter[i].key, key, strlen(key)) == 0) {
				if(configWriter[i].exec) {
					configWriter[i].exec(arg, val);
					break;
				}
			}
		}
		
	}

}

static char *ExecReaderProc(void *arg, char *key)
{
	int i = 0;
	int len = ARRAY_SIZE(configReader) ;
	char *val = NULL;
	for(i = 0; i < len; i++) 
	{
		debug("configKey[%d].key:%s", i, configReader[i].key);
		if(configReader[i].key) {
			if(strncmp(configReader[i].key, key, strlen(key)) == 0) {
				if(configReader[i].exec) {
					val = configReader[i].exec(arg);
					break;
				}
			}
		}
		
	}
	return val;
}


#define CONFG_KEY_GEN(n, s) { "CONFG_KEY_" #n, s },

int ConfigParserReader(IotConfig* config)
{
		int ret = 0, readLen = 0;
		FILE* fp = NULL;
		char *data = NULL;
		struct stat filestat;
		json_object *object = NULL, *root = NULL;
		char *filename = IOT_CONFIG_FILE; 
		
		if((access(filename,F_OK))) {
			error("%s is not exsit", filename);
            system("cp /rom/etc/config/iot.json /etc/config/iot.json");
		}
	
		stat(filename, &filestat);
		if(filestat.st_size <= 0) {
            system("cp /rom/etc/config/iot.json /etc/config/iot.json");
		}

		fp = fopen(filename, "r");
		if(fp == NULL) {
			error("%s fopen error", filename);
			ret = -1;
			goto exit;
		}
		data = (char *)calloc(1,(filestat.st_size + 1) * sizeof(char)); //yes
		if(data == NULL) {
			error("calloc error");
			ret = -1;
			goto exit;
		}
		readLen = fread(data, sizeof(char), filestat.st_size, fp);
		if(readLen !=  filestat.st_size)
		{	
			error("%s fread error", filename);
			ret = -1;
			goto exit;
		}
		debug("data:%s", data);
		root = json_tokener_parse(data);
		if (is_error(root)) 
		{
			ret = -1;
			goto exit;
		}
		
		object = json_object_object_get(root, "appID");
		if(object) {
			config->appID = strdup(json_object_get_string(object));
			//info("config->appID:%s", config->appID);
		}
		
		object = json_object_object_get(root, "apiKeyTuring");
		if(object) {
			config->apiKeyTuring = strdup(json_object_get_string(object));
			//info("config->apiKey:%s", config->apiKey);
		}	
		
		object = json_object_object_get(root, "aesKeyTuring");
		if(object) {
			config->aesKeyTuring = strdup(json_object_get_string(object));
			//info("config->aesKey:%s", config->aesKey);
		}
		
		object = json_object_object_get(root, "toneTuring");
		if(object) {
			config->toneTuring = strdup(json_object_get_string(object));
			//info("config->aesKey:%s", config->aesKey);
		}
	
		object = json_object_object_get(root, "speedTuring");
		if(object) {
			config->speedTuring = strdup(json_object_get_string(object));
			//info("config->aesKey:%s", config->aesKey);
		}
	
		object = json_object_object_get(root, "pitchTuring");
		if(object) {
			config->pitchTuring = strdup(json_object_get_string(object));
			//info("config->aesKey:%s", config->aesKey);
		}
	
		object = json_object_object_get(root, "volumeTuring");
		if(object) {
			config->volumeTuring = strdup(json_object_get_string(object));
			//info("config->aesKey:%s", config->aesKey);
		}

		object = json_object_object_get(root, "apiKeyHuabei");
		if(object) {
			config->apiKeyHuabei = strdup(json_object_get_string(object));
			//info("config->apiKey:%s", config->apiKey);
		}	
		
		object = json_object_object_get(root, "aesKeyHuabei");
		if(object) {
			config->aesKeyHuabei = strdup(json_object_get_string(object));
			//info("config->aesKey:%s", config->aesKey);
		}
		
		object = json_object_object_get(root, "aiWiFiUrl");
		if(object) {
			config->aiWiFiUrl = strdup(json_object_get_string(object));
			//info("config->aiWiFiUrl:%s", config->aiWiFiUrl);
		}
		
		object = json_object_object_get(root, "apiUrl");
		if(object) {
			config->apiUrl = strdup(json_object_get_string(object));
			//info("config->apiUrl:%s", config->apiUrl);
		}

		object = json_object_object_get(root, "iotHost");
		if(object) {
			config->iotHost = strdup(json_object_get_string(object));
			//info("config->iotHost:%s", config->iotHost);
		}
		object = json_object_object_get(root, "discoverType");
		if(object) {
			config->discoverType = strdup(json_object_get_string(object));
			//info("config->discoverType:%s", config->discoverType);
		}
		object = json_object_object_get(root, "mqttUser");
		if(object) {
			config->mqttUser = strdup(json_object_get_string(object));
			//info("config->discoverType:%s", config->discoverType);
		}
		object = json_object_object_get(root, "mqttPassword");
		if(object) {
			config->mqttPassword = strdup(json_object_get_string(object));
			//info("config->discoverType:%s", config->discoverType);
		}
		object = json_object_object_get(root, "mqttPort");
		if(object) {
			config->mqttPort = strdup(json_object_get_string(object));
			//info("config->discoverType:%s", config->discoverType);
		}
		object = json_object_object_get(root, "mqttHost");
		if(object) {
			config->mqttHost = strdup(json_object_get_string(object));
			//info("config->discoverType:%s", config->discoverType);
		}

		object = json_object_object_get(root, "productId");
		if(object) {
			config->productId = strdup(json_object_get_string(object));
			//info("config->discoverType:%s", config->discoverType);
		}
		object = json_object_object_get(root, "showLog");
		if(object) {
			config->showLog = strdup(json_object_get_string(object));
			//info("config->discoverType:%s", config->discoverType);
		}
		object = json_object_object_get(root, "logLevel");
		if(object) {
			config->logLevel = strdup(json_object_get_string(object));
			//info("config->discoverType:%s", config->discoverType);
		}
		object = json_object_object_get(root, "vadCount");
		if(object) {
			config->vadCount = strdup(json_object_get_string(object));
			//info("config->discoverType:%s", config->discoverType);
		}
        object = json_object_object_get(root, "vad_Threshold");
        if(object) {
            config->vad_Threshold = strdup(json_object_get_string(object));
            //info("config->discoverType:%s", config->discoverType);
        }
	exit:
		if(data) {
			free(data);
			data=NULL;
		}
		if(root) {
			json_object_put(root);
			root = NULL;
		}
		if(fp) {
			fclose(fp);
			fp = NULL;
		}
		return ret;

}
int ConfigParserWriter(IotConfig* config)
{
		int ret = 0, writeLen = 0;
		FILE* fp = NULL;
		json_object *object = NULL;
		char *data = NULL;
		size_t len = 0; 
		char *filename = IOT_CONFIG_FILE; 
		assert(config != NULL);

		object = json_object_new_object();
		if(object == NULL) {
			ret = -1;
			goto exit;
		}
		if(config->appID) {
			json_object_object_add(object,"appID", json_object_new_string(config->appID));
		} else { 
			json_object_object_add(object,"appID", NULL);
		}
		
		if(config->apiKeyTuring) {
			json_object_object_add(object,"apiKeyTuring", json_object_new_string(config->apiKeyTuring));
		} else { 
			json_object_object_add(object,"apiKeyTuring", NULL);
		}

		if(config->aesKeyTuring) {
			json_object_object_add(object,"aesKeyTuring", json_object_new_string(config->aesKeyTuring));
		} else { 
			json_object_object_add(object,"aesKeyTuring", NULL);
		}
		if(config->toneTuring) {
			json_object_object_add(object,"toneTuring", json_object_new_string(config->toneTuring));
		} else { 
			json_object_object_add(object,"toneTuring", NULL);
		}
		if(config->speedTuring) {
			json_object_object_add(object,"speedTuring", json_object_new_string(config->speedTuring));
		} else { 
			json_object_object_add(object,"speedTuring", NULL);
		}
		if(config->pitchTuring) {
			json_object_object_add(object,"pitchTuring", json_object_new_string(config->pitchTuring));
		} else { 
			json_object_object_add(object,"pitchTuring", NULL);
		}
		if(config->volumeTuring) {
			json_object_object_add(object,"volumeTuring", json_object_new_string(config->volumeTuring));
		} else { 
			json_object_object_add(object,"volumeTuring", NULL);
		}
		if(config->apiKeyHuabei) {
			json_object_object_add(object,"apiKeyHuabei", json_object_new_string(config->apiKeyHuabei));
		} else { 
			json_object_object_add(object,"apiKeyHuabei", NULL);
		}

		if(config->aesKeyHuabei) {
			json_object_object_add(object,"aesKeyHuabei", json_object_new_string(config->aesKeyHuabei));
		} else { 
			json_object_object_add(object,"aesKeyHuabei", NULL);
		}

		if(config->aiWiFiUrl) {
			json_object_object_add(object,"aiWiFiUrl", json_object_new_string(config->aiWiFiUrl));
		} else { 
			json_object_object_add(object,"aiWiFiUrl", NULL);
		}
		
		if(config->apiUrl) {
			json_object_object_add(object,"apiUrl", json_object_new_string(config->apiUrl));
		} else { 
			json_object_object_add(object,"apiUrl", NULL);
		}
		if(config->iotHost) {
			json_object_object_add(object,"iotHost", json_object_new_string(config->iotHost));
		} else { 
			json_object_object_add(object,"iotHost", NULL);
		}
		if(config->discoverType) {
			json_object_object_add(object,"discoverType", json_object_new_string(config->discoverType));
		} else { 
			json_object_object_add(object,"discoverType", NULL);
		}
		if(config->mqttUser) {
			json_object_object_add(object,"mqttUser", json_object_new_string(config->mqttUser));
		} else { 
			json_object_object_add(object,"mqttUser", NULL);
		}
		if(config->mqttPassword) {
			json_object_object_add(object,"mqttPassword", json_object_new_string(config->mqttPassword));
		} else { 
			json_object_object_add(object,"mqttPassword", NULL);
		}
		if(config->mqttPort) {
			json_object_object_add(object,"mqttPort", json_object_new_string(config->mqttPort));
		} else { 
			json_object_object_add(object,"mqttPort", NULL);
		}
		if(config->mqttHost) {
			json_object_object_add(object,"mqttHost", json_object_new_string(config->mqttHost));
		} else { 
			json_object_object_add(object,"mqttHost", NULL);
		}
		if(config->productId) {
			json_object_object_add(object,"productId", json_object_new_string(config->productId));
		} else { 
			json_object_object_add(object,"productId", NULL);
		}
		if(config->showLog) {
			json_object_object_add(object,"showLog", json_object_new_string(config->showLog));
		} else { 
			json_object_object_add(object,"showLog", NULL);
		}
		if(config->logLevel) {
			json_object_object_add(object,"logLevel", json_object_new_string(config->logLevel));
		} else { 
			json_object_object_add(object,"logLevel", NULL);
		}
		if(config->vadCount) {
				json_object_object_add(object,"vadCount", json_object_new_string(config->vadCount));
		} else { 
				json_object_object_add(object,"vadCount", NULL);
		}
		if(config->vad_Threshold) {
				json_object_object_add(object,"vad_Threshold", json_object_new_string(config->vad_Threshold));
		} else { 
				json_object_object_add(object,"vad_Threshold", NULL);
		}

        

		data = json_object_to_json_string(object);
		len = strlen(data);


		fp = fopen(filename, "w+");
		if(fp == NULL) {
			error("%s fopen error", filename);
			ret = -1;
			goto exit;
		}
		
		writeLen = fwrite(data, sizeof(char), len, fp);
		if(writeLen !=  len) {	
			ret = -1;
			error("%s fwrite error", filename);
			goto exit;
		}

exit:
		if(object) {
			json_object_put(object);
			object = NULL;
		}
		if(fp) {
			fclose(fp);
			fp = NULL;
		}
		return ret;

}


int ConfigParserSetValue(char *key ,char *value)
{
	int ret = 0;
    if(conf == NULL)
        ConfigParserGetValuePre();

	ExecWriterProc(conf, key, value);
	
	ret = ConfigParserWriter(conf);
	info("ret:%d", ret);
	return ret;
}



int  ConfigParserGetValuePre()
{
	int ret = 0;
	conf = calloc(1, sizeof(IotConfig));
	if(!conf) {
		ret = -1;
		goto exit;
	}
	ret = ConfigParserReader(conf);
	//ShowIotConfig(conf);
	
	if(ret < 0) {
		goto exit;
	}
    else
    {
        return 1;
    }
exit:   
    FreeIotConfig();
    return ret;
}


//需要释放空间
char * ConfigParserGetValue(char *key)
{
    if(conf == NULL)
        ConfigParserGetValuePre();
    return ExecReaderProc(conf, key);
}


void ShowIotConfig(IotConfig *config)
{
	if(config) {
		if(config->appID) {
			debug("appID:%s", config->appID);
		}
		if(config->apiKeyTuring) {
			debug("apiKeyTuring:%s",config->apiKeyTuring);
		}
		if(config->aesKeyTuring) {
			debug("aesKeyTuring:%s", config->aesKeyTuring);
		}
		if(config->toneTuring) {
			debug("toneTuring:%s", config->toneTuring);
		}
		if(config->speedTuring) {
			debug("speedTuring:%s", config->speedTuring);
		}
		if(config->pitchTuring) {
			debug("pitchTuring:%s", config->pitchTuring);
		}
		if(config->volumeTuring) {
			debug("volumeTuring:%s", config->volumeTuring);
		}
		if(config->apiKeyHuabei) {
			debug("apiKeyHuabei:%s",config->apiKeyHuabei);
		}
		if(config->aesKeyHuabei) {
			debug("aesKeyHuabei:%s", config->aesKeyHuabei);
		}
		if(config->aiWiFiUrl) {
			debug("aiWiFiUrl:%s", config->aiWiFiUrl);
		}
		if(config->apiUrl) {
			debug("apiUrl:%s", config->apiUrl);
		}		
		if(config->iotHost) {
			debug("iotHost:%s", config->iotHost);
		}
		if(config->discoverType) {
			debug("discoverType:%s", config->discoverType);
		}
		if(config->mqttUser) {
			debug("mqttUser:%s", config->mqttUser);
		}
		if(config->mqttPassword) {
			debug("mqttPassword:%s", config->mqttPassword);
		}
		if(config->mqttPort) {
			debug("mqttPort:%s", config->mqttPort);
		}
		if(config->mqttHost) {
			debug("mqttHost:%s", config->mqttHost);
		}
		if(config->productId) {
			debug("productId:%s", config->productId);
		}
		if(config->showLog) {
			debug("showLog:%s", config->showLog);
		}
		if(config->logLevel) {
			debug("logLevel:%s", config->logLevel);
		}
		if(config->vadCount) {
			debug("vadCount:%s", config->vadCount);
		}
									
	}
	else 
	{
		debug("config == NULL");
	}
}

void FreeIotConfig()
{
	if(NULL != conf)
    {
		if(conf->appID) {
			free(conf->appID);
			conf->appID = NULL;
		}
		if(conf->apiKeyTuring) {
			free(conf->apiKeyTuring);
			conf->apiKeyTuring = NULL;
		}
		if(conf->aesKeyTuring) {
			free(conf->aesKeyTuring);
			conf->aesKeyTuring = NULL;
		}
		if(conf->toneTuring) {
			free(conf->toneTuring);
			conf->toneTuring = NULL;
		}
		if(conf->speedTuring) {
			free(conf->speedTuring);
			conf->speedTuring = NULL;
		}
		if(conf->pitchTuring) {
			free(conf->pitchTuring);
			conf->pitchTuring = NULL;
		}
		if(conf->volumeTuring) {
			free(conf->volumeTuring);
			conf->volumeTuring = NULL;
		}
		if(conf->apiKeyHuabei) {
			free(conf->apiKeyHuabei);
			conf->apiKeyHuabei = NULL;
		}
		if(conf->aesKeyHuabei) {
			free(conf->aesKeyHuabei);
			conf->aesKeyHuabei = NULL;
		}
		if(conf->aiWiFiUrl) {
			free(conf->aiWiFiUrl);
			conf->aiWiFiUrl = NULL;
		}
		if(conf->apiUrl) {
			free(conf->apiUrl);
			conf->apiUrl = NULL;
		}		
		if(conf->iotHost) {
			free(conf->iotHost);
			conf->iotHost = NULL;
		}
		if(conf->discoverType) {
			free(conf->discoverType);
			conf->discoverType = NULL;
		}
		if(conf->mqttUser) {
			free(conf->mqttUser);
			conf->mqttUser = NULL;
		}
		if(conf->mqttPassword) {
			free(conf->mqttPassword);
			conf->mqttPassword = NULL;
		}
		if(conf->mqttPort) {
			free(conf->mqttPort);
			conf->mqttPort = NULL;
		}
		if(conf->mqttHost) {
			free(conf->mqttHost);
			conf->mqttHost = NULL;
		}
		if(conf->productId) {
			free(conf->productId);
			conf->productId = NULL;
		}
		if(conf->showLog) {
			free(conf->showLog);
			conf->showLog = NULL;
		}
		if(conf->logLevel) {
			free(conf->logLevel);
			conf->logLevel = NULL;
		}
		if(conf->vadCount) {
			free(conf->vadCount);
			conf->vadCount = NULL;
		}
		free(conf);
		conf = NULL;
	}
}


void ConfigParserFreeValue(char *value)
{
	if(value) 
		free(value);
}


