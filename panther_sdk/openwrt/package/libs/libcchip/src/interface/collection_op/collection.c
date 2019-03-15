#include <stdio.h>
#include <sys/reboot.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/types.h>
#include <wdk/cdb.h>
#include <sys/stat.h>

#include "collection.h"

int 
add_collectionSong(int keyNum)
{
	int ret=-1;
	char tempbuff[48];
	memset(tempbuff, 0, sizeof(tempbuff));
	sprintf(tempbuff, "Collection %d  /tmp/CollectionListJson.json &", keyNum);
	ret=system(tempbuff);
    return ret;	
}

int 
del_collectionSong(int keyNum)
{
	int ret=-1;
	char tempbuff[48];
	memset(tempbuff, 0, sizeof(tempbuff));
	sprintf(tempbuff, "Collection %d &", keyNum);
	ret=system(tempbuff);
    return ret;	
}

int 
inquiry_collectionSong(int keyNum,char **pjsonstring)
{
	struct stat filestat;
	FILE* fp=NULL;
	int ret=-1;
	char tempbuff[48];
	memset(tempbuff, 0, sizeof(tempbuff));
	sprintf(tempbuff, "/usr/script/playlists/CollectionPlayListInfo0%d.json", keyNum);	
    stat(tempbuff, &filestat);
	if(NULL == (fp = fopen(tempbuff, "r")))
	{   
		ret = -1;
	} else {
		*pjsonstring = (char *)calloc(1, filestat.st_size);
		if(NULL == pjsonstring)
		{
			ret = -1;
		} else {
			ret = fread(*pjsonstring, sizeof(char), filestat.st_size, fp);
		}
		fclose(fp);
		fp = NULL;
		//printf("pjsonstring==[%s]\n",*pjsonstring);
	}
	return ret;
} 
