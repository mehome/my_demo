
//LINUX�±������g++ -g -o Des DES.cpp 32λӦ��
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>

#include <sys/socket.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <poll.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/un.h> 

#include <netinet/in.h>
#include <semaphore.h>
#include <limits.h>
#include <signal.h>


#include "queue.h"
#include "event_queue.h"
//#include "md5.h"

#define PORT_CLIENT  8688					//39009  /39007
#define SERVER_IP	 "47.92.233.33"			//36.189.241.148  /220.171.33.4	/47.92.233.33

#define SIZE 1024
int fd_client=0;
unsigned char read_buf[SIZE];
unsigned char send_buf[SIZE];


pthread_t SocketTcpWritePthread;
pthread_t SocketEmpHeartPthread;
pthread_t SocketTcpReadPthread;

static pthread_mutex_t SocketTcpWriteMtx = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t SocketTcpReadMtx = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t socketQueueMtx  = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t sockcond = PTHREAD_COND_INITIALIZER; 

static SocketEvent event;
static QUEUE *SocketQueue4G = NULL;

//unsigned char keyC[24] = "lqw2xazzi1h9lqw2xazzi1h9";//lqw2xazzi1h9lqw2xazzi1h9
static unsigned char loginkey[]="lqw2xazzi1h9lqw2xazzi1h9";
//static unsigned char ucBufKey1[8*2];
//static unsigned char ucBufKey2[8*2];
//static unsigned char ucBufKey3[8*2];
//static int K_po[100][56];


#define DEVICE_ID	"F8098L48005DFC11153"


typedef struct {

	char 	cmd_typ[100];	
	char 	cmd_id[100];	//业务流水号
	char 	dev_time[100];		//设备时间
	char    sig[100];			//信号值
	char	net[100];			//当前网络类型
	char 	tag_open[100];		//防拆开关状态
	char 	tag_move[100];		//防移开关状态
	char    cell_v[100];			//电池电压
	char 	cell_pwr[100];		//电池电量
	char 	lost_pwr[100];		//是否掉电
	
}Data_Heart_Mgs;			//数据心跳消息

typedef struct {

	char	cmd_typ[100];
	char	cmd_id[100];
	char 	meid[100];		//设备串号
	char 	imsi[100];		//sim卡IMSI
	char	brand[100];		//厂家型号
	char 	hardver[100];	//硬件型号
	char	softver[100];	//软件型号
	char 	net[100];
	char 	sig[100];
}Device_Login;				//设备登陆

typedef struct {

	char 	cmd_typ[100];
	char 	cmd_id[100];
	char 	result[100];

	/////////有可能需要//////////
	char 	file[100];
	char 	server_ip[100];
	char 	server_port[100];
}Device_Resquest_Mgs;	//平台下发数据，设备响应数据

typedef struct {

	char	cmd_typ[100];
	char	cmd_id[100];
	char	cryp[100];		//子纹

}Device_Finger_Mgs;	//指纹上报


typedef struct {

	char	cmd_typ[100];
	char	cmd_id[100];
	char	lst_cnt[100];		//节目列表数量
	char	lst_fm[100];		//AMR频率&是否播放前导音&开始时间&结束时间
}Device_Amr_ListMgs;			//查询设备的AMR节目列表


typedef struct {

	char 	_cmd_typ[100];
	char 	_cmd_id[100];
	int 	_vol;
	int 	_lst_cnt;
	int 	_download;
	char 	_lst_file[100][100];
	char 	_file[100][100];

	char 	_sys_datetime[100];
	char	_random[100];
	int 	_result;
	char 	_lst_fm[100][100];
	char 	_key[100];
	long	_size;
	char 	_server_ip[100];
	char 	_server_port[100];
}Platform_Send_Mgs;


enum cmd_type_device{			//服务器下发的数据类型

	CMD_TYPE_HEARTDATAREP = 0,
	CMD_TYPE_HEARTEMPREP,
	CMD_TYPE_LOGRAND,
	CMD_TYPE_LOGRESU,
	CMD_TYPE_SETAMPLST,
	CMD_TYPE_GETAMPLST,
	CMD_TYPE_SETFMLST,
	CMD_TYPE_GETFMLST,
	CMD_TYPE_FILEDOWNLOAD_SEND,
	CMD_TYPE_GETFILEINFO,
	CMD_TYPE_ALARTSTASEND,
	CMD_TYPE_ALARTDYNSEND,
	CMD_TYPE_DEVALARTSEND,
	CMD_TYPE_SETPARAMETER,
	CMD_TYPE_GETPARAMETER,
	
};


enum request_type{			//上报的类型

	REQUEST_HEART_DATA = 0,
	REQUEST_HEART_EMP_SEND,
	REQUEST_LOG_REP,
	REQUEST_FINGER_DATA,
	REQUEST_SET_AMP_REP_RECV,
	REQUEST_SET_AMP_REP_DOWN,
	REQUEST_GET_AMP_LST_RESP,
	REQUEST_SET_FM_REP,
	REQUEST_GET_FM_LST_RESP,
	REQUEST_FILE_DOWNLOAD_RESP,
	REQUEST_GET_FILE_REP,
	REQUEST_ALART_STA_REP,
	REQUEST_ALART_DYN_REP,
	REQUEST_DEV_ALART_REP,
	REQUEST_SET_PARAMETER_REP,
	REQUEST_GET_PARAMETER_REP,
};





static Data_Heart_Mgs		 g_DataHeart_str;
static Device_Login	  		 g_Device_Login_str;
static Device_Resquest_Mgs 	 g_Device_Resquest_str;
static Device_Finger_Mgs	 g_Device_Finger_str;
static Device_Amr_ListMgs	 g_Device_AmrListMgs_str;

static Platform_Send_Mgs	 g_Platform_Send_str;















typedef unsigned char uint8;
typedef unsigned long long uint16;

//CRC
#define		CRC_START_16		0x0000
#define		CRC_POLY_16		0xA001

static void init_crc16_tab( void );
static char crc_tab16_init = 0;
static uint16 crc_tab16[256];
int K_po[100][56];


/* * static void init_crc16_tab( void ); 
* * For optimal performance uses the CRC16 routine a lookup table with values 
* that can be used directly in the XOR arithmetic in the algorithm. This 
* lookup table is calculated by the init_crc16_tab() routine, the first time 
* the CRC function is called. */
static void init_crc16_tab( void ) 
{	
	uint16 i;	
	uint16 j;
	uint16 crc;	
	uint16 c;	
	for (i=0; i<256; i++) {
		crc = 0;		
	c   = i;		
	for (j=0; j<8; j++) 
	{			
		if ( (crc ^ c) & 0x0001 ) 
			crc = ( crc >> 1 ) ^ CRC_POLY_16;			
		else                      
			crc =   crc >> 1;			
		c = c >> 1;		
	}		
	crc_tab16[i] = crc;	
	}	
	crc_tab16_init = 1;
}

/* * uint16_t crc_16( const unsigned char *input_str, size_t num_bytes ); 
* * The function crc_16() calculates the 16 bits CRC16 in one pass for a byte 
* string of which the beginning has been passed to the function. The number of 
* bytes to check is also a parameter. The number of the bytes in the string is 
* limited by the constant SIZE_MAX. */
uint16 CRC16( const unsigned char *input_str, int num_bytes ) 
{	
	uint16 crc;	
	const unsigned char *ptr;	
	int a;	
	if ( ! crc_tab16_init ) init_crc16_tab();	
	crc = CRC_START_16;	
	ptr = input_str;	
	if ( ptr != NULL ) 
		for (a=0; a<num_bytes; a++) 
		{		
			crc = (crc >> 8) ^ crc_tab16[ (crc ^ (uint16) *ptr++) & 0x00FF ];	
		}	
	return crc;
}  /* crc_16 */
//CRC END

char  *CRC16_Str(const unsigned char *input_str, int num_bytes)
{
	int crc=0;
	char crc_out_put[256]={0};
	char *output_str = (char *)calloc(256,1);
	crc = CRC16(input_str,num_bytes);
	sprintf(crc_out_put,"%04x",crc);
	memcpy(output_str,crc_out_put,strlen(crc_out_put));
	printf("Crc result:%s\n",crc_out_put);
	return output_str;
}



void SocketQueue_Init4G(void)
{
	if(SocketQueue4G == NULL) {
		SocketQueue4G = Initialize_Queue();
		if(!SocketQueue4G) {
			error("Initialize_Queue failed");
			exit(-1);
		}
	}
}


int SocketQueue_Sizes4G()
{
	int ret = 0;
	if(SocketQueue4G)
		ret = Get_Queue_Sizes(SocketQueue4G);
	return ret;
}


QUEUE_ITEM * SocketQueue_Get4G()
{
	if(SocketQueue4G == NULL) {
		error("queue is not Init...");
		return NULL;
	}
	return Get_Queue_Item(SocketQueue4G);
}


int SocketQueue_Put4G(int type, void *data)
{
	if(SocketQueue4G == NULL) {
		error("queue is not Init...");
		return -1;
	}
	Add_Queue_Item(SocketQueue4G,type, data);
	return 0;
}

// 0 not empty 1 empty
int SocketQueue_IsEmpty4G(void)
{
	int len = 0;
	if(SocketQueue4G == NULL)
		return 1;
	if(Get_Queue_Sizes(SocketQueue4G) > 0)
		return 0;
	else 
		return 1;
}


void SocketQueue_Deinit4G()
{
	if(SocketQueue4G) {
		Free_Queue(SocketQueue4G);
			SocketQueue4G = NULL;
	}
}

void SocketQueuePut_notify(int type,void *data)
{	
		int ret; 
	
		pthread_mutex_lock(&socketQueueMtx); 
		SocketQueue_Put4G(type,data);
	//	if(eventQueueWait == 0) {
	//		pthread_cond_signal(&cond);  
	//	}
		pthread_cond_signal(&sockcond);		
		pthread_mutex_unlock(&socketQueueMtx);
		
}



