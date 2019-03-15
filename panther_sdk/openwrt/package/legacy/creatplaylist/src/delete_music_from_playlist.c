#include <stdio.h>
//#include <json/json.h>
#include <json-c/json.h>
#include <sys/stat.h>
#include <unistd.h>
#include "playlist_info_json.h"


struct json_object* delete_one_music_content_to_json_array(WARTI_pMusicList p,int num)
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
	/*
	if(p->KeyNum==c)
	{
		json_object_object_add(object,"keynum", NULL);
	}
	else
	{
		json_object_object_add(object,"keynum", json_object_new_int(p->KeyNum));
	}
	*/
	
	if(p->Num==WIFIAUDIO_REALTIMEINTERFACE_INTERRORCODE)
	{
		json_object_object_add(object,"num", NULL);
	}
	else
	{
		json_object_object_add(object,"num", json_object_new_int((p->Num)-1));
	}
		
		PRINTF("num ===%d\n",p->Num);
	/*
	if(p->pMusicList==NULL)
	{
		json_object_object_add(object,"content", NULL);
	}
	else
	{
	*/
		array = json_object_new_array();
	//	json_object_array_add(array,mpobj);
	//	if((p->Num < 30) && (p->Num >1)){
		//	 json_object_array_add(array,WIFIAudio_RealTimeInterface_pMusicTopJsonObject((p->pMusicList)[0]));
		//	 json_object_array_add(array,mpobj);
		    for(i = 0;i<num-1;i++)
		  {
			  json_object_array_add(array,WIFIAudio_RealTimeInterface_pMusicTopJsonObject((p->pMusicList)[i]));
			//虽然JSON对象不是在栈分配，可能需要手动释放，但是这边已经添加到array所以不需要手动释放
		 }
		  for(i = num;i<p->Num;i++)
		  {
			  json_object_array_add(array,WIFIAudio_RealTimeInterface_pMusicTopJsonObject((p->pMusicList)[i]));
		 }
//	}else if(p->Num ==30){
//			 json_object_array_add(array,WIFIAudio_RealTimeInterface_pMusicTopJsonObject((p->pMusicList)[0]));
//			 json_object_array_add(array,mpobj);
	//	    for(i = 2;i<(p->Num-1);i++)
//		  {
	//		  json_object_array_add(array,WIFIAudio_RealTimeInterface_pMusicTopJsonObject((p->pMusicList)[i]));
			//虽然JSON对象不是在栈分配，可能需要手动释放，但是这边已经添加到array所以不需要手动释放
//		 }
//	}
		json_object_object_add(object,"musiclist",array);
	//}
	
	return object;//这边不是在栈中分配呢,可以直接返回，但是记得使用完需要释放
 }



int delete_one_music_to_playlistinfo(char *infilepatch,char *outfilepatch,int num){
   WARTI_pMusicList p;
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
	/**********************cqq改**************************/

	if((access(infilepatch,F_OK))==0){
  lp = getplaylist_parse_json(infilepatch);  //yes
  
	createobj = delete_one_music_content_to_json_array(lp,num);
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