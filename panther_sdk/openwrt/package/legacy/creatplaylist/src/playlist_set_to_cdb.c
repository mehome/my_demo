
#include <stdio.h>
#include <string.h>
#include "uciConfig.h"
#include "playlist_set_to_cdb.h"
int del_str_line(char *str)
{
  char ch;
    while('\n' != *str && *str)
    {
    ++str;
    }
    *str = '\0';
    return 0;
}

int playlist_set_to_cdb(char *cdb_playlist)
{
	FILE *fp;
	char buf[512], *line;
	int ret = 1;
	char cdb_buf[4096];

	fp = fopen(cdb_playlist, "rb");
	
	if( fp ) {
		line = fgets(buf, sizeof(buf), fp);
		del_str_line(line);
		strncat(cdb_buf,line,sizeof(buf));
		strncat(cdb_buf,",",sizeof(char));
		while ((fgets(line, sizeof(buf), fp)) != NULL) 
		{
			  if(strlen(line)!=1){  
				   // snprintf(buf2, sizeof(buf),"%s", line);  //每一行连接成串，
				   del_str_line(line);
				   strncat(cdb_buf,line,sizeof(buf));
				   strncat(cdb_buf,",",sizeof(char));
				   PRINTF("cdb_buf = %s\n",cdb_buf);
				  }
		}
		fclose(fp);
	}
	cdb_set("$wifi_play_list", cdb_buf);
	//cdb_commit();
	return 0;
}

int json_playlist_set_to_cdb(char *json_playlist)
{
	FILE *fp;
	char tmp[4096*10];
	int ret = 1;

	fp = fopen(json_playlist, "rb");
	
	if( fp ) {
		fread(tmp, sizeof(tmp), 1, fp);  
		
		fclose(fp);
	}
	PRINTF("tmp = %s\n",tmp);
	cdb_set("$wifi_play_list_json", tmp);
//	cdb_commit();
	return 0;
}
