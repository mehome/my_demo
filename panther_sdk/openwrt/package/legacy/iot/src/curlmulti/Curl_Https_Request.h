#ifndef __CURL_HTTPS_REQUEST_H__
#define __CURL_HTTPS_REQUEST_H__

#include "globalParam.h"


#define DOWN_CHANNEL_STREAM_INDEX       0
#define RECOGNIZER_INDEX      1


//extern void InitEasyHandler(GlobalParam *g);
extern void CreateCurlHttpsRequestPthread(void);
extern void SubmitPingPackets(void);
extern void ClearEasyHandler(CURL * curl_handle);

#endif /* __CURL_HTTPS_REQUEST_H__ */



