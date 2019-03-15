#include <stdio.h>
//#include <json/json.h>
#include <json-c/json.h>
#include <sys/stat.h>
#include <unistd.h>
#include "playlist_info_json.h"

#define SHOW_LINE()   fprintf(stderr,"\033[32mEnter [%s:%s:%d]\033[0m\n",__FILE__, __FUNCTION__, __LINE__);

struct json_object* create_music_content_object(WARTI_pMusic p)
{
	char *ret;
	struct json_object *object;
	
	if(p==NULL)
	{
		PRINTF("传入指向WARTI_Music结构体指针为NULL，出错返回\r\n");
		return NULL;
	}
	object = json_object_new_object();//新建一个空的JSON对相集
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
	
	return object;//这边不是在栈中分配呢,可以直接返回，但是记得使用完需要释放
}

struct json_object* add_music_content_to_json_array(WARTI_pMusicList p,struct json_object *mpobj)
{
	char *ret;
	int i = 0;
	struct json_object *object;
	struct json_object *array;
	
	
	PRINTF("ppppiiiiii num ===%d\n",p->Num);
	if(p==NULL)
	{
		PRINTF("传入指向WARTI_Music结构体指针为NULL，出错返回\r\n");
		return NULL;
	}
	object = json_object_new_object();//新建一个空的JSON对相集

	
	if(p->Num==WIFIAUDIO_REALTIMEINTERFACE_INTERRORCODE)
	{
		json_object_object_add(object,"num", NULL);
	}
	else
	{
		json_object_object_add(object,"num", json_object_new_int((p->Num)+1));
	}
	PRINTF("ooooo num ===%d\n",p->Num);
	array = json_object_new_array();
	PRINTF("mpobj ===%s\n", json_object_to_json_string(mpobj));
	json_object_array_add(array,mpobj);
		for(i = 0;i<(p->Num);i++)
		{
		  json_object_array_add(array,WIFIAudio_RealTimeInterface_pMusicTopJsonObject((p->pMusicList)[i]));
		}

		json_object_object_add(object,"musiclist",array);
	//}
	
	return object;//这边不是在栈中分配呢,可以直接返回，但是记得使用完需要释放
}

struct json_object* add_one_music_content_to_json_array(WARTI_pMusicList p,struct json_object *mpobj)
{
	char *ret;
	int i = 0;
	struct json_object *object;
	struct json_object *array;
	
	PRINTF("pnum ===%d\n",p->Num);
	if(p==NULL)
	{
		PRINTF("传入指向WARTI_Music结构体指针为NULL，出错返回\r\n");
		return NULL;
	}
	object = json_object_new_object();//新建一个空的JSON对相集	
	if(p->Num==WIFIAUDIO_REALTIMEINTERFACE_INTERRORCODE)
	{
		json_object_object_add(object,"num", NULL);
	}
	else		//Edited by lgm 2016-11-18 13:56:24
	{
		json_object_object_add(object,"num", json_object_new_int((p->Num)+1));
	}
		
    array = json_object_new_array();
    char buff[1024];
	int insert_place=0;
    

     FILE *fp =NULL;
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
  

	char *rp=strstr(buff,"[playing] #");
   
    if(rp == NULL){
    	 rp=strstr(buff,"[paused]  #");
    	}
      if(rp != NULL)
	{
	            
		        insert_place = parse_currentplaylist_to_index();
		
		
				for(i = 0;i<insert_place;i++)
				{
					json_object_array_add(array,WIFIAudio_RealTimeInterface_pMusicTopJsonObject((p->pMusicList)[i]));
				}
				json_object_array_add(array,mpobj);
				for(i = insert_place;i<(p->Num);i++)
				{ 
					json_object_array_add(array,WIFIAudio_RealTimeInterface_pMusicTopJsonObject((p->pMusicList)[i]));
				}
	}
	else
	{
	     
			json_object_array_add(array,WIFIAudio_RealTimeInterface_pMusicTopJsonObject((p->pMusicList)[0]));
			json_object_array_add(array,mpobj);
			for(i = 2;i<(p->Num);i++)
			{
				json_object_array_add(array,WIFIAudio_RealTimeInterface_pMusicTopJsonObject((p->pMusicList)[i]));
			}
	}
	
	json_object_object_add(object,"musiclist",array);

	return object;
 }


struct json_object* add_one_musicjson_to_json_array(WARTI_pMusicList p,struct json_object *mpobj,int direction)
{
	char *ret;
	int i = 0;
	struct json_object *object;
	struct json_object *array;


	
	
	
	
