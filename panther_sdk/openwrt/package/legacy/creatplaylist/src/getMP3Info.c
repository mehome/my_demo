#include <stdio.h>  
#include <stdlib.h>  
#include <stddef.h>  
#include <string.h>  
#include <errno.h>
#include "getMusicInfo.h"

#define PRINTF //

int VS_StrTrim(char*pStr)  //去除空格
{  
    char *pTmp = pStr; 
	int i=0;
      
    while (*pStr != '\0')   
    {  
         
        if (*pStr != ' ')  
        {  
            *pTmp++ = *pStr;
			i++;
        }  
        ++pStr; 
	
    }  
    *pTmp = '\0'; 
	return i;
}  





static int mp3_parse_id3v1(char *path,WIGMI_pMusicInformation info)
{
	FILE *fpmp3 = NULL;
	unsigned char buff[128] = {0};
	int nread = 0;
	int ret = -1;
	unsigned int flag;
	
	unsigned char *fa= " ";
	
PRINTF("b\n");

	if(NULL == path)
	{
		ret = -1;
		SHOWERROR("Fatal error\n");
		goto PATH_ERROR;
	}
	  
	fpmp3 = fopen(path, "rb");
	if (NULL == fpmp3)
	{
		ret = -1;
		SHOWERROR("Fatal error\n");
		goto OPEN_ERROR;
	}
	 
	fseek(fpmp3, -128, SEEK_END);
	//buff = (unsigned char *)malloc(128);
	//if(NULL == buff)
	//{
	//	ret = -1;
	//	SHOWERROR("Fatal error\n");
	//	goto CALLOC_ERROR;
	//}
	
	memset(buff,0,128);
	 
	nread = fread(buff,1,128,fpmp3);
	if(nread != 128)
	{
		ret = -1;
		SHOWERROR("Fatal error\n");
		goto READ_ERROR;
	}
	fclose(fpmp3);
	if(!memcmp("TAG",buff,3))
	{
		SHOWDEBUG("Found ID3V1...................\n");
        SHOWDEBUG("\n\n\n------------ID3V1-------------\n\n\\n");
		  
		unsigned char convert_buff[128];		//代码段用完即释放
		unsigned char tmp[30];
		int size = 0;
		
		//info->pTitle  = (unsigned char *)calloc(1, 30+1);
		//info->pArtist = (unsigned char *)calloc(1, 30+1);
		info->pAlbum  = (unsigned char *)calloc(1, 30+1);
		if(NULL == info->pAlbum)
			{		
			PRINTF("malloc failed!\n");		
			exit(1);   
			}
		
PRINTF("b-1\n");

/********************************************************/
		copy_data_to_array(tmp,&buff[3],30);
		VS_StrTrim(tmp);
		memset(convert_buff,0,128);
		size = gb2312_to_utf8(convert_buff,tmp,strlen(tmp));
		if(size <= 0)
		{
			SHOWERROR("Can not convert title\n");
		}
		
		flag=strcmp(convert_buff,"null");
		flag=strcmp(convert_buff," ");
		
		 if(convert_buff ==NULL || strlen(convert_buff)==0||!flag)
		{
		 
         strcpy(convert_buff, fa);
		}
		info->pTitle = strdup(convert_buff);
		//copy_data_to_array(info->pTitle,convert_buff,size);
		
/********************************************************/	
		copy_data_to_array(tmp,&buff[33],30);
		VS_StrTrim(tmp);
		memset(convert_buff,0,128);		
		size = gb2312_to_utf8(convert_buff,tmp,strlen(tmp));
		
		if(size <= 0)
		{
			SHOWERROR("Can not convert\n");
		}

	   flag=strcmp(convert_buff,"null");
	   flag=strcmp(convert_buff," ");

       if(convert_buff ==NULL || strlen(convert_buff)==0||!flag)
		{
         strcpy(convert_buff, fa);
		}
		
		
		info->pArtist = strdup(convert_buff);
        //copy_data_to_array(info->pArtist,convert_buff,size);
		
		
	
/********************************************************/
PRINTF("b-2\n");

		
		copy_data_to_array(tmp,&buff[63],30);
		VS_StrTrim(tmp);
		memset(convert_buff,0,128);
		size = gb2312_to_utf8(convert_buff,tmp,strlen(tmp));
		if(size <= 0)
		{
			SHOWERROR("Can not convert\n");
		}

        
		flag=strcmp(convert_buff,"null");
		flag=strcmp(convert_buff," ");
		
		if(convert_buff ==NULL || strlen(convert_buff)==0||!flag)
		{
         strcpy(convert_buff, fa);
		// copy_data_to_array(info->pAlbum,convert_buff,sizeof("null"));
		}
		
		
		//info->pAlbum = strdup(convert_buff);
        //copy_data_to_array(info->pAlbum,convert_buff,size);
		strcpy(info->pAlbum,convert_buff);
/********************************************************/		
	
		if(strlen(info->pTitle)>0 || strlen(info->pArtist)> 0 ||strlen(info->pAlbum))
		{
			ret = 1;		//有一个存在就返回TAG的数据，否则将对应字段设为unknown
		}
		
	}
	else
	{
		ret = -1;
	}
        
PRINTF("b-3\n");

	   
READ_ERROR:
CALLOC_ERROR:
OPEN_ERROR:
PATH_ERROR:	

PRINTF("b-6\n");

    
	return ret;
}

