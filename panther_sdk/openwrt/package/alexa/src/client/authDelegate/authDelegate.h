#ifndef __AUTH_DELEGATE_H__
#define __AUTH_DELEGATE_H__

#define SIGNIN_FAILED  	0
#define SIGNIN_SUCCEED	1
#define SIGNINING		2


// 登陆时所需要的参数
typedef struct authDelegate{
	char *clientId;
	char *redirectUri;
	char *code_verifier;
	char *authorizationCode;
}AUTHDELEGATE_STRUCT;


extern int OnAuthDelegate(void);
extern int OnRefreshToken(void);

#endif


