#ifndef __ALEXA_JASON_H__
#define __ALEXA_JASON_H__

#include <json/json.h>

#define ALEXA_JASON_PATH    "/tmp/alexa_jsonStr.json"

extern char * ReadStrFromFile(char *pFilePath);
//extern void CreateCommonJsonStr(void);
extern char * UpDateAlexaCommonJsonStr(void);
extern json_object * Create_SettingsUpdatedJsonStr(char *pKey, char *pValue);

#endif


