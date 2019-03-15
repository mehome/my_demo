#include <stdio.h>
#include <stdlib.h>
#include <string.h> 
#include "getMusicInfo.h"
#include "getAPEInfo.h"

static unsigned int get_apev_tag_length(unsigned char *buf)
{
	return  (*(buf-5))<<24 | (*(buf-6))<<16 | (*(buf-7))<<8 | (*(buf-8)) ;
}



//二进制比较，不区分大小写
//不等就返回 -1；相等 返回0
static int str_case_cmp(unsigned char *dst,unsigned char *src,int n)
{
	int i = 0;
	for(i = 0; i< n ;i++)
	{
		if((dst[i]==src[i]) || (dst[i]-src[i]==32) || (src[i]-dst[i] == 32))
			continue;
		else
		{
			return -1;
		}
		
		
	}
	SHOW_LINE();
	return 0;
}


static unsigned int * import_ape_info(unsigned char* buf,int size, WIGMI_pMusicInformation info,int length[], int flag)
{
	unsigned char *pSearch = NULL;
	unsigned int len = 0;
	int i = 0,j=0;
	int pic_len = 0;
	char suffix[10];
	int counter = 0;
	
	SHOW_LINE();
	memset(suffix,0,10);
	
	for(i=0;i< size && counter < 3;i++)
	{
		//SHOWDEBUG("i = %d,buf[%d] = %02X,size = %d\n",i,i,buf[i],size);
		if(!str_case_cmp(&buf[i],"Album",5))
		{
			
			len = get_apev_tag_length(&buf[i]);			//记录要解析的数据的长度
			if(len > 256)
			{
				continue;
			}
			SHOWDEBUG("Album len = %d\n",len);
			if(!flag)
			{
				pSearch = &buf[i+6];					//定位到解析数据的头部
				copy_data_to_array(info->pAlbum,pSearch,len);
			}
			else
			{
				length[0] = len;
			}
			i +=( len + 5);
			counter++;
			
		}
		if(!str_case_cmp(&buf[i],"Artist",6))
		{
			len = get_apev_tag_length(&buf[i]);			//记录要解析的数据的长度
			
			if(len > 256)
			{
				continue;
			}
			SHOWDEBUG("Artist len = %d\n",len);
			if(!flag)
			{
				pSearch = &buf[i+7];		//定位到解析数据的头部
				copy_data_to_array(info->pArtist,pSearch,len);
			}
			else
			{
				length[1] = len;
			}
			i +=( len + 6);
			counter++;
		}
		if(!str_case_cmp(&buf[i],"Title",5))
		{
			len = get_apev_tag_length(&buf[i]);	
			if(len > 256)
			{
				continue;
			}
			SHOWDEBUG("Title len = %d\n",len);//记录要解析的数据的长度
			if(!flag)
			{
				pSearch = &buf[i+6];		//定位到解析数据的头部
				copy_data_to_array(info->pTitle,pSearch,len);
			}
			else
			{
				length[2] = len;
			}
			i +=( len + 5);		//直接跳到下一个标签处
			counter++;
		}
	}

	return 0;
}