	PRINTF("p  num ===%d\n",p->Num);
	if(p==NULL)
	{
		PRINTF("传入指向WARTI_Music结构体指针为NULL，出错返回\r\n");
		return NULL;
	}
	object = json_object_new_object();//新建一个空的JSON对相集	
	if(p->Num==WIFIAUDIO_REALTIMEINTERFACE_INTERRORCODE)
	{
		json_object_object_add(object,"num", NULL);
	}
	else		//Edited by lgm 2016-11-18 13:56:24
	{
		json_object_object_add(object,"num", json_object_new_int((p->Num)+1));
	}
	
	array = json_object_new_array();
	
	if((p->Num)!=0){
	
		if(1==direction)
	  {
		for(i = 0;i<(p->Num);i++)
		{
		json_object_array_add(array,WIFIAudio_RealTimeInterface_pMusicTopJsonObject((p->pMusicList)[i]));
		}
		json_object_array_add(array,mpobj);
	  }
	  else if(0==direction)
	   	{
	   	json_object_array_add(array,mpobj);
		
	   	for(i = 0;i<(p->Num);i++)
		{
		json_object_array_add(array,WIFIAudio_RealTimeInterface_pMusicTopJsonObject((p->pMusicList)[i]));
		}
		
	   	
	   	}
		
	}	
	else
	{
	
			  
			json_object_array_add(array,mpobj);
			
	
	}
	

	json_object_object_add(object,"musiclist",array);
	
	return object;
 }


