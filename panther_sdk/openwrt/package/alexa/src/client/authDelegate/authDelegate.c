#include "../globalParam.h"

#define DEL_HTTPHEAD_EXPECT  "Expect:"
#define DEL_HTTPHEAD_ACCEPT  "Accept:"

AUTHDELEGATE_STRUCT *g_AuthDelegateParam = NULL;

void OnFreeAuthDelegateParam(AUTHDELEGATE_STRUCT *param) {
	if (param) {
		if (param->clientId){
			free(param->clientId);
		}
		if (param->redirectUri){
			free(param->redirectUri);
		}

		if (param->code_verifier){
			free(param->code_verifier);
		}

		if (param->authorizationCode){
			free(param->authorizationCode);
		}
		free(param);
	}
}

void OnSetAuthDelegateParam(char *codeVerifier, char *authorizationCode, char *redirectUri, char *clientId) {
	OnFreeAuthDelegateParam(g_AuthDelegateParam);
	g_AuthDelegateParam = (AUTHDELEGATE_STRUCT*)calloc(1, sizeof(AUTHDELEGATE_STRUCT));
	g_AuthDelegateParam->code_verifier = codeVerifier;
	g_AuthDelegateParam->authorizationCode = authorizationCode;
	g_AuthDelegateParam->redirectUri = redirectUri;
	g_AuthDelegateParam->clientId = clientId;
}

int OnCheckParamValid() {
	int iRet = -1;
	if (g_AuthDelegateParam && \
		g_AuthDelegateParam->authorizationCode && \
		g_AuthDelegateParam->clientId && \
		g_AuthDelegateParam->code_verifier && \
		g_AuthDelegateParam->redirectUri) {
		iRet = 0;
	}

	return iRet;
}

static size_t OnAuthDelegate_callback(void *buffer,size_t size,size_t count,void **response) {
	char * ptr = NULL;	

	ptr = (char *) malloc(count*size);  
	memset(ptr, 0, count*size);
	memcpy(ptr, buffer, strlen(buffer)); 

	*response = ptr;
	
	return count*size;
}

/*
 * byFlag:表示当前需要解析的是登陆还是定时刷新token
 * 0:刷新token
 * 1:登陆获取token
 */
static int OnProcessAuthDelegateData(char *pData, char byFlag)
{
	int iRet = -1;
	char *pTmpValue = NULL;
	struct json_object *root, *access_token, *refresh_token;

	root = json_tokener_parse(pData);

	access_token = json_object_object_get(root, "access_token");
	if(access_token == NULL) {
		DEBUG_PRINTF("json_object_object_get payload failed\n");
		return iRet;
	}

	refresh_token = json_object_object_get(root, "refresh_token");
	if(refresh_token == NULL) {
		DEBUG_PRINTF("json_object_object_get payload failed\n");
		return iRet;
	}

	if (0 == byFlag)
	{
		pTmpValue = strdup(json_object_get_string(access_token));
		WriteConfigValueString("accessToken", pTmpValue);
		free(pTmpValue);
		fprintf(stderr, LIGHT_GREEN "Refresh_AccessToken succeed !\n" NONE);
		iRet = 0;
	}
	else
	{
		pTmpValue = strdup(json_object_get_string(access_token));
		WriteConfigValueString("accessToken", pTmpValue);
		free(pTmpValue);

		pTmpValue = strdup(json_object_get_string(refresh_token));
		WriteConfigValueString("refreshToken", pTmpValue);
		free(pTmpValue);

		fprintf(stderr, LIGHT_GREEN "refresh Token succeed !\n" NONE);
		iRet = 0;
		OnWriteMsgToUartd(ALEXA_LOGIN_SUCCED);
	}

	json_object_put(refresh_token);
	json_object_put(access_token);
	json_object_put(root);

	return iRet;
}