WIGMI_pMusicInformation WIFIAudio_GetAPEInformation_APEPathTopAPEInformation(char *papepath)
{
	FILE *fpape = NULL;
	int nread = 0;
	unsigned int apev_data_len[3]={0};
	WIGMI_pMusicInformation papeinformation = NULL;
	int block_size = 0;
	int find_all_flag = 0;
	ape_flag_t ape;
	unsigned char *buf = NULL;
	int version = 0;
	int len = 0;
	int tag_size = 0;
	
	
	if(NULL == papepath)
	{
		SHOWERROR("Fatal error\n");
		return NULL;
	}
	if(NULL == strstr(papepath,"ape") && NULL == strstr(papepath,"APE"))
	{
		SHOWERROR(" This file seems not to be a ape file\n");
		return (WIGMI_pMusicInformation)0x1000;
	}
	
	fpape = fopen(papepath,"rb");
	if(NULL == fpape)
	{
		SHOWERROR("Fatal error\n");
		return NULL;
	}
	if(1 == check_if_have_id3(fpape))
	{
		SHOWDEBUG("Invalid ID3 tag found\n");
	}
	
	papeinformation = (WIGMI_pMusicInformation)calloc(1, sizeof(WIGMI_MusicInformation)); //yes
	if(NULL == papeinformation)
	{
		SHOWERROR("Fatal error\n");
		goto CALLOC_ERROR;
	}
	
	memset(ape.buf,0,4);
	nread = fread(ape.buf,1,4,fpape);
	if(nread != 4)
	{
		SHOWERROR("Fatal error\n");
		goto READ_ERROR;
	}	
	if(memcmp(ape.buf,"MAC ",4))
	{	
		SHOWERROR("This is not a ape file\n");
		goto READ_ERROR;
	}
	buf = (unsigned char *)calloc(1,32);  //yes
	if(NULL == buf)
	{
		SHOWERROR("Fatal error\n");
		goto READ_ERROR;
	}

	fseek(fpape,-32,SEEK_END);
	nread = fread(buf,1,32,fpape);
	if(32 != nread)
	{
		SHOWERROR("Fatal error\n");
		goto READ_ERROR;
	}

	if(strncmp(buf,"APETAGEX",8))
	{
		SHOWERROR("No ape flag in this file\n");
		free(buf);
		goto READ_ERROR;
	}
	version = (buf[9]<<8) + buf[8]; 								//版本号
	len = (buf[15]<<24) + (buf[14]<<16) + (buf[13]<<8) + buf[12];
	
	SHOWDEBUG("APEV version [0x%X]=[%d]+[%d]	len = 0x%X\n",version,buf[9]<<8,buf[8],len);

	fseek(fpape,-len,SEEK_END); 		//定位到apev数据开始处

	free(buf);
	SHOW_LINE();
	buf = (unsigned char *)calloc(1,len+1);   //yes
	if(NULL == buf)
	{
		SHOWERROR("Fatal error\n");
		goto READ_ERROR;
	}
	nread = fread(buf,1,len,fpape);
	if(len != nread)
	{
		SHOWDEBUG("nread = %d\n",nread);
		SHOWERROR("Fatal error\n");
		goto READ_ERROR;
	}
	SHOW_LINE();
	import_ape_info(buf,len,papeinformation,apev_data_len,1);	//先获取标签内容长度
	SHOWDEBUG("apev_data_len = %d %d %d %d\n",apev_data_len[0],apev_data_len[1],apev_data_len[2],apev_data_len[3]);
	papeinformation->pAlbum = (unsigned char *)calloc(1,apev_data_len[0]+1);  //yes
	if(NULL == papeinformation->pAlbum)
	{
		SHOWERROR("Fatal error\n");
		goto READ_ERROR;
	}
	
	papeinformation->pArtist= (unsigned char *)calloc(1,apev_data_len[1]+1);  //yes
	if(NULL == papeinformation->pArtist)
	{
		SHOWERROR("Fatal error\n");
		goto ARTIST_ERROR;
	}
	
	papeinformation->pTitle= (unsigned char *)calloc(1,apev_data_len[2]+1);   //yes
	if(NULL == papeinformation->pTitle)
	{
		SHOWERROR("Fatal error\n");
		goto TITLE_ERROR;
	}
		
	import_ape_info(buf,len,papeinformation,apev_data_len,0);
		
	fclose(fpape);
	free(buf);

	return papeinformation;

TITLE_ERROR:
	free(papeinformation->pArtist);
ARTIST_ERROR:
	free(papeinformation->pAlbum);
READ_ERROR:
	free(papeinformation);
CALLOC_ERROR:
	fclose(fpape);

	return NULL;
}




int WIFIAudio_GetAPEInformation_freeAPEInformation(WIGMI_pMusicInformation papeinformation)
{
	
	int ret = 0;
	
	if(NULL == papeinformation)
	{
		SHOWERROR("Fatal error\n");
		ret = -1;
	}
	else
	{
		SHOW_LINE();
		if(NULL != papeinformation->pTitle)
		{
			free(papeinformation->pTitle);
			papeinformation->pTitle = NULL;
		}
		if(NULL != papeinformation->pArtist)
		{
			free(papeinformation->pArtist);
			papeinformation->pArtist = NULL;
		}
		if(NULL != papeinformation->pAlbum)
		{
			free(papeinformation->pAlbum);
			papeinformation->pAlbum = NULL;
		}

		free(papeinformation);
		papeinformation = NULL;
	}
	printf("ape.....................................\n");
	return ret;
}

