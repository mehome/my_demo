#include <stdio.h>
#include <stdlib.h>
#include <string.h> 
#include "getMusicInfo.h"
#include "getFLACInfo.h"

static unsigned int get_flac_tag_size(unsigned char buf[])
{
	return (buf[0]<<16) | (buf[1]<<8) | buf[2];
}

//二进制比较，不区分大小写
//不等就返回 -1；相等 返回0
static int bin_case_cmp(unsigned char *dst,char *src,int n)
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


/*
注释部分的编码格式如下:(小端存储)
  向量长度		 内容		 注释个数		第一个注释长度	 第一个注释的内容  	第二个注释长度	 第二个注释的内容  
00 00 00 00    xxxxxxxx     00 00 00 00      00 00 00 00      title=xxxxxxx       00 00 00 00      artist=xxxxx

*/
static int parse_misc_tags(WIGMI_pMusicInformation pflacinfo,unsigned char buf[],int size)
{	
	int comment_size = 0;
	int i = 0;
	int counter = 0;

	for(i = 0;i < size && counter < 3 ;i++)
	{
		if(0 == bin_case_cmp(&buf[i],"title=",6))
		{
			comment_size = get_item_size_little(&buf[i-4]);
			if(comment_size > 256 || comment_size < 0 )
			{
				continue;
			}
			pflacinfo->pTitle = (unsigned char *)calloc(1,comment_size-6+1);   //yes
			if(NULL == pflacinfo->pTitle)
			{
				SHOWERROR("Fatal error\n");
				return -1;
			}
			copy_data_to_array(pflacinfo->pTitle,&buf[i+6],comment_size-6);
			counter++;
		}
		else if(0 == bin_case_cmp(&buf[i],"artist=",7))
		{
			comment_size = get_item_size_little(&buf[i-4]);
			if(comment_size > 256 || comment_size < 0 )
			{
				continue;
			}
			pflacinfo->pArtist= (unsigned char *)calloc(1,comment_size-7+1);  //yes
			if(NULL == pflacinfo->pArtist)
			{
				SHOWERROR("Fatal error\n");
				return -2;
			}
			copy_data_to_array(pflacinfo->pArtist,&buf[i+7],comment_size-7);
			counter++;
		}
		else if(0 == bin_case_cmp(&buf[i],"album=",6))
		{
			comment_size = get_item_size_little(&buf[i-4]);
			if(comment_size > 256 || comment_size < 0 )
			{
				continue;
			}
			pflacinfo->pAlbum= (unsigned char *)calloc(1,comment_size-6+1);   //yes
			if(NULL == pflacinfo->pAlbum)
			{
				SHOWERROR("Fatal error\n");
				return -3;
			}
			copy_data_to_array(pflacinfo->pAlbum,&buf[i+6],comment_size-6);
			counter++;
		}
	}
	
	return 0;
}