//返回值: 0 正常  -1  未找到  

/*
//标签格式
struct id3v2.3_v2.4 
{
	char FrameID[4];         //用三个字符标示一个帧ID，比如 TIT2(歌曲名)，TPE1(艺术家)
	char Size[4];            //帧体内容大小，不包含帧ID，不小于1
	char flag[2];   		//帧体字符所用编码
	char charset;          	//帧体字符
	char *data;
};
*/
static int  mp3_parse_id3v2(unsigned char *path,WIGMI_pMusicInformation pmp3information)
{
	FILE *fpmp3 = NULL;
	int ret = 0;
	unsigned char *buff = NULL;
	unsigned char *frame = NULL;
	unsigned char *utf8 = NULL;
	int nread = 0;
	int id3v2_framelen = 0;
	int cur_frame_len = 0;
	int len = 0,i = 0;
	unsigned char *pPic = NULL;
	unsigned int pic_len = 0;
	unsigned int utf8_len = 0;
	int counter = 0;

	fpmp3 = fopen(path,"rb");
	
	if(NULL == fpmp3)
	{       
		SHOWERROR("Fatal error:%s\n",strerror(errno));
		
		return -1;
	}
	 
	buff = (unsigned char *)calloc(1,10);
	if(NULL == buff)
    {		
        PRINTF("malloc failed!\n");		
        exit(1);   
    }
	
	nread = fread(buff, 1, 10, fpmp3);								//ID3V2前10字节中含有ID3标签头
	if(10 != nread)
	{
		SHOWERROR("Fatal error\n");
		return -1;
	}


	 
	//只解析ID3V2.3和ID3V2.4,不解析ID3V2.2
	if(!memcmp("ID3",buff,3) && (0x03 == buff[3] || 0x04 == buff[3]))
	{
        SHOWERROR("if---------------1\n");
		   
		SHOWDEBUG("Find ID3V2...............\n");
		//计算标签长度，不包括一开始的10个字节，每个字节只用低7位
		id3v2_framelen = (buff[6]<<21) | (buff[7]<<14) | (buff[8]<<7) | buff[9];
		
		len = id3v2_framelen;
		id3v2_framelen = id3v2_framelen + 10;		
		SHOWDEBUG("ID3V2 total length [0x%X]\n", id3v2_framelen);
                
		
		while((len > 0) && (counter < 3))
		{
            
			  
			nread = fread(buff, 1, 10, fpmp3);			//读取一个帧头
			if(10 != nread)
			{
				SHOWERROR("Fatal error\n");
				return -1;
			}
			len = len - 10;
			cur_frame_len = (buff[4]<<24) | (buff[5]<<16) | (buff[6]<<8) | buff[7];	//计算帧内容的长度
			if(cur_frame_len <= 0)
			{
				ret = -1;
				//SHOWERROR("Abnormal end\n");
				break;
			}
			len = len - cur_frame_len;
			if(len <= 0)
			{
				SHOWERROR("Bad value\n");
				break;
			}
			SHOWDEBUG("len = %d,cur = %d\n",len,cur_frame_len);
			
			if(!memcmp("TIT2",buff,4) || !memcmp("TPE1",buff,4) || !memcmp("TALB",buff,4) )		//标题
			{

				
				
				frame = (unsigned char *)calloc(1,cur_frame_len+1);
				if(NULL == frame)
				{
					SHOWERROR("Fatal error\n");
					return -1;
				}
				
				nread = fread(frame,1,cur_frame_len,fpmp3);
				if(cur_frame_len != nread)
				{
					SHOWERROR("Fatal error\n");
					return -1;
				}
				
				
				if(1 == frame[0] || 2 == frame[0])			//Unicode的字符编码
				{
					SHOWDEBUG("Found Unicode.........+++++++++++\n");
					utf8 = (unsigned char *)calloc(1,cur_frame_len*2);	//之所以乘以2，是为了确保装得下转换后的utf8
					if(NULL == utf8)
					{
						SHOWERROR("Fatal error\n");
						return -1;
					}
					memset(utf8,0,cur_frame_len*2);
					
					utf8_len = UnicodeStringToUTF8String(utf8, &frame[1], cur_frame_len - 1);
					//直接将unicode转换为GB2312
					//utf8_len = unicode_to_gb2312(&frame[1],cur_frame_len - 1,utf8);

					if(!memcmp("TIT2",buff,4))
					{
						SHOWDEBUG("Found TIT2\n");
						if(utf8_len > 256 )
						{
							SHOWERROR("Bad value");
							continue;
						}
							
						pmp3information->pTitle = (unsigned char *)calloc(1, utf8_len + 1);
						if(NULL == pmp3information->pTitle)
						{
							WIFIAudio_GetMP3Information_freeMP3Information(pmp3information);
							break;
						}
						else
						{
							
							copy_data_to_array(pmp3information->pTitle, utf8,utf8_len);//标题信息
							counter++;

						}
					}
					else if(!memcmp("TPE1",buff,4))
					{
						SHOWDEBUG("Found TPE1\n");
						if(utf8_len > 256 )
						{
							SHOWERROR("Bad value");
							continue;
						}
						pmp3information->pArtist = (unsigned char *)calloc(1, utf8_len + 1);
						if(NULL == pmp3information->pArtist)
						{
							WIFIAudio_GetMP3Information_freeMP3Information(pmp3information);
							break;
						}
						else
						{
							copy_data_to_array(pmp3information->pArtist, utf8,utf8_len);//艺术家信息
							counter++;
						}
					}
					else if(!memcmp("TALB",buff,4))
					{
						SHOWDEBUG("Found TALB\n");
						if(utf8_len > 256 )
						{
							SHOWERROR("Bad value");
							continue;
						}
						pmp3information->pAlbum = (unsigned char *)calloc(1, utf8_len + 1);
						if(NULL == pmp3information->pAlbum)
						{
							WIFIAudio_GetMP3Information_freeMP3Information(pmp3information);

							break;
						}
						else
						{
							copy_data_to_array(pmp3information->pAlbum, utf8,utf8_len);//专辑信息
							counter++;
						}
					}
					free(utf8);
					free(frame);
				}
				
				else if(3 == frame[0])		//utf-8编码
				{
				        
				
					if(!memcmp("TIT2",buff,4))
					{
						SHOWDEBUG("Found TIT2\n");
						if(cur_frame_len > 256 )
						{
							SHOWERROR("Bad value");
							continue;
						}
						
						pmp3information->pTitle = (unsigned char *)calloc(1, cur_frame_len + 1);
						if(NULL == pmp3information->pTitle)
						{
							WIFIAudio_GetMP3Information_freeMP3Information(pmp3information);

							break;
						}
						else
						{
							copy_data_to_array(pmp3information->pTitle, &frame[1],cur_frame_len-1);//标题信息
							counter++;
						}
					}
					else if(!memcmp("TPE1",buff,4))
					{
						SHOWDEBUG("Found TPE1\n");
						if(cur_frame_len > 256 )
						{
							SHOWERROR("Bad value");
							continue;
						}
						pmp3information->pArtist= (unsigned char *)calloc(1, cur_frame_len + 1);
						if(NULL == pmp3information->pArtist)
						{
							WIFIAudio_GetMP3Information_freeMP3Information(pmp3information);

							break;
						}
						else
						{
							copy_data_to_array(pmp3information->pArtist, &frame[1],cur_frame_len-1);//标题信息
							counter++;
						}
					}
					else if(!memcmp("TALB",buff,4))
					{
						SHOWDEBUG("Found TALB\n");
						if(cur_frame_len > 256 )
						{
							SHOWERROR("Bad value");
							continue;
						}
						pmp3information->pAlbum= (unsigned char *)calloc(1, cur_frame_len + 1);
						if(NULL == pmp3information->pAlbum)
						{
							WIFIAudio_GetMP3Information_freeMP3Information(pmp3information);

							break;
						}
						else
						{
							copy_data_to_array(pmp3information->pAlbum, &frame[1],cur_frame_len-1);//标题信息
							counter++;
						}
					}
					free(frame);
				}

				
				else if(0 == frame[0])	
				{
				    
					unsigned char convert_buff[256];		//代码段用完即释放
					int size = 0;
					
					memset(convert_buff,0,256);
					SHOWDEBUG("Found ISO8559-1..............\n");
					size = gb2312_to_utf8(convert_buff,&frame[1],cur_frame_len-1);
					if(size <= 0)
					{
						SHOWERROR("Can not convert\n");
						continue;
					}

					
					if(!memcmp("TIT2",buff,4))
					{
						SHOWDEBUG("Found TIT2\n");
						if(size > 256 )
						{
							SHOWERROR("Bad value");
							continue;
						}
						pmp3information->pTitle = (unsigned char *)calloc(1, size + 1);
						if(NULL == pmp3information->pTitle)
						{
							WIFIAudio_GetMP3Information_freeMP3Information(pmp3information);
							break;
						}
						else
						{
							copy_data_to_array(pmp3information->pTitle,convert_buff,size);//标题信息
							counter++;
						}
					}
					else if(!memcmp("TPE1",buff,4))
					{
						SHOWDEBUG("Found TPE1\n");
						if(size > 256 )
						{
							SHOWERROR("Bad value");
							continue;
						}
						pmp3information->pArtist= (unsigned char *)calloc(1, size + 1);
						if(NULL == pmp3information->pArtist)
						{
							WIFIAudio_GetMP3Information_freeMP3Information(pmp3information);
							break;
						}
						else
						{
							copy_data_to_array(pmp3information->pArtist, convert_buff,size);//标题信息
							counter++;
						}
					}
					else if(!memcmp("TALB",buff,4))
					{
						SHOWDEBUG("Found TALB\n");
						if(size > 256 )
						{
							SHOWERROR("Bad value");
							continue;
						}
						pmp3information->pAlbum= (unsigned char *)calloc(1, size + 1);
						if(NULL == pmp3information->pAlbum)
						{
							WIFIAudio_GetMP3Information_freeMP3Information(pmp3information);

							break;
						}
						else
						{
							copy_data_to_array(pmp3information->pAlbum, convert_buff,size);//标题信息
							counter++;
						}
					}
				
				}
				
			}
			else
			{  
			   
			       
				SHOWDEBUG("Can not parse this tag,len = %d\n",cur_frame_len);
				if(fseek(fpmp3, cur_frame_len, SEEK_CUR))//不是需要的标签，直接跳过当前这个帧
				{
					SHOWDEBUG("Fatal error\n");
					free(pmp3information);
					pmp3information = NULL;
					break;
				}
			}
		}
		
#ifdef PARSE_DURATION	
		//快速定位到数据处，不再从头开始查找
		if(fseek(fpmp3, id3v2_framelen, SEEK_SET))
		{
			SHOWERROR("Fatal error\n");
			free(pmp3information);
			return -1;
		}
		pmp3information->Time = WIFIAudio_GetMP3Information_GetMP3Time(fpmp3);
#endif

	}
	else
	{
		
		fclose(fpmp3);
		free(buff);
		return -1;
	}

	

	
	fclose(fpmp3);
	free(buff);
	
	return counter;
	
}


