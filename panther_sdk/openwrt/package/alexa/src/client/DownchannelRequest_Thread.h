#ifndef __DOWNCHANNELREQUEST_THREAD_H__
#define __DOWNCHANNELREQUEST_THREAD_H__

extern bool down_channel_curl_easy_init(void);
extern void CreateDownchannelRequestPthread(void);
extern bool Queue_Put(int iVluae);
extern void CloseDownchannel(void);

#endif


