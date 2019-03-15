#include <stdio.h>  
#include <stdlib.h>  
#include <stddef.h>  
#include <string.h>  
#include "getMusicInfo.h"


// 定义用到的ATOM的ID
typedef enum CMP4TAGATOM_ID
{
	CMP4TAGATOM_ERROR = 0,     // 初始化值
	CMP4TAGATOM_ALBUM,         // 专辑
	CMP4TAGATOM_ARTIST,        // 艺术家
	CMP4TAGATOM_NAME,          // 名称
	CMP4TAGATOM_DATE,          // 日期
	CMP4TAGATOM_GENRE,         // 流派
	CMP4TAGATOM_COVER,         // 封面
}atom_e;


WIGMI_pMusicInformation WIFIAudio_GetM4AInformation_M4APathTopM4AInformation(char *filename)
{
	FILE *pMp4File = NULL;
	unsigned char cBuf[4] = {0};
	atom_e currentAtom = CMP4TAGATOM_ERROR;
	WIGMI_pMusicInformation pm4ainformation = NULL;
	unsigned int tag_len = 0;
	long lRealBytes = 0;
	unsigned char* pRealBuf = NULL;
	unsigned char tag_buf[4]={0};
	int counter = 0;
	int tag_size = 0;

	if(NULL == filename)
	{
		SHOWERROR("Fatal error\n");
		return NULL;
	}
	if(NULL == strstr(filename,"m4a") && NULL == strstr(filename,"M4A"))
	{
		SHOWERROR(" This file seems not to be a m4a file\n");
		return (WIGMI_pMusicInformation)0x1000;
	}
	pMp4File = fopen(filename, "rb");
	if (NULL == pMp4File)
	{	
		SHOWERROR("Fatal error\n");
		return NULL;
	}
	if(1 == check_if_have_id3(pMp4File))
	{
		SHOWDEBUG("Invalid ID3 tag found\n");
	}

	pm4ainformation = (WIGMI_pMusicInformation)calloc(1, sizeof(WIGMI_MusicInformation));
	if(NULL == pm4ainformation)
	{
		SHOWERROR("Fatal error\n");
		goto MALLOC_ERROR;
	}
	if (fread(tag_buf, 4, 1, pMp4File) != 1)
	{		
		SHOWERROR("Fatal error\n");
		goto READ_ERROR;
	}
	if (fread(cBuf, sizeof(cBuf), 1, pMp4File) != 1)
	{
		SHOWERROR("Fatal error\n");
		goto READ_ERROR;
	}
	if(memcmp(cBuf,"ftyp",4) != 0)
	{		
		SHOWERROR("This is not a standard m4a file\n");
		goto READ_ERROR;
	}

	// 把文件指针移动到ATOM开始的位置
	tag_len = get_item_size_big(tag_buf);
	fseek(pMp4File, tag_len-8, SEEK_CUR);
	memset(tag_buf, 0, sizeof(tag_buf));
	memset(cBuf, 0, sizeof(cBuf));

	//读取box的大小和标签名称
	while ((1 == fread(tag_buf, 4, 1, pMp4File)) && (1 ==fread(cBuf, 4, 1, pMp4File)) && counter < 4)
	{
		SHOW_LINE();
		currentAtom = CMP4TAGATOM_ERROR;
		SHOWDEBUG("buf = %02X %02X %02X %02X\n",tag_buf[0],tag_buf[1],tag_buf[2],tag_buf[3]);
		SHOWDEBUG("cbuf = %02X%02X%02X%02X,%c%c%c%c\n",cBuf[0],cBuf[1],cBuf[2],cBuf[3],cBuf[0],cBuf[1],cBuf[2],cBuf[3]);
		// 这些ATOM之后紧跟着是所属子层次的ATOM，不需要移动文件指针
		if (   (cBuf[0]=='m' && cBuf[1]=='o' && cBuf[2]=='o' && cBuf[3]=='v') // moov
			|| (cBuf[0]=='t' && cBuf[1]=='r' && cBuf[2]=='a' && cBuf[3]=='k') // trak
			|| (cBuf[0]=='m' && cBuf[1]=='d' && cBuf[2]=='i' && cBuf[3]=='a') // mdia
			|| (cBuf[0]=='m' && cBuf[1]=='i' && cBuf[2]=='n' && cBuf[3]=='f') // minf
			|| (cBuf[0]=='s' && cBuf[1]=='t' && cBuf[2]=='b' && cBuf[3]=='l') // stbl
			|| (cBuf[0]=='u' && cBuf[1]=='d' && cBuf[2]=='t' && cBuf[3]=='a') // udta
			|| (cBuf[0]=='i' && cBuf[1]=='l' && cBuf[2]=='s' && cBuf[3]=='t') // ilst，TAG信息都在这个ATOM之下
			)
		{			
			SHOW_LINE();
			continue;
		}
		// 此ATOM之后的4个字节是大小需要向后移动4个字节的文件指针
		else if (0 == memcmp(cBuf,"meta",4))
		{			
			SHOW_LINE();
			fseek(pMp4File, 4, SEEK_CUR);
			continue;
		}			
		// 解析专辑、艺术家、名称，这些第一个字节值为0xA9
		else if (cBuf[0] == 0xA9)
		{	
			SHOW_LINE();
			// 专辑
			if (0 == memcmp(&cBuf[1],"alb",3))
			{
				currentAtom = CMP4TAGATOM_ALBUM;
				counter++;
			}
			// 艺术家
			else if (0 == memcmp(&cBuf[1],"ART",3))
			{
				currentAtom = CMP4TAGATOM_ARTIST;
				counter++;
			}
			// 名称
			else if (0 == memcmp(&cBuf[1],"nam",3))
			{	
				currentAtom = CMP4TAGATOM_NAME;
				counter++;
			}
		}
		// 解析封面图片
#ifdef PARSE_DURATION		
		else if (0 == memcmp(cBuf,"mvhd",4))	//获取时长
		{
			unsigned char tmp[4] = {0};
			int time_scale = 0;
			int duration = 0;
			fseek(pMp4File,12,SEEK_CUR);		//定位
			fread(tmp,1,4,pMp4File);
			time_scale = get_item_size_big(tmp);
			fread(tmp,1,4,pMp4File);
			duration = get_item_size_big(tmp);
				
			pm4ainformation->Time = duration/time_scale;
			fseek(pMp4File,-12,SEEK_CUR);
		}
#endif
		// 是需要解析的ATOM
		if ( CMP4TAGATOM_ERROR == currentAtom)
		{
			SHOWDEBUG("Can not parse this tag\n");
			tag_len = get_item_size_big(tag_buf);
			SHOWDEBUG("tag_len = %d\n",tag_len);
			fseek(pMp4File, tag_len-8, SEEK_CUR);
			continue;
		}
		if (fread(tag_buf, 4, 1, pMp4File) != 1 || fread(cBuf, sizeof(cBuf), 1, pMp4File) != 1)
		{
			break;
		}
		// 计算实际数据长度
		lRealBytes = get_item_size_big(tag_buf) - 16;
		SHOW_LINE();
		SHOWDEBUG("buf = %02X %02X %02X %02X\n",tag_buf[0],tag_buf[1],tag_buf[2],tag_buf[3]);
		SHOWDEBUG("cbuf = %02X%02X%02X%02X,%c%c%c%c\n",cBuf[0],cBuf[1],cBuf[2],cBuf[3],cBuf[0],cBuf[1],cBuf[2],cBuf[3]);

		// 判断长度及标识符是否正确
		if ( 0 == memcmp(cBuf,"data",4)&& lRealBytes > 0)
		{
			SHOW_LINE();
			// 当前文件指针位于ver开始处，向后移动8个字节到实际数据处
			fseek(pMp4File, 8, SEEK_CUR);
			
			pRealBuf = (unsigned char *)calloc(1,lRealBytes+1);
			if (pRealBuf == NULL)
			{
				break;
			}
			// 读取实际数据
			if (fread(pRealBuf, lRealBytes, 1, pMp4File) != 1)
			{
				free( pRealBuf);
				pRealBuf = NULL;
				lRealBytes = 0;
				break;
			}
			switch(currentAtom)
			{
				case CMP4TAGATOM_ALBUM:
					if(lRealBytes > 256)
					{
						break;
					}
					pm4ainformation->pAlbum = (unsigned char *)calloc(1,lRealBytes+1);
					if(NULL == pm4ainformation->pAlbum)
						goto ALBUM_ERROR;
					SHOWDEBUG("pRealBuf = [%s]\n",pRealBuf);
					copy_data_to_array(pm4ainformation->pAlbum, pRealBuf,lRealBytes);//专辑
				break;

				case CMP4TAGATOM_ARTIST:
					if(lRealBytes > 256)
					{
						break;
					}
					pm4ainformation->pArtist = (unsigned char *)calloc(1,lRealBytes+1);
					if(NULL == pm4ainformation->pArtist)
						goto ARITIST_ERROR;
					SHOWDEBUG("pRealBuf = [%s]\n",pRealBuf);
					copy_data_to_array(pm4ainformation->pArtist, pRealBuf,lRealBytes);//艺术家
				break;
				
				case CMP4TAGATOM_NAME:
					if(lRealBytes > 256)
					{
						break;
					}
					pm4ainformation->pTitle = (unsigned char *)calloc(1,lRealBytes+1);
					if(NULL == pm4ainformation->pTitle)
						goto TITLE_ERROR;
					SHOWDEBUG("pRealBuf = [%s]\n",pRealBuf);
					copy_data_to_array(pm4ainformation->pTitle, pRealBuf,lRealBytes);//标题
				break;
				
				
			}
			free( pRealBuf);
			pRealBuf = NULL;
			lRealBytes = 0;

			continue;

		}
		// 格式不对，移动文件指针到下一个ATOM开始的位置，基本不会发生这种情况
		else
		{	SHOW_LINE();
			fseek(pMp4File, get_item_size_big(tag_buf)-8-8, SEEK_CUR);
		}
	}
	
		
	return pm4ainformation;
	
TITLE_ERROR:
	free(pm4ainformation->pArtist);	
ARITIST_ERROR:
	free(pm4ainformation->pAlbum );
ALBUM_ERROR:
	free( pRealBuf);
READ_ERROR:
	free(pm4ainformation);
MALLOC_ERROR:
	fclose(pMp4File);

	return pm4ainformation;
}




int WIFIAudio_GetM4AInformation_freeM4AInformation(WIGMI_pMusicInformation ppm4ainformation)
{
	int ret = 0;
	
	if(NULL == ppm4ainformation)
	{
		SHOWERROR("Fatal error\n");
		ret = -1;
	}
	else
	{
		SHOW_LINE();
		if(NULL != ppm4ainformation->pTitle)
		{
			free(ppm4ainformation->pTitle);
			ppm4ainformation->pTitle = NULL;
		}
		if(NULL != ppm4ainformation->pArtist)
		{
			free(ppm4ainformation->pArtist);
			ppm4ainformation->pArtist = NULL;
		}
		if(NULL != ppm4ainformation->pAlbum)
		{
			free(ppm4ainformation->pAlbum);
			ppm4ainformation->pAlbum = NULL;
		}
		
		free(ppm4ainformation);
		ppm4ainformation = NULL;
	}
	printf("m4a.....................................\n");
	return ret;
}

