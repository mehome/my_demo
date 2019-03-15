#include <stdio.h>
//#include <json/json.h>
#include <json-c/json.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include "playlist_info_json.h"
#include "uciConfig.h"
#include "playlist_set_to_cdb.h"
#include <wdk/cdb.h>
#include <mon.h>

#define CONFIG  1
#define COLL 1
#define ALEXA_ORI 0
#define ALEXA_CHANGE 1
#define INSERT  1



struct json_object* WIFIAudio_RealTimeInterface_pMusicTopJsonObject(WARTI_pMusic p)
{
	char *ret;
	struct json_object *object;


	if(p==NULL)
	{
		PRINTF("json_object* WIFIAudio_RealTimeInterface_pMusicTopJsonObject error\n");
		return NULL;
	}
	object = json_object_new_object();

	if(NULL == p->pArtist)
	{
		json_object_object_add(object,"artist", NULL);
	}
	else
	{
		json_object_object_add(object,"artist", json_object_new_string(p->pArtist));
	}

	if(NULL == p->pTitle)
	{
		json_object_object_add(object,"title", NULL);
	}
	else
	{
		json_object_object_add(object,"title", json_object_new_string(p->pTitle));
	}
	
	if(NULL == p->pAlbum)
	{
		json_object_object_add(object,"album", NULL);
	}
	else
	{
		json_object_object_add(object,"album", json_object_new_string(p->pAlbum));
	}
	
	
	if(NULL == p->pContentURL)
	{
		json_object_object_add(object,"content_url", NULL);
	}
	else
	{
		json_object_object_add(object,"content_url", json_object_new_string(p->pContentURL));
	}
	
	if(NULL == p->pCoverURL)
	{
		json_object_object_add(object,"cover_url", NULL);
	}
	else
	{
		json_object_object_add(object,"cover_url", json_object_new_string(p->pCoverURL));
	}
	
	if(NULL == p->pToken)
	{
		json_object_object_add(object,"token", NULL);
	}
	else
	{
		json_object_object_add(object,"token", json_object_new_string(p->pToken));
	}
  
		 /*
	if(NULL == p->pCharFlag)
	{
		json_object_object_add(object,"pCharFlag", NULL);
	}
	else
	{
		json_object_object_add(object,"pCharFlag", json_object_new_string(p->pCharFlag));
	}

	*/
	return object;
}


struct json_object* WIFIAudio_RealTimeInterface_pMusicListInfoTopJsonObject(WARTI_pMusicList inlp,WARTI_pMusicList outlp)
{
	char *ret;
	int i = 0;
	struct json_object *object;
	struct json_object *array;
	
	
	PRINTF("inlp->Num num ===%d\n",inlp->Num);
	PRINTF("outlp->Num num ===%d\n",outlp->Num);
	if(inlp==NULL)
	{
		PRINTF(" WIFIAudio_RealTimeInterface_pMusicListInfoTopJsonObject error\n");
		return NULL;
	}
	object = json_object_new_object();
	if(inlp->Index==WIFIAUDIO_REALTIMEINTERFACE_INTERRORCODE && outlp->Index==WIFIAUDIO_REALTIMEINTERFACE_INTERRORCODE )
	{
		  json_object_object_add(object,"index", NULL);
	}
  else{
	    json_object_object_add(object,"index", json_object_new_int(inlp->Index));
	}
	
	if(inlp->Num==WIFIAUDIO_REALTIMEINTERFACE_INTERRORCODE && outlp->Num==WIFIAUDIO_REALTIMEINTERFACE_INTERRORCODE )
	{
		json_object_object_add(object,"num", NULL);
	}
	
	else if((inlp->Num + outlp->Num)<30)
	{
		json_object_object_add(object,"num", json_object_new_int(inlp->Num + outlp->Num));
	}else if((inlp->Num + outlp->Num)>30 ||(inlp->Num + outlp->Num)==30)
	{
		json_object_object_add(object,"num", json_object_new_int(30));
	} 
	
	if(inlp->pMusicList==NULL && outlp->pMusicList==NULL)
	{
		json_object_object_add(object,"content", NULL);
	}else if(inlp->Num ==30){
		array = json_object_new_array();
		PRINTF("inlp->Num ====30 \r\n");
		 {  for(i = 0;i<(inlp->Num);i++)
		   {
			   json_object_array_add(array,WIFIAudio_RealTimeInterface_pMusicTopJsonObject((inlp->pMusicList)[i]));
			   PRINTF("inlp->pMusicList)[i]  %s\n",(inlp->pMusicList)[i]->pTitle);
			
		  }
	   }
	 }
	else if((inlp->Num + outlp->Num)<30)
	{
		array = json_object_new_array();
		if((inlp->Num)>0)
		 {  for(i = 0;i<(inlp->Num);i++)
		   {
			   json_object_array_add(array,WIFIAudio_RealTimeInterface_pMusicTopJsonObject((inlp->pMusicList)[i]));
			    PRINTF("555555inlp->pMusicList)[i]  %s\n",(inlp->pMusicList)[i]->pTitle);
			
		  }
	   }
	   if((outlp->Num)>0)
		 {  for(i = 0;i<(outlp->Num);i++)
		   {
			   json_object_array_add(array,WIFIAudio_RealTimeInterface_pMusicTopJsonObject((outlp->pMusicList)[i]));
			 PRINTF("555555outlp->pMusicList)[i]  %s\n",(outlp->pMusicList)[i]->pTitle);
		
		  }
	   }
	 }
	else if(((inlp->Num + outlp->Num)>30) || ((inlp->Num + outlp->Num)==30))
		{
			array = json_object_new_array();
			if((inlp->Num)>0)
		 {  for(i = 0;i<(inlp->Num);i++)
		   {
			   json_object_array_add(array,WIFIAudio_RealTimeInterface_pMusicTopJsonObject((inlp->pMusicList)[i]));
		  }
	   }
			 if((outlp->Num)>0)
		 {  for(i = 0;i<(30-(inlp->Num));i++)
		   {
			   json_object_array_add(array,WIFIAudio_RealTimeInterface_pMusicTopJsonObject((outlp->pMusicList)[i]));
			
		  }
	   }
		}
		   
		json_object_object_add(object,"musiclist",array);
	
	
	return object;//这边不是在栈中分配呢,可以直接返回，但是记得使用完需要释放
}

int my_system(const char * cmd)   
{   
FILE * fp;   
int res; char buf[1024];

if (cmd == NULL)   
 {   
 printf("my_system cmd is NULL!\n");  
 return -1;  
 } 

if ((fp = popen(cmd, "r") ) == NULL)   
{   
 perror("popen");  
 printf("popen error: %s/n", strerror(errno)); return -1;   
}else  
{  
 while(fgets(buf, sizeof(buf), fp))   
   {   
     printf("%s", buf);   
   } 
 
if ( (res = pclose(fp)) == -1)   
  {   
    printf("close popen file pointer fp error!\n"); return res;  
  }   
else if (res == 0)   
  {  
    return res;  
  }   
else   
  {   
  printf("popen res is :%d\n", res); return res;   
  }   
}  
}   

int cp_rw(int srcfd,int dstfd,char *buf,int len)
{
    int nread;
    while((nread=read(srcfd,buf,len))>0)
    {
        if(write(dstfd,buf,nread)!=nread)
    {
     error("write error");
     return -1;
    }
    }
    if(nread ==-1)
    {
       error("read error");
       return -1;
    }
    return 0;
}


int copy_file(char *dest, char *src)
{   char buf[1024];
	int srcfd,dstfd;
	int ret=-1;
	

	if((srcfd=open(dest,O_RDONLY))==-1)
		{
			error("open %s error",dest);
		}
		if((dstfd=open(src,O_RDWR|O_CREAT|O_TRUNC,0666))==-1)
		{
			error("creat %s error",src);
		}
		ret=cp_rw(srcfd,dstfd,buf,sizeof(buf));
		if(ret!=0)
			{
				error("cp_rw error");
				return -1;
			}
		
		close(srcfd);
		close(dstfd);
     return 0;
}
/****************************************/
char * get_file_name(char * long_name)
{
    int i = 0;
    char *point = NULL;
    int len = 0,total_len = 0;
    char *short_name = NULL; 
    int copy_len = 0;
    

    if(0 != strncmp(long_name,"/media",6))
    {
          return long_name;
    }
    total_len = strlen(long_name);
    short_name = (char *)malloc(total_len+1);
    if(NULL == short_name)
    {
        return long_name;
    }
    memset(short_name,0,total_len);
    for(i=0 ; i< total_len; i++)
    {
        short_name[i] = long_name[total_len-i-1];
    }
    if(NULL !=(point = strchr(short_name,'/')))
    {
        copy_len = point - short_name ;  
    }
    else
    {
        free(short_name);
        return long_name;
    }
    len = total_len - copy_len;
    memset(short_name,0,total_len);
    strncpy(short_name,long_name+len,copy_len);

    return short_name;
}