WIGMI_pMusicInformation WIFIAudio_GetFLACInformation_FLACPathTopFLACInformation(char *pflacpath)
{
	FILE *fpflac = NULL;
	flac_flag_t flac;
	int nread = 0;
	WIGMI_pMusicInformation pflacinformation = NULL;
	pstream_block pstream = NULL;
	int block_size = 0;
	int find_all_flag = 0;
	int ret = 0;
	unsigned char buff[10]={0};
	int tag_size = 0;
	
	if(NULL == pflacpath)
	{
		SHOWERROR("Fatal error\n");
		return NULL;
	}
	if(NULL == strstr(pflacpath,"flac") && NULL == strstr(pflacpath,"FLAC"))
	{
		SHOWERROR(" This file seems not to be a flac file\n");
		return (WIGMI_pMusicInformation)0x1000;
	}
	
	fpflac = fopen(pflacpath,"rb");
	if(NULL == fpflac)
	{
		SHOWERROR("Fatal error\n");
		return NULL;
	}
	pflacinformation = (WIGMI_pMusicInformation)calloc(1, sizeof(WIGMI_MusicInformation));  //yes
	if(NULL == pflacinformation)
	{
		SHOWERROR("Fatal error\n");
		goto CALLOC_ERROR;
	}
	if(1 == check_if_have_id3(fpflac))
	{
		SHOWDEBUG("Invalid ID3 tag found\n");
	}
	
	memset(flac.buf,0,4);
	nread = fread(flac.buf,1,4,fpflac);
	if(nread != 4)
	{
		SHOWERROR("Fatal error\n");
		goto READ_ERROR;
	}	
	if(memcmp(flac.buf,"fLaC",4))
	{	
		SHOWERROR("This is not a flac file\n");
		goto READ_ERROR;
	}

	pstream = (pstream_block)calloc(1,sizeof(stream_block)); //yes
	if(NULL == pstream)
	{
		SHOWERROR("Fatal error\n");
		goto READ_ERROR;
	}
	
	nread = fread(&(pstream->flag),1,1,fpflac);
	if(nread != 1)
	{
		SHOWERROR("Fatal error\n");
		goto READ_TAG_ERROR;
	}	
	nread = fread(pstream->buf,1,3,fpflac);
	if(nread != 3)
	{
		SHOWERROR("Fatal error\n");
		goto READ_TAG_ERROR;
	}	
	while(pstream->flag != 1 && find_all_flag < 2 )
	{				
		block_size = get_flac_tag_size(pstream->buf);
		if(block_size <= 0)
		{
			SHOWDEBUG("No more tag found\n");
			break;
		}
		SHOWDEBUG("flag = %03d,block_size = %06X,find_all_flag = %d\n",pstream->flag,block_size,find_all_flag);
		switch(pstream->flag)
		{
			case 0:
			case 1:
			case 2:
			case 3:		
			case 5:
			case 6:	
				fseek(fpflac,block_size,SEEK_CUR);
				break;	
			case 4:
				pstream->meta_data=(unsigned char *)calloc(1,block_size+1);   //yes
				if(NULL == pstream->meta_data)
				{	
					SHOWERROR("Fatal error\n");
					goto READ_TAG_ERROR;
				}
				nread = fread(pstream->meta_data,1,block_size,fpflac);
				if(nread != block_size)
				{
					SHOWERROR("Fatal error\n");
					goto READ_TAG_ERROR;
				}	
				parse_misc_tags(pflacinformation,pstream->meta_data,block_size);
				free(pstream->meta_data);
				find_all_flag ++;
				break;
		}
		nread = fread(&(pstream->flag),1,1,fpflac);
		if(nread != 1)
		{
			SHOWERROR("Fatal error\n");
			goto READ_TAG_ERROR;
		}	
		nread = fread(pstream->buf,1,3,fpflac);
		if(nread != 3)
		{
			SHOWERROR("Fatal error\n");
			goto READ_TAG_ERROR;
		}	
	}
	

	free(pstream);
	fclose(fpflac);
	return pflacinformation;
	
READ_TAG_ERROR:
	free(pstream);
READ_ERROR:
	free(pflacinformation);
CALLOC_ERROR:
	fclose(fpflac);

	return NULL;
}




int WIFIAudio_GetFLACInformation_freeFLACInformation(WIGMI_pMusicInformation pflacinformation)
{
	int ret = 0;
	
	if(NULL == pflacinformation)
	{
		SHOWERROR("Fatal error\n");
		ret = -1;
	}
	else
	{
		SHOW_LINE();
		if(NULL != pflacinformation->pTitle)
		{
			free(pflacinformation->pTitle);
			pflacinformation->pTitle = NULL;
		}
		if(NULL != pflacinformation->pArtist)
		{
			free(pflacinformation->pArtist);
			pflacinformation->pArtist = NULL;
		}
		if(NULL != pflacinformation->pAlbum)
		{
			free(pflacinformation->pAlbum);
			pflacinformation->pAlbum = NULL;
		}
		
		free(pflacinformation);
		pflacinformation = NULL;
	}
	printf("flac.....................................\n");
	return ret;
}


