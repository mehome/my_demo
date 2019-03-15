#ifndef __WIFIAUDIO_UCICONFIG_H__
#define __WIFIAUDIO_UCICONFIG_H__

#ifdef __cplusplus
extern "C"
{
#endif

//#include <uci.h>

/*
#ifdef WIFIAUDIO_MONTAGE
#define WIFIAUDIO_CONFIG_PATH "/etc/conf/"
#endif

#ifdef WIFIAUDIO_MTK
#define WIFIAUDIO_CONFIG_PATH "/etc/config/"
#endif
*/

#define WIFIAUDIO_CONFIG_PATH "/etc/conf/"
extern int WIFIAudio_UciConfig_FreeValueString(char **ppstring);

extern char * WIFIAudio_UciConfig_SearchValueString(char *psearch);

extern int WIFIAudio_UciConfig_SearchAndSetValueString(char *psearch, char *pstring);

extern int WIFIAudio_UciConfig_SetValueString(char *psearch, char *pstring);

#ifdef __cplusplus
}
#endif

#endif