void init_device_mgs(void)
{
	memset(&g_DataHeart_str,0,sizeof(Data_Heart_Mgs));
	memset(&g_Device_Login_str,0,sizeof(Device_Login));
	memset(&g_Device_Resquest_str,0,sizeof(Device_Resquest_Mgs));
	memset(&g_Device_Finger_str,0,sizeof(Device_Finger_Mgs));
	memset(&g_Device_AmrListMgs_str,0,sizeof(Device_Amr_ListMgs));

	memset(&g_Platform_Send_str,0,sizeof(Platform_Send_Mgs));
}

void get_nowtime(char nowtime[100])
{
	time_t rawtime;
    struct tm *timeinfo;
	int year,month,day,hour,min,sec;
    time(&rawtime);
	
    timeinfo = localtime(&rawtime);
    year = 	1900+timeinfo->tm_year;
    month = 1+timeinfo->tm_mon;
    day = 	timeinfo->tm_mday;
    hour = 	timeinfo->tm_hour;
    min = 	timeinfo->tm_min;
    sec = 	timeinfo->tm_sec;
	sprintf(nowtime,"%4d%02d%02d%02d%02d%02d",year, month,day,hour,min,sec);
	//printf ( "你需要的格式:%s\n",nowtime);
}



char *device_heart_request_data(int net,int tag_open,int tag_move,float cell_v,int cell_pwr,int lost_pwr)
{
	char *device_heart_string = (char *)calloc(1024,1);
	char nowtime[100];
	char Crc16Value[20]={0};
	//char *device_heart_string = NULL;

	//device_heart_string = (char *)calloc(1,sizeof(device_heart_string));
	memset(nowtime,0,sizeof(nowtime));
	memset(device_heart_string,0,sizeof(device_heart_string));
	memset(&g_DataHeart_str,0,sizeof(Data_Heart_Mgs));
	get_nowtime(nowtime);

	
	//g_DataHeart_str.cmd_typ = strdup("heart_data_send");
	//g_DataHeart_str.cmd_id  = strdup("00000000000000000_00000005");
	
	sprintf(g_DataHeart_str.cmd_typ,"cmd_typ=%s","heart_data_send");
	sprintf(g_DataHeart_str.cmd_id,"cmd_id=%s","00000000000000000_00000005");
	sprintf(g_DataHeart_str.dev_time,"dev_time=%s",nowtime);
	sprintf(g_DataHeart_str.sig,"sig=%s","31");
//	sprintf(g_DataHeart_str.net,"net=%s","3");
//	sprintf(g_DataHeart_str.tag_open,"tag_open=%s","1");
//	sprintf(g_DataHeart_str.tag_move,"tag_move=%s","1");
//	sprintf(g_DataHeart_str.cell_v,"cell_v=%s","4");
//	sprintf(g_DataHeart_str.cell_pwr,"cell_pwr=%s","35");
//	sprintf(g_DataHeart_str.lost_pwr,"lost_pwr=%s","0");


	sprintf(g_DataHeart_str.net,"net=%d",net);
	sprintf(g_DataHeart_str.tag_open,"tag_open=%d",tag_open);
	sprintf(g_DataHeart_str.tag_move,"tag_move=%d",tag_move);
	sprintf(g_DataHeart_str.cell_v,"cell_v=%.2f",cell_v);
	sprintf(g_DataHeart_str.cell_pwr,"cell_pwr=%d",cell_pwr);
	sprintf(g_DataHeart_str.lost_pwr,"lost_pwr=%d",lost_pwr);

	
	sprintf(device_heart_string,"%s,%s,%s,%s,%s,%s,%s,%s,%s,%s",g_DataHeart_str.cmd_typ,
				g_DataHeart_str.cmd_id,g_DataHeart_str.dev_time,g_DataHeart_str.sig,
				g_DataHeart_str.net,g_DataHeart_str.tag_open,g_DataHeart_str.tag_move,
				g_DataHeart_str.cell_v,g_DataHeart_str.cell_pwr,g_DataHeart_str.lost_pwr);

	int crc_value = CRC16(device_heart_string,strlen(device_heart_string));
	sprintf(Crc16Value,"crc=%d",crc_value);

	sprintf(device_heart_string,"%s,%s",device_heart_string,Crc16Value);
//	printf("device_heart_string:%s\n",device_heart_string);
	return device_heart_string;
}


char *device_blank_request_data(void)
{
	char *blank_string = (char *)calloc(1024,1);
	char Crc16Value[20]={0};
	memset(blank_string,0,sizeof(blank_string));
	memset(&g_DataHeart_str,0,sizeof(Data_Heart_Mgs));
	
	sprintf(g_DataHeart_str.cmd_typ,"cmd_typ=%s","heart_emp_send");

	sprintf(blank_string,"%s",g_DataHeart_str.cmd_typ);
	
	char *value = CRC16_Str(blank_string,strlen(blank_string));
	sprintf(Crc16Value,"crc=%s",value);

	sprintf(blank_string,"%s,%s",blank_string,Crc16Value);
	printf("blank_string:%s\n",blank_string);
	return blank_string;
	
}


/*设备登陆时候，上报的参数*/
char *device_login_request_data(char *meid,char *imsi)
{
	char *login_string = (char *)calloc(1024,1);
	char Crc16Value[20]={0};
//	char crc_out_put[256]={0};
	memset(login_string,0,sizeof(login_string));
	memset(&g_Device_Login_str,0,sizeof(Device_Login));
	
	sprintf(g_Device_Login_str.cmd_typ,"cmd_typ=%s","log_req");
	sprintf(g_Device_Login_str.cmd_id,"cmd_id=%s","00000000000000000_00000000");
	sprintf(g_Device_Login_str.meid,"meid=%s",meid);
	sprintf(g_Device_Login_str.imsi,"imsi=%s",imsi);
	
//	sprintf(g_Device_Login_str.brand,"brand=%s",brand);
//	sprintf(g_Device_Login_str.hardver,"hardver=%s",hardver);
//	sprintf(g_Device_Login_str.softver,"softver=%s",softver);
//	sprintf(g_Device_Login_str.net,"net=%d",net);
//	sprintf(g_Device_Login_str.sig,"sig=%s","31");
	
#if 0
	sprintf(login_string,"%s,%s,%s,%s,%s,%s,%s,%s,%s",g_Device_Login_str.cmd_typ,
				g_Device_Login_str.cmd_id,g_Device_Login_str.meid,g_Device_Login_str.imsi,
				g_Device_Login_str.brand,g_Device_Login_str.hardver,g_Device_Login_str.softver,
				g_Device_Login_str.net,g_Device_Login_str.sig);
#else
	sprintf(login_string,"%s,%s,%s,%s",g_Device_Login_str.cmd_typ,g_Device_Login_str.cmd_id,g_Device_Login_str.meid,g_Device_Login_str.imsi);
#endif	
#if 0
	int crc_value = CRC16(login_string,strlen(login_string));
	sprintf(crc_out_put,"%04x",crc_value);
//	printf("crc_value:%d\n",crc_value);
	sprintf(Crc16Value,"crc=%s",crc_out_put);
#else
	char *value = CRC16_Str(login_string,strlen(login_string));
#endif
	sprintf(Crc16Value,"crc=%s",value);
	sprintf(login_string,"%s,%s",login_string,Crc16Value);
	
	printf("login_string:%s\n",login_string);
	return login_string;
}


/*指纹请求的数据*/
char *device_finger_request_data(char *cryp)
{
	char *finger_string = (char *)calloc(1024,1);
	char Crc16Value[20]={0};
	//char *finger_string = (char *)calloc(1,sizeof(finger_string));
	memset(finger_string,0,sizeof(finger_string));
	memset(&g_Device_Finger_str,0,sizeof(Device_Finger_Mgs));

	sprintf(g_Device_Finger_str.cmd_typ,"cmd_typ=%s","log_crpy");
	sprintf(g_Device_Finger_str.cmd_id,"cmd_id=%s","00000000000000000_00000002");
	sprintf(g_Device_Finger_str.cryp,"cryp=%s",cryp);

	sprintf(finger_string,"%s,%s,%s",g_Device_Finger_str.cmd_typ,g_Device_Finger_str.cmd_id,
																	g_Device_Finger_str.cryp);
	
	char *value = CRC16_Str(finger_string,strlen(finger_string));		//生成校验码
	sprintf(Crc16Value,"crc=%s",value);
	
	sprintf(finger_string,"%s,%s",finger_string,Crc16Value);
	printf("finger_string:%s\n",finger_string);
	return finger_string;
}

char *device_success_repond_data(char *cmd_typ,char *cmd_id,int result)
{
	char *repond_string = (char *)calloc(1024,1);
	char Crc16Value[20]={0};
	//char *repond_string = (char *)calloc(1,sizeof(repond_string));
	memset(repond_string,0,sizeof(repond_string));
	memset(&g_Device_Resquest_str,0,sizeof(Device_Resquest_Mgs));

	sprintf(g_Device_Resquest_str.cmd_typ,"cmd_typ=%s",cmd_typ);
	sprintf(g_Device_Resquest_str.cmd_id,"cmd_id=%s",cmd_id);
	sprintf(g_Device_Resquest_str.result,"result=%d",result);

	sprintf(repond_string,"%s,%s,%s",g_Device_Resquest_str.cmd_typ,g_Device_Resquest_str.cmd_id,g_Device_Resquest_str.result);

	char *value = CRC16_Str(repond_string,strlen(repond_string));
	sprintf(Crc16Value,"crc=%s",value);
	
	sprintf(repond_string,"%s,%s",repond_string,Crc16Value);
	printf("repond_string:%s",repond_string);
	return repond_string;
}

char *device_AmrListMgs_repond_data()
{
	//char amrlist_string[1024];
	
}


#define CMD_TYP			"cmd_typ="
#define CMD_ID			"cmd_id="
#define VOL				"vol="
#define LST_CNT			"lst_cnt="
#define DOWNLOAD		"download="
#define RANDOM			"random="
#define SYS_TIME		"sys_datetime="
#define RESULT			"result="

