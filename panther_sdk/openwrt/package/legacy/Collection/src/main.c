
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/time.h>
#include <time.h>
#include <signal.h>
#include <unistd.h>
#include <stdio.h>

#include <json-c/json.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <curl/curl.h>
#include <curl/easy.h> 

#include "slog.h"

#define Collection_Num   10

typedef unsigned char uchar_t;
typedef unsigned int  uint_t;
typedef unsigned long int ulong_t;
#define WIFIAUDIO_SYSTEMCOMMAND_HTTPGETBUFF_PATHNAME WIFIAUDIO_HTTPGETBUFF_PATHNAME
#define WIFIAUDIO_HTTPGETBUFF_PATHNAME "/tmp/HttpGetBuff.txt"
#define JSON_INIT_STR1 "{\"tasknum\":0,\"musicnum\":0,\"alarm\":[],\"music\":[]}"
FILE 	*fpcurl = NULL;

int my_system(const char * cmd)   
{   
	int res; 
	char buf[1024] = {0};
	FILE * fp;   

	if(cmd == NULL)   
 	{   
 		printf("my_system cmd is NULL!\n");  
		
 		return -1;  
 	} 

	if((fp = popen(cmd, "r") ) == NULL)   
	{   
 		perror("popen");  
		
		//printf("popen error: %s/n", strerror(errno)); 

		return -1;   
	}
	else  
	{  
 		while(fgets(buf, sizeof(buf), fp))   
   		{   
     		printf("%s", buf);   
    	} 
 
		if((res = pclose(fp)) == -1)   
  		{   
    		printf("close popen file pointer fp error!\n"); 

			return res;  
  		}   
		else if (res == 0)   
  		{  
    		return res;  
  		}   
		else   
  		{   
  			printf("popen res is :%d\n", res);

			return res;   
  		}   
	}
	
	return 0;
	
}  

char *Parse_JsonFile_to_String(char *jsonfile, int autocreate)
{
	FILE *fp = NULL;
	char *pjsonstring = NULL;
	int ret = 0, fail = 1;

	struct stat filestat;
	
	fp = fopen(jsonfile, "r");
	if(fp == NULL)
	{
		if(autocreate)
		{
			inf("First create file!\n");
			pjsonstring = (char *)calloc(1, strlen(JSON_INIT_STR1) + 1); 
			strcpy(pjsonstring, JSON_INIT_STR1);
		}
		else
		{
			inf("fopen() failed!\n");
			goto OPEN_ERROR;
		}
	}
	else
	{
		stat(jsonfile, &filestat);
		
		pjsonstring = (char *)calloc(1, filestat.st_size + 1); 
		if(NULL == pjsonstring)
		{
			inf("calloc() failed!\n");
		
			goto CALLOC_ERROR;
		}
		
		ret = fread(pjsonstring, 1, filestat.st_size, fp);
		if(ret != filestat.st_size)
		{
			inf("fread() failed!\n");

		   	goto READ_ERROR;
		}
			
	}

	fail = 0;

	return pjsonstring;

READ_ERROR:
	free(pjsonstring);	

CALLOC_ERROR:
	fclose(fp);
	
OPEN_ERROR:
	if(fail)return NULL;

}

size_t write_data(void *ptr, size_t size, size_t nmemb, void *stream)
{
	int written = fwrite(ptr, size, nmemb, (FILE *)fpcurl);
	return written;
}


