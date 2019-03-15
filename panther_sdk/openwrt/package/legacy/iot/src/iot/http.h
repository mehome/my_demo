#ifndef __HTTP_H__
#define __HTTP_H__

#include <json/json.h>


int HttpPost(char *host , char* path, char* body, void *data);

#ifndef ENABLE_HUABEI
int UploadFile(char *url, char * file, void *data);
#else
int UploadFile(char *url, char *path, char *apikey, char *body, int type, void *data);
#endif
int    HttpDownLoadFile(char *url ,char *fileName);
int HttpGet(char* url);
double GetDownloadFileLenth(const char *url);





#endif