#define LST_FM			"lst_fm="
#define FILE			"file="
#define CMD_SIZE		"size="
#define KEY				"key="

#define CMD_VO2			"vo2="
#define SERVER_IP1		"server_ip1="
#define SERVER_PORT1	"server_port1="

#define RANDOM			"random="
#define CRC_16				"crc="

//cmd_typ=set_amp_lst,cmd_id=20190106190249145_56781795,vol=2,lst_cnt=3,download=1,
//lst_file=/au/klmy/grp009/MP3A.amr&32156&129976&0&090010&093000,
//lst_file=/au/klmy/grp009/MP3B.amr&32151&203986&1&100010&103000,
//lst_file=/au/klmy/grp009/MP3C.amr&32154&745289&0&110010&113000

void cmdtype_set_amp_lst_event(char *_send_string)
{
	char *_str = NULL;

	
//	memset(_send_string,0,strlen(_send_string));
//	strcpy(_send_string,"cmd_typ=set_amp_lst,cmd_id=20190106190249145_56781795,vol=2,lst_cnt=3,download=1,lst_file=/au/klmy/grp009/MP3A.amr&32156&129976&0&090010&093000,lst_file=/au/klmy/grp009/MP3B.amr&32151&203986&1&100010&103000,lst_file=/au/klmy/grp009/MP3C.amr&32154&745289&0&110010&113000");
	memset(&g_Platform_Send_str,0,sizeof(Platform_Send_Mgs));


	_str = strtok(_send_string,",");
	if(_str != NULL){
		strncpy(g_Platform_Send_str._cmd_typ,_str + (sizeof(CMD_TYP)-1) ,strlen(_str) - strlen(CMD_TYP));	
		_str = strtok(NULL,",");
	}

	if(_str != NULL){
		strncpy(g_Platform_Send_str._cmd_id, _str + (sizeof(CMD_ID)-1),strlen(_str) - strlen(CMD_ID));
		_str = strtok(NULL,",");
	}

	if(_str != NULL){
		char vol[10] = {0};
		strncpy(vol, _str + (sizeof(VOL)-1),strlen(_str) - strlen(VOL));
		g_Platform_Send_str._vol = atoi(vol);
		_str = strtok(NULL,",");
	}

	if(_str != NULL){
		char lst_cnt[10]={0};
		strncpy(lst_cnt, _str + (sizeof(LST_CNT)-1),strlen(_str) - strlen(LST_CNT));
		g_Platform_Send_str._lst_cnt = atoi(lst_cnt);
		_str = strtok(NULL,",");
	}

	if(_str != NULL){
		char download[10]={0};
		strncpy(download, _str + (sizeof(DOWNLOAD)-1),strlen(_str) - strlen(DOWNLOAD));
		g_Platform_Send_str._download = atoi(download);
		_str = strtok(NULL,",");	
	}

	int i;
	if(_str != NULL){
		for(i=0;i < g_Platform_Send_str._lst_cnt; i++)
		{
			sprintf(g_Platform_Send_str._lst_file[i], _str + (sizeof(LST_CNT)-1) ,strlen(_str) - strlen(LST_CNT));
			_str = strtok(NULL,",");	
			if(_str == NULL)
				break;
		}
	}
	
	printf("_cmd_typ:%s,_cmd_id:%s,vol:%d,lst_cnt:%d,download:%d\n",g_Platform_Send_str._cmd_typ,g_Platform_Send_str._cmd_id,
	          							 g_Platform_Send_str._vol,g_Platform_Send_str._lst_cnt,g_Platform_Send_str._download);

	for(i=0;i < g_Platform_Send_str._lst_cnt; i++)
		printf("lst_file[%d]:%s\n",i,g_Platform_Send_str._lst_file[i]);

	
}

/*cmd_typ=get_amp_lst,cmd_id=20190106190249145_56781795*/
void cmdtype_get_amp_lst_event(char *_send_string)
{
	char *_str = NULL;
	memset(&g_Platform_Send_str,0,sizeof(Platform_Send_Mgs));
	_str = strtok(_send_string,",");
	if(_str != NULL){
		strncpy(g_Platform_Send_str._cmd_typ,_str + (sizeof(CMD_TYP)-1) ,strlen(_str) - strlen(CMD_TYP));	
		_str = strtok(NULL,",");
	}

	if(_str != NULL){
		strncpy(g_Platform_Send_str._cmd_id, _str + (sizeof(CMD_ID)-1),strlen(_str) - strlen(CMD_ID));
		_str = strtok(NULL,",");
	}

	printf("_cmd_typ:%s,_cmd_id:%s\n",g_Platform_Send_str._cmd_typ,g_Platform_Send_str._cmd_id);
}


/*
cmd_typ=set_fm_lst,cmd_id=20190106190249145_56781765,lst_cnt=2,
vo2=3,lst_fm=103.9&0&090010&093000,
lst_fm=106.9&1&100010&103000
*/
void cmdtype_set_fm_lst_event(char *_send_string)
{
	char *_str = NULL;

//	memset(_send_string,0,strlen(_send_string));
//	strcpy(_send_string,"cmd_typ=set_fm_lst,cmd_id=20190106190249145_56781765,lst_cnt=2,vo2=3,lst_fm=103.9&0&090010&093000,lst_fm=106.9&1&100010&103000");
	memset(&g_Platform_Send_str,0,sizeof(Platform_Send_Mgs));
	_str = strtok(_send_string,",");
	if(_str != NULL){
		strncpy(g_Platform_Send_str._cmd_typ,_str + (sizeof(CMD_TYP)-1) ,strlen(_str) - strlen(CMD_TYP));	
		_str = strtok(NULL,",");
	}

	if(_str != NULL){
		strncpy(g_Platform_Send_str._cmd_id, _str + (sizeof(CMD_ID)-1),strlen(_str) - strlen(CMD_ID));
		_str = strtok(NULL,",");
	}

	if(_str != NULL){
		char lstcnt[10]={0};
		strncpy(lstcnt, _str + (sizeof(LST_CNT)-1),strlen(_str) - strlen(LST_CNT));
		g_Platform_Send_str._lst_cnt = atoi(lstcnt);
		_str = strtok(NULL,",");
	}

	if(_str != NULL){
		char vol[10]={0};
		strncpy(vol,_str +  (sizeof(VOL)-1),strlen(_str) - strlen(VOL));
		g_Platform_Send_str._vol = atoi(vol);
		_str = strtok(NULL,",");
	}
	
	int i;
	if(_str != NULL){
		
		for(i=0;i<g_Platform_Send_str._lst_cnt;i++)
		{
			strncpy(g_Platform_Send_str._lst_fm[i],_str +	(sizeof(LST_FM)-1),strlen(_str) - strlen(LST_FM));
			_str = strtok(NULL,",");
			if(_str == 	NULL)
				break;
		}	
	}

	printf("_cmd_typ:%s,_cmd_id:%s,_lst_cnt:%d,_vol:%d\n",g_Platform_Send_str._cmd_typ,g_Platform_Send_str._cmd_id,
												g_Platform_Send_str._lst_cnt,g_Platform_Send_str._vol);

	for(i=0;i<g_Platform_Send_str._lst_cnt;i++)
		printf("lst_fm[%d]:%s\n",i,g_Platform_Send_str._lst_fm[i]);
}


/*cmd_typ=get_fm_lst,cmd_id=20190106190249145_56781795*/
void cmdtype_get_fm_lst_event(char *_send_string)
{
	char *_str = NULL;
	
	memset(&g_Platform_Send_str,0,sizeof(Platform_Send_Mgs));
	_str = strtok(_send_string,",");
	if(_str != NULL){
		strncpy(g_Platform_Send_str._cmd_typ,_str + (sizeof(CMD_TYP)-1) ,strlen(_str) - strlen(CMD_TYP));	
		_str = strtok(NULL,",");
	}

	
	if(_str != NULL){
		strncpy(g_Platform_Send_str._cmd_id, _str + (sizeof(CMD_ID)-1),strlen(_str) - strlen(CMD_ID));
	}

	printf("_cmd_typ:%s,_cmd_id:%s\n",g_Platform_Send_str._cmd_typ,g_Platform_Send_str._cmd_id);
}


/*cmd_typ=file_download_send,cmd_id=20190106190249145_56781765,file=/au/klmy/grp009/MP3A.amr,
size=102298,key=98732
*/
void cmdtype_file_download_send_event(char *_send_string)
{
	char *_str = NULL;	
	memset(&g_Platform_Send_str,0,sizeof(Platform_Send_Mgs));
	_str = strtok(_send_string,",");
	if(_str != NULL){
		strncpy(g_Platform_Send_str._cmd_typ,_str + (sizeof(CMD_TYP)-1) ,strlen(_str) - strlen(CMD_TYP));	
		_str = strtok(NULL,",");
	}

	if(_str != NULL){
		strncpy(g_Platform_Send_str._cmd_id, _str + (sizeof(CMD_ID)-1),strlen(_str) - strlen(CMD_ID));
		_str = strtok(NULL,",");
	}

	if(_str != NULL){
		strncpy(g_Platform_Send_str._file[0], _str + (sizeof(FILE)-1),strlen(_str) - strlen(FILE));
		_str = strtok(NULL,",");
	}

	if(_str != NULL){
		char filesize[100]={0};
		strncpy(filesize, _str + (sizeof(CMD_SIZE)-1),strlen(_str) - strlen(CMD_SIZE));
		g_Platform_Send_str._size = atoi(filesize);
		_str = strtok(NULL,",");
	}

	if(_str != NULL){
		strncpy(g_Platform_Send_str._key, _str + (sizeof(KEY)-1),strlen(_str) - strlen(KEY));
		_str = strtok(NULL,",");
	}

	
	printf("_cmd_typ:%s,_cmd_id:%s,_file:%s,_size:%ld,_key:%s\n",g_Platform_Send_str._cmd_typ,g_Platform_Send_str._cmd_id,
										g_Platform_Send_str._file[0],g_Platform_Send_str._size,g_Platform_Send_str._key);
}


