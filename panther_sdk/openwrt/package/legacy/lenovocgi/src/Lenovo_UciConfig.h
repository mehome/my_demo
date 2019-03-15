#ifndef __LENOVO_UCICONFIG_H__
#define __LENOVO_UCICONFIG_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include <uci.h>

extern int Lenovo_UciConfig_FreeValueString(char **ppstring);

extern char * Lenovo_UciConfig_SearchValueString(char *psearch);

extern int Lenovo_UciConfig_SearchAndSetValueString(char *psearch, char *pstring);

extern int Lenovo_UciConfig_SetValueString(char *psearch, char *pstring);


#ifdef __cplusplus
}
#endif

#endif