int insert_music_to_playlistinfo(char *infilepatch,char *outfilepatch,char *artist,char *title, \
                               char *album,char *contentURL,char *coverURL){
   WARTI_pMusicList p=NULL;
   struct json_object *createobj;
	struct stat filestat;
	char filename[128];
	FILE *fp = NULL;
	char *pjsonstring = NULL;
	char *pgetstring = NULL;
	WARTI_pMusicList lp=NULL;
	struct json_object *array;
	PRINTF("infilepatch == %s\n", infilepatch);
	int i = 0,ret;
	struct json_object *addmusicobject;
	WARTI_pMusic mp;
	char *pjsontostring = NULL;
	mp = (WARTI_pMusic)calloc(1,sizeof(WARTI_Music));  //yes
	if(mp==NULL)
	{
		return NULL;
	}
	mp->pArtist = artist;
	mp->pTitle = title;
	mp->pAlbum = album;
	mp->pContentURL = contentURL;
	mp->pCoverURL = coverURL;
		PRINTF("mmmmmp->pArtist = %s\n",mp->pArtist);
	addmusicobject = create_music_content_object(mp);
	PRINTF("addmusicobject ===%s\n", json_object_to_json_string(addmusicobject));
	
	if((access(infilepatch,F_OK))==0){
  lp = getplaylist_parse_json(infilepatch);  //yes
  
	createobj = add_music_content_to_json_array(lp,addmusicobject);
   
	
	//pjsontostring =  json_object_to_json_string(createobj);
  }else {
  	createobj = json_object_new_object();
  	json_object_object_add(createobj,"num",json_object_new_int(1));
  	array = json_object_new_array();
  	json_object_array_add(array,addmusicobject);
  	json_object_object_add(createobj,"musiclist",array);
  	
  	
  	//pjsontostring =  json_object_to_json_string(addmusicobject);
  	}
  	pjsontostring =  json_object_to_json_string(createobj);
   if(NULL != (fp = fopen(outfilepatch, "w+")))
								{
									ret = fwrite(pjsontostring, sizeof(char), strlen(pjsontostring) + 1, fp);
									free(pjsontostring);
									pjsonstring = NULL;				
									fclose(fp);
									fp = NULL;
								}
	  return 0;                              	
                                 	
	}


	int insert_one_music_to_playlistinfo(char *infilepatch,char *outfilepatch,char *artist,char *title, \
                               char *album,char *contentURL,char *coverURL){
   WARTI_pMusicList p=NULL;
   struct json_object *createobj;
	struct stat filestat;
	char filename[128];
	FILE *fp = NULL;
	char *pjsonstring = NULL;
	char *pgetstring = NULL;
	WARTI_pMusicList lp=NULL;
	struct json_object *array;
	PRINTF("infilepatch == %s\n", infilepatch);
	int i = 0,ret;
	struct json_object *addmusicobject;
	WARTI_pMusic mp=NULL;
	char *pjsontostring = NULL;

	
	mp = (WARTI_pMusic)calloc(1,sizeof(WARTI_Music)); //yes
	if(mp==NULL)
	{
		PRINTF("分配空间返回为空，出错返回\r\n");
		return NULL;
	}
	mp->pArtist = artist;
	mp->pTitle = title;
	mp->pAlbum = album;
	mp->pContentURL = contentURL;
	mp->pCoverURL = coverURL;
	
	addmusicobject = create_music_content_object(mp);
	
	PRINTF("addmusicobject=%s\n", json_object_to_json_string(addmusicobject));
	
	if((access(infilepatch,F_OK))==0){
    lp = getplaylist_parse_json(infilepatch);  //yes
	
   
    
	createobj = add_one_music_content_to_json_array(lp,addmusicobject);
	
    
	
  }else {
  
  	createobj = json_object_new_object();
  	json_object_object_add(createobj,"num",json_object_new_int(1));
  	array = json_object_new_array();
  	json_object_array_add(array,addmusicobject);
  	json_object_object_add(createobj,"musiclist",array);
  	//pjsontostring =  json_object_to_json_string(addmusicobject);
  	
  	}
	
  	pjsontostring =  json_object_to_json_string(createobj);
	
	PRINTF("pjsontostring=%s\n",pjsontostring);
	
   if(NULL != (fp = fopen(outfilepatch, "w+")))
								{
								    
									ret = fwrite(pjsontostring, sizeof(char), strlen(pjsontostring) + 1, fp);
                                  
									free(pjsontostring);
									pjsonstring = NULL;				
									fclose(fp);
									fp = NULL;
								}

  
	  return 0;                              	                          	
	}

	
	int add_alexa_music_to_playlist(char *infilepatch,char *outfilepatch,char *artist,char *title, \
                               char *album,char *contentURL,char *coverURL,char *token){
	FILE *fp = NULL;
	char *pjsontostring = NULL;
	WARTI_pMusicList lp;
	struct json_object *array, *createobj,*addmusicobject;
	int i = 0,ret;
	WARTI_pMusic mp;
	mp = (WARTI_pMusic)calloc(1,sizeof(WARTI_Music));  //yes
	if(mp==NULL)
	{
		PRINTF("分配空间返回为空，出错返回\r\n");
		return NULL;
	}
	mp->pArtist = artist;
	mp->pTitle = title;
	mp->pAlbum = album;
	mp->pContentURL = contentURL;
	mp->pCoverURL = coverURL;
	mp->pToken = token;
	addmusicobject = create_music_content_object(mp);
	
  	createobj = json_object_new_object();
  	json_object_object_add(createobj,"num",json_object_new_int(1));
  	json_object_object_add(createobj,"index",json_object_new_int(1));
  	array = json_object_new_array();
  	json_object_array_add(array,addmusicobject);
  	json_object_object_add(createobj,"musiclist",array);
  	pjsontostring =  json_object_to_json_string(createobj);
   if(NULL != (fp = fopen(outfilepatch, "w+")))
								{
									ret = fwrite(pjsontostring, sizeof(char), strlen(pjsontostring) + 1, fp);
									free(pjsontostring);
									pjsontostring = NULL;				
									fclose(fp);
									fp = NULL;
								}							
	  return 0;                              	                               	
	}



 //为了第二个模式，需要向亚马逊txt 添加json数据，前面的不能删除
 // direction  1  为  放到列表后面    0 为放到前面
 int again_add_alexa_music_to_playlist(char *outfilepatch,char *artist,char *title, \
							char *album,char *contentURL,char *coverURL,char *token,int direction){


 
 FILE *fp = NULL;
 char *pjsontostring = NULL;
 WARTI_pMusicList lp;
 struct json_object *array, *createobj,*addmusicobject;
 int i = 0,ret;
 WARTI_pMusic mp;
 mp = (WARTI_pMusic)calloc(1,sizeof(WARTI_Music));  //yes
 if(mp==NULL)
 {
	 PRINTF("分配空间返回为空，出错返回\r\n");
	 return NULL;
 }
 mp->pArtist = artist;
 mp->pTitle = title;
 mp->pAlbum = album;
 mp->pContentURL = contentURL;
 mp->pCoverURL = coverURL;
 mp->pToken=token;

//  1  为  放到列表后面    0 为放到前面
 json_to_currentplaylist(outfilepatch,mp,direction);
				 
 return 0;																 
 }


