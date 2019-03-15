#include <stdio.h>
#include <unistd.h>
#include <json/json.h>
#include "generateParameter.h"
#include "configuration.h"
#include <string.h>
#include <stdlib.h>
#include <stddef.h>
#include <sys/file.h>


char *ReadSystemConfigValueString(char *pValue) {
	char *pvalue = NULL;
	struct json_object *root = NULL, *value = NULL;

	char *pJsonData = getConfigJsonData(SYSTEM_CONFIG_PATH);
	if (pJsonData != NULL) {
		if (strstr(pJsonData, pValue)) {
			root = json_tokener_parse(pJsonData);
			if (is_error(root)) 
			{
				printf("json_tokener_parse root failed");
				return NULL;
			}
			value = json_object_object_get(root, pValue);
			if (NULL != value) {
				pvalue = strdup(json_object_get_string(value));
				//printf("[ReadConfigValue] pvalue is %s\n", pvalue);
			}
			json_object_put(root);
		} else {
//			printf("%s, %d, No such data...", __FILE__, __LINE__);
			return NULL;
		}
	}

	if (pJsonData) {
		free(pJsonData);
	}
	return pvalue;
}

/*
 * 读取json配置文件中的值
 */
char *ReadConfigValueString(char *pValue) {
	char *pvalue = NULL;
	struct json_object *root = NULL, *value = NULL;

	char *pJsonData = getConfigJsonData(CONFIG_PATH);
	if (pJsonData != NULL) {
		if (strstr(pJsonData, pValue)) {
			root = json_tokener_parse(pJsonData);
			if (is_error(root)) 
			{
				printf("json_tokener_parse root failed");
				return NULL;
			}
			value = json_object_object_get(root, pValue);
			if (NULL != value) {
				pvalue = strdup(json_object_get_string(value));
				//printf("[ReadConfigValue] pvalue is %s\n", pvalue);
			}
			json_object_put(root);
		} else {
//			printf("%s, %d, No such data...", __FILE__, __LINE__);
		}
		
		if (pJsonData) {
			free(pJsonData);
		}
	}

	return pvalue;
}

int ReadConfigValueInt(char *pValue) {
	int iRet = -1;
	struct json_object *root = NULL, *value = NULL;

	char *pJsonData = getConfigJsonData(CONFIG_PATH);
	if (pJsonData != NULL) {
		if (strstr(pJsonData, pValue)) {
			root = json_tokener_parse(pJsonData);
			if (is_error(root))
			{
				printf("json_tokener_parse root failed");
				return -1;
			}
			value = json_object_object_get(root, pValue);
			if (NULL != value) {
				iRet = json_object_get_int(value);
				//printf("[ReadConfigValue] pvalue is %d\n", iRet);
			}
			json_object_put(root);
		} else {
//			printf("%s, %d, No such data...", __FILE__, __LINE__);
		}
		
		if (pJsonData) {
			free(pJsonData);
		}
	}

	return iRet;

}


/*
 * 修改json配置文件中的值
 */
int WriteConfigValueString(char *pKey, char *pValue) 
{
	int iRet = -1;
	char *readbuf = NULL;
	unsigned int lock_file_times = 0;
	struct json_object *root = NULL, *Key = NULL, *value = NULL;

	char *pJsonData = getConfigJsonData(CONFIG_PATH);
	if (pJsonData != NULL && (strlen(pJsonData) > 0)) {
		root = json_tokener_parse(pJsonData);
		if (is_error(root))
		{
			printf("json_tokener_parse root failed");
			return -1;
		}
		//WARNING: if pJsonDate is not json format data, this function will lead to a segementation fault
		json_object_object_add(root, pKey, json_object_new_string(pValue));
		char *pData = strdup(json_object_get_string(root));
		FILE *fp = NULL;
		if((fp = fopen(CONFIG_PATH, "w+")) == NULL){
			printf("file %s open error!\n", CONFIG_PATH);
			return -1;
		}
		while(flock(fileno(fp), LOCK_EX | LOCK_NB) < 0){
			if(++lock_file_times < LOCK_FILE_TRY_TIMES){
				usleep(10*1000);
			}else{
				return -1;
			}
		}
		
		int iLength = strlen(pData);
		if (fwrite(pData, 1, iLength, fp) != iLength) {
			printf("文件写入错误。!\n");
		} else {
			iRet = 0;
		}
		
		flock(fileno(fp), LOCK_UN);
		fclose(fp);
		
		if(pData){
			free(pData);
		}

	}

	if (pJsonData) {
		free(pJsonData);
	}

	json_object_put(root);

	return iRet;
}

int WriteConfigValueInt(char *pKey, int pValue) {
	int iRet = -1;
	char *readbuf = NULL;
	unsigned int lock_file_times = 0;
	struct json_object *root = NULL, *Key = NULL, *value = NULL;

	char *pJsonData = getConfigJsonData(CONFIG_PATH);
	if (pJsonData != NULL && (strlen(pJsonData) > 0)) {
		root = json_tokener_parse(pJsonData);
		if (is_error(root))
		{
			printf("json_tokener_parse root failed");
			return -1;
		}
		json_object_object_add(root, pKey, json_object_new_int(pValue));
		char *pData = strdup(json_object_get_string(root));
		FILE *fp = NULL;
   		if((fp = fopen(CONFIG_PATH, "w+")) == NULL){
   			printf("file %s open error!\n", CONFIG_PATH);
			return -1;
   		}
		
		while(flock(fileno(fp), LOCK_EX | LOCK_NB) < 0){
			printf("%s, %d, flock failed %d\n", __func__, __LINE__, lock_file_times);
			if(++lock_file_times < LOCK_FILE_TRY_TIMES){
				usleep(10*1000);
			}else{
				return -1;
			}
		}
		int iLength = strlen(pData);
		if (fwrite(pData, 1, strlen(pData), fp) != strlen(pData)) {
	    	printf("文件写入错误。!\n");
		}else {
			iRet = 0;
		}
		if(pData){
			free(pData);
		}
		flock(fileno(fp), LOCK_UN);
	    fclose(fp);
	}

	if (pJsonData) {
		free(pJsonData);
	}

	json_object_put(root);

	return iRet;
}