int OnRefreshToken(void)
{
	int res = -1;
	CURLcode code;
	CURLMcode mcode;

	char *pReceive = NULL;
	char reqGrant[2560] = {0};
	char *pRefreshToken = NULL;	
	char *pClientId = NULL;

	DEBUG_PRINTF("Get AccessToken..");

	CURL *curl = curl_easy_init();
	if (curl == NULL) 
	{
		DEBUG_PRINTF("curl_easy_init() failed\n");
		return res;
	}

	curl_easy_setopt(curl, CURLOPT_URL, "https://api.amazon.com/auth/O2/token");

	struct curl_slist *httpHeaderSlist = NULL;
	httpHeaderSlist = curl_slist_append(httpHeaderSlist, DEL_HTTPHEAD_EXPECT);
	httpHeaderSlist = curl_slist_append(httpHeaderSlist, DEL_HTTPHEAD_ACCEPT);
	httpHeaderSlist = curl_slist_append(httpHeaderSlist, "Connection: close");
	httpHeaderSlist = curl_slist_append(httpHeaderSlist, "Content-Type: application/x-www-form-urlencoded;charset=UTF-8");
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, httpHeaderSlist);

	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0);

	if (DEBUG_ON == g_byDebugFlag)
	{
		curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);
	}

	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, OnAuthDelegate_callback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &pReceive);

	curl_easy_setopt(curl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_2_0);

	/* Now specify the POST data */ 
	pClientId	  = ReadConfigValueString("clientId");
	pRefreshToken = ReadConfigValueString("refreshToken");
	snprintf(reqGrant, 2560, "grant_type=refresh_token&refresh_token=%s&client_id=%s", pRefreshToken, pClientId);
	free(pRefreshToken);
	free(pClientId);

	DEBUG_PRINTF("reqGrant:%s", reqGrant);

	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, reqGrant);
	curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, strlen(reqGrant));

	curl_easy_setopt(curl, CURLOPT_TIMEOUT, 15); 

	mcode = curl_easy_perform(curl);
	DEBUG_PRINTF("curl_easy_perform = %d", mcode); 
	if(CURLE_OK == mcode)
	{
		DEBUG_PRINTF(LIGHT_RED "curl_easy_perform ok..\n" NONE);
		if (0 == OnProcessAuthDelegateData(pReceive, 0))
		{
			SetAmazonLogInStatus(SIGNIN_SUCCEED);
			res = 0;
		}
	}
	else
	{
		res = -1;
	}


	if (httpHeaderSlist)
      curl_slist_free_all(httpHeaderSlist);

	curl_easy_cleanup(curl);

	return res;
}


int OnAuthDelegate(void)
{
	int iRet = -1;
	CURLcode code;
	CURLMcode mcode;
	char *reqGrant = NULL;
	char *pReceive = NULL;

	char byRunning = 1;
	char byCount = 0;

	SetAmazonLogInStatus(SIGNINING);// 正在获取token

	CURL *curl = curl_easy_init();
	if (curl == NULL) 
	{
		DEBUG_PRINTF("curl_easy_init() failed");
		return iRet;
	}

	curl_easy_setopt(curl, CURLOPT_URL, "https://api.amazon.com/auth/O2/token");
	
	struct curl_slist *httpHeaderSlist = NULL;
	httpHeaderSlist = curl_slist_append(httpHeaderSlist, DEL_HTTPHEAD_EXPECT);
	httpHeaderSlist = curl_slist_append(httpHeaderSlist, DEL_HTTPHEAD_ACCEPT);
	httpHeaderSlist = curl_slist_append(httpHeaderSlist, "Connection: close");
	httpHeaderSlist = curl_slist_append(httpHeaderSlist, "Content-Type: application/x-www-form-urlencoded;charset=UTF-8");

	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, httpHeaderSlist);
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0);

	if (DEBUG_ON == g_byDebugFlag)
	{
		curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);
	}

	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, OnAuthDelegate_callback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &pReceive);
	curl_easy_setopt(curl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_2_0);

	reqGrant = (char *)calloc(1, 4096);
	
	/* Now specify the POST data */
	if (0 == OnCheckParamValid()) {
		snprintf(reqGrant, 4096, "grant_type=authorization_code&code=%s&redirect_uri=%s&client_id=%s&code_verifier=%s",
			g_AuthDelegateParam->authorizationCode, g_AuthDelegateParam->redirectUri, g_AuthDelegateParam->clientId, g_AuthDelegateParam->code_verifier);
	}
	else
	{
		byRunning = 0;
		fprintf(stderr, LIGHT_PURPLE "Get LogIn Info failed, exit login.\n" NONE);
		SetAmazonLogInStatus(SIGNIN_FAILED);
		goto end;
	}
	
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, reqGrant);
	curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, strlen(reqGrant));
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, 15); 

	while (1)
	{
		mcode = curl_easy_perform(curl);
		if(mcode == CURLE_OK)
		{
			DEBUG_PRINTF("curl_easy_perform ok..pReceive:%s", pReceive);
			if (0 == OnProcessAuthDelegateData(pReceive, 1))
			{
				DEBUG_PRINTF("clientId:%s", g_AuthDelegateParam->clientId);
				WriteConfigValueString("clientId", g_AuthDelegateParam->clientId);
				SetAmazonLogInStatus(SIGNIN_SUCCEED);
				iRet = 0;
				break;
			}
		}
		else
		{
			byCount++;
			DEBUG_PRINTF("curl_wasy_perform error = %s\n", curl_easy_strerror(mcode));  
		}

		if (byCount >= 3)
		{
			fprintf(stderr, LIGHT_RED "Login Amazon alexa failed.." NONE);
			SetAmazonLogInStatus(SIGNIN_FAILED);
			break;
		}
	}

end:

	if (httpHeaderSlist)
      curl_slist_free_all(httpHeaderSlist);
	
	curl_easy_cleanup(curl);

	if (reqGrant)
		free(reqGrant);

	if (pReceive)
		free(pReceive);

	return iRet;
}