//返回值: 0 正常  -1  未找到 
/*
//标签格式
struct id3v22 
{
	char FrameID[3];         //用三个字符标示一个帧ID，比如 TT2(歌曲名)，TP1(艺术家)
	char Size[3];            //帧体内容大小，不包含帧ID，不到小于1
	char FrameCont_encode;   //帧体字符所用编码
	char FrameCont;          //帧体字符
};
*/

//TT2:   标题               
//TP1:   艺术家
//TAL:   专辑名

static int  mp3_parse_id3v22(unsigned char *path,WIGMI_pMusicInformation pmp3information)
{
	FILE *fpmp3 = NULL;
	unsigned char *buff = NULL;
	unsigned char *frame = NULL;
	unsigned char *utf8 = NULL;
	int nread = 0;
	int id3v22_framelen = 0;
	int cur_frame_len = 0;
	int len = 0,i = 0;
	unsigned char *pPic = NULL;
	unsigned int pic_len = 0;
	int utf8_len = 0;
	int counter = 0;

	fpmp3 = fopen(path,"rb");
	if(NULL == fpmp3)
	{
		SHOWERROR("Fatal error\n");
		return -1;
	}
	buff = (unsigned char *)calloc(1,10);
	if(NULL == buff)
			{		
			PRINTF("malloc failed!\n");		
			exit(1);   
			}
	nread = fread(buff, 1, 10, fpmp3);		//ID3V2前10字节中含有ID3标签头
	if(10 != nread)
	{
		SHOWERROR("Fatal error\n");
		return -1;
	}
	if(!memcmp("ID3",buff,3) && buff[3] == 2)
	{
		SHOWDEBUG("Find ID3V2.2..............\n");
		//计算标签长度，不包括一开始的10个字节，每个字节只用低7位
		id3v22_framelen = (buff[6]<<21) | (buff[7]<<14) | (buff[8]<<7) | buff[9];
		
		len = id3v22_framelen;
		id3v22_framelen = id3v22_framelen + 10;		//记录ID3V2.3的帧长度，包括标签头长度10字节
		SHOWDEBUG("ID3V2.2 total length [0x%X]\n", id3v22_framelen);

		
		while((len > 0) && (counter < 3))
		{
			nread = fread(buff, 1, 6, fpmp3);			//读取一个帧头  FrameID(3) + size(3) + charset(1)
			if(6 != nread)
			{
				SHOWERROR("Fatal error\n");
				return -1;
			}
			len = len - 6;
			cur_frame_len = (buff[3]<<16) | (buff[4]<<8) | buff[5];	//计算帧内容的长度
			if(0 >= cur_frame_len)
			{
				break;
			}
			len = len - cur_frame_len;
			if(len < 0 || len >0x100000)
			{	
				break;
			}
			SHOWDEBUG("len = %d,cur = %d\n",len,cur_frame_len);
			if(!memcmp("TT2",buff,3) || !memcmp("TP1",buff,3) || !memcmp("TAL",buff,3) )		
			{
				
				frame = (char *)calloc(1,cur_frame_len+1);
				
				if(NULL == frame)
				{
					SHOWERROR("Fatal error\n");
					return -1;
				}
				nread = fread(frame,1,cur_frame_len,fpmp3);
				if(cur_frame_len != nread)
				{
					SHOWERROR("Fatal error\n");
					return -1;
				}
				
				if(1 == frame[0] || 2 == frame[0])			//Unicode的字符编码
				{
					SHOWDEBUG("Found Unicode.........\n");
					utf8 = (unsigned char *)calloc(1,cur_frame_len*2);	//之所以乘以2，是为了确保装得下转换后的utf8
					if(NULL == utf8)
					{
						SHOWERROR("Fatal error\n");
						return -1;
					}
										
					utf8_len = UnicodeStringToUTF8String(utf8, &frame[1], cur_frame_len - 1);
					if(utf8_len <= 0)
					{
						continue;
					}

					if(!memcmp("TT2",buff,3))
					{
						SHOWDEBUG("Found TT2\n");
						if(cur_frame_len > 256 )
						{
							SHOWERROR("Bad value");
							continue;
						}
						pmp3information->pTitle = (unsigned char *)calloc(1, utf8_len + 1);
						
						if(NULL == pmp3information->pTitle)
						{
							WIFIAudio_GetMP3Information_freeMP3Information(pmp3information);
							break;
						}
						else
						{
							copy_data_to_array(pmp3information->pTitle, utf8,utf8_len);//标题信息
							counter++;

						}
					}
					else if(!memcmp("TP1",buff,3))
					{
						SHOWDEBUG("Found TP1\n");
						if(cur_frame_len > 256 )
						{
							SHOWERROR("Bad value");
							continue;
						}
						pmp3information->pArtist = (unsigned char *)calloc(1, utf8_len + 1);
						if(NULL == pmp3information->pArtist)
						{
							WIFIAudio_GetMP3Information_freeMP3Information(pmp3information);
							break;
						}
						else
						{
							copy_data_to_array(pmp3information->pArtist, utf8,utf8_len);//艺术家信息
							counter++;
						}
					}
					else if(!memcmp("TAL",buff,3))
					{
						SHOWDEBUG("Found TAL\n");
						if(cur_frame_len > 256 )
						{
							SHOWERROR("Bad value");
							continue;
						}
						pmp3information->pAlbum = (unsigned char *)calloc(1, utf8_len + 1);
						if(NULL == pmp3information->pAlbum)
						{
							WIFIAudio_GetMP3Information_freeMP3Information(pmp3information);

							break;
						}
						else
						{
							copy_data_to_array(pmp3information->pAlbum, utf8,utf8_len);//专辑信息
							counter++;
						}
					}
					free(utf8);
				}
				else if(3 == frame[0])		//utf-8编码
				{
					SHOWDEBUG("Found utf-8..............\n");

					if(!memcmp("TT2",buff,3))
					{
						SHOWDEBUG("Found TIT2\n");
						if(cur_frame_len > 256 )
						{
							SHOWERROR("Bad value");
							continue;
						}
						pmp3information->pTitle = (unsigned char *)calloc(1, cur_frame_len + 1);
						if(NULL == pmp3information->pTitle)
						{
							WIFIAudio_GetMP3Information_freeMP3Information(pmp3information);

							break;
						}
						else
						{
							copy_data_to_array(pmp3information->pTitle, &frame[1],cur_frame_len-1);//标题信息
							counter++;
						}
					}
					else if(!memcmp("TP1",buff,3))
					{
						SHOWDEBUG("Found TPE1\n");
						if(cur_frame_len > 256 )
						{
							SHOWERROR("Bad value");
							continue;
						}
						pmp3information->pArtist= (unsigned char *)calloc(1, cur_frame_len + 1);
						if(NULL == pmp3information->pArtist)
						{
							WIFIAudio_GetMP3Information_freeMP3Information(pmp3information);

							break;
						}
						else
						{
							copy_data_to_array(pmp3information->pArtist, &frame[1],cur_frame_len-1);//标题信息
							counter++;
						}
					}
					else if(!memcmp("TAL",buff,3))
					{
						SHOWDEBUG("Found TALB\n");
						if(cur_frame_len > 256 )
						{
							SHOWERROR("Bad value");
							continue;
						}
						pmp3information->pAlbum= (unsigned char *)calloc(1, cur_frame_len + 1);
						if(NULL == pmp3information->pAlbum)
						{
							WIFIAudio_GetMP3Information_freeMP3Information(pmp3information);

							break;
						}
						else
						{
							copy_data_to_array(pmp3information->pAlbum, &frame[1],cur_frame_len-1);//标题信息
							counter++;
						}
					}
					
				}
				else if(0 == frame[0])	
				{
					SHOWDEBUG("Found ISO8559-1..............\n");
					unsigned char convert_buff[256];		//代码段用完即释放
					int size = 0;
					
					memset(convert_buff,0,256);
					SHOWDEBUG("Found ISO8559-1..............\n");
					size = gb2312_to_utf8(convert_buff,&frame[1],cur_frame_len-1);
					if(size <= 0)
					{
						SHOWERROR("Can not convert\n");
						continue;
					}
					
					if(!memcmp("TT2",buff,3))
					{
						SHOWDEBUG("Found TIT2\n");
						if(cur_frame_len > 256 )
						{
							SHOWERROR("Bad value");
							continue;
						}
						pmp3information->pTitle = (unsigned char *)calloc(1, size + 1);
						if(NULL == pmp3information->pTitle)
						{
							WIFIAudio_GetMP3Information_freeMP3Information(pmp3information);

							break;
						}
						else
						{
							copy_data_to_array(pmp3information->pTitle, convert_buff,size);//标题信息
							counter++;
						}
					}
					else if(!memcmp("TP1",buff,3))
					{
						SHOWDEBUG("Found TPE1\n");
						if(cur_frame_len > 256 )
						{
							SHOWERROR("Bad value");
							continue;
						}
						pmp3information->pArtist= (unsigned char *)calloc(1, size + 1);
						if(NULL == pmp3information->pArtist)
						{
							WIFIAudio_GetMP3Information_freeMP3Information(pmp3information);

							break;
						}
						else
						{
							copy_data_to_array(pmp3information->pArtist, convert_buff,size);//标题信息
							counter++;
						}
					}
					else if(!memcmp("TAL",buff,3))
					{
						SHOWDEBUG("Found TALB\n");
						if(cur_frame_len > 256 )
						{
							SHOWERROR("Bad value");
							continue;
						}
						pmp3information->pAlbum= (unsigned char *)calloc(1, size + 1);
						if(NULL == pmp3information->pAlbum)
						{
							WIFIAudio_GetMP3Information_freeMP3Information(pmp3information);

							break;
						}
						else
						{
							copy_data_to_array(pmp3information->pAlbum, convert_buff,size);//标题信息
							counter++;
						}
					}
					
				}

				free(frame);
			}
			else
			{
				SHOWDEBUG("Can not parse this tag,len = %d\n",cur_frame_len);
				if(fseek(fpmp3, cur_frame_len, SEEK_CUR))//不是需要的标签，直接跳过当前这个帧
				{
					SHOWDEBUG("Fatal error\n");
					free(pmp3information);
					pmp3information = NULL;
					break;
				}
			}
		}
#ifdef PARSE_DURATION	
		//快速定位到数据处，不再从头开始查找
		if(fseek(fpmp3, id3v2_framelen, SEEK_SET))
		{
			SHOWERROR("Fatal error\n");
			free(pmp3information);
			return -1;
		}
		pmp3information->Time = WIFIAudio_GetMP3Information_GetMP3Time(fpmp3);
#endif

	}
	else
	{
		fclose(fpmp3);
		free(buff);
		return -1;
	}
	
	fclose(fpmp3);
	free(buff);
	
	return counter;
}


