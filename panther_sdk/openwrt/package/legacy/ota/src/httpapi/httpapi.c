#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PROGRESS_DIR "/tmp/progress"

int main(int argc,char *argv[])
{
#if 1

	//模拟数据直接进行测试
//	char data[] = "command=UpgradeWiFiAndBluetoothState";

	char *data;
	char Comment[512];
	FILE *fp = NULL;
	char *pStr = NULL;
	int Progress;
	memset(Comment, 0, 512);
	data = getenv("QUERY_STRING");
	if (NULL == data)
		return -1;
	
	if (sscanf(data, "command=%s", Comment) != 1)
	{	
		printf("{ \"ret\":\"FAIL\"}");
		return -1;
	} 
//		printf("command=%s", Comment);

	if(Comment == NULL)
		return -1;
	
	if(strcmp(Comment,"UpgradeWiFiAndBluetoothState") != 0){
		printf("{ \"ret\":\"FAIL\"}");
		return -1;
	}else{
		fp = fopen(PROGRESS_DIR, "r");
		if(NULL == fp){
			printf("File or directory does not exist");
			pStr = NULL;
			return -1;
		}
		pStr = (char *)calloc(1, 10 + 1);
		if(NULL == pStr){
			pStr = NULL;
		}else{
			memset(pStr, 0, 10 + 1);
			if(NULL == fgets(pStr, 10, fp)){
				free(pStr);
				pStr = NULL;
			}
		}
		fclose(fp);
		fp = NULL; 
		
		Progress = atoi(strtok(pStr, " .%%"));
		if(Progress == 99)
			Progress = 100;
		printf("{ \"ret\": \"OK\", \"state\": 4, \"progress\": %d }",Progress);
	}
#endif
	return 0;
}

