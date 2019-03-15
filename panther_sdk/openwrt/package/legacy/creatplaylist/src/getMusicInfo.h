#ifndef __WIFIAUDIO_GETMUSICINFORMATION_H__
#define __WIFIAUDIO_GETMUSICINFORMATION_H__

#ifdef __cplusplus
extern "C"
{
#endif


#define SAVE_APIC 		0		//�Ƿ񱣴�ר��ͼƬ��1:���棬0:������
#define DEBUG			0		//�Ƿ��ӡ������Ϣ��1:��ӡ��0:����ӡ
#undef PARSE_DURATION			//�������ļ�ʱ��



#if DEBUG					
#define SHOW_LINE()   fprintf(stderr,"\033[32mEnter [%s:%s:%d]\033[0m\n",__FILE__, __FUNCTION__, __LINE__);
#define DEBUG_LINE()  fprintf(stderr,"\033[33mEnter [%s:%s:%d]\033[0m",__FILE__, __FUNCTION__, __LINE__);
#define ERROR_LINE()  fprintf(stderr,"\033[31mEnter [%s:%s:%d]\033[0m",__FILE__, __FUNCTION__, __LINE__);
#define SHOWDEBUG(fmt, arg...) DEBUG_LINE();fprintf(stderr,fmt, ##arg);
#define SHOWERROR(fmt, arg...) ERROR_LINE();fprintf(stderr,fmt, ##arg);

#else
#define SHOW_LINE() //
#define SHOWDEBUG	//
#define SHOWERROR   //


#endif





typedef struct _WIGMI_MusicInformation
{
	unsigned char *pTitle;
	unsigned char *pArtist;
	unsigned char *pAlbum;
}WIGMI_MusicInformation,*WIGMI_pMusicInformation;


int get_item_size_big(unsigned char buf[]);//��˷�ʽ
int get_item_size_little(unsigned char buf[]);//С�˷�ʽ
int copy_binary_to_array(unsigned char *dest,unsigned char *src,unsigned int n,char *str);
int copy_data_to_array(unsigned char *dest,unsigned char *src,unsigned int n);
int append_binary_to_array(unsigned char *dest,unsigned char *src,unsigned int n,int num);
int utf8_to_gb2312(unsigned char *src, int src_len, unsigned char *dst);
int unicode_to_gb2312(unsigned char *src, int src_len, unsigned char *dst);
int utf8_to_unicode(unsigned char *src, int src_len, unsigned char *dst);
int check_if_have_id3(FILE *path);
int UnicodeToUTF8(unsigned char *putf8, unsigned char high, unsigned char low);
int UnicodeStringToUTF8String(unsigned char *putf8string, unsigned char *punicodestring, int num);
int gb2312_to_utf8(unsigned char *buff,unsigned char *src,int len) ;



#ifdef __cplusplus
}
#endif

#endif