int write_playing_song_to_config(WARTI_pMusicList inlp,char *current_song)
{
	char *ret=NULL;
	int i = 0;
	struct json_object *object;
    int find_url =0;
		
	if(current_song!=NULL){
	
	PRINTF("inlp->Num num =============%d\n",inlp->Num);

	  if(inlp!=NULL)
	  {	
		object = json_object_new_object();

		if(inlp->Index==WIFIAUDIO_REALTIMEINTERFACE_INTERRORCODE)
			{
			json_object_object_add(object,"index", NULL);
			}
		else{
			json_object_object_add(object,"index", json_object_new_int(inlp->Index));
			}

		if(inlp->Num==WIFIAUDIO_REALTIMEINTERFACE_INTERRORCODE)
			{
			json_object_object_add(object,"num", NULL);
			}


		char tempbuff[1024];
		for(i=0;i<(inlp->Num);i++)
			{        
				PRINTF("(inlp->pMusicList)[%d]->pContentURL===========[%s]\n",i,(inlp->pMusicList)[i]->pContentURL);
				if(strncmp((inlp->pMusicList)[i]->pContentURL,current_song,strlen((inlp->pMusicList)[i]->pContentURL)) == 0)
				 {

					find_url =1;


					if((inlp->pMusicList)[i]->pArtist!="null"&&(inlp->pMusicList)[i]->pArtist!=NULL&&(inlp->pMusicList)[i]->pArtist!="")
						{
						memset(tempbuff, 0, sizeof(tempbuff)/sizeof(char));
						sprintf(tempbuff, "%s", (inlp->pMusicList)[i]->pArtist);
					//	ret = WIFIAudio_UciConfig_SearchAndSetValueString("xzxwifiaudio.config.current_artist", tempbuff);
					    ret =cdb_set("$current_artist",tempbuff);
						}else
						{
						//ret = WIFIAudio_UciConfig_SearchAndSetValueString("xzxwifiaudio.config.current_artist", " ");
						ret =cdb_set("$current_artist","");
						}


					if((inlp->pMusicList)[i]->pTitle!="null"&&(inlp->pMusicList)[i]->pTitle!=NULL&&(inlp->pMusicList)[i]->pTitle!="")
						{
						memset(tempbuff, 0, sizeof(tempbuff)/sizeof(char));
						sprintf(tempbuff, "%s", (inlp->pMusicList)[i]->pTitle);
						//ret = WIFIAudio_UciConfig_SearchAndSetValueString("xzxwifiaudio.config.current_title", tempbuff);
						ret =cdb_set("$current_title",tempbuff);
						}else{
						//ret = WIFIAudio_UciConfig_SearchAndSetValueString("xzxwifiaudio.config.current_title", " ");
						ret =cdb_set("$current_title","");
						}


					if((inlp->pMusicList)[i]->pAlbum!="null"&&(inlp->pMusicList)[i]->pAlbum!=NULL&&(inlp->pMusicList)[i]->pAlbum!="")
						{
						memset(tempbuff, 0, sizeof(tempbuff)/sizeof(char));
						sprintf(tempbuff, "%s", (inlp->pMusicList)[i]->pAlbum);
						//ret = WIFIAudio_UciConfig_SearchAndSetValueString("xzxwifiaudio.config.current_album", tempbuff);
						ret =cdb_set("$current_album",tempbuff);
						}
					else{   
						//ret = WIFIAudio_UciConfig_SearchAndSetValueString("xzxwifiaudio.config.current_album", " ");
						ret =cdb_set("$current_album","");
						}


					if((inlp->pMusicList)[i]->pContentURL!="null"&&(inlp->pMusicList)[i]->pContentURL!=NULL&&(inlp->pMusicList)[i]->pContentURL!="")
						{
						memset(tempbuff, 0, sizeof(tempbuff)/sizeof(char));
						sprintf(tempbuff, "%s", (inlp->pMusicList)[i]->pContentURL);
						//ret = WIFIAudio_UciConfig_SearchAndSetValueString("xzxwifiaudio.config.current_contentURL", tempbuff);
						ret =cdb_set("$current_contentURL",tempbuff);
						}else{
					  // ret = WIFIAudio_UciConfig_SearchAndSetValueString("xzxwifiaudio.config.current_contentURL", " ");
                        ret =cdb_set("$current_contentURL","");
					    }


					if((inlp->pMusicList)[i]->pCoverURL!="null"&&(inlp->pMusicList)[i]->pCoverURL!=NULL&&(inlp->pMusicList)[i]->pCoverURL!="")
						{
						memset(tempbuff, 0, sizeof(tempbuff)/sizeof(char));
						sprintf(tempbuff, "%s", (inlp->pMusicList)[i]->pCoverURL);
						//ret = WIFIAudio_UciConfig_SearchAndSetValueString("xzxwifiaudio.config.current_coverURL", tempbuff);
						ret =cdb_set("$current_coverURL",tempbuff);
						}else{
					  //ret = WIFIAudio_UciConfig_SearchAndSetValueString("xzxwifiaudio.config.current_coverURL", " ");
                        ret =cdb_set("$current_coverURL","");
						}


					if((inlp->pMusicList)[i]->pToken!="null"&&(inlp->pMusicList)[i]->pToken!=NULL&&(inlp->pMusicList)[i]->pToken!="")
						{
						memset(tempbuff, 0, sizeof(tempbuff)/sizeof(char));
						sprintf(tempbuff, "%s", (inlp->pMusicList)[i]->pToken);
						//ret = WIFIAudio_UciConfig_SearchAndSetValueString("xzxwifiaudio.config.token", tempbuff);
						ret =cdb_set("$token",tempbuff);
						}else{
					 //ret = WIFIAudio_UciConfig_SearchAndSetValueString("xzxwifiaudio.config.token", " ");
					 ret =cdb_set("$token","");
					    }

					break;

					}

				}
		 }
	   else{
	   	PRINTF(" write_playing_song_to_config error\n");
	   	}
	 if(find_url ==0){
	 	/*
			ret =WIFIAudio_UciConfig_SearchAndSetValueString("xzxwifiaudio.config.current_artist", " ");
			ret =WIFIAudio_UciConfig_SearchAndSetValueString("xzxwifiaudio.config.current_title", " ");
			ret =WIFIAudio_UciConfig_SearchAndSetValueString("xzxwifiaudio.config.current_album", " ");
			ret =WIFIAudio_UciConfig_SearchAndSetValueString("xzxwifiaudio.config.current_contentURL", current_song);
			ret =WIFIAudio_UciConfig_SearchAndSetValueString("xzxwifiaudio.config.current_coverURL", " ");
       */
            ret =cdb_set("$current_artist","");
		    ret =cdb_set("$current_title","");
			ret =cdb_set("$current_album","");
		    ret =cdb_set("$current_contentURL",current_song);
			ret =cdb_set("$current_coverURL","");
			} 
	    }
		
    
	json_object_put(object);
	return ret;
}

struct json_object* pMusicListInfoTopJsonObject(WARTI_pMusicList p)
{
	char *ret;
	int i = 0;
	struct json_object *object;
	struct json_object *array;
	
	
	printf("pppp num ===%d\n",p->Num);
	if(p==NULL)
	{
		PRINTF("pMusicListInfoTopJsonObject error\n");
		return NULL;
	}
	object = json_object_new_object();
	if(p->Index==WIFIAUDIO_REALTIMEINTERFACE_INTERRORCODE)
	{
		  json_object_object_add(object,"index", NULL);
	}
  else{
	    json_object_object_add(object,"index", json_object_new_int(p->Index));
	}
	if(p->Num==WIFIAUDIO_REALTIMEINTERFACE_INTERRORCODE)
	{
		json_object_object_add(object,"num", NULL);
	}
	else
	{
		json_object_object_add(object,"num", json_object_new_int(p->Num));
	}
	
	if(p->pMusicList==NULL)
	{
		json_object_object_add(object,"content", NULL);
	}
	else
	{
		array = json_object_new_array();
		
		for(i = 0;i<(p->Num);i++)
		{
			json_object_array_add(array,WIFIAudio_RealTimeInterface_pMusicTopJsonObject((p->pMusicList)[i]));
			
		}
		
		json_object_object_add(object,"musiclist",array);
	}
	
	return object;
}

WARTI_pMusicList getplaylist_parse_json(char *filepatch){
	
	WARTI_pMusicList p=NULL;
	struct stat filestat;
	char filename[128];
	FILE *fp = NULL;
	char *pjsonstring = NULL;
	char *pgetstring = NULL;
	
	int i = 0,ret;

   	
	fp = fopen(filepatch, "r");
		if(NULL != fp)
		{

			stat(filepatch, &filestat);
			pjsonstring = (char *)calloc(1,(filestat.st_size + 1) * sizeof(char)); //yes
			if(NULL != pjsonstring)
			{    

				ret = fread(pjsonstring, sizeof(char), filestat.st_size, fp);
				if(ret > 0)
				{   
				p = WIFIAudio_RealTimeInterface_pStringTopGetCurrentPlaylist(pjsonstring);
				}
				
			}
			fclose(fp);



			free(pjsonstring);
			pjsonstring=NULL;
		}
	return p;
	}


WARTI_pMusicList gethttp_playlist_parse_json(char *filepatch){
	
	WARTI_pMusicList p;
	struct stat filestat;
	char filename[128];
	FILE *fp = NULL;
	char *pjsonstring = NULL;
	char *pgetstring = NULL;
	PRINTF("playlist http filepatch == %s\n", filepatch);
	int i = 0,ret;
	  fp = fopen(filepatch, "r");
		if(NULL != fp)
		{
			stat(filepatch, &filestat);
			pjsonstring = (char *)calloc(1,(filestat.st_size + 1) * sizeof(char)); //yes
			if(NULL != pjsonstring)
			{
				ret = fread(pjsonstring, sizeof(char), filestat.st_size, fp);
				if(ret > 0)
				{
						PRINTF("pjsonstring == %s\n", pjsonstring);
					p = WIFIAudio_RealTimeInterface_pStringTopGetCurrentPlaylist(pjsonstring);
				
				}
				
			}
			fclose(fp);
			free(pjsonstring);
			pjsonstring=NULL;
    }
	return p;
	}