/*cmd_typ=get_file_info,cmd_id=20190106190249145_56781765,file=MP3A.amr,file=MP3B.amr,file=MP3C.amr*/
void cmdtype_get_file_info_event(char *_send_string)
{
	
	
}

/*cmd_typ=alart_sta_send,cmd_id=20190106190249145_56781765,file=MSOS0000.amr,size=107765*/
void cmdtype_alart_sta_send_event(char *_send_string)
{


}

/*cmd_typ=alart_dyn_send,cmd_id=20190106190249145_56781765,file=/alart/klmy/grp076/MSOS1050.amr,size=102298*/
void cmdtype_alart_dyn_send_event(char *_send_string)
{


}

/*cmd_typ=dev_alart_send,cmd_id=00000000000000000_00000099*/
void cmdtype_dev_alart_send_event(char *_send_string)
{


}


/*cmd_typ=set_parameter,cmd_id=20190106190249145_56781765,server_ip1=192.168.1.2,server_port1=12298*/
void cmdtype_set_parameter_event(char *_send_string)
{


}


/*cmd_typ=get_parameter,cmd_id=20190106190249145_56781765,server_ip1=?,server_port1=?*/
void cmdtype_get_parameter_event(char *_send_string)
{


}

/*设备登陆后,平台响应的随机数*/
/*cmd_typ=log_rand,cmd_id=00000000000000000_00000001,random=e8a4b0d61ada47fda7addff0fa18091e493f,crc=43e2(随机数)*/
int cmdtype_log_rand_event(char *_send_string)
{
	char *_str = NULL;
	
	char md5_Str[256]={0};
	char md5_outStr[256]={0};
	
	char *pCrc=NULL;
	char Crc16[20]={0};
	int  CrcValue=0;
	char *CmdStr = (char *)calloc(256,1);
	
	memset(&g_Platform_Send_str,0,sizeof(Platform_Send_Mgs));
	memset(CmdStr,0,sizeof(CmdStr)-1);
	
	pCrc = strstr(_send_string,CRC_16);
	memcpy(Crc16,pCrc+strlen(CRC_16),strlen(pCrc)-strlen(CRC_16));
//	CrcValue = atoi(Crc16);
	memcpy(CmdStr,_send_string,strlen(_send_string) - (strlen(pCrc)+1));
	printf("CmdStr:%s\n",CmdStr);
	
	char *str = CRC16_Str(CmdStr,strlen(CmdStr));
	if(strcmp(str,Crc16) != 0)				//crc16校验
	{
		free(CmdStr);
		printf("crc16 checksum failed\n");
		return -1;
	}
	
	_str = strtok(_send_string,",");
	if(_str != NULL){
		strncpy(g_Platform_Send_str._cmd_typ,_str + (sizeof(CMD_TYP)-1) ,strlen(_str) - strlen(CMD_TYP));	
		_str = strtok(NULL,",");
	}

	if(_str != NULL){
		strncpy(g_Platform_Send_str._cmd_id, _str + (sizeof(CMD_ID)-1),strlen(_str) - strlen(CMD_ID));
		_str = strtok(NULL,",");
	}

	if(_str != NULL){
		strncpy(g_Platform_Send_str._random,_str + (sizeof(RANDOM)-1) ,strlen(_str) - strlen(RANDOM));	
	}

	printf("_cmd_typ:%s,_cmd_id:%s,_random:%s\n",g_Platform_Send_str._cmd_typ,g_Platform_Send_str._cmd_id,g_Platform_Send_str._random);

	/*将平台下发的随机数和设备ID组合,利用MD5加密，生成一个新的MD5密文串*/
	sprintf(md5_Str,"%s%s",g_Platform_Send_str._random,DEVICE_ID);
	printf("md5_Str:%s\n",md5_Str);

	/*MD5加密生成密文*/
	MD5Trans32bitStr(md5_Str,md5_outStr,strlen(md5_Str));
	printf("md5_outStr:%s\n",md5_outStr);
	char *fingerBuf = device_finger_request_data(md5_outStr);
	SocketQueuePut_notify(REQUEST_FINGER_DATA,fingerBuf);

	free(CmdStr);
	return 0;
}


/*平台返回的数据心跳包*/
/*cmd_typ=heart_data_rep,cmd_id=00000000000000000_00000005,sys_datetime=20181216121315,crc=dffg*/
int cmdtype_heart_data_rep_event(char *_send_string)
{
	char *_str = NULL;	

	char *pCrc=NULL;
	char Crc16[20]={0};
	int  CrcValue=0;
	char *CmdStr = (char *)calloc(256,1);
	memset(&g_Platform_Send_str,0,sizeof(Platform_Send_Mgs));
	memset(&CmdStr,0,strlen(CmdStr));

	pCrc = strstr(_send_string,CRC_16);
	memcpy(Crc16,pCrc+strlen(CRC_16),strlen(pCrc)-strlen(CRC_16));
	memcpy(CmdStr,_send_string,strlen(_send_string) - (strlen(pCrc)+1));
	printf("CmdStr:%s\n",CmdStr);
	
	char *str = CRC16_Str(CmdStr,strlen(CmdStr));
	if(strcmp(str,Crc16) != 0)				//crc16校验
	{
		free(CmdStr);
		printf("crc16 checksum failed\n");
		return -1;
	}
		
	_str = strtok(_send_string,",");
	if(_str != NULL){
		strncpy(g_Platform_Send_str._cmd_typ,_str + (sizeof(CMD_TYP)-1) ,strlen(_str) - strlen(CMD_TYP));	
		_str = strtok(NULL,",");
	}
	
	if(_str != NULL){
		strncpy(g_Platform_Send_str._cmd_id, _str + (sizeof(CMD_ID)-1),strlen(_str) - strlen(CMD_ID));
		_str = strtok(NULL,",");
	}

	if(_str != NULL){
		strncpy(g_Platform_Send_str._sys_datetime, _str + (sizeof(SYS_TIME)-1),strlen(_str) - strlen(SYS_TIME));
		_str = strtok(NULL,",");
	}

	printf("_cmd_typ:%s,_cmd_id:%s,_sys_datetime:%s\n",g_Platform_Send_str._cmd_typ,g_Platform_Send_str._cmd_id,g_Platform_Send_str._sys_datetime);

//	char *rev_buf = device_heart_request_data(3,1,1,4,35,0);	
	//printf("rev_buf=%s\n",rev_buf);
//	SocketQueuePut_notify(REQUEST_HEART_DATA,rev_buf);
	free(CmdStr);
	return 0;
}


/*空白心跳包*/
/*cmd_typ=heart_emp_rep,crc=sdgd*/
int cmdtype_heart_emp_rep_event(char *_send_string)
{
	char *_str = NULL;

	char *pCrc=NULL;
	char Crc16[20]={0};
	int  CrcValue=0;
	char *CmdStr = (char *)calloc(256,1);
	memset(&g_Platform_Send_str,0,sizeof(Platform_Send_Mgs));
	memset(&CmdStr,0,strlen(CmdStr));

	pCrc = strstr(_send_string,CRC_16);
	memcpy(Crc16,pCrc+strlen(CRC_16),strlen(pCrc)-strlen(CRC_16));
	memcpy(CmdStr,_send_string,strlen(_send_string) - (strlen(pCrc)+1));
	printf("CmdStr:%s\n",CmdStr);
	
	char *str = CRC16_Str(CmdStr,strlen(CmdStr));
	if(strcmp(str,Crc16) != 0)						//crc16校验
	{
		free(CmdStr);
		printf("crc16 checksum failed\n");
		return -1;
	}

	_str = strtok(_send_string,",");
	if(_str != NULL){
		strncpy(g_Platform_Send_str._cmd_typ,_str + (sizeof(CMD_TYP)-1) ,strlen(_str) - strlen(CMD_TYP));	
	}

	printf("_cmd_typ:%s\n",g_Platform_Send_str._cmd_typ);
}

/*strstr:寻找字串时，返回第一个字串出现的位置*/
/*平台响应 登录结果*/
/*cmd_typ=log_resu,cmd_id=00000000000000000_00000003,result=0,crc=603e*/
int cmdtype_log_resu_event(char *_send_string)
{
	char *_str = NULL;
	
	char *pCrc=NULL;
	char Crc16[20]={0};
	int  CrcValue=0;
	char *CmdStr = (char *)calloc(256,1);
	memset(&g_Platform_Send_str,0,sizeof(Platform_Send_Mgs)-1);
	memset(CmdStr,0,sizeof(CmdStr)-1);
	
	pCrc = strstr(_send_string,CRC_16);
	memcpy(Crc16,pCrc+strlen(CRC_16),strlen(pCrc)-strlen(CRC_16));

	memcpy(CmdStr,_send_string,strlen(_send_string) - (strlen(pCrc)+1));
	printf("CmdStr:%s\n",CmdStr);
	char *str = CRC16_Str(CmdStr,strlen(CmdStr));
	if(strcmp(str,Crc16) != 0)						//crc16校验
	{
		free(CmdStr);
		printf("crc16 checksum failed\n");
		return -1;
	}
	
	
		_str = strtok(_send_string,",");
		if(_str != NULL){
			strncpy(g_Platform_Send_str._cmd_typ,_str + (sizeof(CMD_TYP)-1) ,strlen(_str) - strlen(CMD_TYP));
			_str = strtok(NULL,",");
		}

		if(_str != NULL){
			strncpy(g_Platform_Send_str._cmd_id, _str + (sizeof(CMD_ID)-1),strlen(_str) - strlen(CMD_ID));
			_str = strtok(NULL,",");
		}

		if(_str != NULL){
			char result[10] = {0};
			strncpy(result, _str + (sizeof(RESULT)-1),strlen(_str) - strlen(RESULT));
			g_Platform_Send_str._result = atoi(result);
		}

		
		printf("_cmd_typ:%s,_cmd_id:%s,_result:%d\n",g_Platform_Send_str._cmd_typ,g_Platform_Send_str._cmd_id,g_Platform_Send_str._result);
		free(CmdStr);
			
	return 0;
}





