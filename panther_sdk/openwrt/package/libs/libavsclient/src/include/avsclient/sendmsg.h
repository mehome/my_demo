#ifndef __SEND_MSG_H__
#define __SEND_MSG_H__

extern int OnWriteMsgToAlexa(char* cmd);
extern int OnSendSignInInfo(char *codeVerifier, char *authorizationCode, char *redirectUri, char *clientId);
extern int OnGetAVSSigninStatus();
extern int OnSignUPAmazonAlexa();
extern int OnSetAlexaLanguage(char *pData);
extern char *OnGetAlexaLanguage();
extern int OnSendCommandToAmazonAlexa(char *cmd);

#endif