static unsigned int get_apev_data_length(unsigned char *buf)
{
	return  (*(buf-5))<<24 | (*(buf-6))<<16 | (*(buf-7))<<8 | (*(buf-8)) ;
}

static int  import_info(unsigned char* buf,int size, WIGMI_pMusicInformation info,int length[], int flag)
{
	unsigned char *pSearch = NULL;
	unsigned int len = 0;
	int i = 0,j=0;
	int pic_len = 0;
	char suffix[10];
	int counter = 0;
	
	memset(suffix,0,10);
	
	for(i=0;i< size && counter < 4;i++)
	{
		//SHOWDEBUG("i = %d,buf[%d] = %02X,size = %d\n",i,i,buf[i],size);
		if(('A' == buf[i]) && ('l' == buf[i+1]) && ('b' == buf[i+2]) &&('u' == buf[i+3]) &&('m' == buf[i+4]))
		{
			
			len = get_apev_data_length(&buf[i]);			//记录要解析的数据的长度
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
			i +=( len + 14);
			counter++;
		}
		if(('A' == buf[i]) && ('r' == buf[i+1]) && ('t' == buf[i+2]) &&('i' == buf[i+3]) &&('s' == buf[i+4])&&('t' == buf[i+5]))
		{
			len = get_apev_data_length(&buf[i]);			//记录要解析的数据的长度
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
			i +=( len + 15);
			counter++;
		}
		if(('T' == buf[i]) && ('i' == buf[i+1]) && ('t' == buf[i+2]) &&('l' == buf[i+3]) &&('e' == buf[i+4]))
		{
			len = get_apev_data_length(&buf[i]);	
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
			i +=( len + 14);		//直接跳到下一个标签处
			counter++;
		}		
	
	}

	return counter;
}