/*服务器平台下发的数据类型*/
void cmd_type_process(int cmd_type ,char *_send_string)
{
	printf("cmd=%d\n",cmd_type);
	printf("cmd string:%s\n",_send_string);
	switch(cmd_type)
	{
		case CMD_TYPE_HEARTDATAREP:
			cmdtype_heart_data_rep_event(_send_string);
			break;

		case CMD_TYPE_HEARTEMPREP:
			cmdtype_heart_emp_rep_event(_send_string);
			break;

		case CMD_TYPE_LOGRAND:

			break;

		case CMD_TYPE_LOGRESU:
			cmdtype_log_resu_event(_send_string);
			break;

		case CMD_TYPE_SETAMPLST:
			cmdtype_set_amp_lst_event(_send_string);
			break;

		case CMD_TYPE_GETAMPLST:
			cmdtype_get_amp_lst_event(_send_string);
			break;

		case CMD_TYPE_SETFMLST:
			cmdtype_set_fm_lst_event(_send_string);
			break;

		case CMD_TYPE_GETFMLST:
			cmdtype_get_fm_lst_event(_send_string);
			break;

		case CMD_TYPE_FILEDOWNLOAD_SEND:
			cmdtype_file_download_send_event(_send_string);
			break;

		case CMD_TYPE_GETFILEINFO:

			break;

		case CMD_TYPE_ALARTSTASEND:

			break;

		case CMD_TYPE_ALARTDYNSEND:

			break;

		case CMD_TYPE_DEVALARTSEND:

			break;

		case CMD_TYPE_SETPARAMETER:

			break;

		case CMD_TYPE_GETPARAMETER:

			break;

		default :
			printf("cmd type error \n");
			break;
	}

}

void Parse_Platform_string(char *send_str)
{
	int iLength = 0;
	char _send_string[1024] = {0};
	char _cmd_str[1024] = {0};
	int cmd_type = -1;

	strcpy(_send_string,send_str);
	strcpy(_cmd_str,send_str);

//	strcpy(_send_string,"cmd_typ=log_resu,cmd_id=00000000000000000_00000003,result=0");
//	strcpy(_cmd_str,"cmd_typ=log_resu,cmd_id=00000000000000000_00000003,result=0");
	
	char *_str = NULL;
	if(_cmd_str != NULL)
	{
		_str = strtok(_cmd_str,",");
		
//		strncpy(g_Platform_Send_str._cmd_typ,_str + (sizeof(CMD_TYP)-1) ,strlen(_str) - strlen(CMD_TYP));
//		printf("g_Platform_Send_str._cmd_typ:%s\n",g_Platform_Send_str._cmd_typ);

		printf("Parse cmd_type:%s\n",_str);		//_str:cmd_typ=set_amp_lst
		
		if(strcmp("cmd_typ=heart_data_rep",_str) == 0){
			cmd_type = CMD_TYPE_HEARTDATAREP;
		}
		else if(strcmp("cmd_typ=heart_emp_rep",_str) == 0){
			cmd_type = CMD_TYPE_HEARTEMPREP;
		}
		else if(strcmp("cmd_typ=log_rand",_str) == 0){
			cmd_type = CMD_TYPE_LOGRAND;
		}

		else if(strcmp("cmd_typ=log_resu",_str) == 0){
			cmd_type = CMD_TYPE_LOGRESU;
		}

		else if(strcmp("cmd_typ=set_amp_lst",_str) == 0){
			cmd_type = CMD_TYPE_SETAMPLST;
		}

		else if(strcmp("cmd_typ=get_amp_lst",_str) == 0){
			cmd_type = CMD_TYPE_GETAMPLST;
		}
		
		else if(strcmp("cmd_typ=set_fm_lst",_str) == 0){
			cmd_type = CMD_TYPE_SETFMLST;
		}

		else if(strcmp("cmd_typ=get_fm_lst",_str) == 0){
			cmd_type = CMD_TYPE_GETFMLST;
		}

		else if(strcmp("cmd_typ=file_download_send",_str) == 0){
			cmd_type = CMD_TYPE_FILEDOWNLOAD_SEND;
		}

		else if(strcmp("cmd_typ=get_file_info",_str) == 0){
			cmd_type = CMD_TYPE_GETFILEINFO;
		}

		else if(strcmp("cmd_typ=alart_sta_send",_str) == 0){
			cmd_type = CMD_TYPE_ALARTSTASEND;
		}
		
		else if(strcmp("cmd_typ=alart_dyn_send",_str) == 0){
			cmd_type = CMD_TYPE_ALARTDYNSEND;
		}

		else if(strcmp("cmd_typ=dev_alart_send",_str) == 0){
			cmd_type = CMD_TYPE_DEVALARTSEND;
		}

		else if(strcmp("cmd_typ=set_parameter",_str) == 0){
			cmd_type = CMD_TYPE_SETPARAMETER;
		}

		else if(strcmp("cmd_typ=get_parameter",_str) == 0){
			cmd_type = CMD_TYPE_GETPARAMETER;
		}

		cmd_type_process(cmd_type,_send_string);		//执行接收到对应指令的处理函数
	}

	
	

}


int Client_Select_TimeOver( int fd, int sec, int usec)
{
	int ret;
	fd_set fds;
	struct timeval timeout;
 
	timeout.tv_sec = sec;
	timeout.tv_usec = usec;
 
	FD_ZERO(&fds); //每次循环都要清空集合，否则不能检测描述符变化
	FD_SET(fd,&fds); //添加描述符
 
	ret = select(fd+1,&fds,&fds,NULL,&timeout);
	if(ret < 0)
	{
		printf("client read data select error\n");
        	return -1;	
	}
	else if (ret == 0)
	{
	        printf("client read data select timeout\n");
	        return 0;
	}
    else 
	{
		if(FD_ISSET(fd,&fds)) //测试sock是否可读，是否被设置成功(即是否网络上有数据)
		{
			printf("有数据可读\n");
			return 1;
 
		}
		else
		{	
			printf("没有数据可读\n");
			return 0;
		}
    }
	
 
}



uint8 IP[64] = {
	58 , 50 , 42 , 34 , 26 , 18 , 10 ,  2 ,
	60 , 52 , 44 , 36 , 28 , 20 , 12 ,  4 ,
	62 , 54 , 46 , 38 , 30 , 22 , 14 ,  6 ,
	64 , 56 , 48 , 40 , 32 , 24 , 16 ,  8 ,
	57 , 49 , 41 , 33 , 25 , 17 ,  9 ,  1 ,
	59 , 51 , 43 , 35 , 27 , 19 , 11 ,  3 ,
	61 , 53 , 45 , 37 , 29 , 21 , 13 ,  5 ,
	63 , 55 , 47 , 39 , 31 , 23 , 15 ,  7 };
	
uint8 FP[64] = {
	40 ,  8 , 48 , 16 , 56 , 24 , 64 , 32 ,
	39 ,  7 , 47 , 15 , 55 , 23 , 63 , 31 ,
	38 ,  6 , 46 , 14 , 54 , 22 , 62 , 30 ,
	37 ,  5 , 45 , 13 , 53 , 21 , 61 , 29 ,
	36 ,  4 , 44 , 12 , 52 , 20 , 60 , 28 ,
	35 ,  3 , 43 , 11 , 51 , 19 , 59 , 27 ,
	34 ,  2 , 42 , 10 , 50 , 18 , 58 , 26 ,
	33 ,  1 , 41 ,  9 , 49 , 17 , 57 , 25 };
	
uint8 KM[16] = {
	1 ,  1 ,  2 ,  2 ,  2 ,  2 ,  2 ,  2 ,  1 ,  2 ,  2 ,  2 ,  2 ,  2 ,  2 ,  1 };
	
uint8 PC1[56] = {
	57 , 49 , 41 , 33 , 25 , 17 ,  9 ,
	1 , 58 , 50 , 42 , 34 , 26 , 18 ,
	10 ,  2 , 59 , 51 , 43 , 35 , 27 ,
	19 , 11 ,  3 , 60 , 52 , 44 , 36 ,
	63 , 55 , 47 , 39 , 31 , 23 , 15 ,
	7 , 62 , 54 , 46 , 38 , 30 , 22 ,
	14 ,  6 , 61 , 53 , 45 , 37 , 29 ,
	21 , 13 ,  5 , 28 , 20 , 12 ,  4
};
  
uint8 PC2[48] = {
	14 , 17 , 11 , 24 ,  1 ,  5 ,  3 , 28 ,
	15 ,  6 , 21 , 10 , 23 , 19 , 12 ,  4 ,
	26 ,  8 , 16 ,  7 , 27 , 20 , 13 ,  2 ,
	41 , 52 , 31 , 37 , 47 , 55 , 30 , 40 ,
	51 , 45 , 33 , 48 , 44 , 49 , 39 , 56 ,
	34 , 53 , 46 , 42 , 50 , 36 , 29 , 32 };

uint8	E[48] = {
	32 ,  1 ,  2 ,  3 ,  4 ,  5 ,  4 ,  5 ,
	6 ,  7 ,  8 ,  9 ,  8 ,  9 , 10 , 11 ,
	12 , 13 , 12 , 13 , 14 , 15 , 16 , 17 ,
	16 , 17 , 18 , 19 , 20 , 21 , 20 , 21 ,
	22 , 23 , 24 , 25 , 24 , 25 , 26 , 27 ,
	28 , 29 , 28 , 29 , 30 , 31 , 32 ,  1 };


