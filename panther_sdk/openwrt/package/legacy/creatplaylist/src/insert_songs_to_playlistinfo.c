#include <stdio.h>
//#include <json/json.h>
#include <json-c/json.h>
#include <sys/stat.h>
#include <unistd.h>
#include "playlist_info_json.h"
struct json_object* Insert_Songs_pMusicListInfoTopJsonObject(WARTI_pMusicList inlp,WARTI_pMusicList outlp)
{
	char *ret;
	int i = 0;
	struct json_object *object;
	struct json_object *array;
	int get_playing_number;
	
	PRINTF("inlp->Num num ===%d\n",inlp->Num);
	PRINTF("outlp->Num num ===%d\n",outlp->Num);
	if(inlp==NULL)
	{
		PRINTF("1111111����ָ��WARTI_Music�ṹ��ָ��ΪNULL��������\r\n");
		return NULL;
	}
	object = json_object_new_object();//�½�һ���յ�JSON���༯
	get_playing_number = parse_currentplaylist_to_index();
	if(inlp->Num==WIFIAUDIO_REALTIMEINTERFACE_INTERRORCODE && outlp->Num==WIFIAUDIO_REALTIMEINTERFACE_INTERRORCODE )
	{
		json_object_object_add(object,"num", NULL);
	}
	/*
	else if((inlp->Num + outlp->Num)<30)
	{
		json_object_object_add(object,"num", json_object_new_int(inlp->Num + outlp->Num));
	}else if((inlp->Num + outlp->Num)>30 ||(inlp->Num + outlp->Num)==30)
	{
		json_object_object_add(object,"num", json_object_new_int(30));
	} 
	*/
	json_object_object_add(object,"num", json_object_new_int(inlp->Num + outlp->Num));

	json_object_object_add(object,"index", json_object_new_int(outlp->Index));
	
	if(inlp->pMusicList==NULL && outlp->pMusicList==NULL)
	{
		json_object_object_add(object,"content", NULL);
	}else if(inlp->Num >0 || outlp->Num >0){
		 
		array = json_object_new_array();
		
		 if((outlp->Num)>0)
		 {  for(i = 0;i< get_playing_number;i++)
		   {
		   	 PRINTF("inlp->pMusicList)[i===%d]  %s\n",i,(outlp->pMusicList)[i]->pTitle);
			   json_object_array_add(array,WIFIAudio_RealTimeInterface_pMusicTopJsonObject((outlp->pMusicList)[i]));
			//��ȻJSON��������ջ���䣬������Ҫ�ֶ��ͷţ���������Ѿ���ӵ�array���Բ���Ҫ�ֶ��ͷ�
		  }
	   }
		 if((inlp->Num)>0){
		 	for(i = 0;i<(inlp->Num);i++)
		 	{
			   json_object_array_add(array,WIFIAudio_RealTimeInterface_pMusicTopJsonObject((inlp->pMusicList)[i]));
			   PRINTF("inlp->pMusicList)[i===%d]  %s\n",i,(inlp->pMusicList)[i]->pTitle);
			//��ȻJSON��������ջ���䣬������Ҫ�ֶ��ͷţ���������Ѿ���ӵ�array���Բ���Ҫ�ֶ��ͷ�
		  }
	   }
	   
	   for(i = (get_playing_number);i<(outlp->Num);i++)
		   {
		   	 PRINTF("outlp->Num -----=%d\n",outlp->Num);
		   	 PRINTF("get_playing_number -----=%d\n",get_playing_number);
		   	 PRINTF("outlp->pMusicList)[111111111111------i===%d]  %s\n",i,(outlp->pMusicList)[i]->pTitle);
		   }
	   if((outlp->Num - get_playing_number)>0){
	   	for(i = (get_playing_number);i<(outlp->Num);i++)
		   {
		   	 PRINTF("outlp->Num -----=%d\n",outlp->Num);
		   	 PRINTF("get_playing_number -----=%d\n",get_playing_number);
		   	 PRINTF("outlp->pMusicList)[------i===%d]  %s\n",i,(outlp->pMusicList)[i]->pTitle);
			   json_object_array_add(array,WIFIAudio_RealTimeInterface_pMusicTopJsonObject((outlp->pMusicList)[i]));
			//��ȻJSON��������ջ���䣬������Ҫ�ֶ��ͷţ���������Ѿ���ӵ�array���Բ���Ҫ�ֶ��ͷ�
		  }
	   	PRINTF("out ======\n");
	   	}
	}
	PRINTF("out 222222======\n");
	    char buf[1024];
      // sprintf(buf,"creatPlayList insertplaylist %s %s %s %s %s",argv[2],argv[3],argv[4],argv[5],argv[6]);
      if((inlp->Num)>0){
		 			for(i = 0;i<(inlp->Num);i++)
		 			{  
		 				int j =inlp->Num -i-1;
		 				PRINTF("out 333333======\n");
		 		  	 memset(buf, 0, 1024);
		 		  	 PRINTF("j ==%d\n",j);
			   		 PRINTF("inlp->pMusicList)[%d]= %s\n",j,(inlp->pMusicList)[j]->pContentURL);
			    	 sprintf(buf,"mpc insert %s",(inlp->pMusicList)[j]->pContentURL);
             PRINTF("buf ==%s\n",buf);
             system(buf);
			//��ȻJSON��������ջ���䣬������Ҫ�ֶ��ͷţ���������Ѿ���ӵ�array���Բ���Ҫ�ֶ��ͷ�
		      }
	   }	   
		json_object_object_add(object,"musiclist",array);
	
	return object;//��߲�����ջ�з�����,����ֱ�ӷ��أ����Ǽǵ�ʹ������Ҫ�ͷ�
}