static int mp3_parse_apev_from_the_end(char *path,WIGMI_pMusicInformation info)
{
	FILE *fpmp3 = NULL;
	unsigned char *buf = NULL;
	unsigned int nread = 0;
	unsigned int version = 0;
	int len = 0,counter = 0;
	unsigned int num = 0,i = 0;
	unsigned int apev_data_len[4]={0};
	char *pSearch = NULL;
	int ret = -1;

	fpmp3 = fopen(path, "rb");
	if (NULL == fpmp3)
	{
		SHOWERROR("Fatal error\n");
		goto OPEN_ERROR;
	}
	buf = (unsigned char *)calloc(1,128);
	if(NULL == buf)
	{
		SHOWERROR("Fatal error\n");
		goto OPEN_ERROR;
	}

	nread = fread(buf,1,128,fpmp3);
	if(128 != nread)
	{
		SHOWERROR("Fatal error\n");
		goto READ_ERROR;
	}
	if(!strncmp("TAG",buf,3))			//有ID3V1存在
	{
		memset(buf,0,128);
		fseek(fpmp3,-160,SEEK_END);		//移动到 APEv头部
		nread = fread(buf,1,32,fpmp3);
		if(32 != nread)
		{
			SHOWERROR("Fatal error\n");
			goto READ_ERROR;
		}
	}
	else
	{
		memset(buf,0,128);
		fseek(fpmp3,-32,SEEK_END);		//移动到 APEv头部
		nread = fread(buf,1,32,fpmp3);
		if(32 != nread)
		{
			SHOWERROR("Fatal error\n");
			goto READ_ERROR;
		}
	}

	if(strncmp(buf,"APETAGEX",8))
	{
		goto READ_ERROR;
	}
	version = (buf[9]<<8) + buf[8];									//版本号
	len = (buf[15]<<24) + (buf[14]<<16) + (buf[13]<<8) + buf[12];
	
	SHOWDEBUG("APEV version [0x%X]=[%d]+[%d]    len = 0x%X\n",version,buf[9]<<8,buf[8],len);

	fseek(fpmp3,-len,SEEK_END);			//定位到apev数据开始处

	free(buf);
	
	buf = (unsigned char *)calloc(1,len+1);
	if(NULL == buf)
	{
		SHOWERROR("Fatal error\n");
		goto READ_ERROR;
	}
	nread = fread(buf,1,len,fpmp3);
	if(len != nread)
	{
		SHOWDEBUG("nread = %d\n",nread);
		SHOWERROR("Fatal error\n");
		goto READ_ERROR;
	}

	counter = import_info(buf,len,info,apev_data_len,1);	//先获取标签内容长度
	SHOWDEBUG("apev_data_len = %d %d %d %d\n",apev_data_len[0],apev_data_len[1],apev_data_len[2],apev_data_len[3]);
	info->pAlbum = (unsigned char *)calloc(1,apev_data_len[0]+1);
	if(NULL == info->pAlbum)
	{
		SHOWERROR("Fatal error\n");
		goto ALBUM_ERROR;
	}
	
	info->pArtist= (unsigned char *)calloc(1,apev_data_len[1]+1);
	if(NULL == info->pArtist)
	{
		SHOWERROR("Fatal error\n");
		goto ARTIST_ERROR;
	}
	
	info->pTitle= (unsigned char *)calloc(1,apev_data_len[2]+1);
	if(NULL == info->pTitle)
	{
		SHOWERROR("Fatal error\n");
		goto TITLE_ERROR;
	}
	
	
	import_info(buf,len,info,apev_data_len,0);
	
#ifdef PARSE_DURATION	
	fseek(fpmp3,0,SEEK_SET);
	info->Time = WIFIAudio_GetMP3Information_GetMP3Time(fpmp3);	
#endif	

	fclose(fpmp3);
	free(buf);

	return counter;
	
TITLE_ERROR:
	free(info->pArtist);
ARTIST_ERROR:
	free(info->pAlbum);
ALBUM_ERROR:
	free(info);	
CALLOC_ERROR:
	fclose(fpmp3);
READ_ERROR:
	free(buf);
OPEN_ERROR:	
	
	return -1;
}