/*	
int json_freeInformation(WARTI_pMusicList jsoninformation)
	{
		int ret = 0;
		int i;
		
		if(NULL == jsoninformation)
		{
			SHOWERROR("Fatal error1111222\n");
			ret = -1;
		}
		else
		{
			SHOW_LINE();

			if(NULL != jsoninformation->pRet)
			{
			    free(jsoninformation->pRet);
				jsoninformation->pRet = NULL;
			}

			
		for(i=0;i<jsoninformation->Num;i++)
		 {
			if(NULL != jsoninformation->pMusicList)[i]->pArtist)
			{
				free(jsoninformation->pMusicList)[i]->pArtist);
				jsoninformation->pMusicList)[i]->pArtist = NULL;
			}
			if(NULL != jsoninformation->pMusicList)[i]->pTitle)
			{
				free(jsoninformation->pMusicList)[i]->pTitle);
				jsoninformation->pMusicList)[i]->pTitle = NULL;
			}
			if(NULL != jsoninformation->pMusicList)[i]->pAlbum)
			{
				free(jsoninformation->pMusicList)[i]->pAlbum);
				jsoninformation->pMusicList)[i]->pAlbum = NULL;
			}
			if(NULL != jsoninformation->pMusicList)[i]->pCoverUrl)
			{
				free(jsoninformation->pMusicList)[i]->pCoverUrl);
				jsoninformation->pMusicList)[i]->pCoverUrl = NULL;
			}
			
			free(jsoninformation);
			jsoninformation = NULL;
		 }
		}
		
		return ret;
	}

*/
	
	/**************************************************************/
	int json_freeInformation(WARTI_pMusicList ppmusiclist)
	{
		int i;
		int ret = 0;
		
		if(NULL == ppmusiclist)
		{
			PRINTF("Error of parameter : NULL == ppmusiclist or NULL == *ppmusiclist in WIFIAudio_RealTimeInterface_pGetCurrentPlaylistFree\r\n");
			ret = -1;
		}
		else
		{
			if(NULL != ppmusiclist->pRet)
			{
				free(ppmusiclist->pRet);
				ppmusiclist->pRet = NULL;
			}
			if(NULL != ppmusiclist->pMusicList)
			{
				for(i = 0; i<(ppmusiclist->Num); i++)
				{
					if(NULL != ((ppmusiclist->pMusicList)[i]))
					{
						json_pMusicFree((ppmusiclist->pMusicList)[i]);
					}
				}
				free(ppmusiclist->pMusicList);
				ppmusiclist->pMusicList = NULL;
			}
			free(ppmusiclist);
			ppmusiclist = NULL;
		}
			
		return ret;
	}
	
	int json_pMusicFree(WARTI_pMusic ppmusic)
	{
		int ret = 0;
		int i = 0;

		
		if(NULL == ppmusic)
		{
			PRINTF("Error of Parameter : NULL == ppmusic or NULL == *ppmusic in WIFIAudio_RealTimeInterface_pMusicFree\r\n");
			ret = -1;
		}
		else
		{
			
			if(NULL != ppmusic->pArtist)
			{
			   
				
				PRINTF("ppmusic->pArtist==[%p]\n",ppmusic->pArtist);
				PRINTF("ppmusic->pArtist==[%s]\n",ppmusic->pArtist);
				free(ppmusic->pArtist);
				ppmusic->pArtist = NULL;
			}
			
			if(NULL != ppmusic->pTitle)
			{
			   
				
				PRINTF("ppmusic->pTitle==[%p]\n",ppmusic->pTitle);
				PRINTF("ppmusic->pTitle==[%s]\n",ppmusic->pTitle);
				free(ppmusic->pTitle);
				ppmusic->pTitle = NULL;
			}
			
			if(NULL != ppmusic->pAlbum)
			{
			  
			
				PRINTF("ppmusic->pAlbum==[%p]\n",ppmusic->pAlbum);
				PRINTF("ppmusic->pAlbum==[%s]\n",ppmusic->pAlbum);
				free(ppmusic->pAlbum);
				
				ppmusic->pAlbum = NULL;
			
			}

			

			
			if(NULL != ppmusic->pContentURL)
			{
				
				free(ppmusic->pContentURL);
				ppmusic->pContentURL = NULL;
			}
			if(NULL != ppmusic->pToken)
			{
			
				free(ppmusic->pToken);
				ppmusic->pToken = NULL;
			}
		  

			free(ppmusic);
			ppmusic = NULL;

		
		}
		
		return ret;
	}


/********************************************************************/	
	