int parse_playlistinfo_add_playlist(char *infilepatch,char *outplaylist){
		struct stat filestat;
	  char filename[128];
	  char tempbuff[256];
	  FILE *fp = NULL;
	  int i = 0,ret,num;
	
	  struct json_object *object,*getnumobject,*getlistobject,*musiclistobj,*urlobject;
	  char *pjsonstring = NULL;
	   fp = fopen(infilepatch, "r");
		if(NULL != fp)
		{
			stat(infilepatch, &filestat);
			pjsonstring = (char *)calloc(1, (filestat.st_size + 1) * sizeof(char)); //yes
			if(NULL == pjsonstring )
			{		
			printf("malloc failed!\n");		
			exit(1);   
			}
		}
			if(NULL != pjsonstring)
			{
				ret = fread(pjsonstring, sizeof(char), filestat.st_size, fp);
				if(ret > 0)
				{
					
					object = json_tokener_parse(pjsonstring);

				}
			}
	
			PRINTF("fp is ==%s\n",fp);
			PRINTF("pjsonstring is ==%s\n",pjsonstring);
			getnumobject = json_object_object_get(object,"num");
	   if(getnumobject==NULL)
	   {
		  PRINTF("getnumobject is error");
	   }
	   else
	   {
		   num = json_object_get_int(getnumobject);
		   PRINTF("Num =%d\n",num);
	   }
	  getlistobject = json_object_object_get(object,"musiclist");//这边获取到的是JSONarray对象

	  if(outplaylist!=NULL)
	 { 
	 	  fp = fopen(outplaylist, "w+");
	    for(i = 0;i<num;i++)
	    {
		    musiclistobj =json_object_array_get_idx(getlistobject,i);
     	  urlobject = json_object_object_get(musiclistobj, "content_url");
     	  if(NULL == urlobject) {

					PRINTF("json_object_object_get get urlobject failed");
					goto failed;
				}
			
				char *musicurl = json_object_get_string(urlobject);
				if(NULL == musicurl) {
			
						PRINTF("json_object_get_string get musicurl failed");
						goto failed;
				}
		   PRINTF("musicurl == %s\n",musicurl);
			
		    //modify by xudh 2017-11-08
		   	if(strlen(musicurl) < 10) //解决Json文件中音乐url地址为空(ex:http://)
				continue;
			
		   fputs(musicurl, fp);
		   fputs("\n", fp);
		  
		
	   }
	    fclose(fp);
	     fp = NULL;
	  
	     memset(tempbuff, 0, sizeof(tempbuff)/sizeof(char));
			 sprintf(tempbuff, "playlist.sh load %s",outplaylist);
			 my_system(tempbuff);
	 
	}else{
		    musiclistobj =json_object_array_get_idx(getlistobject,0);
     	  urlobject = json_object_object_get(musiclistobj, "content_url");
     	  if(NULL == urlobject) {

					PRINTF("json_object_object_get get urlobject failed");
					goto failed;
				}
				
				char *musicurl = json_object_get_string(urlobject);
		 
		   memset(tempbuff, 0, sizeof(tempbuff)/sizeof(char));
			 sprintf(tempbuff, "playlist.sh add \"%s\"", musicurl);
			 my_system(tempbuff);
		}

/********************************************/
	  json_object_put(object);

  
	   
	   return 0;
	  failed:
		ret = -1;
		//parse_failed();
		return ret;			
	
}	

int add_parse_playlistinfo_add_playlist(char *infilepatch,char *outplaylist){
		struct stat filestat;
	  char filename[128];
	  char tempbuff[256];
	  FILE *fp = NULL;
	  int i = 0,ret,num;
	
	  struct json_object *object,*getnumobject,*getlistobject,*musiclistobj,*urlobject;
	  char *pjsonstring = NULL;
	   fp = fopen(infilepatch, "r");
		if(NULL != fp)
		{
			stat(infilepatch, &filestat);
			pjsonstring = (char *)calloc(1, (filestat.st_size + 1) * sizeof(char));  //yes
			if(NULL == pjsonstring )
			{		
			printf("malloc failed!\n");		
			exit(1);   
			}
		}
			if(NULL != pjsonstring)
			{
				ret = fread(pjsonstring, sizeof(char), filestat.st_size, fp);
				if(ret > 0)
				{
					
					object = json_tokener_parse(pjsonstring);
				
				}
			}
	
			PRINTF("fp is ==%s\n",fp);
			PRINTF("pjsonstring is ==%s\n",pjsonstring);
			getnumobject = json_object_object_get(object,"num");
	   if(getnumobject==NULL)
	   {
		  PRINTF("getnumobject is error");
	   }
	   else
	   {
		   num = json_object_get_int(getnumobject);
		   PRINTF("Num =%d\n",num);
	   }
	  getlistobject = json_object_object_get(object,"musiclist");
	 
	  if(outplaylist!=NULL)
	 { 
	 	  fp = fopen(outplaylist, "w+");
	    for(i = 0;i<num;i++)
	    {
		    musiclistobj =json_object_array_get_idx(getlistobject,i);
     	  urlobject = json_object_object_get(musiclistobj, "content_url");
     	  if(NULL == urlobject) {

					PRINTF("json_object_object_get get urlobject failed");
					goto failed;
				}
			
				char *musicurl = json_object_get_string(urlobject);
				if(NULL == musicurl) {
			
						PRINTF("json_object_get_string get musicurl failed");
						goto failed;
				}
		   PRINTF("musicurl == %s\n",musicurl);

		    //modify by xudh 2017-11-08
		   	if(strlen(musicurl) < 10) //解决Json文件中音乐url地址为空(ex:http://)
				continue;
			
		   fputs(musicurl, fp);
		   fputs("\n", fp);

		   memset(tempbuff, 0, sizeof(tempbuff)/sizeof(char));
		   sprintf(tempbuff, "mpc add %s",musicurl);
		   my_system(tempbuff);
		  
		
	   }
	     fclose(fp);
	     fp = NULL;
	  
	        // memset(tempbuff, 0, sizeof(tempbuff)/sizeof(char));
			// sprintf(tempbuff, "playlist.sh replace %s",outplaylist);
			// my_system(tempbuff);
			 
     
			 
	 
	}

	   json_object_put(object);
	  
	   return 0;
	  failed:
		ret = -1;
		//parse_failed();
		return ret;			
	
}	



int load_parse_playlistinfo_add_playlist(char *infilepatch,char *outplaylist){
		struct stat filestat;
	  char filename[128];
	  char tempbuff[256];
	  FILE *fp = NULL;
	  int i = 0,ret,num,index;
	 // WARTI_pMusic addMusic; 
	  struct json_object *object,*getnumobject,*getlistobject,*musiclistobj,*urlobject,*getIndexObject;
	  char *pjsonstring = NULL;
	   fp = fopen(infilepatch, "r");
		if(NULL != fp)
		{
			stat(infilepatch, &filestat);
			pjsonstring = (char *)calloc(1, (filestat.st_size + 1) * sizeof(char));  //yes
			if(NULL == pjsonstring )
			{		
			printf("malloc failed!\n");		
			exit(1);   
			}
		}
			if(NULL != pjsonstring)
			{
				ret = fread(pjsonstring, sizeof(char), filestat.st_size, fp);
				if(ret > 0)
				{
				
					object = json_tokener_parse(pjsonstring);
				
				}
			}
	
			PRINTF("fp is ==%s\n",fp);
			PRINTF("pjsonstring is ==%s\n",pjsonstring);
			getnumobject = json_object_object_get(object,"num");
	   if(getnumobject==NULL)
	   {
		  PRINTF("getnumobject is error");
	   }
	   else
	   {
		   num = json_object_get_int(getnumobject);
		   PRINTF("Num =%d\n",num);
	   }
	   
	   getIndexObject = json_object_object_get(object,"index");
	   if(getnumobject==NULL)
	   {
		  PRINTF("getIndexObject is error");
	   }
	   else
	   {
		   index = json_object_get_int(getIndexObject);
		   PRINTF("index =%d\n",index);
	   }
	   
	  getlistobject = json_object_object_get(object,"musiclist");
	
	  if(outplaylist!=NULL)
	 { 
	 	  fp = fopen(outplaylist, "w+");
	    for(i = 0;i<num;i++)
	    {
		    musiclistobj =json_object_array_get_idx(getlistobject,i);
     	  urlobject = json_object_object_get(musiclistobj, "content_url");
     	  if(NULL == urlobject) {

					PRINTF("json_object_object_get get urlobject failed");
					goto failed;
				}
				
				char *musicurl = json_object_get_string(urlobject);
				if(NULL == musicurl) {
			
						PRINTF("json_object_get_string get musicurl failed");
						goto failed;
				}
		   PRINTF("musicurl == %s\n",musicurl);
			
		    //modify by xudh 2017-11-08
		   	if(strlen(musicurl) < 10) //解决Json文件中音乐url地址为空(ex:http://)
				continue;
			
		   fputs(musicurl, fp);
		   fputs("\n", fp);
		  
		
	   }
	    fclose(fp);
	     fp = NULL;
	  
	         memset(tempbuff, 0, sizeof(tempbuff)/sizeof(char));
			 sprintf(tempbuff, "playlist.sh load %s %d",outplaylist,index);
			 my_system(tempbuff);
			
	 
	}else{
		    musiclistobj =json_object_array_get_idx(getlistobject,0);
     	  urlobject = json_object_object_get(musiclistobj, "content_url");
     	  if(NULL == urlobject) {

					PRINTF("json_object_object_get get urlobject failed");
					goto failed;
				}
				
				char *musicurl = json_object_get_string(urlobject);
		  
		     memset(tempbuff, 0, sizeof(tempbuff)/sizeof(char));
			 sprintf(tempbuff, "playlist.sh add %s", musicurl);
			 my_system(tempbuff);
		}

     json_object_put(object);
	  
	   return 0;
	  failed:
		ret = -1;
		//parse_failed();
		return ret;			
	
}	


