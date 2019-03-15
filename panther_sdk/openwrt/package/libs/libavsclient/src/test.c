#include <stdio.h>
#include "generateParameter.h"
#include "configuration.h"

int main(void)
{
	printf("111\n");
#if 0
	// 获取亚马逊登录所需要的参数
	LOGIN_INFO_STRUCT* loginfo;
//	loginfo = logInInfo_new();
	if (loginfo != NULL){
		printf("pCodeChallengeMethod:%s\n", loginfo->pCodeChallengeMethod);
		printf("pCodeVerifier:%s\n", loginfo->pCodeVerifier);
		printf("pCodeChallenge:%s\n", loginfo->pCodeChallenge);
		printf("pProductId:%s\n", loginfo->pProductId);
		printf("pDeviceSerialNumber:%s\n", loginfo->pDeviceSerialNumber);
		logInInfo_free(loginfo);
	}
	printf("2222\n");
	// 获取json数据
//	printf("endpoint:%s\n", ReadConfigValueString("endpoint"));
//	ReadConfigValueString("clientId");

//	ReadConfigValueInt("vad_Threshold");

	// 设置json数据
	WriteConfigValueString("clientId", "12345");
	WriteConfigValueInt("vad_Threshold", 20000);

	// 发送数据到alexa
	OnWriteMsgToAlexa("abcd");

	OnSendSignInInfo("012", "123", "456", "789");
#endif
	return 0;
}
