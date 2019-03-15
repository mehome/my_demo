
#ifndef __HUABREI_UART_H__
#define __HUABREI_UART_H__


#define HUABEI_TYPE_DATA 					'D'  //0x44
#define HUABEI_TYPE_CONFIG 				 	'L'  //0x4C
#define HUABEI_TYPE_OPERATE_COMMAND 		'C'  //0x43
#define HUABEI_TYPE_OPERATE_STATE  		  'S'    //0x53
#define HUABEI_TYPE_OPERATE_KEY 		 	'K'  //0x4B
#define HUABEI_TYPE_OPERATE_PLAY		 	'P'  //0x50
#define HUABEI_TYPE_OPERATE_OPERATE			'O'  //0x4F
#define HUABEI_TYPE_OPERATE_RECV			'R'  //0x52



enum{
	HUABEI_COMMAND_PLAY = 0x01,
    HUABEI_COMMAND_PAUSE,
    HUABEI_COMMAND_EXIT ,
    HUABEI_COMMAND_POWER_OFF ,
    HUABEI_COMMAND_GET_STATE,
    HUABEI_COMMAND_CONNECT_SERVER,
    HUABEI_COMMAND_GET_WECHAT_MSG,
};
enum{
	HUABEI_PLAY_DONE = 0x02,
	HUABEI_PLAY_PLAY = 0x04,
};

typedef     struct  HuabeiUartMsg {
	unsigned char type;
	unsigned char operate;
	unsigned int len;
    char data[128];
}HuabeiUartMsg;

int NotifyToHuabei(HuabeiUartMsg *msg);

typedef      struct HuabeiPacket {
	unsigned char type;
	unsigned char operate;
	unsigned char arg0;
	unsigned char arg1;
}HuabeiPacket;


/*typedef struct       HuabeiUartRecvMsg {
	unsigned char type;
	unsigned char operate;
	unsigned char arg0;
	unsigned char arg1;
	unsigned char res0;
	unsigned char res1;
	unsigned int len;
    char data[128];
} HuabeiUartRecvMsg;*/
enum {
	HUABEI_PACKET_TYPE_DATA 		= 0,
	HUABEI_PACKET_TYPE_COMMAND 		= 1,
};

typedef struct HuabeiUartOperate {
	unsigned char operate;
	unsigned char arg0;
	unsigned char arg1;
	unsigned char res0;
	unsigned char res1;
}HuabeiUartOperate;

typedef struct HuabeiUartData {
	unsigned short len;
	unsigned char res;
	char data[128];
}HuabeiUartData;


typedef struct      HuabeiUartRecvMsg {
	int type;
	union {
		HuabeiUartOperate oper;
		HuabeiUartData data;
	}packet;
} HuabeiUartRecvMsg;

#endif