int insert_songs_parse_playlistinfo_add_playlist(char *infilepatch,char *outplaylist){
	  struct stat filestat;
	  char filename[128];
	  char tempbuff[256];
	  FILE *fp = NULL;
	  int i = 0,ret,num;
	  struct json_object *object,*getnumobject,*getlistobject,*musiclistobj,*urlobject;
	  char *pjsonstring = NULL;
	   fp = fopen(infilepatch, "r");
		if(NULL != fp)
		{
			stat(infilepatch, &filestat);
			pjsonstring = (char *)calloc(1, (filestat.st_size + 1) * sizeof(char)); //yes
			if(NULL == pjsonstring )
			{		
			printf("malloc failed!\n");		
			exit(1);   
			}
		}
			if(NULL != pjsonstring)
			{
				ret = fread(pjsonstring, sizeof(char), filestat.st_size, fp);
				if(ret > 0)
				{
					
					object = json_tokener_parse(pjsonstring);
					}
			}
	
			PRINTF("fp is ==%s\n",fp);
			PRINTF("pjsonstring is ==%s\n",pjsonstring);
			getnumobject = json_object_object_get(object,"num");
	   if(getnumobject==NULL)
	   {
		  PRINTF("getnumobject is error");
	   }
	   else
	   {
		   num = json_object_get_int(getnumobject);
		   PRINTF("Num =%d\n",num);
	   }
	  getlistobject = json_object_object_get(object,"musiclist");
	 // PRINTF("getlistobject =%s\n",json_object_to_json_string(getlistobject));
	  if(outplaylist!=NULL)
	 { 
	 	  fp = fopen(outplaylist, "w+");
	    for(i = 0;i<num;i++)
	    {
		    musiclistobj =json_object_array_get_idx(getlistobject,i);
     	  urlobject = json_object_object_get(musiclistobj, "content_url");
     	  if(NULL == urlobject) {

					PRINTF("json_object_object_get get urlobject failed");
					goto failed;
				}
				//addMusic->pContentURL = json_object_get_string(urlobject);
				char *musicurl = json_object_get_string(urlobject);
				if(NULL == musicurl) {
			
						PRINTF("json_object_get_string get musicurl failed");
						goto failed;
				}
		   PRINTF("musicurl == %s\n",musicurl);
			
		    //modify by xudh 2017-11-08
		   	if(strlen(musicurl) < 10) //解决Json文件中音乐url地址为空(ex:http://)
				continue;
			
		   fputs(musicurl, fp);
		   fputs("\n", fp);
		  
	   }
	    fclose(fp);
	     fp = NULL;	 
	}else{
		    musiclistobj =json_object_array_get_idx(getlistobject,0);
     	  urlobject = json_object_object_get(musiclistobj, "content_url");
     	  if(NULL == urlobject) {

					PRINTF("json_object_object_get get urlobject failed");
					goto failed;
				}
				
				char *musicurl = json_object_get_string(urlobject);
		   
		 //  memset(tempbuff, 0, sizeof(tempbuff)/sizeof(char));
			// sprintf(tempbuff, "playlist.sh add %s", musicurl);
		//	 my_system(tempbuff);
		}

		json_object_put(object);
	   
	   return 0;
	  failed:
		ret = -1;
		//parse_failed();
		return ret;			
	
}


	int update_playlist(char *infilepatch,char *outplaylist){
		struct stat filestat;
	  char filename[128];
	  char tempbuff[256];
	  FILE *fp = NULL;
	  int i = 0,ret,num;
	 // WARTI_pMusic addMusic; 
	  struct json_object *object,*getnumobject,*getlistobject,*musiclistobj,*urlobject;
	  char *pjsonstring = NULL;
	   fp = fopen(infilepatch, "r");
		if(NULL != fp)
		{
			stat(infilepatch, &filestat);
			pjsonstring = (char *)calloc(1, (filestat.st_size + 1) * sizeof(char));//yes
			if(NULL == pjsonstring )
			{		
			printf("malloc failed!\n");		
			exit(1);   
			}
		}
			if(NULL != pjsonstring)
			{
				ret = fread(pjsonstring, sizeof(char), filestat.st_size, fp);
				if(ret > 0)
				{
					//	PRINTF("pjsonstring == %s\n", pjsonstring);
					object = json_tokener_parse(pjsonstring);
				//	pstr = WIFIAudio_RealTimeInterface_newStr(WARTI_STR_TYPE_SYNCHRONOUSCONFIGINFO, pjsonstring);
				}
			}
	
			PRINTF("fp is ==%s\n",fp);
			PRINTF("pjsonstring is ==%s\n",pjsonstring);
			getnumobject = json_object_object_get(object,"num");
	   if(getnumobject==NULL)
	   {
		  PRINTF("getnumobject is error");
	   }
	   else
	   {
		   num = json_object_get_int(getnumobject);
		   PRINTF("Num =%d\n",num);
	   }
	  getlistobject = json_object_object_get(object,"musiclist");
	 // PRINTF("getlistobject =%s\n",json_object_to_json_string(getlistobject));
	  if(outplaylist!=NULL)
	 { 
	 	  fp = fopen(outplaylist, "w+");
	    for(i = 0;i<num;i++)
	    {
		    musiclistobj =json_object_array_get_idx(getlistobject,i);
     	  urlobject = json_object_object_get(musiclistobj, "content_url");
     	  if(NULL == urlobject) {

					PRINTF("json_object_object_get get urlobject failed");
					goto failed;
				}
				//addMusic->pContentURL = json_object_get_string(urlobject);
				char *musicurl = json_object_get_string(urlobject);
				if(NULL == musicurl) {
			
						PRINTF("json_object_get_string get musicurl failed");
						goto failed;
				}
		   PRINTF("musicurl == %s\n",musicurl);
			
		    //modify by xudh 2017-11-08
		   	if(strlen(musicurl) < 10) //解决Json文件中音乐url地址为空(ex:http://)
				continue;
			
		   fputs(musicurl, fp);
		   fputs("\n", fp);
		  
		//my_system("playlist.sh load outplaylist");
	   }
	    fclose(fp);
	     fp = NULL;
	   
	     memset(tempbuff, 0, sizeof(tempbuff)/sizeof(char));
			 sprintf(tempbuff, "playlist.sh update \"%s\"",outplaylist);
			 my_system(tempbuff);
	 
	}else{
		    musiclistobj =json_object_array_get_idx(getlistobject,0);
     	  urlobject = json_object_object_get(musiclistobj, "content_url");
     	  if(NULL == urlobject) {

					PRINTF("json_object_object_get get urlobject failed");
					goto failed;
				}
				//addMusic->pContentURL = json_object_get_string(urlobject);
				char *musicurl = json_object_get_string(urlobject);
		   //my_system("playlist.sh add addMusic->pContentURL");
		   memset(tempbuff, 0, sizeof(tempbuff)/sizeof(char));
			 sprintf(tempbuff, "playlist.sh add \"%s\"", musicurl);
			 my_system(tempbuff);
		}


	json_object_put(object);
	
	return 0;
	failed:
		ret = -1;
		//parse_failed();
		return ret;			
	
	}	

int  parse_currentplaylist_to_index(void)
{


	FILE *fp =NULL;
	char buff[1024];
    int i=0;
    unsigned int sum=0;
     
   // relute= cmd_system("mpc");

    my_system("mpc >/tmp/file.txt");
    if(NULL != (fp = fopen("/tmp/file.txt", "r")))
	    {
			int ret=fread(buff,1,sizeof(buff),fp);
			if(ret==0)
			{
				printf("fread error\n");
			}
		}
		fclose(fp);
	
    PRINTF("buff:%s\n",buff);
	
    char *rp=strstr(buff,"[playing] #");
 
   
	if(rp == NULL)
		{
    	 rp=strstr(buff,"[paused]  #");
		}
	
    if(rp!=NULL)
    {
		
		while(buff[i]!='#')
			i++;
			
		while(buff[i]!='/')
	      {
			

			if(buff[i]>='0' && buff[i]<='9')
			{			
			  sum=sum*10+(buff[i]-'0');
			}
			i++;
		 }
	   }
	
	


	return sum;
}

char* parse_currentplaylist_to_URL(char *currentplaylist,int num)
{
    FILE *dp =NULL;
	char *buf,*deline;
    int i=0;


   if(num!=0){
    buf=(char*)malloc(256);
	if(NULL == buf)
		{		
		printf("malloc failed!\n");	
		exit(1);   
		}
	memset(buf,0,256);
	    dp= fopen(currentplaylist, "r");
	    if( dp ) 
          { 
            
	         for(i=0;i<num;i++)
	         {
			 deline = fgets(buf,256, dp);
			 if(deline==NULL)
			 	{
			 	 PRINTF("parse currentplaylist to URL = error ");
				 return 0;
			 	}
			 }
	      	 PRINTF("deline===[%s]\n",deline);
			 
			 fclose(dp);
			 dp= NULL;
		  }
		return  deline;
  	}	
	return 0;
}