char * Alarm_TimingMusic_HttpGetUrl(char *purl)
{
	char *pstring = NULL;
	CURL *curl;
	CURLcode res;
	struct stat filestat;//用来读取文件大小
	//int sem_id = 0;//信号量ID
	
	if(NULL == purl)
	{
		inf("Error of Parameter : NULL == purl in Alarm_TimingMusic_HttpGetUrl\r\n");
		pstring = NULL;
	}
	else
	{
			
		res = curl_global_init(CURL_GLOBAL_ALL);
		if(0 == res)
		{
				
			curl = curl_easy_init();
			if (NULL != curl)
			{	
				curl_easy_setopt(curl, CURLOPT_URL, purl);
				
				//sem_id = WIFIAudio_Semaphore_CreateOrGetSem(\
				//(key_t)WIFIAUDIO_SYSTEMCOMMAND_HTTPGETBUFF_SEMKEY, 1);
				//WIFIAudio_Semaphore_Operation_P(sem_id);
				if(NULL == (fpcurl = fopen(\
				WIFIAUDIO_SYSTEMCOMMAND_HTTPGETBUFF_PATHNAME, "w+")))//文件内容会覆盖，不存在则创建,可读
				{	
					curl_easy_cleanup(curl);
					curl_global_cleanup();
					pstring = NULL;
				}
				else
				{	
					//将后续的动作交给write_data回调函数处理，libcurl接收到数据后将数据保存
					curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
					res = curl_easy_perform(curl);
					curl_easy_cleanup(curl);
					fclose(fpcurl);
					fpcurl = NULL;
					if(0 == res)
					{
						stat(WIFIAUDIO_SYSTEMCOMMAND_HTTPGETBUFF_PATHNAME, &filestat);
						pstring = (char *)calloc(1, filestat.st_size * sizeof(char));
						if(NULL != pstring)
						{
							fpcurl = fopen(WIFIAUDIO_SYSTEMCOMMAND_HTTPGETBUFF_PATHNAME, "r");
							if(NULL != fpcurl)
							{
								fread(pstring, sizeof(char), filestat.st_size, fpcurl);
								fclose(fpcurl);
								fpcurl = NULL;
							}
						}
					}					
				}
				//WIFIAudio_Semaphore_Operation_V(sem_id);
			}
			curl_global_cleanup();
		}
	}
			
	return pstring;
}
int parse_string_to_m3u(char *pstring,char *filename)
{
    int num = 0;
    int per_page = 0;
    int total_count = 0;
    int total_page = 0;
    int i = 0;
    char *purl = NULL;
    FILE *fp_m3u = NULL;
    struct json_object *getobject;
	struct json_object *object;
	struct json_object *tmp;
	struct json_object *uri_object = NULL;
	
	fp_m3u = fopen(filename,"w+");
    if(NULL == fp_m3u)
    {
        return -1;
    }
	
	object = json_tokener_parse(pstring);
	
	free(pstring);
	
	getobject = json_object_object_get(object,"total_count");   //获取本专辑歌曲总个数
	total_count = json_object_get_int(getobject);	
	getobject = json_object_object_get(object,"per_page");      //每页有几首歌曲
	per_page = json_object_get_int(getobject);
	getobject = json_object_object_get(object,"page");          //有几页
	total_page = json_object_get_int(getobject);
    if(total_page*per_page >= total_count)
    {
        num = total_count;
    }
    else
    {   
        num = total_page*per_page;
    }
	
	getobject = json_object_object_get(object,"album");
	getobject = json_object_object_get(getobject,"tracks");
	
	for(i = 0; i < num ;i++)
	{
	    tmp = json_object_array_get_idx(getobject,i);
		
        uri_object = json_object_object_get(tmp,"play_url_64");
        if(NULL != uri_object)
        {
            purl = json_object_get_string(uri_object);
            fputs(purl,fp_m3u);
            fputs("\n", fp_m3u);
            uri_object = NULL;
        }
        else
        {
            purl = NULL;
        }
	}
	
	fclose(fp_m3u);	
    json_object_put(object);
	
	return 0;
	
}


