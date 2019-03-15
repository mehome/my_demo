#ifndef __CONFIGUARTION_H__
#define __CONFIGUARTION_H__

#define CONFIG_PATH  "/etc/config/AlexaClientConfig.json"
#define SYSTEM_CONFIG_PATH "/etc/config/SystemConfiguration.json"

extern char *ReadConfigValueString(char *pValue);
extern int ReadConfigValueInt(char *pValue);
extern int WriteConfigValueString(char *pKey, char *pValue);
extern int WriteConfigValueInt(char *pKey, int pValue);
extern char *ReadSystemConfigValueString(char *pValue);

#endif