#if 0
WARTI_pMusic parse_currentplaylist_to_JSON(char *curplaylistinfo,int num)
{
    char *ret;
	WARTI_pMusicList lp;
	int i;

	PRINTF("num====[%d]\n",num);
    lp = getplaylist_parse_json(curplaylistinfo);
	PRINTF("lp->Num==========[%d]\n",lp->Num);
	PRINTF("(lp->pMusicList)[num]->pTitle=======[%s]\n",(lp->pMusicList)[num-1]->pTitle);
    //ret = strdup((lp->pMusicList)[num]);
    return (lp->pMusicList);
}
#endif

// direction   0   1 

int json_to_currentplaylist(char *collplaylist,WARTI_pMusic pmusiclist,int direction)
{

 struct json_object *createobj;
 struct json_object *array;
 struct json_object *addmusicobject;
 
 char *pjsontostring=NULL;
 FILE *fp =NULL;

 
 WARTI_pMusicList lp;
 
 PRINTF("pmusiclist->pToken=====[%s\n]",pmusiclist->pToken);

 addmusicobject = create_music_content_object(pmusiclist);
 PRINTF("addmusicobject=%s\n", json_object_to_json_string(addmusicobject));

 if((access(collplaylist,F_OK))==0){
    
	 
	 lp = getplaylist_parse_json(collplaylist);  //yes

	//// direction  0    1 
	 createobj = add_one_musicjson_to_json_array(lp,addmusicobject,direction);
	
	 }else{
	
     createobj = json_object_new_object();
	
	 json_object_object_add(createobj,"num",json_object_new_int(1));
	
	 array = json_object_new_array();
	
	 json_object_array_add(array,addmusicobject);
	
	 json_object_object_add(createobj,"musiclist",array);
	
	 }	


	

    pjsontostring = json_object_to_json_string(createobj);
	
    PRINTF("pjsontostring=============[%s]\n",pjsontostring);


	if(NULL != (fp = fopen(collplaylist, "w+")))
	{
	    
		int ret=fwrite(pjsontostring, sizeof(char), strlen(pjsontostring) + 1, fp);
		if(ret==0)
			{
			  PRINTF("collplaylist no write............\n");
			  return 0;
			}
		free( pjsontostring);
		pjsontostring=NULL;
		fclose(fp);
		fp = NULL;
	}


return 0;

}

void PrintSysTime(char *pSrc) 
{
#if 0
	struct   tm     *ptm; 
	long       ts; 
	int         y,m,d,h,n,s; 

	ts  = time(NULL); 
	ptm = localtime(&ts); 

	y = ptm-> tm_year+1900; 
	m = ptm-> tm_mon+1;     
	d = ptm-> tm_mday;      
	h = ptm-> tm_hour;      
	n = ptm-> tm_min;       
	s = ptm-> tm_sec;       

	DEBUG_PRINTF("[%s]---sys_time:%02d.%02d.%02d.%02d.%02d.%02d", pSrc, y, m, d, h, n, s);
#endif
	struct timeval start;
    gettimeofday( &start, NULL );
   	printf("\033[1;31;40m [%s]---sys_time:%d.%d \033[0m\n", pSrc, start.tv_sec, start.tv_usec);
}


#define LOCKFILE "/var/run/creatPlayList.pid"
#define LOCKMODE (S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH)

int lockfile(int fd)
{
	struct flock fl;
	fl.l_type = F_WRLCK;
	fl.l_start = 0;
	fl.l_whence = SEEK_SET;
	fl.l_len = 0;
	return(fcntl(fd, F_SETLK, &fl));
}

int creatPlayList_running(void)
{
	int fd;
	char buf[16];
	fd = open(LOCKFILE, O_RDWR|O_CREAT, LOCKMODE);
	if (fd < 0) {
		printf("cannot open %s: %s, creatPlayList is running.\n", LOCKFILE, strerror(errno));
		exit(1);
	}
	if (lockfile(fd) < 0) {
	if (errno == EACCES || errno == EAGAIN) {
		printf("cannot lock %s: %s, creatPlayList is running.\n", LOCKFILE, strerror(errno));
		close(fd);
		exit(1);
	}
		printf("cannot lock %s: %s, creatPlayList is running.\n", LOCKFILE, strerror(errno));
		exit(1);
	}
	ftruncate(fd, 0);
	sprintf(buf, "%ld", (long)getpid());
	write(fd, buf, strlen(buf)+1);
	return 0;
}


int loadplaylist(char *argv_2)
{
      
	       WARTI_pMusicList inlp,outlp;
		   struct json_object *creatobject;
		   int ret,len=0,rlen=0;
		   char *pjsontostring=NULL;
		   FILE *fp =NULL;
		   FILE *dp =NULL;
		   
		   char *tmpPlaylist = "/tmp/httpPlayList.m3u";    
		   char *outplaylistinfo = "/usr/script/playlists/httpPlayListInfoJson.txt";
		   
		   inform_ctrl_msg(MONDLNA,NULL);
		   inlp = getplaylist_parse_json(argv_2);
		   PRINTF("enter inlp==%d\n",inlp->Num);
		   if((access(outplaylistinfo,F_OK))==0){
		   dp=fopen(outplaylistinfo,"r");
		   if(dp==NULL){	   
		   PRINTF("dp=====NULL\n");  
		   }else{
		   PRINTF("dp ==========%d\n",dp);	
		   fseek(dp,0L,SEEK_END); 
		   len=ftell(dp); 
		   PRINTF("len=====%d\n",len);	
		   }
		   fclose(dp);
	  
		   }


		    my_system("mpc playlist > /tmp/httpPlayList.m3u");
			if((access(tmpPlaylist,F_OK))==0){
			dp=fopen(tmpPlaylist,"r");
			if(dp==NULL){		
			PRINTF("dp=====NULL\n");  
			}else{
			PRINTF("dp ==========%d\n",dp);  
			fseek(dp,0L,SEEK_END); 
			rlen=ftell(dp); 
			PRINTF("len=====%d\n",len);  
			}
			fclose(dp);
	
			}
		   if((access(outplaylistinfo,F_OK))==0 && (len!=0)&&(rlen!=0))
		   {
	 			   cc_jso_insert(outplaylistinfo,argv_2,-1);  //if idx ==-1, is add 
		   }
		   else
		   {
			     creatobject=pMusicListInfoTopJsonObject(inlp);
			     pjsontostring =	json_object_to_json_string(creatobject);
		   
		   if(NULL != (fp = fopen(outplaylistinfo, "w+")))
		     {
			   ret = fwrite(pjsontostring, sizeof(char), strlen(pjsontostring) + 1, fp);
			   free(pjsontostring);
			   pjsontostring = NULL;
			   fclose(fp);
			   fp = NULL;
		     }
		    }
		 
		   ret = add_parse_playlistinfo_add_playlist(argv_2,tmpPlaylist);
		   copy_file("/tmp/httpPlayList.m3u","/usr/script/playlists/playlist.m3u");
    	   copy_file(outplaylistinfo,"/usr/script/playlists/currentPlaylistInfoJson.txt");
		   my_system("mpc playlist > /usr/script/playlists/currentplaylist.m3u");
		   return ret;
}
int insertplaylist(char *argv_2,char *argv_3,char *argv_4,char *argv_5,char *argv_6)
{
    char *outplaylistinfo = "/usr/script/playlists/httpPlayListInfoJson.txt";
    char *tmpoutplaylistinfo = "/usr/script/playlists/tmphttpPlayListInfoJson.txt";
	
			if(!strcmp(argv_2,"null")){
			argv_2 = NULL;
			}
			if(!strcmp(argv_3,"null")){
			argv_3 = NULL;
			}
			if(!strcmp(argv_4,"null")){
			argv_4 = NULL;
			}
			if(!strcmp(argv_5,"null")){
			argv_5 = NULL;
			}
			if(!strcmp(argv_6,"null")){
			argv_6 = NULL;
			}
	
			insert_music_to_playlistinfo(outplaylistinfo,tmpoutplaylistinfo,argv_2,argv_3, \
			argv_4,argv_5,argv_6);
			rename(tmpoutplaylistinfo,outplaylistinfo);
			int ret = parse_playlistinfo_add_playlist(outplaylistinfo,NULL);							
	        copy_file("/usr/script/playlists/httpPlayListInfoJson.txt","/usr/script/playlists/currentPlaylistInfoJson.txt");
            return ret;
}
int insertonesong(char *argv_2,char *argv_3,char *argv_4,char *argv_5,char *argv_6)
{
           char *outplaylistinfo = "/usr/script/playlists/httpPlayListInfoJson.txt";
           char *tmpoutplaylistinfo = "/usr/script/playlists/tmphttpPlayListInfoJson.txt";
           char *tmpPlaylist = "/tmp/httpPlayList.m3u";  
		
			if(!strcmp(argv_2,"null")){
			argv_2 = NULL;
			}
			if(!strcmp(argv_3,"null")){
			argv_3 = NULL;
			}
			if(!strcmp(argv_4,"null")){
			argv_4 = NULL;
			}
			if(!strcmp(argv_5,"null")){
			argv_5 = NULL;
			}
			if(!strcmp(argv_6,"null")){
			argv_6 = NULL;
			}
			insert_one_music_to_playlistinfo(outplaylistinfo,tmpoutplaylistinfo,argv_2,argv_3, \
			argv_4,argv_5,argv_6);
			char buf[1024];
			// sprintf(buf,"creatPlayList insertplaylist %s %s %s %s %s",argv[2],argv[3],argv[4],argv[5],argv[6]);
	
			sprintf(buf,"mpc insert %s",argv_5);
			int ret = my_system(buf);
			rename(tmpoutplaylistinfo,outplaylistinfo);
			update_playlist(outplaylistinfo,tmpPlaylist);  
			copy_file("/usr/script/playlists/httpPlayListInfoJson.txt","/usr/script/playlists/currentPlaylistInfoJson.txt");
			return ret;					  

}

