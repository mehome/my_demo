#ifndef __BTUP_H__
#define __BTUP_H__

typedef unsigned char 	uint8;
typedef unsigned int 	uint32;
typedef unsigned short	uint16;

#define F_PATH "/tmp/bt_upgrade.bin"

//extern void *uartd_thread(void);
//int set_opt(int fd,int nSpeed, int nBits, char nEvent, int nStop);
#define UARTD_DEBUG 1
#ifdef UARTD_DEBUG  
#define DEBUG_ERR(fmt, args...)    do { fprintf(stderr,"[%s:%d]"#fmt"\r\n", __func__, __LINE__, ##args); } while(0) 
#define DEBUG_INFO(fmt, args...)   do { printf("[%s:%d]"#fmt"\r\n", __func__, __LINE__, ##args); } while(0) 
#else
#define DEBUG_ERR(fmt, args...)   do {} while(0)  
#define DEBUG_INFO(fmt, args...)  do {} while(0)  
#endif
uint8 u_uart_rxbuf[4096] ={0}; 

int m_flashID;
int m_uart_rxLen;
int m_uart_ptr;
int m_erase_to;//uart命令超时设定
int m_uart_retry;//uart命令重试次数设定
int m_uart_error;//uart命令错误代码
uint8 m_uart_rxBuf[5*1024] = {0};//uart操作用的RX命令缓冲区
uint8 *m_pfile_buf;//将要写入flash的bin文件缓冲区，在initdialog时动态分配，需要在程序退出时delete
int m_pfile_len;
uint8 *m_pfileRx_buf;//从flash中读到的bin映像缓冲区，在initdialog时动态分配，需要在程序退出时delete
int m_pfileRx_len;//将要从flash中读取的数据长度
int m_pfileRx_ptr;//读取的数据指针
int m_pRead_addr;//读操作时flash地址
int m_write_ptr;//download时数据指针
int m_write_addr;//download时flash地址
int m_proc_state;
int m_flag_erase;
int rx_repond;



enum{
	BK3266CMD_LinkCheck=0,
	BK3266CMD_ReadReg=3,
	BK3266CMD_WriteReg=1,
	BK3266CMD_SetBaudRate=0x0f,
	BK3266CMD_CheckCRC=0x10,
	BK3266CMD_Reboot=0x0e,
	BK3266CMD_StayRom=0xaa,
	BK3266CMD_Reset=0xfe,
	BK3266CMD_FlashEraseAll=0x0a,
	BK3266CMD_FlashErase4K=0x0b,
	BK3266CMD_FlashErase=0x0f,
	BK3266CMD_FlashWrite4K=0x07,
	BK3266CMD_FlashRead4K=0x09,
	BK3266CMD_FlashWrite=0x06,
	BK3266CMD_FlashRead=0x08,
	BK3266CMD_FlashReadSR=0x0c,
	BK3266CMD_FlashWriteSR=0x0d,
	BK3266CMD_FlashGetMID=0x0e,

};

enum{
	ERR_CODE_Success=1,
	ERR_CODE_Read=-1,
	ERR_CODE_UnProtect=-2,
	ERR_CODE_EraseFlash=-3,
	ERR_CODE_WriteFlash=-4,
	ERR_CODE_WriteCfg=-5,
	ERR_CODE_WriteSec0=-6,
	ERR_CODE_Verify=-7,
	ERR_CODE_Protect=-8,
};


enum{
	PROCSTATE_CAP_INIT=0,
	PROCSTATE_CAP_BUS,
	PROCSTATE_SET_BAUDRATE,
	PROCSTATE_UnProtect,
	PROCSTATE_ERASE_FLASH,
	PROCSTATE_WRITE_SECTORs,
	PROCSTATE_WRITE_SECTOR0,
	PROCSTATE_WRITE_CFG,
	PROCSTATE_VERIFY,
	PROCSTATE_READING,
	PROCSTATE_END
};
enum{
	TARGET_TYP_BK3266=0,//BK3266 IC
		TARGET_TYP_BK3266_OFFLINE_BOARD,//BK3266�ѻ���¼��
		TARGET_TYP_BK3266_OFFLINE_BOARD_SD,//BK3266�ѻ���¼SD��
		TARGET_TYP_BK3266_BTINFO,//BK3266 ������Ϣ����
		TARGET_TYP_BK3266_FLASHUNIT,//BK3266 FLASHָ����ַ
		TARGET_TYP_BK3266_UNKNOWN,

};
enum{
	FLASH_ID_XTX_25F08B=0x14405e,
		FLASH_ID_MXIC_25V8035F=0x1423c2,
		FLASH_ID_UNKNOWN=-1
};


#define BK3266_WDT_BASE_ADDR			0x00F48000
#define BK3266_WDT_CONFIG				(BK3266_WDT_BASE_ADDR+0*4)

#define NUM_RETRY_PROC				3

#define ID_TRUCK_TIMER				1
#define ID_ERASE_TIMER				2
#define ID_WRITE4K_TIMER			3
#define ID_STAYROM_TIMER			4
#define ID_CHECKSR_TIMER			5
#define ID_READ4K_TIMER				6
#define ID_WRITECFG_TIMER			7
#define ID_ERASE4K_TIMER			8
#define ID_RESTORE_TIMER			9
#define ID_ERASE_BLOCK_TIMER		10
#define ID_100MS_TIMER				11
#define ID_20MS_TIMER				12



#endif


