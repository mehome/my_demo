#include <stdio.h>
#include <unistd.h>
#include <pthread.h>

//avsclient

//unsigned int read_fail_count = 0;
//extern unsigned int read_fail_count;

unsigned int pth_read_num = 240;
unsigned int pth_write_num = 104;
void* test_pthread_read(void *a)
{
      int i;
	  char *son_rstring;
	  int son_rint;
//      printf("pthread read: %d\n", ++pth_read_num);
      for(i = 0; i < 1000; i++){
		  	son_rint = OnGetAVSSigninStatus();
		  	printf("===amazonLogInStatus%d: %d\n", i, son_rint);
#if 0	  	
//		  printf("===>PROGRAM %d:\n", pth_read_num);
      	son_rstring = ReadConfigValueString("clientId");
//	  	if(son_rstring)
//			printf("===>PROGRAM %d< clientID:%s\n", pth_read_num, son_rstring);
      	son_rint = ReadConfigValueInt("vad_Threshold");
//		if(son_rint != -1)
//        	printf("===>PROGRAM %d< vad_threshold:%d\n", pth_read_num, son_rint);
#endif
      }
}


void* test_pthread_write(void *a)
{
//	printf("pthread write: %d\n", ++pth_write_num);
	char byTmp[64] = {0};
	int ret = 0;
	int i;
	for(i = 0; i < 1000; i++){		
	    ret = OnGetAVSSigninStatus();
	    printf("+++amazonLogInStatus%d: %d\n", i, ret);
#if 0		
//		printf("===>PROGRAM %d:\n", pth_write_num);
		sprintf(byTmp, "%s%d_%d",__func__,pth_write_num, i);
		WriteConfigValueString("clientId", byTmp);
		WriteConfigValueInt("vad_Threshold", pth_write_num);
#endif		
	}
}


#define PTHREAD_COUNT 10
#define CHILD_NUM 5

int main(void *argv)
{
		printf("111\n");
    OnSignUPAmazonAlexa();
#if 0
	// 获取亚马逊登录所需要的参数
	LOGIN_INFO_STRUCT* loginfo;
	loginfo = logInInfo_new();
	if (loginfo != NULL){
		printf("pCodeChallengeMethod:%s\n", loginfo->pCodeChallengeMethod);
		printf("pCodeVerifier:%s\n", loginfo->pCodeVerifier);
		printf("pCodeChallenge:%s\n", loginfo->pCodeChallenge);
		printf("pProductId:%s\n", loginfo->pProductId);
		printf("pDeviceSerialNumber:%s\n", loginfo->pDeviceSerialNumber);
		logInInfo_free(loginfo);
	}
	printf("2222\n");
	// 获取json数据
	printf("endpoint:%s\n", ReadConfigValueString("endpoint"));
#endif

#if 1
	  pthread_t tread[PTHREAD_COUNT], twrite[PTHREAD_COUNT];
	  pid_t pid[CHILD_NUM];
	  pth_write_num = pth_read_num = ReadConfigValueInt("vad_Threshold");
	int i;
//	pthread_setconcurrency(PTHREAD_COUNT * 2);
	for(i = 0; i < PTHREAD_COUNT ; i++){
	  if(pthread_create(&tread[i], NULL,test_pthread_read, NULL) == -1){
	    printf("pthread %d create error!\n", i);
	  }
	  if(pthread_create(&twrite[i], NULL,test_pthread_write, NULL) == -1){
	    printf("pthread %d create error!\n", i);
	  }
	}
    #if 0
  if(pthread_create(&t1, NULL,test_pthread, NULL) == -1){
    printf("pthread %d create error!\n", pth_num);
  }

  if(pthread_create(&t2, NULL,test_pthread, NULL) == -1){
    printf("pthread %d create error!\n", pth_num);
  }
  if(pthread_create(&t3, NULL,test_pthread, NULL) == -1){
    printf("pthread %d create error!\n", pth_num);
  }
  if(pthread_create(&t4, NULL,test_pthread, NULL) == -1){
    printf("pthread %d create error!\n", pth_num);
  }
  if(pthread_create(&t5, NULL,test_pthread, NULL) == -1){
    printf("pthread %d create error!\n", pth_num);
  }
  #endif
  for(i = 0 ; i < PTHREAD_COUNT; i++){
	  if (pthread_join(tread[i], NULL))
	   {
	        printf("thread is not exit...\n");
	        return -2;
	   }
	  if (pthread_join(twrite[i], NULL))
	   {
	        printf("thread is not exit...\n");
	        return -2;
	   }
 	}
 #if 0
  if (pthread_join(t2, NULL))
   {
        printf("thread is not exit...\n");
        return -2;
   }
  if (pthread_join(t3, NULL))
   {
        printf("thread is not exit...\n");
        return -2;
   }
  if (pthread_join(t4, NULL))
   {
        printf("thread is not exit...\n");
        return -2;
   }
  if (pthread_join(t5, NULL))
   {
        printf("thread is not exit...\n");
        return -2;
   }
#endif
#endif

#if 0
  char *son_rstring;
  char *fat_rstring;
  int son_rint;
  int fat_rint;
 #if 1
 int j;
 for(j = 0; j < CHILD_NUM; j++){
  pid[j] = fork();

  if (pid[j] < 0)
        printf("========> error in fork!");
  else if (pid[j] == 0) {
 #endif
      int i;
      char byTmp[64] = {0};
      for(i = 0; i < 10; i++){
        sprintf(byTmp, "SON%d", i);
        printf("\n\n--------------分界�? SON----------------\n========>progress %s:\n", byTmp);
      	son_rstring = ReadConfigValueString("clientId");
        printf("========>clientID:%s\n", son_rstring);
      	son_rint = ReadConfigValueInt("vad_Threshold");
        printf("========>vad_threshold:%d\n", son_rint);
	    // 设置json数据
      	WriteConfigValueString("clientId", byTmp);
      	WriteConfigValueInt("vad_Threshold", i);
		printf("=====================结束�? SON====================\n\n");
      }
#if 1
  } else{
      int i;
      char byTmp[64] = {0};
      for(i = 0; i < 10; i++){
        sprintf(byTmp, "FATHER%d", i);
        printf("\n\n分界�? FATHER--------------------------------------\n------>progress %s:\n", byTmp);
      	son_rstring = ReadConfigValueString("clientId");
        printf("------>clientID:%s\n", son_rstring);
      	son_rint = ReadConfigValueInt("vad_Threshold");
        printf("------>vad_threshold:%d\n", son_rint);
	    // 设置json数据
      	WriteConfigValueString("clientId", byTmp);
      	WriteConfigValueInt("vad_Threshold", i);
		printf("结束�? FATHER=========================================\n\n");
		waitpid(pid, NULL, 0);
      }
  	}
 }
 #endif
#endif
//	printf("read file failed times: %d\n", read_fail_count);
	// 发送数据到alexa
//	OnWriteMsgToAlexa("abcd");

//	OnSendSignInInfo("012", "123", "456", "789");
	return 0;
}   
                        