int CollectionList_Album_Addto_M3uFile(int musicNum, uchar_t *Album_url)
{
	uchar_t *nretfname = NULL;
	uchar_t mixedm3u_path[64] = {0};
	uchar_t tempm3u_path[64] = {0};
	uchar_t addbuf[128] = {0};
	
	uchar_t tmpbuf[64] = {0};
	uchar_t namebuf[64] = {0};
	int ret;
	
	memset(mixedm3u_path, 0, sizeof(mixedm3u_path)); //清0
	sprintf(mixedm3u_path, "/usr/script/playlists/CollectionPlayListM3U0%d.m3u", musicNum);
	
	memset(tempm3u_path, 0, sizeof(tempm3u_path));
	sprintf(tempm3u_path, "/usr/script/playlists/tempm3u0%d.m3u", musicNum);

	nretfname = Alarm_TimingMusic_HttpGetUrl(Album_url);
	if(nretfname == NULL)
	{
		err("nretfname == NULL!\n");
		return -1;
	}
	
	parse_string_to_m3u(nretfname, tempm3u_path);

	free(nretfname);

	memset(addbuf, 0, sizeof(addbuf));
	sprintf(addbuf, "cat %s >> %s", tempm3u_path, mixedm3u_path);
	
	system(addbuf);

	return 0;
}
int MixedList_Singles_Addto_M3uFile(int musicNum, uchar_t *Singles_url)
{
	FILE *fp = NULL;
	uchar_t mixedm3u_path[64] = {0};

	memset(mixedm3u_path, 0, sizeof(mixedm3u_path)); //清0
	sprintf(mixedm3u_path, "/usr/script/playlists/CollectionPlayListM3U0%d.m3u", musicNum);

	fp = fopen(mixedm3u_path, "a+");
	if(fp == NULL)
	{
		err("fopen() fail!\n");
		return -1;
	}

	fputs(Singles_url, fp);
	fputs("\n", fp);
	
	fclose(fp);
	
	return 0;	
}



int MixedList_Parse_JsonFile(const int musicNum, const uchar_t *mixmusic_path)
{
	int i, ret, fail = 1, mixNum = 0, typeVal = 0;
	uchar_t *mixmusic_str = NULL,  *content_url_str = NULL, *album_url_str = NULL;
	uchar_t *album_platform_str = NULL;
	
	struct json_object *mixmusic_obj = NULL, *mixmusic_array_obj = NULL,*mixmusic_content_obj = NULL;
	struct json_object *Num_obj = NULL, *MixIndex_obj = NULL, *MixType_obj = NULL;
	struct json_object *content_obj = NULL, *content_url_obj = NULL;
	struct json_object *album_content_obj = NULL, *album_url_obj = NULL;
	struct json_object *album_platform_obj = NULL;
	inf("coll MixedList_Parse_JsonFile\n");

	///////////////////////////////混合列表Jsonfile打开-Start//////////////////////////////
	//读取字符串文件
	mixmusic_str = Parse_JsonFile_to_String(mixmusic_path, 0);
	if(mixmusic_str == NULL)
	{
		inf("json file null!\n");
		goto PARSE_ERROR;
	}
	//解析文件
	mixmusic_obj = json_tokener_parse(mixmusic_str);
	if(is_error(mixmusic_obj))
	{
		inf("json parse error\n");
		goto PARSE_ERROR;
	}
	
	mixmusic_content_obj = json_object_object_get(mixmusic_obj, "content");	//获取目标对象
		if(is_error(mixmusic_content_obj))
		{
			inf("json content null\n");
			goto JS_OBJ_ERROR;
		}
	Num_obj = json_object_object_get(mixmusic_content_obj, "num");
	mixNum = json_object_get_int(Num_obj);
	
	mixmusic_array_obj = json_object_object_get(mixmusic_content_obj, "mixedlist");	//获取目标对象
	if(is_error(mixmusic_array_obj))
	{
		inf("json array null\n");
		goto JS_OBJ_ERROR;
	}
	//////////////////////////////////////混合列表Jsonfile打开-End////////////////////////////////////////

	//////////////////////////////////////混合列表Jsonfile读取-Start/////////////////////////////////////
	//inf("mixNum:%d\n", mixNum);
	inf("coll MixedList_Parse_JsonFile\n");
	for(i = 0; i < mixNum; i++)
	{
		MixIndex_obj = json_object_array_get_idx(mixmusic_array_obj, i);
		MixType_obj = json_object_object_get(MixIndex_obj, "type");
		typeVal = json_object_get_int(MixType_obj);
		
		//inf("typeVal:%d\n", typeVal);
		
		if(typeVal == 1) //单曲
		{
			content_obj = json_object_object_get(MixIndex_obj, "content");
			
			content_url_obj = json_object_object_get(content_obj, "content_url");
			content_url_str = json_object_get_string(content_url_obj);
			if(content_url_str == NULL)
			{
				inf("single url null\n");
				goto JS_OBJ_ERROR;
			}
			
			//inf("Singles musicNum:%d,content_url:%s\n", musicNum, content_url_str);

			MixedList_Singles_Addto_M3uFile(musicNum, content_url_str);
					
		}
		else if(typeVal == 2) //专辑
		{
			album_content_obj = json_object_object_get(MixIndex_obj, "content");
			album_url_obj = json_object_object_get(album_content_obj, "content_url");
			album_url_str = json_object_get_string(album_url_obj);
			if(album_url_str == NULL)
			{
				inf("album url null\n");
				goto JS_OBJ_ERROR;
			}

					if(CollectionList_Album_Addto_M3uFile(musicNum, album_url_str) < 0)
					{
						inf("get xmlfile null\n");
						goto JS_OBJ_ERROR;
					}
		}
	}
    //////////////////////////////////////混合列表Jsonfile读取-End////////////////////////////////////////
	inf("coll MixedList_Parse_JsonFile\n");	
	fail = 0;
	
JS_OBJ_ERROR:
	json_object_put(mixmusic_obj); 	//释放对象数组内存
	
PARSE_ERROR:
	free(mixmusic_str);		//释放解析对象内存
	
OTHER_ERROR:
	if(fail)return -1;

	return 0;
	
}