uint8	s_boxes[32][16] = {
	//S1     
		{ 14,4,13,1,2,15,11,8,3,10,6,12,5,9,0,7 },
		{ 0,15,7,4,14,2,13,1,10,6,12,11,9,5,3,8 },
		{ 4,1,14,8,13,6,2,11,15,12,9,7,3,10,5,0 },
		{ 15,12,8,2,4,9,1,7,5,11,3,14,10,0,6,13 },
	//S2  
		{ 15,1,8,14,6,11,3,4,9,7,2,13,12,0,5,10 },
		{ 3,13,4,7,15,2,8,14,12,0,1,10,6,9,11,5 },
		{ 0,14,7,11,10,4,13,1,5,8,12,6,9,3,2,15 },
		{ 13,8,10,1,3,15,4,2,11,6,7,12,0,5,14,9 },
	//S3  
		{ 10,0,9,14,6,3,15,5,1,13,12,7,11,4,2,8 },
		{ 13,7,0,9,3,4,6,10,2,8,5,14,12,11,15,1 },
		{ 13,6,4,9,8,15,3,0,11,1,2,12,5,10,14,7 },
		{ 1,10,13,0,6,9,8,7,4,15,14,3,11,5,2,12 },
	//S4  
		{ 7,13,14,3,0,6,9,10,1,2,8,5,11,12,4,15 },
		{ 13,8,11,5,6,15,0,3,4,7,2,12,1,10,14,9 },
		{ 10,6,9,0,12,11,7,13,15,1,3,14,5,2,8,4 },
		{ 3,15,0,6,10,1,13,8,9,4,5,11,12,7,2,14 },
	//S5  
		{ 2,12,4,1,7,10,11,6,8,5,3,15,13,0,14,9 },
		{ 14,11,2,12,4,7,13,1,5,0,15,10,3,9,8,6 },
		{ 4,2,1,11,10,13,7,8,15,9,12,5,6,3,0,14 },
		{ 11,8,12,7,1,14,2,13,6,15,0,9,10,4,5,3 },
	//S6  
		{ 12,1,10,15,9,2,6,8,0,13,3,4,14,7,5,11 },
		{ 10,15,4,2,7,12,9,5,6,1,13,14,0,11,3,8 },
		{ 9,14,15,5,2,8,12,3,7,0,4,10,1,13,11,6 },
		{ 4,3,2,12,9,5,15,10,11,14,1,7,6,0,8,13 },
	//S7  
		{ 4,11,2,14,15,0,8,13,3,12,9,7,5,10,6,1 },
		{ 13,0,11,7,4,9,1,10,14,3,5,12,2,15,8,6 },
		{ 1,4,11,13,12,3,7,14,10,15,6,8,0,5,9,2 },
		{ 6,11,13,8,1,4,10,7,9,5,0,15,14,2,3,12 },
	//S8  
		{ 13,2,8,4,6,15,11,1,10,9,3,14,5,0,12,7 },
		{ 1,15,13,8,10,3,7,4,12,5,6,11,0,14,9,2 },
		{ 7,11,4,1,9,12,14,2,0,6,10,13,15,3,5,8 },
		{ 2,1,14,7,4,10,8,13,15,12,9,0,3,5,6,11 }
};

uint8 PF[32] = {
	16 ,  7 , 20 , 21 , 29 , 12 , 28 , 17 ,
	1 , 15 , 23 , 26 ,  5 , 18 , 31 , 10 ,
	2 ,  8 , 24 , 14 , 32 , 27 ,  3 ,  9 ,
	19 , 13 , 30 ,  6 , 22 , 11 ,  4 , 25 };



void initial_p(int *plaint_text) 
{
	int tmp[64];
	int i;
	for (i = 0;i < 64;i++)
	{
		tmp[i] = plaint_text[IP[i] - 1];
	}	
	memcpy(plaint_text, tmp, sizeof(tmp));
}

void final_p(int *cipher_text) {
	int tmp[64];
	int i ;
	for (i= 0;i < 64;i++)
		tmp[i] = cipher_text[FP[i] - 1];
	memcpy(cipher_text, tmp, sizeof(tmp));
}

void extend_p(int *text, int *output) {
	int i ;
	for (i= 0;i < 48;i++)
		output[i] = text[E[i] - 1];
}
void permutation1(int *key, int *o) {
	//short tmp[56];
	int i;
	for ( i= 0;i < 56;i++)
		o[i] = key[PC1[i] - 1];
	//memcpy(output, tmp, sizeof(tmp));
}


void permutationP(int *text) {
	int tmp[32];
	int i ;
	for (i= 0;i < 32;i++) {
		tmp[i] = text[PF[i] - 1];
	}
	memcpy(text, tmp, sizeof(tmp));
}

void F_DES(int *R, int *key) {
	int R_48[48];

	int S_in[48];
	int S_out[32];
	int i;
	int j;
		extend_p(R, R_48);
	for ( i= 0;i < 48;i++) {
		S_in[i] = R_48[i] ^ key[i];
	}


	
	for ( i = 0;i < 8;i++) {
		int a = (S_in[i * 6] << 1) + S_in[i * 6 + 5];
		int b = (S_in[i * 6 + 1] << 3) + (S_in[i * 6 + 2] << 2) + (S_in[i * 6 + 3] << 1) + S_in[i * 6 + 4];
		int s = s_boxes[i * 4 + a][b];
		for (j = 0;j < 4;j++) {
			S_out[i * 4 + j] = (s >> (3 - j)) & 1;
		}
	}

	permutationP(S_out);
	memcpy(R, S_out, sizeof(S_out));

}

void Round(int *L, int *R, int *key) {
	int R_1[32];
	int i;
	for (i = 0;i < 32;i++) {
		R_1[i] = R[i];
	}
	F_DES(R_1, key);

	//L XOR R_1
	for ( i = 0;i < 32;i++) {
		R_1[i] ^= L[i];
	}


	for ( i = 0;i < 32;i++) {
		L[i] = R[i];
		R[i] = R_1[i];
	}
	

}
void setK(uint8 *keyC,int (*K)[56]) {

	int keyP[64];
	int key[56];
	int C[84], D[84];
	int i ;
	int j;
	int shift_len = 0;

	for (i= 0;i < 8;i++) {
		for (j = 0;j < 8;j++) {
			keyP[i * 8 + j] = (keyC[i] >> (7 - j)) & 1;
		}
	}

	permutation1(keyP, key);



	for ( i = 0;i < 28;i++) {
		C[56 + i] = C[28 + i] = C[i] = key[i];
		D[56 + i] = D[28 + i] = D[i] = key[28 + i];
	}

	
	for ( i = 0;i < 16;i++) {
		shift_len += KM[i];
		for (j = 0;j < 48;j++) {
			K[i][j] = (PC2[j]<28) ? C[PC2[j] - 1 + shift_len] : D[PC2[j] - 29 + shift_len];
		}
	}
}

void Enc(uint8 *txt, uint8 *enc,int (*K)[56]) {

	//
	int plain[64];
	int L[32], R[32];
	int i;
	int j;
	for ( i = 0;i < 8;i++) {
		for ( j = 0;j < 8;j++) {
			plain[i * 8 + j] = (txt[i] >> (7 - j)) & 1;
		}
	}

	initial_p(plain);


	for ( i = 0;i < 32;i++) {
		L[i] = plain[i];
		R[i] = plain[i + 32];
	}

	for ( i = 0;i < 16;i++) {
		Round(L, R, K[i]);
	}

	for ( i = 0;i < 32;i++) {
		plain[i] = R[i];
		plain[i + 32] = L[i];
	}

	final_p(plain);
	//for (int i = 0;i < 64;i++) {		cout << plain[i];	}	cout << endl;
	for ( i = 0;i < 8;i++)
		for ( j = 7;j >= 0;j--) {
			enc[i] |= plain[i * 8 + (7 - j)] << j;//+����|��
		}


}
void Dec(uint8 *txt, uint8 *dec,int (*K)[56]) {


	int plain[64];
	int L[32], R[32];
	int i;
	int j;
	for ( i = 0;i < 8;i++) {
		for ( j = 0;j < 8;j++) {
			plain[i * 8 + j] = (txt[i] >> (7 - j)) & 1;//plain[i * 8 + j]����0����1
		}
	}
	initial_p(plain);


	for ( i = 0;i < 32;i++) {
		L[i] = plain[i];
		R[i] = plain[i + 32];
	}
	for ( i = 0;i < 16;i++) {
		Round(L, R, K[15 - i]);
	}

	for ( i = 0;i < 32;i++) {
		plain[i] = R[i];
		plain[i + 32] = L[i];
	}
	final_p(plain);

	for ( i = 0;i < 8;i++)
		for ( j = 7;j >= 0;j--) {
			dec[i] |= plain[i * 8 + (7 - j)] << j;
		}
	
}







int test_Enc48(unsigned char * pi_ucTxt, unsigned char * pi_ucKey1, unsigned char * pi_ucOut,int (*K)[56]) 
{
	

	setK((uint8*)pi_ucKey1,K);
	Enc((uint8*)(pi_ucTxt+ 0), (uint8*)(pi_ucOut+ 0) ,K);
	Enc((uint8*)(pi_ucTxt+ 8), (uint8*)(pi_ucOut+ 8) ,K);
	Enc((uint8*)(pi_ucTxt+16), (uint8*)(pi_ucOut+16) ,K);
	Enc((uint8*)(pi_ucTxt+24), (uint8*)(pi_ucOut+24) ,K);
	Enc((uint8*)(pi_ucTxt+32), (uint8*)(pi_ucOut+32) ,K);
	Enc((uint8*)(pi_ucTxt+40), (uint8*)(pi_ucOut+40) ,K);



	return 1;
}


int test_Dec48(unsigned char * pi_ucTxt, unsigned char * pi_ucKey1, unsigned char * pi_ucOut,int (*K)[56]) 
{
	
	
	setK((uint8*)pi_ucKey1,K);

	Dec((uint8*)(pi_ucTxt+ 0), (uint8*)(pi_ucOut+ 0) ,K);
	Dec((uint8*)(pi_ucTxt+ 8), (uint8*)(pi_ucOut+ 8) ,K);
	Dec((uint8*)(pi_ucTxt+16), (uint8*)(pi_ucOut+16) ,K);
	Dec((uint8*)(pi_ucTxt+24), (uint8*)(pi_ucOut+24) ,K);
	Dec((uint8*)(pi_ucTxt+32), (uint8*)(pi_ucOut+32) ,K);
	Dec((uint8*)(pi_ucTxt+40), (uint8*)(pi_ucOut+40) ,K);

	

	return 1;
}