int deleteonesong(char *argv_2)
{
	       char *outplaylistinfo = "/usr/script/playlists/httpPlayListInfoJson.txt";
           char *tmpoutplaylistinfo = "/usr/script/playlists/tmphttpPlayListInfoJson.txt";
           char *tmpPlaylist = "/tmp/httpPlayList.m3u";  
		   
			if(!strcmp(argv_2,"null")){
			argv_2 = NULL;
			}
			int num = atoi(argv_2);
			delete_one_music_to_playlistinfo(outplaylistinfo,tmpoutplaylistinfo,num);
			char buf[1024];
			sprintf(buf,"mpc del %d",num);
			PRINTF("buf ==%s\n",buf);
			int ret = my_system(buf);
			rename(tmpoutplaylistinfo,outplaylistinfo);
			update_playlist(outplaylistinfo,tmpPlaylist);
			copy_file("/usr/script/playlists/httpPlayListInfoJson.txt","/usr/script/playlists/currentPlaylistInfoJson.txt");
            return ret;

}
int replacePlaylist(char *argv_2)
{
	          WARTI_pMusicList inlp;
			FILE *fp =NULL;
            struct json_object *creatobject;
	        int ret;
			char *pjsontostring=NULL;
			  
			char *tmpPlaylist = "/tmp/httpPlayList.m3u";
		    char *outplaylistinfo = "/usr/script/playlists/httpPlayListInfoJson.txt";
	
			inlp = getplaylist_parse_json(argv_2); //yes
			PRINTF("enter inlp==%d\n",inlp->Num);
		    inform_ctrl_msg(MONDLNA,NULL);
		    creatobject=pMusicListInfoTopJsonObject(inlp);
	
		
			PRINTF("creat object==%s\n", json_object_to_json_string(creatobject));
            pjsontostring =  json_object_to_json_string(creatobject);
			
			
			if(NULL != (fp = fopen(outplaylistinfo, "w+")))
			{
				ret = fwrite(pjsontostring, sizeof(char), strlen(pjsontostring) + 1, fp);
				free( pjsontostring);
				pjsontostring=NULL;
				fclose(fp);
				fp = NULL;
			}
			ret = load_parse_playlistinfo_add_playlist(outplaylistinfo,tmpPlaylist);

			return ret;
}
int loadSongs(char *argv_2)
{    
            WARTI_pMusicList inlp,outlp;
            struct json_object *creatobject;
	        int ret=0,len=0,rlen=0;
			char *pjsontostring=NULL;
			FILE *fp =NULL;
	        FILE *dp =NULL;
			
			char *tmpPlaylist = "/tmp/httpPlayList.m3u";	
	        char *outplaylistinfo = "/usr/script/playlists/httpPlayListInfoJson.txt";
			
			inform_ctrl_msg(MONDLNA,NULL);
			inlp = getplaylist_parse_json(argv_2);
			PRINTF("enter inlp==%d\n",inlp->Num);
			if((access(outplaylistinfo,F_OK))==0){
			dp=fopen(outplaylistinfo,"r");
			if(dp==NULL){		
			PRINTF("dp=====NULL\n");  
			}else{
			PRINTF("dp ==========%d\n",dp);  
			fseek(dp,0L,SEEK_END); 
			len=ftell(dp); 
			PRINTF("len=====%d\n",len);  
			}
			fclose(dp);
	
			}
			
			
		    my_system("mpc playlist > /tmp/httpPlayList.m3u");
			if((access(tmpPlaylist,F_OK))==0){
			dp=fopen(tmpPlaylist,"r");
			if(dp==NULL){		
			PRINTF("dp=====NULL\n");  
			}else{
			PRINTF("dp ==========%d\n",dp);  
			fseek(dp,0L,SEEK_END); 
			rlen=ftell(dp); 
			PRINTF("len=====%d\n",len);  
			}
			fclose(dp);
	
			}
			if((access(outplaylistinfo,F_OK))==0 && (len!=0)&&(rlen!=0))
			{
	
				outlp = getplaylist_parse_json(outplaylistinfo); //yes
				PRINTF("inlp NUM==%d\n", inlp->Num);
				creatobject=Insert_Songs_pMusicListInfoTopJsonObject(inlp,outlp);
			}
			else
			{
				creatobject=pMusicListInfoTopJsonObject(inlp);
			}
			PRINTF("creat object==%s\n", json_object_to_json_string(creatobject));
	
			pjsontostring =  json_object_to_json_string(creatobject);
			
			if(NULL != (fp = fopen(outplaylistinfo, "w+")))
			{
				ret = fwrite(pjsontostring, sizeof(char), strlen(pjsontostring) + 1, fp);
				free(pjsontostring);
				pjsontostring = NULL;
				fclose(fp);
				fp = NULL;
			}
				   
			 
			ret =insert_songs_parse_playlistinfo_add_playlist(outplaylistinfo,tmpPlaylist);	 
			copy_file("/tmp/httpPlayList.m3u","/usr/script/playlists/playlist.m3u");
			if(rlen==0){
			my_system("mpc load playlist.m3u");}
			copy_file(outplaylistinfo,"/usr/script/playlists/currentPlaylistInfoJson.txt");
			my_system("mpc playlist > /usr/script/playlists/currentplaylist.m3u");
            return ret;

}
int wPlayingSong(char *argv_2)
{

    int ret=-1;



if(cdb_get_int("$vtf003_usb_tf",0)){
    
   	if(!strncmp(argv_2, "s",1)) {
		 
	     //ret= WIFIAudio_UciConfig_SearchAndSetValueString("xzxwifiaudio.config.usb_last_contentURL",argv_2);
	     ret = cdb_set("$usb_last_contentURL",argv_2);
   	}
	else if(!strncmp(argv_2, "m",1)) {
	     //ret=WIFIAudio_UciConfig_SearchAndSetValueString("xzxwifiaudio.config.sd_last_contentURL",argv_2);
         ret = cdb_set("$sd_last_contentURL",argv_2);
	}
	if(ret==-1)
		printf(" sd_last_contentURL usb_last_contentURL  fail\n");
}	
 	ret=parse_current_song_tag(argv_2);
	return ret;
}
int modifyConf(char *argv_2,char *argv_3)
{
	        char tempbuff[512];
			int ret;
			memset(tempbuff, 0, sizeof(tempbuff)/sizeof(char));
			sprintf(tempbuff, "$%s",argv_2);
			//ret = WIFIAudio_UciConfig_SearchAndSetValueString(tempbuff,argv_3);
			ret = cdb_set(tempbuff,argv_3);
            return ret;
}
int tfmode(void)
{
	char *tfplaylist = "/usr/script/playlists/tfPlaylist";
    char *currentplaylistinfo = "/usr/script/playlists/currentPlaylistInfoJson.txt";
	
	inform_ctrl_msg(MONDLNA,NULL);
	my_system("playlist.sh anothertf");   
	int ret = read_m3u_create_WARTI_pMusicList(tfplaylist);
	copy_file("/usr/script/playlists/tfPlaylistInfoJson.txt","/usr/script/playlists/currentPlaylistInfoJson.txt");
	my_system("mpc repeat on");
    return ret;
}
int usbmode(void)
{
        char *usbplaylist = "/usr/script/playlists/usbPlaylist";
		 
		inform_ctrl_msg(MONDLNA,NULL);		   
		my_system("playlist.sh anotherusb");
		int ret = read_m3u_create_WARTI_pMusicList(usbplaylist);
		copy_file("/usr/script/playlists/usbPlaylistInfoJson.txt","/usr/script/playlists/currentPlaylistInfoJson.txt");
		my_system("mpc repeat on");
        return ret;
}

int createUJ(void)
{
        char *usbplaylist = "/usr/script/playlists/usbPlaylist";
		inform_ctrl_msg(MONDLNA,NULL);
		int ret = read_m3u_create_WARTI_pMusicList(usbplaylist);
        return ret;
}
int createTJ(void)
{       
        char *tfplaylist = "/usr/script/playlists/tfPlaylist";
		inform_ctrl_msg(MONDLNA,NULL);
		int ret= read_m3u_create_WARTI_pMusicList(tfplaylist);
        return ret;
}


int cuptu(void)
{      
        char *tfplaylist = "/usr/script/playlists/tfPlaylist";
		char *usbplaylist = "/usr/script/playlists/usbPlaylist";
		inform_ctrl_msg(MONDLNA,NULL);
		
		my_system("playlist.sh uptu");
		int ret=read_m3u_create_WARTI_pMusicList(usbplaylist);
		ret=read_m3u_create_WARTI_pMusicList(tfplaylist);
        return ret;
}
int playTF(char *argv_2)
{
		inform_ctrl_msg(MONDLNA,NULL);
		
		char tempbuff[32];
		memset(tempbuff, 0, sizeof(tempbuff)/sizeof(char));
		sprintf(tempbuff, "playlist.sh plt %s",argv_2);
		int ret= my_system(tempbuff);
		return ret;

}
int playUSB(char *argv_2)
{
		inform_ctrl_msg(MONDLNA,NULL);
		
		char tempbuff[32];
		memset(tempbuff, 0, sizeof(tempbuff)/sizeof(char));
		sprintf(tempbuff, "playlist.sh plu %s",argv_2);
		int ret = my_system(tempbuff);
		return ret;

}
int wifimode(char *argv_2)
{
		inform_ctrl_msg(MONDLNA,NULL);
		//char *value = WIFIAudio_UciConfig_SearchValueString("xzxwifiaudio.config.playmode");
		int playmode = cdb_get_int("$playmode",0);
		//if(strcmp(value, "000")) {
		if(playmode != 000){  // When it's not WiFi mode, it will broadcast voice when you enter WiFi mode. 
		my_system("uartdfifo.sh inwifimode");
		}	
		int ret = my_system("playlist.sh poweron");
		return ret;

}
int reloadplay(char *argv_2)
{
		char tempbuff[512];
		my_system("playlist.sh poweron");
		memset(tempbuff, 0, sizeof(tempbuff)/sizeof(char));
		sprintf(tempbuff, " mpc play \"%s\"", argv_2);
		int ret= my_system(tempbuff);
		memset(tempbuff, 0, sizeof(tempbuff)/sizeof(char));
        return ret;
}

