#ifndef __WIFIAUDIO_REPLACEJSONKEYVALUE_H__
#define __WIFIAUDIO_REPLACEJSONKEYVALUE_H__

#ifdef __cplusplus
extern "C"
{
#endif

/******************************************
 ** 函数名: setJsonFileKeyValueString
 ** 功  能: 将文件中的json字符串的key值进行修改
 ** 参  数: 文件名，要修改值的key名，可以含层次，还为包含数组（key1.key2.key），要修改的值的字符串
 ** 返回值: 成功返回0，失败返回-1
 ******************************************/ 
extern int setJsonFileKeyValueString(char *pFileName, char *pKey, char *pValueString);


/******************************************
 ** 函数名: replaceJsonKeyValueString
 ** 功  能: 将json字符串的key值进行修改
 ** 参  数: 文件名，要修改值的key名，可以含层次，还为包含数组（key1.key2.key），要修改的值的字符串
 ** 返回值: 成功返回目标字符串，失败返回NULL
 ******************************************/ 
extern char *replaceJsonKeyValueString(char *pJsonString, char *pKey, char *pValue);

#ifdef __cplusplus
}
#endif

#endif