void Rm_Generate_JsonM3uFile(int iIndex)
{
	uchar_t check_fpath[64] = {0};
	uchar_t rm_fpath[64] = {0};

	memset(check_fpath, 0, sizeof(check_fpath));
	sprintf(check_fpath, "/usr/script/playlists/CollectionPlayList0%d.m3u", iIndex);
	if(!access(check_fpath, 0))
	{
		memset(rm_fpath, 0, sizeof(rm_fpath));
		sprintf(rm_fpath, "rm /usr/script/playlists/CollectionPlayList0%d.*", iIndex);
		my_system(rm_fpath);
		
		memset(rm_fpath, 0, sizeof(rm_fpath));
		sprintf(rm_fpath, "rm /usr/script/playlists/CollectionPlayListInfo0%d.*", iIndex);
		my_system(rm_fpath);
	}
	
	
	
}

int Create_CollectionMusic_MixedList(char **argv)
{
	uchar_t *iUrl = NULL;	
	uchar_t iPlatform[128] = {0};
	int ret, iColumn = 0, iProgram = 0;
	int iNum = 0;

	if((argv[1] == NULL) && (argv[2] == NULL))
	{
		inf("argv null\n");
		
		return -1;
	}

	iNum = atoi(argv[1]); iUrl = argv[2];

	if(iNum < 1|| isspace(iUrl))
	{
		inf("argv usage error\n");
		return -1;
	}
	inf("coll \n");
	//因APP不管新建或者修改都是调创建,所以需要删除之前生成的Json和m3u文件
	Rm_Generate_JsonM3uFile(iNum); //Add by xudh 2017-10-31 

	memset(iPlatform, 0, sizeof(iPlatform));

	//先解析PlayMixedListJson0x.json文件后将平台、栏目ID、节目ID参数提取出
	if(CollectionPlayList_GetPlatformID_Info(2, iNum, iUrl, iPlatform, &iColumn, &iProgram) < 0)
	{
		inf("parse platformID error\n");
		return -1;
	}
        inf("coll \n");
		uchar_t mixed_json_fpath[64] = {0};
		memset(mixed_json_fpath, 0, sizeof(mixed_json_fpath));
		sprintf(mixed_json_fpath, "/usr/script/playlists/CollectionPlayListInfo0%d.json", iNum);

		//解析Json文件
		if(MixedList_Parse_JsonFile(iNum, mixed_json_fpath) < 0)
		{
			inf("parse mixedlist jsonfile fail\n");
			return -1;
		}
	
	
	return 0;
	
}