int alexaOne_three(char *argv_2,char *argv_3,char *argv_4,char *argv_5,char *argv_6,char *argv_7)
{

        
		char *alexaplaylistinfo = "/usr/script/playlists/alexaPlayListInfoJson.txt";
		
		char tempbuff[1024];
        int ret;
		
	    int rut=parse_currentplaylist_to_index();	
	    if(rut==NULL)
	    {
		  ret = alexaOne_main(argv_2,argv_3,argv_4,argv_5,argv_6,argv_7);
	    }
	    else{
            memset(tempbuff, 0, sizeof(tempbuff)/sizeof(char));
			sprintf(tempbuff, " mpc add \"%s\"", argv_5);
			ret=my_system(tempbuff);

			again_add_alexa_music_to_playlist(alexaplaylistinfo,argv_2,argv_3, \
			argv_4,argv_5,argv_6,argv_7,1);
			
			my_system("mpc playlist > /usr/script/playlists/alexaPlayList.m3u ");
		    copy_file("/usr/script/playlists/alexaPlayList.m3u","/usr/script/playlists/currentplaylist.m3u");
            copy_file("/usr/script/playlists/alexaPlayListInfoJson.txt","/usr/script/playlists/currentPlaylistInfoJson.txt");
		  
		 }
		 return ret;
}
int alexaOne_other_main(char *tempbuff,int flag,char *argv_2,char *argv_3,char *argv_4,char *argv_5,char *argv_6,char *argv_7)
{
		char *alexaplaylistinfo = "/usr/script/playlists/alexaPlayListInfoJson.txt";
		char *outplaylistinfo = "/usr/script/playlists/httpPlayListInfoJson.txt";
		char *currentplaylistinfo = "/usr/script/playlists/currentPlaylistInfoJson.txt";
        int ret =-1,i;
		
		WARTI_pMusicList pppmusic;
		WARTI_pMusic pp;
		
        

		int rut=parse_currentplaylist_to_index();

		PRINTF("rut=======[%d]\n",rut);

		 if(rut==0)
		 {
		 ret = alexaOne_main(argv_2,argv_3,argv_4,argv_5,argv_6,argv_7);
		 }
		 else
		 {

			ret = my_system(tempbuff);
			
			add_alexa_music_to_playlist(outplaylistinfo,alexaplaylistinfo,argv_2,argv_3, \
		argv_4,argv_5,argv_6,argv_7);
			
			if((access(currentplaylistinfo,F_OK))==0)
			{
			
			if(flag)
			{
			int a=rut-1;
			alexaOne_json(a);
			}
			my_system("mpc playlist > /usr/script/playlists/alexaPlayList.m3u ");
		    copy_file("/usr/script/playlists/alexaPlayList.m3u","/usr/script/playlists/currentplaylist.m3u");
			copy_file("/usr/script/playlists/alexaPlayListInfoJson.txt","/usr/script/playlists/currentPlaylistInfoJson.txt");
			}
		}

    return ret;
}

int alexaOne_json(int a)
{
	    WARTI_pMusicList pppmusic; 
		WARTI_pMusic pp;

	    char *currentplaylistinfo = "/usr/script/playlists/currentPlaylistInfoJson.txt";
		char *alexaplaylistinfo = "/usr/script/playlists/alexaPlayListInfoJson.txt";
		
	    pppmusic=getplaylist_parse_json(currentplaylistinfo);  //yes
		if(NULL!=((pppmusic->pMusicList)[a]))
		{
		pp=((pppmusic->pMusicList)[a]);
		json_to_currentplaylist(alexaplaylistinfo,pp,0);
		}
		
		alexaOne_free(pppmusic,a);
		return 0;
}
int alexaOne_free(WARTI_pMusicList pppmusic,int a)
{
    int i;

	if(NULL != pppmusic->pMusicList)   
	{
	
	for(i = 0; i<(pppmusic->Num); i++)
		{
		
		if(NULL != (pppmusic->pMusicList)[i])
			{
			
			if(i==(a))
			continue;

			json_pMusicFree((pppmusic->pMusicList)[i]);

			}
		}
	
	free(pppmusic->pMusicList);
	pppmusic->pMusicList = NULL;
	}
		free(pppmusic);
		pppmusic = NULL;
        return 0;
}

int alexaOne_main(char *argv_2,char *argv_3,char *argv_4,char *argv_5,char *argv_6,char *argv_7)
{
            char temp[1024];
	        char *alexaplaylistinfo = "/usr/script/playlists/alexaPlayListInfoJson.txt";
			char *outplaylistinfo = "/usr/script/playlists/httpPlayListInfoJson.txt";

			//WIFIAudio_UciConfig_SearchAndSetValueString("xzxwifiaudio.config.token", argv_7);
            cdb_set("$token",argv_7);
			add_alexa_music_to_playlist(outplaylistinfo,alexaplaylistinfo,argv_2,argv_3, \
			argv_4,argv_5,argv_6,argv_7);
				
			memset(temp, 0, sizeof(temp)/sizeof(char));
			sprintf(temp, "playlist.sh addalexa '%s'", argv_5);
			int ret = my_system(temp);
		
			return ret;
}

int alexaOne(char *argv_2,char *argv_3,char *argv_4,char *argv_5,char *argv_6,char *argv_7)
{
	int ret=-1;
	char tempbuff[1024];
	
	cdb_set_int("$current_play_mode",0);
	//ret = WIFIAudio_UciConfig_SearchAndSetValueString("xzxwifiaudio.config.playmode", "000");
	cdb_set_int("$playmode",000);
	if(!strcmp(argv_2,"null")){
	argv_2 = NULL;
	}
	if(!strcmp(argv_3,"null")){
	argv_3 = NULL;
	}
	if(!strcmp(argv_4,"null")){
	argv_4 = NULL;
	}
	if(!strcmp(argv_5,"null")){
	argv_5 = NULL;
	}
	if(!strcmp(argv_6,"null")){
	argv_6 = NULL;
	}
	if(!strcmp(argv_7,"null")){
	argv_7 = NULL;
	}

	 if(argv_2==NULL)
	 {
	    ret = alexaOne_main(argv_2,argv_3,argv_4,argv_5,argv_6,argv_7);
        my_system("mpc repeat off"); 
	 } 	
	 else if(!strcmp(argv_2,"2")) //add
     {
      ret=alexaOne_three(argv_2,argv_3,argv_4,argv_5,argv_6,argv_7);
      my_system("mpc repeat off"); 
	}
	else if(!strcmp(argv_2,"3")) //crop insert  1:add_alexa_music_to_playlist
	{
	  memset(tempbuff, 0, sizeof(tempbuff)/sizeof(char));
	  sprintf(tempbuff, "mpc crop && mpc insert \"%s\" ", argv_5);
	  ret = alexaOne_other_main(tempbuff,1,argv_2,argv_3,argv_4,argv_5,argv_6,argv_7);
      my_system("mpc repeat off");
	}
	else if(!strcmp(argv_2,"4"))//crop
	{
	  memset(tempbuff, 0, sizeof(tempbuff)/sizeof(char));
	  sprintf(tempbuff, "mpc crop ", argv_5);
	  ret = alexaOne_other_main(tempbuff,0,argv_2,argv_3,argv_4,argv_5,argv_6,argv_7);
	  my_system("mpc repeat on");
	}
	else {
	  ret = alexaOne_main(argv_2,argv_3,argv_4,argv_5,argv_6,argv_7);
      my_system("mpc repeat off");
	}

	cdb_set_int("$current_play_mode",5);
	/*
	ret = WIFIAudio_UciConfig_SearchAndSetValueString("xzxwifiaudio.config.playmode", "005");
	ret = WIFIAudio_UciConfig_SearchAndSetValueString("xzxwifiaudio.config.token", argv_7);
	*/
	cdb_set_int("$playmode",005);
	cdb_set("$token",argv_7);
	return ret;
}

int playcollplaylist(char *argv_2)
{       
	    char tempbuff[1024];

		memset(tempbuff, 0, sizeof(tempbuff)/sizeof(char));
		sprintf(tempbuff, "playlist.sh PlayCollplaylist \"%s\"" ,argv_2);
		int ret = my_system(tempbuff);
		memset(tempbuff, 0, sizeof(tempbuff)/sizeof(char));
        return ret;
}
int delcollplaylist(char *argv_2)
{
	    char tempbuff[1024];

		memset(tempbuff, 0, sizeof(tempbuff)/sizeof(char));
		sprintf(tempbuff, "playlist.sh DelCollPlayList \"%s\"",argv_2);
		int ret = my_system(tempbuff);
		memset(tempbuff, 0, sizeof(tempbuff)/sizeof(char)); 
        return ret;
}

