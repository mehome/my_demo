#ifndef __GET_FLAC_INFO_H__
#define __GET_FLAC_INFO_H__

/**
***	????ο?:https://xiph.org/flac/format.html
*** https://www.xiph.org/vorbis/doc/v-comment.html
***/

typedef struct {
	unsigned char buf[4];
}flac_flag_t;

typedef struct {
	unsigned char flag;
	unsigned char buf[3];
	unsigned char *meta_data;
}stream_block,*pstream_block;				


/******************************************
 ** 功  能: 获取*pflacpath路径指向flac文件的标题、艺术家、专辑
 ** 返回值: 成功返回WIGMI_pMusicInformation指针，失败返回NULL,格式不正确返回 0x1000
 ******************************************/ 

WIGMI_pMusicInformation WIFIAudio_GetFLACInformation_FLACPathTopFLACInformation(char *pflacpath);
int WIFIAudio_GetFLACInformation_freeFLACInformation(WIGMI_pMusicInformation pflacinformation);


#endif