static int mp3_parse_apev_from_the_begin(char *path,WIGMI_pMusicInformation info)
{
	FILE *fpmp3 = NULL;
	unsigned char *buf = NULL;
	int nread = 0;
	int frame_len = 0;
	int apev_data_len[4]={0,0,0,0};
	
	fpmp3 = fopen(path,"rb");
	if(NULL == fpmp3)
	{
		SHOWERROR("Fatal error\n");
		return -1;
	}
	buf = (char *)calloc(1,32);
	if(NULL == buf)
	{
		SHOWERROR("Fatal error\n");
		goto CALLOC_ERROR;
	}
	nread = fread(buf, 1, 32, fpmp3);
	if(32 != nread)
	{
		SHOWERROR("Fatal error\n");
		goto READ_ERROR;
	}
	if(strncmp("APETAGEX",buf,8))
	{
		goto READ_ERROR;	
	}
	frame_len = (buf[15]<<24) |(buf[14]<<16) |(buf[13]<<8) | buf[15];
	free(buf);
	
	buf = (unsigned char *)calloc(1,frame_len);
	if(NULL == buf)
	{
		SHOWERROR("Fatal error\n");
		goto CALLOC_ERROR;
	}
	nread = fread(buf,1,frame_len,fpmp3);
	if(nread != frame_len)
	{
		SHOWERROR("Fatal error\n");
		goto READ_ERROR;
	}
	
	import_info(buf,frame_len,info,apev_data_len,1);	//先获取标签内容长度
	//SHOWDEBUG("apev_data_len = %d %d %d %d\n",apev_data_len[0],apev_data_len[1],apev_data_len[2],apev_data_len[3]);
	info->pAlbum = (unsigned char *)calloc(1,apev_data_len[0]+1);
	if(NULL == info->pAlbum)
	{
		SHOWERROR("Fatal error\n");
		goto READ_ERROR;
	}
	
	info->pArtist= (unsigned char *)calloc(1,apev_data_len[1]+1);
	if(NULL == info->pArtist)
	{
		SHOWERROR("Fatal error\n");
		goto ARTIST_ERROR;
	}
	
	info->pTitle= (unsigned char *)calloc(1,apev_data_len[2]+1);
	if(NULL == info->pTitle)
	{
		SHOWERROR("Fatal error\n");
		goto TITLE_ERROR;
	}	

	import_info(buf,frame_len,info,apev_data_len,0);
	
#ifdef PARSE_DURATION	
	fseek(fpmp3,frame_len+32,SEEK_SET);
	info->Time = WIFIAudio_GetMP3Information_GetMP3Time(fpmp3);	
#endif	

	fclose(fpmp3);
	free(buf);

	return 0;
	
TITLE_ERROR:
	free(info->pArtist);
ARTIST_ERROR:
	free(info->pAlbum); 	
READ_ERROR:
	free(buf);
CALLOC_ERROR:
	fclose(fpmp3);
	
	return -1;
}

