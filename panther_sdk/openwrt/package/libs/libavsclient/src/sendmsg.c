#include <unistd.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <sys/socket.h>  
#include <sys/un.h> 
#include <netinet/in.h>
#include <json/json.h>
#include <stdio.h>
#include <stdlib.h>
#include "configuration.h"

#define UNIX_DOMAIN "/tmp/UNIX.alexa"

//NOT FINISHED!!!
//remember to free the returned point
char *OnProcessData(const char *pJsonData, const char *key)
{
	int iRet = -1;
	json_object *parse_object, *child_object;
	parse_object= json_tokener_parse(pJsonData);
	if (is_error(parse_object)) 
	{
		printf("json_tokener_parse root failed");
		return NULL;
	}
	child_object = json_object_object_get(parse_object, key);	
	char *result = strdup(json_object_get_string(child_object));
	//NOTE: GET result but no use now!
	printf("%s : %s\n", key, result);
	if(result){
		free(result);
	}
	return result;
}

int GetSigninStatus(const char *pJsonData)
{	
	int iRet = -1;
	json_object *parse_object, *child_object;
	parse_object= json_tokener_parse(pJsonData);
	if (is_error(parse_object)) 
	{
		printf("json_tokener_parse root failed");
		return iRet;
	}
	child_object = json_object_object_get(parse_object, "amazonLogInStatus");	
	iRet = json_object_get_int(child_object);
	return iRet;
}

char *OnReadWriteMsgToAlexa(char* cmd)
{
	int iRet = -1;
	int iConnect_fd = -1;
	int flags = -1;
	char *pData = NULL;

	struct sockaddr_un srv_addr;

	srv_addr.sun_family = AF_UNIX;
	strcpy(srv_addr.sun_path, UNIX_DOMAIN);

	iConnect_fd = socket(PF_UNIX, SOCK_STREAM, 0);
	if(iConnect_fd < 0)
	{
		printf("\033[1;32;40mcannot create communication socket. \033[0m\n");
		return NULL;
	}

	flags = fcntl(iConnect_fd, F_GETFL, 0);
	fcntl(iConnect_fd, F_SETFL, flags);

	if(0 != connect(iConnect_fd, (struct sockaddr*)&srv_addr, sizeof(srv_addr)))
	{
		close(iConnect_fd);
		printf("\033[1;32;40mconnect server failed..\033[0m\n");
		return NULL;
	}
	
	iRet = send(iConnect_fd, cmd, strlen(cmd), 0);
	if (iRet != strlen(cmd))
	{
		printf("\033[1;32;40mwrite failed..\033[0m\n");
		iRet = -1;
	}
	else
	{
		if (NULL == pData)
		{
			pData = (char *)malloc(32);
			memset(pData, 0, 32);
		}
		iRet = recv(iConnect_fd, pData, 32, 0);
		if (iRet != 0)
			iRet = 0;
	}

	close(iConnect_fd);
	
	return pData;
}



int OnWriteMsgToAlexa(char* cmd)
{
	int iRet = -1;
    int iConnect_fd = -1;
	int flags = -1;
	struct sockaddr_un srv_addr;

	srv_addr.sun_family = AF_UNIX;
	strcpy(srv_addr.sun_path, UNIX_DOMAIN);

	iConnect_fd = socket(PF_UNIX, SOCK_STREAM, 0);
	if(iConnect_fd < 0)
	{
		printf("\033[1;32;40mcannot create communication socket. \033[0m\n");
		return 1;
	}

    flags = fcntl(iConnect_fd, F_GETFL, 0);
    fcntl(iConnect_fd, F_SETFL, flags | O_NONBLOCK);

	if(0 != connect(iConnect_fd, (struct sockaddr*)&srv_addr, sizeof(srv_addr)))
	{
		close(iConnect_fd);
		printf("\033[1;32;40mconnect server failed..\033[0m\n");
		return iRet;
	}

	iRet = write(iConnect_fd, cmd, strlen(cmd));
	if (iRet != strlen(cmd))
	{
		printf("\033[1;32;40mwrite failed..\033[0m\n");
		iRet = -1;
	}
	else
	{
		iRet = 0;
	}

	close(iConnect_fd);
	
	return iRet;
}


int OnSendSignInInfo(char *codeVerifier, char *authorizationCode, char *redirectUri, char *clientId)
{
	int iRet = -1;
	struct json_object *root;
	root = json_object_new_object();

	json_object_object_add(root, "command", json_object_new_string("SignIn"));
	json_object_object_add(root, "codeVerifier", json_object_new_string(codeVerifier));
	json_object_object_add(root, "authorizationCode", json_object_new_string(authorizationCode));
	json_object_object_add(root, "redirectUri", json_object_new_string(redirectUri));
	json_object_object_add(root, "clientId", json_object_new_string(clientId));

	char *pJsonData = strdup(json_object_get_string(root));
	json_object_put(root);

	iRet = OnWriteMsgToAlexa(pJsonData);

	if (pJsonData) {
		free(pJsonData);
	}

	return iRet;
}

/* 
 * 获取亚马逊登陆状态
 * 0:登陆失败或未登陆
 * 1:登陆成功
 * 2:登陆中
 */
int OnGetAVSSigninStatus() {
	int iRet = -1;
	struct json_object *root;
	root = json_object_new_object();

	json_object_object_add(root, "command", json_object_new_string("GetSigninStatus"));

	char *pJsonData = strdup(json_object_get_string(root));
	json_object_put(root);

	char *pData = OnReadWriteMsgToAlexa(pJsonData);
	if (pData != NULL)
	{
		iRet = GetSigninStatus(pData);
//		iRet = OnProcessData(pData, "amazonLogInStatus");
		free(pData);
	}

	if (pJsonData) {
		free(pJsonData);
	}

	return iRet;
}

int OnSendCommandToAmazonAlexa(char *cmd) {
	int iRet = -1;
	struct json_object *root;
	root = json_object_new_object();

	json_object_object_add(root, "command", json_object_new_string(cmd));

	char *pJsonData = strdup(json_object_get_string(root));
	json_object_put(root);

	iRet = OnWriteMsgToAlexa(pJsonData);

	if (pJsonData) {
		free(pJsonData);
	}

	return iRet;
}


// 登出Amazon Alexa
int OnSignUPAmazonAlexa() {
	int iRet = -1;
	struct json_object *root;
	root = json_object_new_object();

	json_object_object_add(root, "command", json_object_new_string("SignUp"));

	char *pJsonData = strdup(json_object_get_string(root));
	json_object_put(root);

	iRet = OnWriteMsgToAlexa(pJsonData);

	if (pJsonData) {
		free(pJsonData);
	}

	return iRet;
}

/* 
 * 设置AmzonAlexa语种
 * 目前支持的语种有：en-AU, en-CA, en-GB, en-IN, en-US, de-DE, ja-JP
 */
int OnSetAlexaLanguage(char *pData) {
	int iRet = -1;
	struct json_object *root;
	root = json_object_new_object();

	json_object_object_add(root, "command", json_object_new_string("SetLanguage"));
	json_object_object_add(root, "language", json_object_new_string(pData));

	char *pJsonData = strdup(json_object_get_string(root));
	json_object_put(root);

	WriteConfigValueString("locale", pData);

	iRet = OnWriteMsgToAlexa(pJsonData);

	if (pJsonData) {
		free(pJsonData);
	}

	return iRet;
}

char *OnGetAlexaLanguage() {
	return ReadConfigValueString("locale");
}