void des3enc(unsigned char * pi_ucTxt, unsigned char * pi_ucKey1, unsigned char * pi_ucOut,int (*K)[56]) 
{
	memset(pi_ucOut,0,48);
	test_Enc48(pi_ucTxt,pi_ucKey1,pi_ucOut,K);

}

void des3dec(unsigned char * pi_ucTxt, unsigned char * pi_ucKey1, unsigned char * pi_ucOut,int (*K)[56]) 
{

	memset(pi_ucOut,0,48);

	test_Dec48(pi_ucTxt,pi_ucKey1,pi_ucOut,K);
}

#if 1

/*char buf[12]="616263"*/
/*buf[0] = 0x61;
  buf[1] = 0x62;
  buf[2] = 0x63;
*/
/*16进制字符串转byte数组(将两个16位的数据重新组合为一个8位的16进制数)*/
int Hex_Change_Str(char s[],unsigned char bits[]) 
{
	int i,n = 0;
	int ilengh = strlen(s) ;
#if 0
	char buf[2] = {0x61,0x62};
	printf("buf=%s\n",buf);
	printf("buf[0]=%c,buf[1]=%c\n",buf[0],buf[1]);
#endif

//	printf("s=[%s]\n",s);
	printf("ilengh=%d\n",ilengh);
	
	for(i = 0; i < ilengh; i += 2) 
	{
		if(s[i] >= 'a' && s[i] <= 'f')
		{
			bits[n] = s[i] - 'a' + 10;
		}
		else 
		{
			bits[n] = s[i] - '0';
		}
		
		if(s[i + 1] >= 'a' && s[i + 1] <= 'f')
		{
			bits[n] = (bits[n] << 4) | (s[i + 1] - 'a' + 10);
		}
		else 
		{
			bits[n] = (bits[n] << 4) | (s[i + 1] - '0');
		}
		++n;
	}
	//printf("n=%d\n",n);
	//printf("[%s]\n",bits);
	return n;
}







/*加密*/
char  *send_Enc48_data(char *inBuf ,char *uckey ,char *putBuf ,int count,int (*K_po)[56])
{
	int i,j;
	int remain=0;
//	printf("inBuf:%s\n",inBuf);
	
	remain = count % 48;
	count  = count / 48;				//需要加密的次数(如1,2,3,4次等)
	if(remain != 0)
	{
		remain = 0;
		count += 1;
	}
	char *outBufHex = (char *)calloc(1024,1);
	
	char outputBuf[1024]={0};
	printf("Enc count=%d\n",count);
	for(i=0;i < count; i++)
	{
		//test_Enc48((unsigned char *)(inBuf + i*48),uckey,(unsigned char *)(putBuf + i*48));	//加密	
		des3enc((unsigned char *)(inBuf + i*48),uckey,(unsigned char *)(putBuf + i*48),K_po);
		for(j=0;j<48;j++)
		{
			sprintf(outBufHex+ i*96 + j*2,"%02x",(unsigned char )putBuf[j+i*48]);	//将加密后的字符转换为16进制的数
		}
	}
	printf("outBufHex:%s\n",outBufHex);

	return outBufHex;
	
}


/*解密*/
char *recv_Des48_data(char *inBufhex ,char *uckey ,char *putBuf ,int count,int (*K_po)[56])
{
	int iLenght = -1;
	int i,j;
	int remain=0;
	
	unsigned char str_outbuf[1024] ={0};
	char *DecodeBuf = calloc(1024,1);

	remain = count % 96;
	count = count / 96;				//需要解密的次数(如1,2,3,4次等)

	if(remain != 0)
	{
		remain = 0;
		count += 1;
	}

	Hex_Change_Str(inBufhex,str_outbuf);		//将16进制字符串转换为byte数组
	for(i=0; i< count; i++)
	{
	//	test_Dec48((str_outbuf + i*48),uckey,(putBuf + i*48));		
		des3dec((str_outbuf + i*48),uckey,(putBuf + i*48),K_po);
	}
	memcpy(DecodeBuf,putBuf,strlen(putBuf));
	printf("DecodeBuf:%s\n",DecodeBuf);
	return DecodeBuf;
}

#endif













int create_socket_client(void)
{
	int rtv;
	int fd_Listen;
	struct sockaddr_in srvaddr;
	
	fd_Listen = socket(AF_INET, SOCK_STREAM, 0);
	if(fd_Listen < 0)
	{
		perror("\033[1;32;40m cannot create communication socket ... \033[0m\n");
		return -1;
	}
	socklen_t len = sizeof(srvaddr);
	//bzero(&srvaddr, len);
	memset(&srvaddr,0,len);
	
	srvaddr.sin_family = AF_INET;
	inet_pton(AF_INET, SERVER_IP, &srvaddr.sin_addr);			//将ip地址转换为整形 36.189.241.148 		 / 220.171.33.4
	srvaddr.sin_port = htons(PORT_CLIENT);							//主机字节序转为网络字节序

	
	rtv = connect(fd_Listen, (struct sockaddr *)&srvaddr,sizeof(srvaddr));
	if(rtv == -1)
	{
		perror("\033[1;32;40m connect socket server failed ... \033[0m\n");
		close(fd_Listen);
		return -1;
	}
	printf("client request successful\n");
	return fd_Listen;
}

void *SocketWritePthread(void *arg)
{
	int rewite = -1;
	char *WriteBuf = NULL;
	char pi_ucOut[1024]={0};
	char inbuf[1024] = {0};
	QUEUE_ITEM *item = NULL;

	
	int fd = (int)arg;
	printf("fd:%d\n",fd);

	
	while(1)
	{
	    while (SocketQueue_Sizes4G() == 0)   
		{   
			printf("Queue is  empty ,waiting...\n");
			//SetEventQueueWait(0);	
	        pthread_cond_wait(&sockcond, &SocketTcpWriteMtx);         		 
	    }
	//	SetEventQueueWait(1);
		pthread_mutex_lock(&SocketTcpWriteMtx);
//		printf("pthread_mutex_lock(&socketQueueMtx)...\n");
		printf("Queue is not empty...\n");
		int size = SocketQueue_Sizes4G();
		printf("QueueSizes:%d\n",size);
		item = SocketQueue_Get4G();
		
		printf("item->type:%d\n",item->type);

		
		switch(item->type) {
			case REQUEST_HEART_DATA:
				printf("REQUEST_HEART_DATA\n");
				//SocketEvent *getHeartStatus = (SocketEvent *)item->data;
				printf("item->data:%s\n",(char *)item->data);
				WriteBuf = send_Enc48_data((char *)item->data,loginkey,pi_ucOut,sizeof((char *)item->data)-1,K_po); 	
				//freeEvent(&getHeartStatus);
				break;
		
			case REQUEST_HEART_EMP_SEND:
				printf("REQUEST_HEART_EMP_SEND\n");
			//	SocketEvent *getEmpStatus = (SocketEvent *)item->data;
				WriteBuf = send_Enc48_data((char *)item->data,loginkey,pi_ucOut,sizeof((char *)item->data)-1,K_po); 	
			//	freeEvent(&getEmpStatus);
				break;
				
			case REQUEST_LOG_REP:
				printf("REQUEST_LOG_REP\n");
				//SocketEvent *getLogStatus = (SocketEvent *)item->data;
				WriteBuf = send_Enc48_data((char *)item->data,loginkey,pi_ucOut,sizeof((char *)item->data)-1,K_po); 	
				//freeEvent(&getLogStatus);
				break;
			
			case REQUEST_FINGER_DATA:
				printf("REQUEST_FINGER_DATA\n");
			//	SocketEvent *getFingerStatus = (SocketEvent *)item->data;
				WriteBuf = send_Enc48_data((char *)item->data,loginkey,pi_ucOut,sizeof((char *)item->data)-1,K_po); 	
			//	freeEvent(&getFingerStatus);
				break;
			
			case REQUEST_SET_AMP_REP_RECV:
			  	printf("REQUEST_SET_AMP_REP_RECV\n");
				printf("item->data:%s\n",(char *)item->data);
			//	SocketEvent *getSetAmpRecvStatus = (SocketEvent *)item->data;
				WriteBuf = send_Enc48_data((char *)item->data,loginkey,pi_ucOut,sizeof((char *)item->data)-1,K_po);	
			//	freeEvent(&getSetAmpRecvStatus);
				break;
			
			case REQUEST_SET_AMP_REP_DOWN:
				printf("REQUEST_SET_AMP_REP_DOWN\n");
				//SocketEvent *getSetAmpDownStatus = (SocketEvent *)item->data;
				WriteBuf = send_Enc48_data((char *)item->data,loginkey,pi_ucOut,sizeof((char *)item->data)-1,K_po); 	
				//freeEvent(&getSetAmpDownStatus);
				break;
			
			case REQUEST_GET_AMP_LST_RESP:
				printf("REQUEST_GET_AMP_LST_RESP\n");
				//SocketEvent *getGetAmpLstStatus = (SocketEvent *)item->data;
				WriteBuf = send_Enc48_data((char *)item->data,loginkey,pi_ucOut,sizeof((char *)item->data)-1,K_po); 	
				//freeEvent(&getGetAmpLstStatus);
				break;
			
			case REQUEST_SET_FM_REP:
				printf("REQUEST_SET_FM_REP\n");
				//SocketEvent *getSetFmStatus = (SocketEvent *)item->data;
				WriteBuf = send_Enc48_data((char *)item->data,loginkey,pi_ucOut,sizeof((char *)item->data)-1,K_po); 	
				//freeEvent(&getSetFmStatus);
				break;
			
			case REQUEST_GET_FM_LST_RESP:
				printf("REQUEST_GET_FM_LST_RESP\n");
				//SocketEvent *getGetFmStatus = (SocketEvent *)item->data;
				WriteBuf = send_Enc48_data((char *)item->data,loginkey,pi_ucOut,sizeof((char *)item->data)-1,K_po); 	
				//freeEvent(&getGetFmStatus);
				break;
			
			case REQUEST_FILE_DOWNLOAD_RESP:
				printf("REQUEST_FILE_DOWNLOAD_RESP\n");
				//SocketEvent *getFileDownStatus = (SocketEvent *)item->data;
				WriteBuf = send_Enc48_data((char *)item->data,loginkey,pi_ucOut,sizeof((char *)item->data)-1,K_po); 	
				//freeEvent(&getFileDownStatus);
				break;
			
			case REQUEST_GET_FILE_REP:
				printf("REQUEST_GET_FILE_REP\n");
				//SocketEvent *getGetFileStatus = (SocketEvent *)item->data;
				WriteBuf = send_Enc48_data((char *)item->data,loginkey,pi_ucOut,sizeof((char *)item->data)-1,K_po); 	
				//freeEvent(&getGetFileStatus);
				break;
			
			case REQUEST_ALART_STA_REP:
				printf("REQUEST_ALART_STA_REP\n");
				//SocketEvent *getAlartStaStatus = (SocketEvent *)item->data;
				WriteBuf = send_Enc48_data((char *)item->data,loginkey,pi_ucOut,sizeof((char *)item->data)-1,K_po); 	
				//freeEvent(&getAlartStaStatus);
				break;
			
			case REQUEST_ALART_DYN_REP:
				printf("REQUEST_ALART_DYN_REP\n");
				//SocketEvent *getAlartDynStatus = (SocketEvent *)item->data;
				WriteBuf = send_Enc48_data((char *)item->data,loginkey,pi_ucOut,sizeof((char *)item->data)-1,K_po); 	
				//freeEvent(&getAlartDynStatus);
				break;
			
			case REQUEST_DEV_ALART_REP:
				printf("REQUEST_DEV_ALART_REP\n");
				//SocketEvent *getDevAlartStatus = (SocketEvent *)item->data;
				WriteBuf = send_Enc48_data((char *)item->data,loginkey,pi_ucOut,sizeof((char *)item->data)-1,K_po); 	
				//freeEvent(&getDevAlartStatus);
				break;
			
			case REQUEST_SET_PARAMETER_REP:
				printf("REQUEST_SET_PARAMETER_REP\n");
				//SocketEvent *getSetParameterStatus = (SocketEvent *)item->data;
				WriteBuf = send_Enc48_data((char *)item->data,loginkey,pi_ucOut,sizeof((char *)item->data)-1,K_po); 	
				//freeEvent(&getSetParameterStatus);
				break;

			case REQUEST_GET_PARAMETER_REP:
				printf("REQUEST_GET_PARAMETER_REP\n");
				//SocketEvent *getGetParameterStatus = (SocketEvent *)item->data;
				WriteBuf = send_Enc48_data((char *)item->data,loginkey,pi_ucOut,sizeof((char *)item->data)-1,K_po); 	
				//freeEvent(&getGetParameterStatus);
				break;
				
			default:
				printf("item->type error \n");
				break;
		}
		Free_Queue_Item(item);
		printf("Free_Queue_Item(item) \n");
		
		rewite = write(fd, WriteBuf, strlen(WriteBuf));
		if(rewite < 0)
		{
			perror("\033[1;32;40mclient write data failed ... \033[0m\n");
			break;
		}
	//	printf("SocketWritePthread rewite length:%d\n",rewite);
		//usleep(1000*1000);
		pthread_mutex_unlock(&SocketTcpWriteMtx);
	}

	close(fd);


}