int collplaylist_write(char *tank)
{
	char *reline=NULL;
	FILE *fp =NULL;
	 
	if(NULL != (fp = fopen(tank, "a+")))
	 {
        char current_contentURL[512] = { 0 };
		//reline=WIFIAudio_UciConfig_SearchValueString("xzxwifiaudio.config.current_contentURL");
		cdb_get_str("$current_contentURL",current_contentURL,sizeof(current_contentURL),"");
		if(reline==NULL)
		{			
			return 0;
		}
		printf("reline =======[%s]\n",reline);
		fputs(current_contentURL,fp);
		fputs("\n", fp);
		fclose(fp);
		fp = NULL;
	 }
       return 0;
}



int collplaylist(char *argv_2)
{            
	 		char *coll_playlist_info=NULL;
			WARTI_pMusicList pppmusic;
			WARTI_pMusic pp;
			//char *plm;
            int ret,i;
            int plm;
	       	char *currentplaylistinfo = "/usr/script/playlists/currentPlaylistInfoJson.txt";
			char *tank_one="/usr/script/playlists/tank_one.m3u";
			char *tank_two="/usr/script/playlists/tank_two.m3u";
			char *tank_three="/usr/script/playlists/tank_three.m3u";
			char *tank_four="/usr/script/playlists/tank_four.m3u";
			char *tank_five="/usr/script/playlists/tank_five.m3u";

			char *coll_playlist_info_one = "/usr/script/playlists/one_collPlaylistInfoJson.txt";
			char *coll_playlist_info_two = "/usr/script/playlists/two_collPlaylistInfoJson.txt";
			char *coll_playlist_info_three = "/usr/script/playlists/three_collPlaylistInfoJson.txt";
			char *coll_playlist_info_four = "/usr/script/playlists/four_collPlaylistInfoJson.txt";
			char *coll_playlist_info_five = "/usr/script/playlists/five_collPlaylistInfoJson.txt";

		  if(!strcmp(argv_2,"null")){
			argv_2 = NULL;
			}
	
		   if(argv_2==NULL)
			{
			  return 0;
			}
		    plm=cdb_get_int("$playmode",0);
		  //plm = WIFIAudio_UciConfig_SearchValueString("xzxwifiaudio.config.playmode");
			switch (plm)
				{
				case 
					0:break;
				case 
					3:break;
				case 
					4:break;
				case 
					5:break;
				default:return -1;
				}
									 
		   ret= parse_currentplaylist_to_index();
		   if(ret==0)
		   	  return 0;
		   
		   if(!strcmp(argv_2,"one"))
		   {
		   	  ret=collplaylist_write(tank_one);
			  coll_playlist_info=strdup(coll_playlist_info_one);
		   }
		   else if(!strcmp(argv_2,"two"))
		   	{
		   	  ret=collplaylist_write(tank_two);
			  coll_playlist_info=strdup(coll_playlist_info_two);
		   	}
		   else if(!strcmp(argv_2,"three"))
		   	{
		   	  ret=collplaylist_write(tank_three);
			  coll_playlist_info=strdup(coll_playlist_info_three);
		   	}
		   else if(!strcmp(argv_2,"four"))
		   	{
		   	  ret=collplaylist_write(tank_four);
			  coll_playlist_info=strdup(coll_playlist_info_four);
		   	}
		   else if(!strcmp(argv_2,"five"))
		   	{
		   	  ret=collplaylist_write(tank_five);
			  coll_playlist_info=strdup(coll_playlist_info_five);
		   	}
			
		 
//////////////////////////////////////////////////json.txt
			if((access(currentplaylistinfo,F_OK))==0)
			{

			int a=ret-1;
			pppmusic=getplaylist_parse_json(currentplaylistinfo);  //yes
			if(NULL!=((pppmusic->pMusicList)[a]))
			{
			pp=((pppmusic->pMusicList)[a]);
			json_to_currentplaylist(coll_playlist_info,pp,0);
			}
            free(coll_playlist_info);

//////////////////////////////////////////////////free
			if(NULL != pppmusic->pMusicList)  
			{
			for( i = 0; i<(pppmusic->Num); i++)
			{
			if(NULL != (pppmusic->pMusicList)[i])
			{
			if(i==(a))
			continue;

			json_pMusicFree((pppmusic->pMusicList)[i]);
			}
			}
			free(pppmusic->pMusicList);
			pppmusic->pMusicList = NULL;
			}
			free(pppmusic);
			pppmusic = NULL;
			}
			
		 return ret;
	
}


int main(int argc, char *argv[])
{
 // creatPlayList_running(); //lock
 int montage=-1;;
  
if(!strcmp(argv[1],"getplaylist"))
{
  return 0;
}
else if(!strcmp(argv[1],"loadplaylist"))     /*  Add list  */
{
  montage=loadplaylist(argv[2]);
}
else if(!strcmp(argv[1],"insertplaylist")){    /**/
	
   montage= insertplaylist(argv[2],argv[3],argv[4],argv[5],argv[6]);
   }
else if(!strcmp(argv[1],"insertonesong"))      /*insert one song,the next song in the current song */
{
     montage=insertonesong(argv[2],argv[3],argv[4],argv[5],argv[6]);
}
else if(!strcmp(argv[1],"deleteonesong"))      /*delete one song*/
{
  montage=deleteonesong(argv[2]);
}
else if(!strcmp(argv[1],"replacePlaylist"))   /*replace current list*/
{
  montage=replacePlaylist(argv[2]);
}
else if(!strcmp(argv[1],"loadSongs"))        /*insert list under current song*/
{
  montage=loadSongs(argv[2]);
}
else if(!strcmp(argv[1],"wPlayingSong"))     /*upload the information that is playing songs to UCI*/
{
  montage=wPlayingSong(argv[2]);
}
else if(!strcmp(argv[1],"modifyConf"))       /* Modify play mode in UCI*/
{
 montage=modifyConf(argv[2],argv[3]);
}
else if(!strcmp(argv[1],"tfmode"))         /*parse the TF Card song information and form the JSON file */
{
  montage=tfmode();
}
else if(!strcmp(argv[1],"usbmode"))        /*parse the USB song information and form the JSON file */
{
  montage=usbmode();
}
else if(!strcmp(argv[1],"createUJ"))      /*parse the TF Card song information */
{
  montage=createUJ();
}
else if(!strcmp(argv[1],"createTJ"))      /*parse the USB song information */
{
  montage=createTJ();
}
else if(!strcmp(argv[1],"cuptu"))        /*update TF Card and USB songs*/
{
  montage=cuptu();
}
else if(!strcmp(argv[1],"playTF"))       /*play TF Card*/
{
   montage=playTF(argv[2]);
}
else if(!strcmp(argv[1],"playUSB"))      /*play USB*/
{
  montage=playUSB(argv[2]);
}
else if(!strcmp(argv[1],"wifimode"))    /*Play online songs or local songs on the phone*/
{
  montage=wifimode(argv[2]);
}
else if(!strcmp(argv[1],"reloadplay"))   /* reload list and play  */
{
  montage=reloadplay(argv[2]);
}
else if(!strcmp(argv[1],"alexaOne"))     /* Amazon play interface */
{
  montage=alexaOne(argv[2],argv[3],argv[4],argv[5],argv[6],argv[7]);
}
else if(!strcmp(argv[1],"playcollplaylist"))  /* play Collection Playlist */
{
  montage=playcollplaylist(argv[2]);
}
else if(!strcmp(argv[1],"delcollplaylist"))   /* delete Collection Playlist */
{
  montage=delcollplaylist(argv[2]);
}
else if(!strcmp(argv[1],"collplaylist"))    /*Collect songs to specified list,Note: Post collection first play*/
{
  montage=collplaylist(argv[2]);
}
else if(!strcmp(argv[1],"CdbGetChar"))    /*Collect songs to specified list,Note: Post collection first play*/
{
 printf("argv[2]==%s\n",argv[2]);
 char set_value [64] ={ 0 };
 char value_char[512] ={ 0 }; 
 sprintf(set_value,"$%s",argv[2]);
  printf("set_value==%s\n",set_value);
 cdb_get_str(set_value,value_char,sizeof(value_char),"");
 printf("value_char==%s\n",value_char);
 montage= 0;
}
else if(!strcmp(argv[1],"CdbGetInt"))    /*Collect songs to specified list,Note: Post collection first play*/
{
	char set_value[64]={ 0 }; 
	printf("argv[2]=%s\n",argv[2]);
	 sprintf(set_value, "$%s",argv[2]);
	 printf("set_value==%s\n",set_value);
  int value_int = cdb_get_int(set_value, 0);
   printf("value_int==%d\n",value_int);
   montage= 0;
}
else if(!strcmp(argv[1],"CdbSetInt"))    /*Collect songs to specified list,Note: Post collection first play*/
{
		char set_value[64]={ 0 };
		sprintf(set_value, "$%s",argv[2]);
		 printf("set_value==%s\n",set_value);
	int value =atoi(argv[3]);
	printf("argv[2]=%s,argv[3]=%d\n",argv[2],value);
   cdb_set_int(set_value, value);
   montage= 0;
}else if(!strcmp(argv[1],"CdbSetChar"))    /*Collect songs to specified list,Note: Post collection first play*/
{
	char set_value[64]={ 0 };
	sprintf(set_value, "$%s",argv[2]);
	printf("set_value==%s\n",set_value);
	printf("argv[2]=%s,argv[3]=%s\n",argv[2],argv[3]);
   cdb_set(set_value, argv[3]);
   montage= 0;
}

printf("montage=[%d]\n",montage);
	
if (montage == 0)
{
	printf("\n creatplaylist ok \n");
}
else
{		
	printf("\n creatplaylist error\n");
}

return 0;
}