char *CollectionMusic_Listfile_to_Jsonfile(int iNum, char *i_fpath)
{
	char *mixed_json_fpath = NULL;
	char tmp_buf[128] = {0};
	
	if(i_fpath != NULL)
	{
		mixed_json_fpath = (char *)malloc(64 * sizeof(char));
		if(mixed_json_fpath == NULL)
		{
			inf("malloc() failed!\n");
			return NULL;
		}
		
		memset(mixed_json_fpath, 0, sizeof(mixed_json_fpath));
		sprintf(mixed_json_fpath, "/usr/script/playlists/CollectionPlayListInfo0%d.json", iNum);

		memset(tmp_buf, 0, sizeof(tmp_buf));
		sprintf(tmp_buf, "cat %s > %s", i_fpath, mixed_json_fpath);
		
		my_system(tmp_buf);

		return mixed_json_fpath;
	}

	return NULL;
	
}

void del_chr( char *s, char ch )
{
    char *t=s; 
    while( *s != '\0' ) 
    {
        if ( *s != ch )     
		*t++=*s;
        s++ ;  
	}
    *t='\0';
}

int CollectionPlayList_GetPlatformID_Info(int ListType, int iNum, uchar_t *filepath, uchar_t * o_platform, int *o_column, int *o_program)
{
	int i, ret, fail = 1, mixNum = 0;
	uchar_t *mixmusic_str = NULL, *retstr = NULL, jsonfile[64] = {0};

	struct json_object *mixmusic_obj = NULL, *mixmusic_array_obj = NULL, *array_index_obj = NULL,*mixmusic_content_obj = NULL;
	struct json_object *content_obj = NULL, *Num_obj = NULL;
	struct json_object *platform_obj = NULL, *column_obj = NULL, *program_obj = NULL;

	if(ListType == 1) //定时列表
	{
		/*
		retstr = AlarmMusic_TxtFile_Conver_JsonFile(iNum, filepath);
		if(retstr == NULL)
		{
			inf("string null\n");
			free(retstr); //free memroy
			return -1;
		}
		memset(jsonfile, 0, sizeof(jsonfile));
		memcpy(jsonfile, retstr, sizeof(jsonfile));
		free(retstr); //free memroy   */
	}
	else if(ListType == 2) //混合列表
	{
		retstr = CollectionMusic_Listfile_to_Jsonfile(iNum, filepath);
		if(retstr == NULL)
		{
			inf("string null\n");
			free(retstr); //free memroy
			goto OTHER_ERROR;
		}
		
		memset(jsonfile, 0, sizeof(jsonfile));
		
		memcpy(jsonfile, retstr, sizeof(jsonfile));
		
		free(retstr); //free memroy
	}
	
	//inf("filepath:%s, jsonfile:%s\n", filepath, jsonfile); 

	///////////////////////////////列表Jsonfile打开-Start//////////////////////////////
	mixmusic_str = Parse_JsonFile_to_String(jsonfile, 0);
	if(mixmusic_str == NULL)
	{
		inf("mixmusic_str is null\n");
		goto PARSE_ERROR;
	}
	del_chr(mixmusic_str,'\n');
	del_chr(mixmusic_str,'\t');
	del_chr(mixmusic_str,'\r');
	//printf("mixmusic_str=====[%s]\n",mixmusic_str);
	mixmusic_obj = json_tokener_parse(mixmusic_str);
	if(mixmusic_obj == NULL)
	{
		inf("parse mixmusic_obj fail\n");
		goto PARSE_ERROR;
	}
	mixmusic_content_obj = json_object_object_get(mixmusic_obj, "content");	//获取目标对象
		if(is_error(mixmusic_content_obj))
		{
			inf("json content null\n");
			goto JS_OBJ_ERROR;
		}
	Num_obj = json_object_object_get(mixmusic_content_obj, "num");
	if(is_error(Num_obj))
		{
			inf("json num null\n");
			goto JS_OBJ_ERROR;
		}
	mixNum = json_object_get_int(Num_obj);
		
	if(ListType == 1) //定时列表
	{
		mixmusic_array_obj = json_object_object_get(mixmusic_content_obj, "musiclist");	//获取目标对象
		if(is_error(mixmusic_array_obj))
		{
			inf("json array null\n");
			goto JS_OBJ_ERROR;
		}
		
		array_index_obj = json_object_array_get_idx(mixmusic_array_obj, 0);
		if(NULL == array_index_obj)
		{
			inf("json array index null");
			goto JS_OBJ_ERROR;
		}
		
		platform_obj = json_object_object_get(array_index_obj, "platform");
		if(NULL == platform_obj)
			o_platform = NULL;
		else
			strcpy(o_platform, json_object_get_string(platform_obj));
				
		column_obj = json_object_object_get(array_index_obj, "column");
		if(NULL == platform_obj)
			*o_column = 0;
		else
			*o_column = json_object_get_int(column_obj);
		
		program_obj = json_object_object_get(array_index_obj, "program");
		if(NULL == platform_obj)
			*o_program = 0;
		else
			*o_program = json_object_get_int(program_obj);	
		
	}
	else if(ListType == 2)//定时混合列表
	{	
		mixmusic_array_obj = json_object_object_get(mixmusic_content_obj, "mixedlist");	//获取目标对象  mixedlist
		if(is_error(mixmusic_array_obj))
		{
			inf("json array null\n");
			goto JS_OBJ_ERROR;
		}
		
		array_index_obj = json_object_array_get_idx(mixmusic_array_obj, 0);
		if(NULL == array_index_obj)
		{
			inf("json array index null");
			goto JS_OBJ_ERROR;
		}
		
		content_obj = json_object_object_get(array_index_obj, "content");
		if(NULL == content_obj)
		{
			inf("json array content null");
			goto JS_OBJ_ERROR;
		}
		
		platform_obj = json_object_object_get(content_obj, "platform");
		if(NULL == platform_obj)
			o_platform = NULL;
		else
			strcpy(o_platform, json_object_get_string(platform_obj));

		column_obj = json_object_object_get(content_obj, "column");
		if(NULL == column_obj)
			*o_column = 0;
		else
			*o_column = json_object_get_int(column_obj);	

		program_obj = json_object_object_get(content_obj, "program");
		if(NULL == program_obj)
			*o_program = 0;
		else
			*o_program = json_object_get_int(program_obj);	
		
	}
	else
	{
		inf("mixedlist type error\n");
		goto JS_OBJ_ERROR;
	}

	//inf("PlayList platform:%s, o_column:%d, o_program:%d\n", o_platform, *o_column, *o_program);
  
	fail = 0;
	
JS_OBJ_ERROR:
	json_object_put(mixmusic_obj); 	////对总对象释放
	
PARSE_ERROR:
	free(mixmusic_str);		//对json文件字符串释放
	
OTHER_ERROR:
	if(fail)return -1;	

	return 0;
}

/* 可能还少一个存放  类似 currrentplaylistjson.txt 这样存放收藏列表信息的文件，文件用于APP观看收藏列表，现在不清楚厦门需不需要       */
int main(int argc, char **argv)
{
	if((!(0 < argv[1])&&(argv[1] < Collection_Num))){
		printf("err: no collection key  value  ");
		return 0;
	}
	
	inf("coll  start \n");
	 
    printf("argc===[%d]\n",argc);	
	if(argc==2){    
		
		 inf("coll  argc==1 \n");  
	}
	else if(argc==3){
		inf("coll \n");
		if((argv[2] != NULL) && (strcmp(argv[2], "/tmp/CollectionListJson.json") == 0)){
			inf("coll \n");
			if(Create_CollectionMusic_MixedList(argv) != 0)
			{
				inf("Collection mixedmusic fail\n");
				exit(1);
			}
			
		}else{
			inf("Collectionmixedmusic argc error\n");
			exit(1);
		}
		   
	}
	inf("coll  end \n"); 
}