void timer_SendEmpHeart_handler(int m)
{

	char *EmpBufData = NULL;
	char OutBuf[48*4]={0};
	char *sendBuf = NULL;
	int rewite ;
//	EmpBufData = device_blank_request_data();	
//	printf("EmpBufData:%s\n",EmpBufData);

//	sendBuf = send_Enc48_data(EmpBufData, loginkey, OutBuf, strlen(EmpBufData),K_po);
//	printf("sendBuf:%s\n",sendBuf);	
	pthread_mutex_lock(&SocketTcpWriteMtx);
//	printf("timer_SendEmpHeart_handler fd:%d\n",fd_client);
	rewite = write(fd_client, sendBuf, strlen(sendBuf));
	if(rewite < 0)
	{
		perror("\033[1;32;40mclient write data failed ... \033[0m\n");
		return ;
	}

	printf("timer_SendEmpHeart_handler rewite ilenght:%d\n",rewite);
	pthread_mutex_unlock(&SocketTcpWriteMtx);

}


void SendEmpHeartData(void)
{
  	fd_set readfds;
	int retval;
	struct itimerval value,ovalue;
	signal(SIGALRM,timer_SendEmpHeart_handler);
	value.it_value.tv_sec = 5;
	value.it_value.tv_usec = 0;

	value.it_interval.tv_sec = 5;
	value.it_interval.tv_usec = 0;
	retval = setitimer(ITIMER_REAL, &value, &ovalue);
	if(retval == -1)
	{
		perror("setitimer error :\n");
	}
}


void  CreateSocketWritePthread(int fd_client)
{
	int iRet;
	pthread_attr_t a_thread_attr; 
	pthread_attr_init(&a_thread_attr);
	pthread_attr_setstacksize(&a_thread_attr, PTHREAD_STACK_MIN*4);
	printf("start CreateSocketWritePthread ...\n");

	iRet = pthread_create(&SocketTcpWritePthread, &a_thread_attr, SocketWritePthread, (void *)fd_client);
	//iRet = pthread_create(&monitorPthread, NULL, MonitorPthread, NULL);
	//pthread_attr_destroy(&a_thread_attr);
	if(iRet == 0)
	{
		pthread_detach(SocketTcpWritePthread);	//设置为分离属行
	}

	SendEmpHeartData();

/*
	if(pthread_create(&SocketEmpHeartPthread, &a_thread_attr, EmpHeartPthread, (void *)&fd_client) == 0)
	{
		pthread_detach(SocketEmpHeartPthread);	
	}
*/	
	
}



void *SocketReadPthread(void *arg)
{

	int fd = (int)arg;
//	char dec_buf[1024]={0};
	char *dec_buf = (char *)calloc(1024,1);
	char ReadBuf[1024]={0};
	int i,j; 

	
	printf("read waiting ......\r\n");
	while(1)
	{
		memset(ReadBuf,0, sizeof(ReadBuf)-1);
		memset(dec_buf,0,sizeof(dec_buf));

#if 0
		int rtv_time = Client_Select_TimeOver(fd,1,0);
		if(rtv_time > 0)
#endif
		
		{
			while(1)
			{
				int rtvread = read(fd, ReadBuf, 1024);
				if(rtvread < 0)
				{
					perror("\033[1;32;40m client read failed ... \033[0m\n");
					break;
				}
				printf("ReadBuf: %s\r\n", ReadBuf);								//获取服务器下发的消息
			}
			
			recv_Des48_data(ReadBuf,loginkey,dec_buf,strlen(ReadBuf),K_po);		//解密接口函数
			//Parse_Platform_string(dec_buf); 
			free(dec_buf);
			usleep(1000*500);
		
		}
		
	}

	close(fd);

}



void  CreateSocketReadPthread(int fd_client)
{
	int iRet;
	pthread_attr_t a_thread_attr; 
	pthread_attr_init(&a_thread_attr);
	pthread_attr_setstacksize(&a_thread_attr, PTHREAD_STACK_MIN*4);
	printf("start CreateSocketReadPthread ...\n");

	iRet = pthread_create(&SocketTcpReadPthread, &a_thread_attr, SocketReadPthread, (void *)fd_client);
	//iRet = pthread_create(&monitorPthread, NULL, MonitorPthread, NULL);
	//pthread_attr_destroy(&a_thread_attr);
	if(iRet == 0)
	{
		pthread_detach(SocketTcpReadPthread);	//设置为分离属行
	}
	
}



#if 1
//add tmp
int main()
{
	int iRet = 0;
	int i;
	unsigned char trans_buf[48*4]={0};
	unsigned char out_buf[1024]={0};
//	unsigned char tmp_buf2[]= "cmd_typ=log_req,cmd_id=00000000000000000_00000000,meid=012345678912345,imsi=460001234567890,brand=huawei,hardver=v2.39,softver=v01.00,net=4,sig=31";
//	unsigned char tmp_buf2[] = "wd,00000000,460001711930796_861477032135319/249a";
	unsigned char tmp_buf2[] = "cmd_typ=heart_emp_send";

	printf("Before Des3enc len:%ld\n",strlen(tmp_buf2));
	printf("Before Des3enc:%s\n",tmp_buf2);
	
//	uint8 loginkey[]="lqw2xazzi1h9lqw2xazzi1h9";
//	des3enc(tmp_buf2 + 0,loginkey,trans_buf + 0,K_po);

	char *SendBufHex = send_Enc48_data(tmp_buf2,loginkey,trans_buf,strlen(tmp_buf2),K_po);	//加密
	recv_Des48_data(SendBufHex,loginkey,out_buf,strlen(SendBufHex),K_po);		//解密

//	des3dec(trans_buf + 0,loginkey,enc + 0,K_po);
	

	//crc
	unsigned char crc_in_put[]= "wd,00000000,460001711930796_861477032135319/";
	char crc_out_put[256]={0};
	int number_byte ;
	uint16 crc;
#if 0
	crc = CRC16(crc_in_put,strlen((char*)crc_in_put));
	printf("crc:%d\n",crc);
	sprintf(crc_out_put,"%04x",crc);
	printf("Crc result:%s\n",crc_out_put);
	printf("\n");
#else 
	CRC16_Str(crc_in_put,strlen(crc_in_put));		//crc校验
#endif	
	return iRet;
}
#endif