static int mp3_return_default_id3(char *path,WIGMI_pMusicInformation pmp3information)
{
	pmp3information->pTitle  = (unsigned char *)calloc(1, 30);
	if(NULL == pmp3information->pTitle)
		        {		
		        PRINTF("malloc failed!\n");	
		        exit(1);   
		        }
	pmp3information->pArtist = (unsigned char *)calloc(1, 30);
	if(NULL == pmp3information->pArtist)
		        {		
		        PRINTF("malloc failed!\n");	
		        exit(1);   
		        }
	pmp3information->pAlbum  = (unsigned char *)calloc(1, 30);
	if(NULL == pmp3information->pAlbum )
		        {		
		        PRINTF("malloc failed!\n");	
		        exit(1);   
		        }


	//在后面  为 "unknown" 作判断  将其改为line, 其他成员为空格
	strcpy(pmp3information->pTitle,"unknown");
	strcpy(pmp3information->pArtist," ");	
	strcpy(pmp3information->pAlbum," ");
	
OPEN_ERROR:

	return 0;
}



WIGMI_pMusicInformation WIFIAudio_GetMP3Information_MP3PathTopMP3Information(unsigned char *pmp3path)
{
	WIGMI_pMusicInformation pmp3information = NULL;
	FILE *fpmp3 = NULL;	
	unsigned char buff[128];
	unsigned char string[64];
	unsigned char name[30];
	int id3v23len = 0;
	int len = 0;
	int framelen = 0;
	int i = 0;
	int j = 0;
	int nread = 0;
	int ret = -1;
	
PRINTF("a\n");
	if(NULL == pmp3path)
	{
		SHOWERROR("Fatal error\n");
		return  NULL;
	}
PRINTF("a-1\n");	 
	if(NULL == strstr(pmp3path,"mp3") && NULL == strstr(pmp3path,"MP3"))
	{
		SHOWERROR(" This file seems not to be a mp3 file\n");
		return (WIGMI_pMusicInformation)0x1000;
	}
PRINTF("a-2\n");	 
	pmp3information = (WIGMI_pMusicInformation)calloc(1, sizeof(WIGMI_MusicInformation));
	if(NULL == pmp3information )
			{		
			PRINTF("malloc failed!\n");		
			exit(1);   
			}
PRINTF("a-3\n");	
	memset(pmp3information,0,sizeof(WIGMI_MusicInformation));
	if(NULL == pmp3information)
	{
		SHOWERROR("Fatal error\n");
		return NULL;
	}
PRINTF("a-4\n");	  
	ret = mp3_parse_id3v2(pmp3path,pmp3information);
	if(ret > 0)
	{
	    SHOWERROR("final----------3.2\n");
		goto FINAL;
	}
	SHOWERROR("1\n");
	SHOWDEBUG("No ID3V2.3 or ID3V2.4 Found\n");
PRINTF("a-5\n");	
	ret = mp3_parse_apev_from_the_begin(pmp3path,pmp3information);
	if(ret > 0)
	{
		goto FINAL;
	}
	SHOWDEBUG("No APEV2 Found from the beginning\n");
PRINTF("a-6\n");	 
	ret = mp3_parse_apev_from_the_end(pmp3path,pmp3information);
	if(ret > 0)
	{
		goto FINAL;
	}
	SHOWDEBUG("No APEV2 Found from the end\n");	
	 
PRINTF("a-7\n");	
	ret = mp3_parse_id3v1(pmp3path,pmp3information);
	if(ret > 0)
	{
		goto FINAL;
	}
	SHOWDEBUG("No ID3V1 Found,Set the default\n");
PRINTF("a-8\n");	 
	ret = mp3_parse_id3v22(pmp3path,pmp3information);		//ID3V2.2用的最少，放在最后来解析
	if(ret > 0)
	{
		goto FINAL;
	}
	SHOWDEBUG("No ID3V2.2 Found\n");
PRINTF("a-9\n");	 
	mp3_return_default_id3(pmp3path,pmp3information);
PRINTF("a-end\n");

FINAL:	
	return pmp3information;
}



int WIFIAudio_GetMP3Information_freeMP3Information(WIGMI_pMusicInformation ppmp3information)
{
	int ret = 0;
	
	
	if(NULL == ppmp3information)
	{
		SHOWERROR("Fatal error\n");
		ret = -1;
	}
	else
	{


                
	    if(!(NULL == ppmp3information->pArtist))
		{
		   
			free(ppmp3information->pArtist);
			ppmp3information->pArtist = NULL;
		}
	        
		if(!(NULL == ppmp3information->pTitle))
		{
		  
			free(ppmp3information->pTitle);
			ppmp3information->pTitle = NULL;
		}
		
		if(!(NULL == ppmp3information->pAlbum))
		{
		  
			free(ppmp3information->pAlbum);
			ppmp3information->pAlbum = NULL;
		}
		
		free(ppmp3information);
		ppmp3information = NULL;
	}
	
	return ret;
}


