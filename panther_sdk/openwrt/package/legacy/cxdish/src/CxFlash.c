/****************************************************************************************
*****************************************************************************************
***                                                                                   ***
***                                 Copyright (c) 2013                                ***
***                                                                                   ***
***                                Conexant Systems, Inc.                             ***
***                                                                                   ***
***                                 All Rights Reserved                               ***
***                                                                                   ***
***                                    CONFIDENTIAL                                   ***
***                                                                                   ***
***               NO DISSEMINATION OR USE WITHOUT PRIOR WRITTEN PERMISSION            ***
***                                                                                   ***
*****************************************************************************************
**
**  File Name:
**      cxflash.c
**
**  Abstract:
**      This code is to download the firmware to HUDSON device via I2C bus. 
**      
**
**  Product Name:
**      Conexant Hudson
**
**  Remark:
**    only suppport boot loader later than version 4.104.0.0
**      
**  Version: 3.14.0.0.
**  
***  
** 
*****************************************************************************************/
#include <stdint.h>
#include <string.h>
#include "CxFlash.h"
#include "capehostx.h"


#define  __BYTE_ORDER       __BIG_ENDIAN
#define  __BIG_ENDIAN       1234
#if defined(_WINDOWS) 
#define __PACKED__  
//#define  __BYTE_ORDER       __LITTLE_ENDIAN
//#define  __LITTLE_ENDIAN    1234
#else
#include <sys/param.h>
#include <stdio.h>
#define __PACKED__ __attribute__((packed))
#endif


#if defined(DEBUG)
#include <assert.h>
#include <signal.h>
#endif 


#if defined( __BIG_ENDIAN) && !defined(__BYTE_ORDER)
#define __BYTE_ORDER __BIG_ENDIAN
#elif defined( __LITTLE_ENDIAN ) && !defined(__BYTE_ORDER)
#define __BYTE_ORDER __LITTLE_ENDIAN
#endif 

#ifndef __BYTE_ORDER
#error __BYTE_ORDER is undefined.
#endif 

#define ENABLE_I2C_BURST_MODE

#ifndef NULL
#define NULL 0
#endif //#ifndef NULL

enum I2C_STATE{I2C_OK,I2C_ERR,I2C_RETRY,I2C_ERASE_CHIPS} ;

#if defined(_WINDOWS )
#define DBG_ERROR  
#define DBG_INFO   
#define LOG( _msg_ ) printf _msg_
#if !defined(DUMP_I2C)
void ShowProgress(int curPos,int bForceRedraw,int eState,const int MaxPos);
void InitShowProgress(const int MaxPos);
//#define InitShowProgress(MaxPos)  printf( "Downloading ... %2d%%", 0);
//#define ShowProgress(curPos,  bForceRedraw, eState,MaxPos) printf( "\b\b\b%2d%%", (char) (((curPos) *100)/MaxPos) );
#else
#define InitShowProgress(MaxPos)
#define ShowProgress(curPos,  bForceRedraw, eState,MaxPos)
#endif
#else
#define DBG_ERROR  
#define DBG_INFO   
#define LOG( _msg_ ) printf _msg_ 
#define InitShowProgress(MaxPos)  printf( "Downloading ... %3d%%", 0); fflush(stdout);
#define ShowProgress(curPos,  bForceRedraw, eState,MaxPos) printf( "\b\b\b\b%3d%%", (char) (((curPos) *100)/MaxPos) ); fflush(stdout);
//#define InitShowProgress(MaxPos)
//#define ShowProgress(curPos,  bForceRedraw, eState,MaxPos)

#endif

#define BIBF(_x_) if(!(_x_)) break;
#define BIF(_x_) {err_no=_x_; if(err_no) break;}

#ifndef BIBF
#define BIBF( _x_ ) if(!(_x_)) break;
#endif 


#define SFS_MAGIC(d,c,b,a)	((dword)(((a)<<(0*8))|((b)<<(1*8))|((c)<<(2*8))|((d)<<(3*8))))
#define SFS_MAGIC_HEADER	SFS_MAGIC('S','F','S','E')		// ...0x45->...0100 0101
#define SFS_MAGIC_BOOT		SFS_MAGIC('S','F','S','L')		// ...0x4C->..0100 1100	
#define SFS_MAGIC_BOOT_H	SFS_MAGIC('S','F','S','H')	//	0x48	     0100 1000	
#define SFS_MAGIC_END		SFS_MAGIC('S','F','S','O')	//	0x4F       0100 1111
#define SFS_MAGIC_HDRECP	SFS_MAGIC('S','F','S','M')	// ... 0x4D -> ... 0100 1101 [6] ->
#define SFS_MAGIC_HDRPAD	SFS_MAGIC('S','F','S','K')	// ... 0x4B -> ... 0100 1011 [5] ->
#define SFS_MAGIC_HDRDEL	SFS_MAGIC('S','F','S','A')	// ... 0x41 -> ... 0100 0001 [0]
#define SFS_BLKSIZE 4*1024 //4096
#define BLKNONE (blknr)-1
#define FIRST_FREE_BLK	2

typedef unsigned char byte;
typedef unsigned short word;
typedef unsigned long dword;
typedef unsigned int uint;
typedef unsigned short blknr;
//#define offsetof(s,m)	((uint)(int)&(((s *)0)->m))

#define SFS_HEADER_DEF_POS	0
#define SFS_HEADER_ALT_POS	1
#define SFS_FIRST_FREE_BLK	2
#define SFS_SECOND_FREE_BLK	3


#define BADBEFF  0X8BADD00D
#define READY_POOLING_INTERVAL 10 /*10 ms*/
#define READY_POOLING_LOOP (2000/READY_POOLING_INTERVAL) /*2 s*/
#define READY_POOLING_LOOP_MANUALLY (10000/READY_POOLING_INTERVAL) /*10 s*/



#if defined(_WINDOWS) 
#pragma pack(push,1)
#endif

struct sfs_end_s {
	dword magic;			// ID of a sfs end record
} __PACKED__;

struct sfs_padded_s {
	dword magic;			// ID for the sfs file record
	blknr	asize;			// allocated size of memory in blocks
} __PACKED__;

struct sfs_encapsulated_s {
	dword magic;			// ID for the sfs file record
	blknr	asize;			// allocated size of memory in blocks
} __PACKED__;

enum i2c_cmd_e
{
	I2C_PING,			//  0 ping
	I2C_RSTAT,			//  1 get status
	I2C_WSTAT,			//  2 write status
	I2C_ERASES,			//  3 erase 4K sector
	I2C_ERASEC,			//  4 erase chip
	I2C_READ,			//  5 read from a 4K block, from offset 0
	I2C_WRITE_NORMAL,		//  6 write to a 4K block, from offset 0
	I2C_WRITE_VERIFY,		//  7 write to a 4K block, from offset 0, verify after write
	I2C_ERASE_WRITE_VERIFY,	//  8 erase, write 4K block from offset 0 and verify
	I2C_PROTECT,			//  9 change sw protection (on or off)
	I2C_RTUNNEL,			// 10 send a SPI read command
	I2C_WTUNNEL,			// 11 send a SPI write command
	I2C_START_AUTO,		// 12 Start auto (ping-pong) mode
	I2C_VERIFY_IMG,		// 13 verify image CRC
} ;


struct i2c_message_s
{
	int   cmd:6;			// the command
	uint  err:1;			// error in message
	uint  repl:1;			// done - command completed
	uint  blk:8;			// block nr
	uint  len:16;			// total length of mesage (in bytes)
	dword crc;			// crc over message
} __PACKED__;				//

#define I2C_PAYLOAD_SIZE	4096
#define I2C_PAYLOAD_SIZE_B	(I2C_PAYLOAD_SIZE/sizeof(dword))
#define SFS_LOG2BLKSIZ		12			//
#define SFS_BLOCKSIZE		4096			// size of a SFS block (min allocation unit)
#define SFS_BLK2POS(n)		((n)<<SFS_LOG2BLKSIZ)	//

#define BYTES_IN_A_WORD		2
#define BYTES_IN_A_DWORD	4
#define SHIFT_BYTES_TO_WORDS	1
#define SHIFT_BYTES_TO_DWORDS	2
#define SHIFT_BYTES_TO_DWORDS	2
#define dwsizeb(a)	    (((a)+BYTES_IN_A_DWORD-1)>>SHIFT_BYTES_TO_DWORDS)
//#define I2C_PAYLOAD_SIZE_B	4096
#define I2C_PAYLOAD_SIZE_DW	dwsizeb(I2C_PAYLOAD_SIZE_B)

struct i2c_ping_cmd_s		{ struct i2c_message_s hdr; } __PACKED__;
struct i2c_ping_rpl_s		{ struct i2c_message_s hdr; dword id; } __PACKED__;
struct i2c_rstat_cmd_s		{ struct i2c_message_s hdr; } __PACKED__;
struct i2c_rstat_rpl_s		{ struct i2c_message_s hdr; dword status; } __PACKED__;
struct i2c_wstat_cmd_s		{ struct i2c_message_s hdr; dword status; } __PACKED__;
struct i2c_wstat_rpl_s		{ struct i2c_message_s hdr; dword status; } __PACKED__;
struct i2c_erases_cmd_s		{ struct i2c_message_s hdr; word  sector; word padding; } __PACKED__;
struct i2c_erases_rpl_s		{ struct i2c_message_s hdr; dword status; } __PACKED__;
struct i2c_erasec_cmd_s		{ struct i2c_message_s hdr; } __PACKED__;
struct i2c_erasec_rpl_s		{ struct i2c_message_s hdr; dword status; } __PACKED__;
struct i2c_read_cmd_s		{ struct i2c_message_s hdr; word  sector; word length; } __PACKED__;
///////////////////////////////////////////////////////////////////////////////////////
//In order to redcued the memory usage, we splite the large structures into header and payload.
//struct i2c_read_rpl_s		{ struct i2c_message_s hdr; word  sector; word length; dword data[I2C_PAYLOAD_SIZE_B]; };
struct i2c_read_rpl_h_s		{ struct i2c_message_s hdr; word  sector; word length;} __PACKED__; 
struct i2c_read_rpl_d_s		{ dword data[I2C_PAYLOAD_SIZE_B]; } __PACKED__;
//struct i2c_write_cmd_s		{ struct i2c_message_s hdr; word  sector; word length; dword data[I2C_PAYLOAD_SIZE_B]; };
struct i2c_write_cmd_h_s		{ struct i2c_message_s hdr; word  sector; word length; } __PACKED__;
struct i2c_write_cmd_d_s		{  dword data[I2C_PAYLOAD_SIZE_B]; } __PACKED__;
struct i2c_write_rpl_s		{ struct i2c_message_s hdr; dword status; } __PACKED__;
struct i2c_protect_cmd_s	{ struct i2c_message_s hdr; dword state;  } __PACKED__;
struct i2c_protect_rpl_s	{ struct i2c_message_s hdr; dword state;  } __PACKED__;
struct i2c_rtunnel_cmd_s	{ struct i2c_message_s hdr; dword cmd; word clen; word rlen; } __PACKED__;
//struct i2c_rtunnel_rpl_s	{ struct i2c_message_s hdr; dword cmd; word clen; word rlen; dword data[I2C_PAYLOAD_SIZE_B]; };
struct i2c_rtunnel_rpl_h_s	{ struct i2c_message_s hdr; dword cmd; word clen; word rlen; } __PACKED__;
struct i2c_rtunnel_rpl_d_s	{ dword data[I2C_PAYLOAD_SIZE_B]; } __PACKED__;
//struct i2c_wtunnel_cmd_s	{ struct i2c_message_s hdr; dword cmd; word clen; word wlen; dword data[I2C_PAYLOAD_SIZE_B]; };
struct i2c_wtunnel_cmd_h_s	{ struct i2c_message_s hdr; dword cmd; word clen; word wlen; } __PACKED__;
struct i2c_wtunnel_cmd_d_s	{ dword data[I2C_PAYLOAD_SIZE_B]; } __PACKED__;
struct i2c_wtunnel_rpl_s	{ struct i2c_message_s hdr; } __PACKED__;
struct i2c_verify_img_cmd_s	{ struct i2c_message_s hdr; word header; word first; word last; word padding; dword magic; dword autom;} __PACKED__;
struct i2c_verify_img_rpl_s	{ struct i2c_message_s hdr; dword status; } __PACKED__;

struct i2c_start_auto_cmd_h_s   { struct i2c_message_s hdr; dword part_size; dword hdr_len; } __PACKED__;
struct i2c_start_auto_cmd_s     { struct i2c_message_s hdr; dword part_size; dword hdr_len; dword data[I2C_PAYLOAD_SIZE_DW]; } __PACKED__;
struct i2c_start_auto_rpl_s     { struct i2c_message_s hdr; dword status;  } __PACKED__;



struct i2c_xfer_s
{
	int                 xfer_flag;
	unsigned int        block_index;
	unsigned int        check_sum;
} __PACKED__;

struct i2c_xfer_reno_s
{
	struct i2c_xfer_s   xfer;
	unsigned int        num_dword; // unit is dword.
} __PACKED__;

struct i2c_legacy_s             { struct i2c_xfer_reno_s hdr; dword data[I2C_PAYLOAD_SIZE_DW]; } __PACKED__;


// reno
struct sfs_header_s
{
	dword magic;			// 0000 ID of a sfs header
	blknr asize;			// 0004 allocated size of this chunk, should be 1
	blknr sfssize;		// 0006 maximum size of the sfs file system
	blknr nxthdr;			// 0008 should be 1 or -1
	word  flags;			// 000A
#define SFS_NO_USB_BOOT	(1<<0)	//  1=do not boot over USB (default)
#define SFS_PIMARY_BOOT	(1<<1)	//  1=primary image is image[0], 0=primary image is image[1]
	blknr image[2];		// 000C location of the boot images, primary and secondary
	blknr uflash;			// 0010 location of the uflash boot loader (if any)
	blknr iflash;			// 0012 location of the iflash boot loader (if any)
	blknr xflash[4];		// 0014 location of the aux boot codes
	dword version;		// 001c version of sfs
	dword crc_seed;		// 0020 crc seed
	word  usb_rst_timeout;	// 0024 timeout (in ms) until USB reset must be received
	word  usb_dfu_timeout;	// 0026 timeout (in ms) until 1st DFU packet must be received
	dword _reserved1;		// 002A
	blknr sfsbeg;			// 002C start of sfs data area
	blknr sfsend;			// 002E end of of sfs data area
} __PACKED__;


struct sfs_header_v300_s
{
	dword magic;			// 0000 ID of a sfs header
	blknr asize;			// 0004 allocated size of this chunk, should be 1
	blknr sfssize;		// 0006 maximum size of the sfs file system
	blknr nxthdr;			// 0008 should be 1 or -1
	blknr boot;			// 000A location of the boot loader
	blknr image;			// 000C location of the boot images (if any)
	word  _reserved_w1;	// 000E
	dword version;		// 0010 version of sfs
	dword crc_seed;		// 0014 crc seed
	blknr uflash;			// 0018 location of the uflash boot loader (if any)
	blknr iflash;			// 001a location of the iflash boot loader (if any)
	dword _reserved_d2;	// 001c
	dword _reserved1;		// 0020
	blknr sfsbeg;			// 002C start of sfs data area
	blknr sfsend;			// 002E end of of sfs data area
	dword descr[4];		// 0030 description (8-bit ASCII)
} __PACKED__;


union sfs_record_u
{
	struct sfs_header_s  hdr;
	dword		       d[SFS_BLKSIZE/4];
	byte		       b[SFS_BLKSIZE];
} __PACKED__;


#if defined(_WINDOWS) 
#pragma pack(pop)
#endif

#define INIT_I2CS_MESSAGE 0x000080

#undef ERROR 
#undef ERROR_TIMEOUT
#undef ERROR_NOT_SUPPORTED

enum {
	ERROR_BLOCK_NR=-1,
	ERROR_CHECKSUM=-2,
	ERROR_LENGTH=-3,
	ERROR_TIMEOUT=-4,
	ERROR_NOT_SUPPORTED=-5,
	ERROR_ERASE=-6,
	ERROR_WRITE=-7,
	ERROR_VERIFY=-8,
	ERROR_INVALID_CMD=-9,
	ERROR_CHIP_NSUPP=-10,

	ERROR_I2C_INTERNAL=-20,
};


/*
set S  [binary format i 83]; # device->host: standing by for next packet
set T  [binary format i 84]; # device->host: bad packet. retry.
set F  [binary format i 70]; # device->host: fatal error

set R  [binary format i 82]; # host->device: packet transfer complete
set D  [binary format i 68]; # host->device: file transfer complete
set A  [binary format i 65]; # host->device: aborting transfer
set C  [binary format i 67]; # host->device: packet transfer complete
*/


#define    TRANSFER_STATE_COMPLETE        0x43u  //host->device: packet transfer complete. For bootloader.
#define    TRANSFER_STATE_ABORT_TRANSFER  0x41u  // # host->device: aborting transfer
#define    TRANSFER_STATE_FILE_DONE       0x44u  //# host->device: file transfer complete
#define    TRANSFER_STATE_PACKET_READY    0x52u  //# host->device: packet transfer complete
#define    TRANSFER_STATE_FATAL           0x46u  //# device->host: fatal error
#define    TRANSFER_STATE_BAD_PACKET      0x54u  //# device->host: bad packet. retry
#define    TRANSFER_STATE_STANDBY         0x53u  //#  device->host: standing by for next packet.
#define    TRANSFER_STATE_RENO_LOADER_READY  0X80u
#define    TRANSFER_STATE_TRANSFER_COMPLETE  0x4fu//0x8000fbdf //0x4fu  //#  device->host: Trnasfer complete, report CRC





#define TIME_OUT_MS          100   //50 ms
#define MANUAL_TIME_OUT_MS   5000  //1000 ms
#define POLLING_INTERVAL_MS  1     // 1 ms
#define RESET_INTERVAL_MS    200    //200 ms
#define IMG_BLOCK_SIZE       2048
#define IMG_BLOCK_SIZE_RENO  4096
#define MAX_RETRIES          3
#define MAX_I2C_SUB_ADDR_SIZE 4
#define ALIGNMENT_SIZE       (sizeof(long))
#define MAX_WRITE_BUFFER     (sizeof(struct i2c_verify_img_cmd_s))
#define MAX_READ_BUFFER      (sizeof(struct i2c_rtunnel_rpl_h_s) ) 
#define BUFFER_SIZE          (ALIGNMENT_SIZE+MAX_WRITE_BUFFER+ MAX_READ_BUFFER )
#define ALIG_SIZE(_x_)       ((sizeof(_x_) + ALIGNMENT_SIZE-1)/ALIGNMENT_SIZE)
#define MAX_SMALL_WRITE      0x10
#define _SFS_VERSION(a,b,c,d)	((uint32_t)((a)<<24)|((b)<<16)|((c)<<8)|((d)<<0))

fn_I2cReadMem        g_I2cReadMemPtr            = NULL;
fn_I2cWriteMem       g_I2cWriteMemPtr           = NULL;
fn_SetResetPin       g_SetResetPinPtr           = NULL;

unsigned char *      g_pBuffer                  = NULL;    
unsigned char *      g_pWrBuffer                = NULL; 
unsigned char *      g_pRdBuffer                = NULL; 

unsigned int        g_cbMaxI2cWrite            = 0;
unsigned int        g_cbMaxI2cRead             = 0;
void *               g_pContextI2cWrite         = NULL;
void *               g_pContextI2cWriteThenRead = NULL;
void *               g_pContextSetResetpin      = NULL;
unsigned char        g_bChipAddress             = 0x00;                   /*Specify the i2c chip address*/
unsigned char        g_bAddressOffset           = 2;
int                  g_bIsRenoDev               = 1;
int                  g_bBootTimeUpgrade         = 1; //fix me
int                  g_legacy_device            = 0;
uint                 g_i2c_blknr                = 0; 
unsigned int         g_bEraseChip               = 0;
int                  g_is_partial_img           = 1;
word                 g_partial_offset           = 0;
int                  g_is_dual_img              = 0;
uint32_t             g_firmware_version[4]      = {(uint32_t)-1};
SFS_UPDATE_PARTITION g_update_mode              = SFS_UPDATE_BOTH;
uint32_t			 g_sfs_version			    = 0;

#ifdef __cplusplug
extern "C" {
#endif
extern int debugflag;

#ifdef __cplusplug
}
#endif
const char* const update_mode_str[]=
{
	"Auto",  "0",  "1",  "Both"
};

// supported SPI memory.
#define ID_S25FL032P		0x00150201
#define ID_AT25DF041A		0x0001441F
#define ID_AT25DQ161		0x0000861F
#define ID_AT25DQ321A		0x0000871F
#define ID_AT25DF641		0x0000481F
#define ID_AT25DF321A		0x0001471F
#define ID_AT25SF321		0x0001871F
#define ID_GD25Q80B			0x001440C8
#define ID_GD25Q32B			0x001640C8
#define ID_GD25Q32C			0x001640C8
#define ID_GD25LQ64C		0x001760C8
#define ID_GD25LQ32C		0x001660C8
#define ID_MX25L4006E		0x001320C2
#define ID_MX25L8006E		0x001420C2
#define ID_MX25L1606E		0x001520C2
#define ID_MX25L3206E		0x001620C2
#define ID_MX25L6406E		0x001720C2
#define ID_S25FL116K		0x00154001
#define ID_S25FL132K		0x00164001
#define ID_S25FL164K		0x00174001
#define ID_SST25VF016B		0x004125BF
#define ID_W25Q80DV		    0x001440EF
#define ID_W25Q16DV		    0x001540EF
#define ID_W25Q32FV		    0x001640EF
#define ID_W25Q64FV		    0x001740EF
#define ID_W25Q128FV		0x001840EF
#define ID_W25Q64FW			0x001660EF
#define ID_N25Q32AE			0x0016BA20
#define ID_PM25LQ32C		0x15469D7F



struct id_2_name{
	uint   id;
	char   name[20];
};

struct err_2_name{
	int    err_no;
	char   name[20];
};


#define ID_NAME(_x_) { ID_##_x_,#_x_}
const struct id_2_name g_spiid_2_name_tlb[]=
{
  ID_NAME(S25FL032P),
  ID_NAME(AT25DF041A),
  ID_NAME(AT25DQ161),
  ID_NAME(AT25DQ321A),
  ID_NAME(AT25DF641),
  ID_NAME(AT25DF321A),
  ID_NAME(AT25SF321),
  ID_NAME(GD25Q80B),
  ID_NAME(GD25Q32B),
  ID_NAME(GD25Q32C),
  ID_NAME(GD25LQ64C),
  ID_NAME(GD25LQ32C),
  ID_NAME(MX25L4006E),
  ID_NAME(MX25L8006E),
  ID_NAME(MX25L1606E),
  ID_NAME(MX25L3206E),
  ID_NAME(MX25L6406E),
  ID_NAME(S25FL116K),
  ID_NAME(S25FL132K),
  ID_NAME(S25FL164K),
  ID_NAME(SST25VF016B),
  ID_NAME(W25Q80DV),
  ID_NAME(W25Q16DV),
  ID_NAME(W25Q32FV),
  ID_NAME(W25Q64FV),
  ID_NAME(W25Q128FV),
  ID_NAME(W25Q64FW),
  ID_NAME(N25Q32AE),
  ID_NAME(PM25LQ32C)
};

#define BLERR_NAME(_x_) { BL##_x_,#_x_}
const struct err_2_name g_loader_err_2_name[] =
{
	BLERR_NAME(ERR_BLOCK_NR),
	BLERR_NAME(ERR_CHECKSUM),
	BLERR_NAME(ERR_LENGTH),
	BLERR_NAME(ERR_TIMEOUT),
	BLERR_NAME(ERR_NSUPP),
	BLERR_NAME(ERR_NPROT),
	BLERR_NAME(ERR_ERASE),
	BLERR_NAME(ERR_WRITE),
	BLERR_NAME(ERR_VERIFY),
	BLERR_NAME(ERR_INVALID_CMD),
	BLERR_NAME(ERR_NOTHING_TO_DO),
	BLERR_NAME(ERR_INVALID_SFS_HDR),
	BLERR_NAME(ERR_I2C_INTERNAL),
};

void test_rw();
word num_non_ff(word range, const uint *data);

/*
* The SetupI2cWriteMemCallback sets the I2cWriteMem callback function.
* 
* PARAMETERS
*  
*    pCallbackContext [in] - A pointer to a caller-defined structure of data items
*                            to be passed as the context parameter of the callback
*                            routine each time it is called. 
*
*    I2cWritePtr      [in] - A pointer to a i2cwirte callback routine, which is to 
*                            write I2C data. The callback routine must conform to 
*                            the following prototype:
* 
*                        int (*fn_I2cWriteMem)(  
*                                void * context,
*                                unsigned char slave_addr,
*                                unsigned long sub_addr,
*                                unsigned long write_len, 
*                                unsigned char* write_buf,
*                             );
*
*                        The callback routine parameters are as follows:
*
*                        context          [in] - A pointer to a caller-supplied 
*                                                context area as specified in the
*                                                CallbackContext parameter of 
*                                                SetupI2cWriteMemCallback. 
*                        slave_addr       [in] - The i2c chip address.
*                        sub_addr         [in] - The i2c data address.
*                        write_len        [in] - The size of the output buffer, in bytes.
*                        write_buf        [in] - A pointer to the putput buffer that contains 
*                                                the data required to perform the operation.
*
*    cbMaxWriteBufSize  [in] - Specify the maximux transfer size for a I2c continue 
*                              writing with 'STOP'. This is limited in I2C bus Master
*                              device. The size can not less then 4.
*
* RETURN
*  
*    None
*
*/
extern void SetupI2cWriteMemCallback( void * pCallbackContext,
	fn_I2cWriteMem         I2cWritePtr,
	unsigned int          cbMaxWriteBufSize)
{
	g_pContextI2cWrite  = pCallbackContext;
	g_I2cWriteMemPtr    = I2cWritePtr;
	g_cbMaxI2cWrite     = cbMaxWriteBufSize&0xfffffffc;
}

/*
* The SetupI2cReadMemCallback sets i2cReadMem callback function.
* 
* PARAMETERS
*  
*    pCallbackContext    [in] - A pointer to a caller-defined structure of data items
*                               to be passed as the context parameter of the callback
*                               routine each time it is called. 
*
*    pI2cReadMemPtr      [in] - A pointer to a i2cwirte callback routine, which is to 
*                               write I2C data. The callback routine must conform to 
*                               the following prototype:
*
*                        int (*fn_I2cReadMem)(  
*                                void * context,
*                                unsigned char slave_addr,
*                                unsigned long sub_addr,
*                                unsigned long rd_len
*                                void*         rd_buf,
*                             );
*
*                        The callback routine parameters are as follows:
*
*                         context            [in]  - A pointer to a caller-supplied 
*                                                    context area as specified in the
*                                                    CallbackContext parameter of 
*                                                    SetupI2cWriteMemCallback. 
*                         slave_addr         [in]  - The i2c chip address.
*                         sub_addr           [in]  - slave addr.
*                         rd_buf             [out] - A pointer to the input buffer 
*                         rd_len             [in]  - Specify the read data size.
*
*   cbMaxI2CRead         [in] - Specify the maximux transfer size for a I2c continue 
*                               read. The size can not less then 4.
* RETURN
*      None
* 
*/
extern void SetupI2cReadMemCallback( void * pCallbackContext,
	fn_I2cReadMem pI2cReadMemPtr, 
	unsigned int cbMaxI2CRead)
{
	g_pContextI2cWriteThenRead  = pCallbackContext;
	g_I2cReadMemPtr             = pI2cReadMemPtr;
	g_cbMaxI2cRead              = cbMaxI2CRead & ~((unsigned int) 0x3); // Needs 4 bytes aligned.
}


/*
* Set the SetResetPin callback function.
* 
* PARAMETERS
*  
*    pCallbackContext    [in] - A pointer to a caller-defined structure of data items
*                               to be passed as the context parameter of the callback
*                               routine each time it is called. 
*
*    SetResetPinPtr      [in] - A pointer to a i2cwirte callback routine, which is to 
*                               write I2C data. The callback routine must conform to 
*                               the following prototype:
*
*                        int (*fn_SetResetPin)(  
*                                  void * pCallbackContext,
*                                  int    bSet );
*
*                        The callback routine parameters are as follows:
*
*                         pCallbackContext [in] - A pointer to a caller-supplied 
*                                                 context area as specified in the
*                                                 CallbackContext parameter of 
*                                                 SetupI2cWriteMemCallback. 
*                         bSet             [in] - Indicates whether to high or low the GPIO pin.
*
* RETURN
*  
*    If the operation completes successfully, the return value is ERRNO_NOERR.
*    Otherwise, return ERRON_* error code. 
*
*/
extern void SetupSetResetPin(  void * pCallbackContext,
	fn_SetResetPin  SetResetPinPtr)
{
	g_SetResetPinPtr = SetResetPinPtr;
	g_pContextSetResetpin = pCallbackContext;
}


/*
* cx_get_buffer_size.
*
*  Calculates the buffer size required for firmware update processing.. 
* 
* PARAMETERS
*    None
*
* RETURN
*  
*    return buffer size required for firmware update processing.. 
*
*/
extern uint32_t GetSizeOfBuffer(void)
{
	return BUFFER_SIZE;
}




/*
* Convert a 4-byte number from a ByteOrder into another ByteOrder.
*/
uint32_t  byte_swap_ulong(uint32_t i)
{
	return((i&0xff)<<24)+((i&0xff00)<<8)+((i&0xff0000)>>8)+((i>>24)&0xff);
}

/*
* Convert a 2-byte number from a ByteOrder into another ByteOrder.
*/
uint16_t byte_swap_ushort(uint16_t i)
{
	return ((i>>8)&0xff)+((i << 8)&0xff00);
}


/*
* Convert a 4-byte number from generic byte order into Little Endia
*/
uint32_t to_little_endia_ul(uint32_t i)
{
#if  __BYTE_ORDER != __LITTLE_ENDIAN
	return byte_swap_ulong(i);
#else

	return i;
#endif
}

/*
* Convert a 2-byte number from generic byte order into Little Endia
*/
uint16_t to_little_endia_us(uint16_t i)
{
#if __BYTE_ORDER != __LITTLE_ENDIAN

	return byte_swap_ushort(i);
#else

	return i;
#endif
}



/*
* Convert a 4-byte number from little endia into generic byte
*/
uint32_t from_little_endia_ul(uint32_t i)
{
#if  __BYTE_ORDER != __LITTLE_ENDIAN

	return byte_swap_ulong(i);
#else

	return i;
#endif
}

// return 1 if the cpu is little_endian
int is_little_endian()
{
	unsigned int endianness = 1;
	int x=0x12345678; /* 305419896 */  
  
	unsigned char *p=(char *)&x;  
  
//	printf("%0x % 0x %0x %0x",p[0],p[1],p[2],p[3]);
	return ((char*)(&endianness))[0]==1;
}


void signal_mem_handler  ( int n )
{
	LOG((DBG_ERROR "A segmentation fault occurs. number = %d\n",n));
}
/*
* Read a byte from the specified  register address.
* 
* PARAMETERS
*  
*    i2c_sub_addr             [in] - Specifies the register address.
*    pErrNo              [out]  A pointer to a int, to retrieve the error code value. 
*                               If operation is successful, then the number is zero. Otherwise,
*                               return ERRON_* error code. 
*
* RETURN
*  
*    Returns the byte that is read from the specified register address.
*
*/
uint32_t i2c_sub_read(uint32_t i2c_sub_addr, int *pErrNo)
{
	uint32_t  val;
//	DEBUG_ERR("i2c_sub_addr = 0x%x",i2c_sub_addr);
	if(!g_I2cReadMemPtr)
	{
		LOG((DBG_ERROR "i2C function is not set.\n"));
		if(pErrNo) *pErrNo = -ERRNO_I2CFUN_NOT_SET;
		return 0;
	}

	if ( i2c_sub_addr & 0x3)
	{
		LOG((DBG_ERROR "The I2C read address is NOT 4 bytes aligned \n"));
		if(pErrNo) *pErrNo =-ERROR_I2CREAD_ADDR_MISALIGNEMNT;
	}
//	i2c_sub_addr = to_little_endia_ul(i2c_sub_addr);
	if(g_I2cReadMemPtr(g_pContextI2cWriteThenRead, g_bChipAddress,
		i2c_sub_addr,4,(unsigned char*)&val)<0)
	{
		if(pErrNo) *pErrNo = -ERROR_I2C_ERROR;
	}
	else
	{
 	 //  DEBUG_ERR( "read val = 0x%x\n",val);
		if(pErrNo) *pErrNo = ERRNO_NOERR;
		val = from_little_endia_ul(val);
	//  DEBUG_ERR( "read from_little_endia_ul val= 0x%x\n",val));
	}
	return val;
}


/*
* Write a byte from the specified register address.
* 
* PARAMETERS
*  
*    i2c_sub_addr             [in] - Specifies the register address.
*
* RETURN
*  
*    Returns the byte that is read from the specified register address.
*
* REMARK
* 
*    The g_I2cWriteMemPtr must be set before calling this function.
*/
int i2c_sub_write(uint32_t i2c_sub_addr, uint32_t i2c_data)
{

	if(!g_I2cWriteMemPtr)
	{
		LOG((DBG_ERROR "i2C function is not set.\n"));
		return -ERRNO_I2CFUN_NOT_SET;
	}

	if ( i2c_sub_addr & 0x3)
	{
		LOG((DBG_ERROR "The I2C write address is NOT 4 bytes align \n"));
		return -ERROR_I2CWRITE_ADDR_MISALIGNEMNT;
	}

	// the address is big-endian, but the data is little-endian.
	i2c_data= to_little_endia_ul(i2c_data);
//	i2c_sub_addr= to_little_endia_ul(i2c_sub_addr);  //单独转换地址，会出现读到0x54的情况
//同时转换和单独转换数据效果一样，能读到43 53 80
	return g_I2cWriteMemPtr(g_pContextI2cWrite,g_bChipAddress,i2c_sub_addr,4,(uint8_t*) &i2c_data);

}

/*
*  Writes a number of bytes from a buffer to Channel via I2C bus.
*  
* PARAMETERS
*  
*    num_byte         [in] - Specifies the number of bytes to be written
*                              to the memory address.
*    pData              [in] - Pointer to a buffer from an array of I2C data
*                              are to be written.
*  
* RETURN
*  
*    If the operation completes successfully, the return value is ERRNO_NOERR.
*    Otherwise, return ERRON_* error code. 
*/
int i2c_sub_burst_write(uint32_t startAddr, uint32_t num_byte, const unsigned char *pData)
{
	int err_no    = ERRNO_NOERR;
	uint32_t  BytesToProcess       = 0;

	uint32_t  cbMaxDataLen         = g_cbMaxI2cWrite;
	unsigned char *p = NULL;
	p = pData;
//		int i;
//		printf("write\n");
//	for (i = 0;i < 16;i++)
//	printf(" 0x%x ",pData[i]);
//	printf("\n");

	if( num_byte & 0x3 )  
	{
		LOG((DBG_ERROR "The data size for I2C write is NOT 4 bytes aligned \n"));
		return -ERROR_I2CWRITE_DATA_MISALIGNMENT;
	}
	if ( startAddr & 0x3)
	{
		LOG((DBG_ERROR "The I2C write address is NOT 4 bytes align \n"));
		return -ERROR_I2CWRITE_ADDR_MISALIGNEMNT;
	}

	if(!g_I2cWriteMemPtr )
	{
		LOG((DBG_ERROR "i2C function is not set.\n"));
		return -ERRNO_I2CFUN_NOT_SET;
	}

	for(;num_byte;)
	{

		BytesToProcess = num_byte > cbMaxDataLen ? cbMaxDataLen :num_byte;
//		startAddr= to_little_endia_ul(startAddr);//如转换这个地址，读不到53并且不能读取fw-version
//		*(uint16_t*)pData = to_little_endia_us(*(uint16_t*)pData);//转换这个，出现53 54交替读取，并且不能读到fw-version

		if(g_I2cWriteMemPtr(g_pContextI2cWrite,g_bChipAddress, startAddr, BytesToProcess,(uint8_t*)pData)<0)
		{
			err_no = -ERROR_I2C_ERROR ;
			break;
		}
		num_byte-=BytesToProcess;
		startAddr+=BytesToProcess;
		pData+=  BytesToProcess;
	}

	return err_no;
}
/*
*  Read a number of bytes from  I2C bus.
*  
* PARAMETERS
*  
*    startAddr            [in] - Specifies the i2c address.
*    num_byte             [in] - Specifies the number of bytes to read 
*    pData                [out] - A pointer to read buffer.
*                              
*  
* RETURN
*  
*    If the operation completes successfully, the return value is ERRNO_NOERR.
*    Otherwise, return ERRON_* error code. 
*/

int i2c_sub_burst_read(uint32_t startAddr, uint32_t num_byte, uint8_t *pData)
{
	int err_no = 0;
	uint32_t cbByteToRead ; 

	int i;
//	printf("read before\n");
//		for (i = 0;i < 16;i ++)
//			printf(" 0x%x ",pData[i]);
//	printf("\n"); 

	if( num_byte & 0x3 )  
	{
		LOG((DBG_ERROR "The data size for I2C read is NOT 4 bytes aligned \n"));
		return -ERROR_I2CREAD_DATA_MISALIGNMENT;
	}
	if ( startAddr & 0x3)
	{
		LOG((DBG_ERROR "The I2C read address is NOT 4 bytes align \n"));
		return -ERROR_I2CREAD_ADDR_MISALIGNEMNT;
	}

	if(!g_I2cReadMemPtr)
	{
		LOG((DBG_ERROR "i2C function is not set.\n"));
		err_no = -ERRNO_I2CFUN_NOT_SET;
		return err_no;
	}


	for(;num_byte >0;)
	{
		cbByteToRead  = num_byte > g_cbMaxI2cRead ?  g_cbMaxI2cRead : num_byte;


		// the address is big-endian, but the data is little-endian.
	//	startAddr = byte_swap_ulong(startAddr); //如果转换地址，读出的fw-version会大小端转换，并且值不对6.22.0.3x
	//	*(uint16_t*)pData = byte_swap_ushort(*(uint16_t*)pData); //此处无影响

		if(g_I2cReadMemPtr(g_pContextI2cWriteThenRead, g_bChipAddress,
			startAddr,cbByteToRead,(uint8_t*)pData)<0)
		{
			err_no = -ERROR_I2C_ERROR;
			break;
		}
		else
		{
			err_no = ERRNO_NOERR;
		}
				
	//	int i;
	//	printf("read after\n");
	//		for (i = 0;i < 16;i ++)
	//			printf(" 0x%x ",pData[i]);
	//	printf("\n"); 

		//update pointer;
	//	DEBUG_ERR("num_byte = %d cbByteToRead = %d",num_byte,cbByteToRead);
		num_byte -= cbByteToRead;
		pData      += cbByteToRead;
		startAddr  +=  cbByteToRead;

	};

	return err_no;
}

/*
*  Transmit a command via I2C bus.
*  
* PARAMETERS
*  
*    header              [in] - A pointer to write buffer of header.
*    num_header          [in] - Specifies the number of bytes to write 
*    payload             [in] - A pointer to write buffer of command.                           
*    num_payload         [in] - Specifies the number of bytes to write 
*  
* RETURN
*  
*    If the operation completes successfully, the return value is ERRNO_NOERR.
*    Otherwise, return ERRON_* error code. 
*/
int i2c_transmit( const unsigned char *header, uint32_t num_header, const unsigned char *payload, unsigned num_payload  )
{
	int err_no = 0;
	int i;
//	for(i = 0;i < num_header;i++){
//		DEBUG_ERR(" 0x%x ",header[i]);		
//	}
//	if(payload != NULL)
//	for (i = 0;i< 16;i++)
//		printf("payload : 0x%x\n",payload[i]);

	if( num_payload && (err_no==ERRNO_NOERR) )
	{
		if( payload == NULL )
		{
			err_no = ERROR_NULL_POINTER;
		}
		else
		{
			err_no = i2c_sub_burst_write(num_header,num_payload, payload);
		}
	}

	err_no = err_no==ERRNO_NOERR ? i2c_sub_burst_write(4,num_header-4, header+4): err_no;
	err_no = err_no==ERRNO_NOERR ? i2c_sub_burst_write(0,4, header): err_no;
	return err_no;
}

// Returns the actual size of the image (without padding)
struct sfs_header_s * ParseImageHeader(const unsigned char * image_data,
	uint32_t      * image_size,
	int *is_unpadded)
{
	uint32_t  cbImageDataInUse;
	union sfs_record_u *pCurRecord;
	struct sfs_header_s *header1;


	struct sfs_header_s *header = (struct sfs_header_s *)image_data;
	header->sfssize = to_little_endia_us(header->sfssize);
	header->magic = byte_swap_ulong((uint)header->magic);


	if (header->magic != SFS_MAGIC_HEADER)
	{
		header = (struct sfs_header_s *)(image_data+SFS_BLKSIZE);
		if ((uint) header->magic != (uint)SFS_MAGIC_HEADER)

		{
			DEBUG_ERR("header->magic  = 0x%x SFS_MAGIC_HEADER = 0x%x ",header->magic,SFS_MAGIC_HEADER);
			return NULL;
		}
	}


	/*Check if the firmware version information is avaiable or not*/
	if( image_data[0x40+3] != 0xff )
	{	int j;
	for( j=0;j<4;j++)
	{
		g_firmware_version[j] = 
			from_little_endia_ul(*((uint32_t*)&image_data[0x40+j*sizeof(uint32_t)]));
		printf(" %d ",g_firmware_version[j]);
	}
	}


	header1 =  (struct sfs_header_s *) &image_data[SFS_HEADER_ALT_POS*SFS_BLKSIZE];
	header1->magic = byte_swap_ulong(header1->magic);
	DEBUG_ERR("header1->magic= %x SFS_MAGIC_HEADER =%x",header1->magic,SFS_MAGIC_HEADER);
	g_is_dual_img = (header1->magic == SFS_MAGIC_HEADER) ? 1:0;
	DEBUG_ERR("g_is_dual_img = %d",g_is_dual_img);
	printf("\n*image_size = 0x%x  (uint32_t)(header->sfssize  = 0x%x g_is_dual_img = %d\n",*image_size,(uint32_t)(header->sfssize),g_is_dual_img);
	if (*image_size > (uint32_t)(header->sfssize*SFS_BLKSIZE))
	{
		return NULL;
	}

	// check if it is an unpadding image. 
	if (*image_size < (uint32_t)(header->sfssize*SFS_BLKSIZE))
	{
		
		header->sfssize = to_little_endia_us(header->sfssize);
		header->magic = byte_swap_ulong((uint)header->magic);
		if(is_unpadded) *is_unpadded = 1;
		return header;
	}
	else
	{
		if(is_unpadded) *is_unpadded = 0;
	}

	cbImageDataInUse  = 0;
	pCurRecord = (union sfs_record_u *)&image_data[FIRST_FREE_BLK*SFS_BLKSIZE];

	while (pCurRecord->hdr.magic != SFS_MAGIC_END)
	{
		uint32_t ulAllocatedSize = pCurRecord->hdr.asize*SFS_BLKSIZE;
		pCurRecord        += pCurRecord->hdr.asize;
		cbImageDataInUse  += ulAllocatedSize;
		if ((cbImageDataInUse + (FIRST_FREE_BLK+1)*SFS_BLKSIZE) > *image_size) {
			return NULL;
		}
	}

	cbImageDataInUse += (FIRST_FREE_BLK+1)*SFS_BLKSIZE;

	*image_size = cbImageDataInUse;
	return header;
}
void swap_p4_p15(const word *d,word *p,uint tlen)
{
		word *dat =d;
/*		int i;		
	for(i = 0;i < 8;i++){
		DEBUG_ERR(" 0x%x ",dat[i]);		
	}
			printf("\n");
	*/
	if(tlen == 12){
		p[0] = dat[0];
		p[1] = (byte_swap_ushort(dat[1]));
		p[2] = (byte_swap_ushort(dat[2]));
		p[3] = (byte_swap_ushort(dat[3]));		
		p[4] =((dat[4]));
		p[5] =((dat[5]));
		p[6] = dat[7];
		p[7] = dat[6];
	}else
	{
		p[0] = ((dat[0]));
		p[1] = (byte_swap_ushort(dat[1]));
		p[2] = (byte_swap_ushort(dat[2]));
		p[3] = (byte_swap_ushort(dat[3]));
		
		p[4] =((dat[4]));
		p[5] =((dat[5]));
		p[6] = ((dat[6]));
		p[7] = (byte_swap_ushort(dat[7]));		
	}
	
/*		for(i = 0;i < 8;i++){
		DEBUG_ERR(" 0x%x ",p[i]);		
	}
			printf("\n");
	*/
}

void swap_p8_p11(const word *d,word *p)
{
	word *dat =d;
		int i;		
/*	for(i = 0;i < 8;i++){
		DEBUG_ERR(" 0x%x ",dat[i]);		
	}
			DEBUG_ERR("\n");
*/	
	for(i = 0;i < 5;i++){
		p[i] = dat[i];		
	}
//			printf("\n");
	p[0] = (byte_swap_ushort(dat[0]));
	p[1] = ((dat[1]));
	p[2] = ((dat[2]));
	p[3] = ((dat[3]));
	p[4] = (byte_swap_ushort(dat[4]));
	p[5] = (byte_swap_ushort(dat[5]));
	p[6] =(byte_swap_ushort(dat[6]));
	p[7] = ((dat[6]));
/*		for(i = 0;i < 8;i++){
		DEBUG_ERR(" 0x%x ",p[i]);		
	}
			DEBUG_ERR("\n");
*/	

}

void swap_p8_p15(const word *d,word *p)
{
	word *dat =d;
/*		DEBUG_ERR("swap_p8_p15\n");
		int i;		
	for(i = 0;i < 8;i++){
		DEBUG_ERR(" 0x%x ",dat[i]);		
	}
			DEBUG_ERR("\n");
	*/
	p[0] = (byte_swap_ushort(dat[0]));
	p[1] = dat[1];
	p[2] = dat[2];
	p[3] = dat[3];

	p[4] = (byte_swap_ushort(dat[4]));
	p[5] = (byte_swap_ushort(dat[5]));
	p[6] = (byte_swap_ushort(dat[6]));
	p[7] = ((dat[7]));
/*		for(i = 0;i < 8;i++){
		DEBUG_ERR(" 0x%x ",p[i]);		
	}
			DEBUG_ERR("\n");
*/	

}

/* RENO specific code*/
uint hash_1(const void *dat,uint n)
{

	const word *d=(const word *)dat;
	const word *p = NULL;
	int i = 0;
	p = (unsigned char *)malloc(n);
/*	for(i = 0;i < 8;i++){
		DEBUG_ERR(" 0x%x ",d[i]);		
	}
			printf("\n");
*/
	if(n ==12){
		swap_p8_p11(d,p);
	}else{
		swap_p8_p15(d,p);
	}
		

	uint s1=0xffff;
	uint s2=0xffff;
	n=n/sizeof(short);
	while(n)
	{
		uint32_t l=n>359?359:n;
		uint j = 0;
		n-=l;
		do {
			
			s2+=s1+=p[j++];
		
		} while (--l);
		s1=(s1&0xffff)+(s1>>16);
		s2=(s2&0xffff)+(s2>>16);
	}
	free(p);
	/* Second reduction step to reduce sums to 16 bits */
	s1=(s1&0xffff)+(s1>>16);
	s2=(s2&0xffff)+(s2>>16);


	return (s2<<16)|s1;
//	return (s1 << 16)|s2;

}

int esase_flage =0;

char *ping_swap(unsigned char *d,unsigned char *p)
{
	unsigned char * dat =d;
//	const word *p = NULL;
	p = (unsigned char *)malloc(8);
	int i;		
/*	for(i = 0;i < 8;i++){
		DEBUG_ERR(" 0x%x ",dat[i]);		
	}
	printf("\n");
	*/
	if (esase_flage == 1){
		p[0] = dat[0];
		p[1] = dat[1];
		p[2] = dat[3];
		p[3] = dat[2];
		
		p[4] = ((dat[7]));
		p[5] = ((dat[6]));
		p[6] =((dat[5]));
		p[7] = ((dat[4]));
		

	}else{
		p[0] = dat[1];
		p[1] = dat[0];
		p[2] = dat[3];
		p[3] = dat[2];

		p[4] = ((dat[6]));
		p[5] = ((dat[7]));
		p[6] =((dat[4]));
		p[7] = ((dat[5]));
	}
/*
	for(i = 0;i < 8;i++){
		DEBUG_ERR(" 0x%x ",p[i]);		
	}
	printf("\n");
		DEBUG_ERR("33333");
*/
	return p;

}


void hash_swap(const unsigned char *d,unsigned char *p)
{
	unsigned char *dat = d;
	int i;
//	for (i = 0;i < 8;i++)
//		DEBUG_ERR("d%d = 0x%x",i,dat[i]);
	p[0] = dat[1];
	p[1] = dat[0];
	if(esase_flage ==1){
		p[2] = dat[2];
		p[3] = dat[3];
		}else{
		p[2] = dat[3];
		p[3] = dat[2];
	}
	p[4] = dat[4];
	p[5] = dat[5];
	p[6] = dat[6];
	p[7] = dat[7];
	
	
//	DEBUG_ERR("");
//	for (i = 0;i < 8;i++)
//		DEBUG_ERR("d = 0x%x",p[i]);

}

uint hash(unsigned char *d,uint n)
{
//	const word *d=(const word *)dat;
	uint s1=0xffff;
	uint s2=0xffff;
	const word *p = NULL;
	p = (unsigned char *)malloc(n);
	n=n/sizeof(short);
	hash_swap(d,p);

	int i = 0;
/*	for (i = 0;i<4;i++)
		DEBUG_ERR("d = 0x%x\n",p[i]);
*/	
	while(n)
	{
		uint32_t l=n>359?359:n;
		n-=l;
		do {
			s2+=s1+=*p++;
		} while (--l);
		s1=(s1&0xffff)+(s1>>16);
		s2=(s2&0xffff)+(s2>>16);
	}
	/* Second reduction step to reduce sums to 16 bits */
	s1=(s1&0xffff)+(s1>>16);
	s2=(s2&0xffff)+(s2>>16);
	//return (s2<<16)|s1;
	return (s1 << 16)|s2;
}


const char* const get_loader_err_msg(int id)
{
	unsigned int i;
	for (i = 0; i<sizeof(g_loader_err_2_name) / sizeof(g_loader_err_2_name[0]);
		i++)
	{
		if (id == g_loader_err_2_name[i].err_no)
			return g_loader_err_2_name[i].name;
	}
	return "Unknown Error";
}

int A(struct i2c_message_s *msg,unsigned char *p){
//	p[0] = ((msg->cmd << 2) & 0xfc)  + ((msg->err << 1)&0x2) + ((msg->repl) & 0x1);
	p[0] = ((msg->cmd) & 0x3f)  + (((msg->err) << 6)&0x40) + (((msg->repl) << 7) & 0x80); //数据高位在低地址

	p[1] = msg->blk;
	p[2] = (msg->len >> 8)&0xff;
	p[3] = msg->len &0xff;
	


	p[4] = (msg->crc >> 8) & 0xff;
	p[5] = (msg->crc) & 0xff;
	p[6] = (msg->crc >> 24) & 0xff;
	p[7] = (msg->crc >> 16) & 0xff;
	int i;
	printf("A\n");
	for (i = 0;i < 8;i ++)
		printf(" 0x%x ",p[i]);
	printf("\n");
}

int B(struct i2c_message_s *msg,unsigned char *p){

//	msg->cmd = (p[0] >> 2) & 0x3f;
	msg->cmd = (p[0] << 2) & 0xfc;
//	msg->err = (p[0] >> 1) & 0x01;
	msg->err = (p[0] >> 6) & 0x02;
//	msg ->repl = (p[0]) & 0x01;
	msg->repl =(unsigned char)((p[0] >>7)&0x01);
	int i = 0;
	i = ((p[0] >>7)&0x01);
	msg->repl = i;
	msg->blk = p[1];
	msg->len =((p[2] << 8) & 0xff00) + ((p[3])&0xff); 
	msg->crc = (p[4] << 8) & 0xff00 + ((p[5]) & 0xff) + (p[6] << 24) & 0xff000000 + ((p[7] << 16) & 0xff0000);
	return 0;
}

int verify_flage = 0;

int i2c_send(const void *tbuf,uint tlen,void *rbuf,uint rlen, const void *tpayld, uint tpaylen, void *rpayld, uint rpayldlen)
{
	int err_no = ERRNO_NOERR;
	struct i2c_message_s *msg;
	int loader_err;
	int cc = 0;
	unsigned char *p = NULL;
	p = (unsigned char *)malloc(tlen);	
	int i;
	unsigned char *ptr = NULL;

	msg=(struct i2c_message_s *)tbuf;
	msg->blk=g_i2c_blknr;
	msg->len= byte_swap_ushort((uint16_t)tlen);
	msg->crc=0;
	if(tlen == 8){
	//	ptr = ping_swap((unsigned char *)msg,p);
		if (esase_flage == 1){
			msg->crc=(hash(msg,tlen));
		}else{			
			msg->crc=(hash(msg,tlen));
		}	
	}else{
		msg->crc=(hash_1(msg,tlen));
	}
	
	if(tlen == 8){
		ptr = ping_swap((unsigned char *)msg,p);		
		err_no = i2c_transmit((uint8_t*)ptr,tlen,(byte *)tpayld,tpaylen);
	}else{
		swap_p4_p15((word*)msg,p,tlen);
		err_no = i2c_transmit((uint8_t*)p,tlen,(byte *)tpayld,tpaylen);
	}
	free(p);
	if(ERRNO_NOERR != err_no) return err_no;
	 
//	DEBUG_ERR("11msg->cmd = %d  msg-> repl = 0x%x	msg-> blk = 0x%x msg->err = 0x%x msg->len = 0x%x msg->crc = 0x%x \n",msg->cmd,msg->repl,msg->blk,msg->err,msg->len,msg->crc); 
		

	msg=(struct i2c_message_s *)rbuf;

	for(;;)
	{
		if(i2c_sub_burst_read(0,4,(uint8_t*)msg) != ERRNO_NOERR)
		{
			return ERROR_I2C_ERROR;
		}
		else if (msg->repl==1){
				break;
		}
		else
			{
		cc ++;
		
	//	if (cc >=20)	
	//	break;
  		}
	}
//	B(msg,p1);
	g_i2c_blknr++;
	
//	printf("22cc = %d msg->cmd = %d  msg-> repl = %d msg-> blk = %d msg->err = %d msg->len = %d  msg->crc = 0x%x \n",cc,msg->cmd,msg->repl,msg->blk,msg->err,msg->len,msg->crc); 
/*	if (verify_flage == 1){
		unsigned char *pt = NULL;
		pt = (unsigned char *)msg;
		for (i = 0;i<8;i++)
			DEBUG_ERR("msg = 0x%x",pt[i]);	
	}
*/
	
	if (msg->err)
	{
		loader_err = msg->cmd;
		if (loader_err == BLERR_BLOCK_NR)
		{
			LOG((DBG_ERROR "\nGot I2C message error: cmd = %d , block nr = %d\n", loader_err, msg->repl));
			g_i2c_blknr = msg->repl;
			return -ERROR_I2C_BLOCK_NR_ERROR;
		} else if (loader_err == BLERR_NSUPP) {
			LOG((DBG_ERROR "\nUnsupported flash memory!\n"));
			return -ERROR_UNSUPPORTED_FLASH_MEMORY;
		}
		LOG((DBG_ERROR "\nGot an error message from bootload : num = %d, %s \n", loader_err,
			get_loader_err_msg(msg->cmd)));

		return -ERROR_SEND_I2C_ERROR;
	}

	if(i2c_sub_burst_read(0,rlen,((uint8_t*)msg)) != ERRNO_NOERR)
	{
		return ERROR_I2C_ERROR;
	}

	if( rpayldlen) 
	{
		err_no = i2c_sub_burst_read(rlen,rpayldlen,(uint8_t*)rpayld) ;
	}
	
	return err_no;
}


int i2c_ping(dword *pRet)
{
	int err_no = ERRNO_NOERR;

	struct i2c_ping_cmd_s *cmd = ( struct i2c_ping_cmd_s *) g_pWrBuffer;
	struct i2c_ping_rpl_s *rpl = (struct i2c_ping_rpl_s *)g_pRdBuffer;
	if(pRet == NULL) return -ERROR_NULL_POINTER;
	cmd->hdr.cmd=I2C_PING;
	cmd->hdr.err=0;
	cmd->hdr.repl=0;
//	test_rw();

	err_no = i2c_send(cmd,sizeof(*cmd),rpl,sizeof(*rpl),NULL,0,NULL,0);
//	DEBUG_ERR("rpl->id = 0x%x err_no = %d",rpl->id,err_no);
	if( err_no == ERRNO_NOERR)
	{
		*pRet = from_little_endia_ul(rpl->id);
	}
	return err_no;
}

int i2c_read_status(dword *pRet)
{
	int err_no = ERRNO_NOERR;
	struct i2c_rstat_cmd_s *cmd =(struct i2c_rstat_cmd_s *) g_pWrBuffer;
	struct i2c_rstat_rpl_s *rpl = (struct i2c_rstat_rpl_s *)g_pRdBuffer;
	if(pRet == NULL) return -ERROR_NULL_POINTER;
	cmd->hdr.cmd=I2C_RSTAT;
	cmd->hdr.err=0;
	cmd->hdr.repl=0;

	err_no = i2c_send(cmd,sizeof(*cmd),rpl,sizeof(*rpl),NULL,0,NULL,0);

	if( err_no == ERRNO_NOERR)
	{
		*pRet = from_little_endia_ul (rpl->status);
	}
	return err_no;
}

int i2c_write_status(dword status,dword *pRet)
{
	int err_no = ERRNO_NOERR;
	struct i2c_wstat_cmd_s *cmd = (struct i2c_wstat_cmd_s *)g_pWrBuffer;
	struct i2c_wstat_rpl_s *rpl = (struct i2c_wstat_rpl_s *)g_pRdBuffer;


	cmd->hdr.cmd=I2C_WSTAT;
	cmd->hdr.err=0;
	cmd->hdr.repl=0;
	cmd->status=to_little_endia_ul(status);
	err_no = i2c_send(cmd,sizeof(*cmd),rpl,sizeof(*rpl),NULL,0,NULL,0);
	if( pRet)
	{
		*pRet = from_little_endia_ul(rpl->status);
	}
	return err_no;
}

int i2c_erase_sector(word sector,dword *pRet)
{
	int err_no = ERRNO_NOERR;
	struct i2c_erases_cmd_s *cmd = (struct i2c_erases_cmd_s *)g_pWrBuffer;
	struct i2c_erases_rpl_s *rpl = (struct i2c_erases_rpl_s *)g_pRdBuffer;

	if( (sector >=2) && (g_update_mode!=SFS_UPDATE_AUTO)  ) sector +=g_partial_offset;

	cmd->hdr.cmd=I2C_ERASES;			//  erase 4K sector
	cmd->hdr.err=0;
	cmd->hdr.repl=0;
	cmd->sector=to_little_endia_us(sector);
	cmd->padding=0;
	err_no = i2c_send(cmd,sizeof(*cmd),rpl,sizeof(*rpl),NULL,0,NULL,0);
	if( pRet)
	{
		*pRet = from_little_endia_ul(rpl->status);
	}
	return err_no;
}

int i2c_erase_chip(dword *pRet)
{
	int err_no = ERRNO_NOERR;
	struct i2c_erasec_cmd_s *cmd = (struct i2c_erasec_cmd_s *)g_pWrBuffer;
	struct i2c_erasec_rpl_s *rpl = (struct i2c_erasec_rpl_s *)g_pRdBuffer;

	cmd->hdr.cmd=I2C_ERASEC;			//  erase chip
	cmd->hdr.err=0;
	cmd->hdr.repl=0;

	
	err_no = i2c_send(cmd,sizeof(*cmd),rpl,sizeof(*rpl),NULL,0,NULL,0);
	if( pRet)
	{
		*pRet = from_little_endia_ul( rpl->status);
	}
	return err_no;
}

int i2c_read(word sector,word length,void *data,dword *pRet)
{
	int err_no = ERRNO_NOERR;
	struct i2c_read_cmd_s   *cmd = (struct i2c_read_cmd_s   *)g_pWrBuffer;
	struct i2c_read_rpl_h_s *rpl = (struct i2c_read_rpl_h_s *)g_pRdBuffer;

	if( (sector>=2) && (g_update_mode!=SFS_UPDATE_AUTO) ) sector +=g_partial_offset;

	cmd->hdr.cmd=I2C_READ;			//  read from a 4K block, from offset 0
	cmd->hdr.err=0;
	cmd->hdr.repl=0;
	cmd->sector=to_little_endia_us(sector);
	cmd->length=(word)to_little_endia_ul(length);

	err_no = i2c_send(cmd,sizeof(*cmd),rpl,sizeof(*rpl),NULL,0,(uint8_t*)data,length);

	if( pRet)
	{   
		*pRet = from_little_endia_ul(rpl->length);
	}
	return err_no;
}

int i2c_write_normal(word sector,word length,const void *data,dword *pRet)
{
	int err_no = ERRNO_NOERR;
	struct i2c_write_cmd_h_s  *cmd = (struct i2c_write_cmd_h_s  *)g_pWrBuffer;
	struct i2c_write_rpl_s    *rpl = (struct i2c_write_rpl_s    *)g_pRdBuffer;

	if( (sector >= 2)  && (g_update_mode!=SFS_UPDATE_AUTO)  ) sector +=g_partial_offset;

	cmd->hdr.cmd=I2C_WRITE_NORMAL;			//  write to a 4K block, from offset 0
	cmd->hdr.err=0;
	cmd->hdr.repl=0;
	cmd->sector=to_little_endia_us(sector);
	cmd->length=(word)to_little_endia_ul(length);

	err_no =  i2c_send(cmd,sizeof(*cmd),rpl,sizeof(*rpl),(byte*)data,length,NULL,0);

	if( pRet)
	{ 
		*pRet = from_little_endia_ul(rpl->status);
	}
	return err_no;
}

int i2c_start_automode(const byte *base, dword part_size, dword *pRet)
{
	int i;
	uint hdrs[2*SFS_BLOCKSIZE/sizeof(uint)]={0};
	int len=num_non_ff(SFS_BLOCKSIZE/sizeof(uint),(uint*)&base[SFS_BLK2POS(0)]);
	uint length=len*sizeof(uint)*2;
	int err_no = ERRNO_NOERR;

	struct i2c_start_auto_cmd_h_s *cmd = (struct i2c_start_auto_cmd_h_s  *)g_pWrBuffer;
	struct i2c_start_auto_rpl_s   *rpl = (struct i2c_start_auto_rpl_s    *)g_pRdBuffer;

	cmd->hdr.cmd=I2C_START_AUTO;			//  Start auto-update
	cmd->hdr.err=0;
	cmd->hdr.repl=0;
	struct sfs_end_s * x;
	x=(struct sfs_end_s *)&base[SFS_BLK2POS(0)];
	x->magic = byte_swap_ulong(x->magic);
	for (i=0; i<len; i++) 
	{
		hdrs[i+  0]=((uint*)&base[SFS_BLK2POS(0)])[i];
		hdrs[i+len]=((uint*)&base[SFS_BLK2POS(1)])[i];
	}

	cmd->hdr_len=(word)to_little_endia_ul(length);
	cmd->part_size = part_size * SFS_BLOCKSIZE;

	err_no =  i2c_send(cmd,sizeof(*cmd),rpl,sizeof(*rpl),(byte*)hdrs,length,NULL,0);

	if( pRet)
	{ 
		*pRet = from_little_endia_ul(rpl->status);
	}
	return err_no;
}

int i2c_write_verify(word sector,word length,word erase,const void *data, dword *pRet)
{
	int err_no = ERRNO_NOERR;
	struct i2c_write_cmd_h_s  *cmd = (struct i2c_write_cmd_h_s  *)g_pWrBuffer;
	struct i2c_write_rpl_s    *rpl = (struct i2c_write_rpl_s    *)g_pRdBuffer;

	if( (sector >= 2)  && (g_update_mode!=SFS_UPDATE_AUTO) ) sector +=g_partial_offset;

	if (erase)
		cmd->hdr.cmd=I2C_ERASE_WRITE_VERIFY;
	else
		cmd->hdr.cmd=I2C_WRITE_VERIFY;			//  write to a 4K block, from offset 0
	cmd->hdr.err=0;
	cmd->hdr.repl=0;
	cmd->sector=to_little_endia_us(sector);
	cmd->length=(word)to_little_endia_ul(length);
	verify_flage = 1;

	err_no =  i2c_send(cmd,sizeof(*cmd),rpl,sizeof(*rpl),data,length,NULL,0);

	if( pRet)
	{ 
		*pRet = from_little_endia_ul(rpl->status);
	}
	return err_no;
}

int i2c_protect(uint state, dword *pRet)
{
	int err_no = ERRNO_NOERR;
	struct i2c_protect_cmd_s  *cmd = (struct i2c_protect_cmd_s  *)g_pWrBuffer;
	struct i2c_protect_rpl_s  *rpl = (struct i2c_protect_rpl_s  *)g_pRdBuffer;

	cmd->hdr.cmd=I2C_PROTECT;			//  change sw protection (on or off)
	cmd->hdr.err=0;
	cmd->hdr.repl=0;
	cmd->state=to_little_endia_ul(state);

	err_no =   i2c_send(cmd,sizeof(*cmd),rpl,sizeof(*rpl),NULL,0,NULL,0);

	if( pRet)
	{ 
		*pRet =  from_little_endia_ul(rpl->state);
	}
	return err_no;


}

int  i2c_rtunnel(dword rcmd,word clen,word rlen,void *rbuf)
{
	int err_no = ERRNO_NOERR;
	struct i2c_rtunnel_cmd_s   *cmd = (struct i2c_rtunnel_cmd_s   *)g_pWrBuffer;
	struct i2c_rtunnel_rpl_h_s *rpl = (struct i2c_rtunnel_rpl_h_s *)g_pRdBuffer;

	cmd->hdr.cmd=I2C_RTUNNEL;			//  send a SPI read command to SPI memory
	cmd->hdr.err=0;
	cmd->hdr.repl=0;
	cmd->cmd= to_little_endia_ul(rcmd);
	cmd->clen= to_little_endia_us( clen);
	cmd->rlen= to_little_endia_us(rlen);

	err_no =   i2c_send(cmd,sizeof(*cmd),rpl,sizeof(*rpl),NULL,0,rbuf,rlen);

	return err_no;
}

// retunr number of NON 0xffffffff data within specified range.
word num_non_ff(word range, const uint *data)
{
	const uint *cur = data + range -1;
	for(;cur >= data;cur--)
	{
		if( *cur != 0xffffffff) 
		{
			return (word)(cur-data+1);
		}
	}
	return range;
}

dword i2c_verify_image(word hdr,word fst,word lst,dword magic)
{


	struct i2c_verify_img_cmd_s *cmd = (struct i2c_verify_img_cmd_s *)g_pWrBuffer;
	struct i2c_verify_img_rpl_s *rpl = (struct i2c_verify_img_rpl_s *)g_pRdBuffer;

	if (g_update_mode!=SFS_UPDATE_AUTO)
	{
		fst +=g_partial_offset;
		lst +=g_partial_offset;
	}

	cmd->hdr.cmd=I2C_VERIFY_IMG;			//  send a SPI read command to SPI memory
	cmd->hdr.err=0;
	cmd->hdr.repl=0;
	cmd->header=hdr;
	cmd->first=fst;
	cmd->last=lst;
	cmd->padding=0;
	cmd->magic=magic;
	cmd->autom=(g_update_mode==SFS_UPDATE_AUTO);
	i2c_send(cmd,sizeof(*cmd),rpl,sizeof(*rpl),NULL,0,NULL,0);
	return rpl->status;
}

int i2c_small_write(word blk,byte num_uint, const byte *base)
{
	int err_no = ERRNO_NOERR;
	uint buf[MAX_SMALL_WRITE];
	byte n;
	const uint * src = (uint *)&base[SFS_BLK2POS(blk)];

	do{
		BIF(num_uint > MAX_SMALL_WRITE); 
		BIF(i2c_read(blk,num_uint*sizeof(uint),buf,NULL));
		for( n=0;n!=num_uint;n++)
		{
			if( buf[n] != src[n] )
			{
				break;
			}
		}
		if ( n != num_uint)  /* not idential */
		{
			if (!g_bEraseChip) {
				for( n=0;n!=num_uint;n++)
				{
					if( buf[n] != 0xffffffff )
					{
						break;
					}
				}
			}
			err_no = i2c_write_verify((word)blk,num_uint*sizeof(uint),(!g_bEraseChip && (n!=num_uint)),src,NULL);

		}
	}while(0);
	return err_no;
}

int i2c_download_block(word fst,word end,const byte *base, uint cur, uint total)
{
	int err_no = ERRNO_NOERR;

	word              c;
	uint              len;
	struct sfs_padded_s * p;
	dword sum = 0;

	for(c=fst;c<end && (err_no == ERRNO_NOERR) ;c++,cur++)
	{ 
		p=(struct sfs_padded_s *)&base[SFS_BLK2POS(c)];
	//	DEBUG_ERR("p->magic = 0x%x SFS_MAGIC_HDRPAD = 0x%x",p->magic,SFS_MAGIC_HDRPAD);		
//		p->magic = byte_swap_ulong(p->magic);
		if( p->magic == SFS_MAGIC_HDRPAD)
		{
			if (g_update_mode==SFS_UPDATE_AUTO)
			{

				p->magic = byte_swap_ulong(p->magic);
				BIF(i2c_write_verify((word)c,SFS_BLOCKSIZE,!g_bEraseChip,&base[SFS_BLK2POS(c)],NULL));
			}
			else
			{
				BIF(i2c_small_write(c,ALIG_SIZE(struct sfs_padded_s) ,base));
			}
			err_no = ERRNO_NO_MORE_DATA;
		}
		else
		{
			len = num_non_ff(SFS_BLOCKSIZE/sizeof(uint),(uint*)&base[SFS_BLK2POS(c)]);
			if(len || (g_update_mode==SFS_UPDATE_AUTO))
			{
			sum +=len;
		//	 printf("sum = 0x%x len  = 0x%x	(word)len*sizeof(uint) = 0x%x fst = %d end = %d c = %d\n",sum,len,(word)len*sizeof(uint),fst,end,c);
				err_no = i2c_write_verify((word)c,(word)len*sizeof(uint),!g_bEraseChip,&base[SFS_BLK2POS(c)],NULL);
			} 
			else if (!g_bEraseChip)
			{
				BIF(i2c_erase_sector((word)c, NULL));   
			}
		}

		if( err_no < 0)
		{
			ShowProgress(cur,0,I2C_ERR,total);
			break;
		}
		else
		{
			ShowProgress(cur,0,I2C_OK,total);
		}
	}
	return err_no;
}


int i2c_download_partition(word part,const byte *base)
{
	int err_no = ERRNO_NOERR;

	struct sfs_header_s *sfshdr=(struct sfs_header_s *)&base[SFS_BLK2POS(part)];
	struct sfs_encapsulated_s *e;
	struct sfs_end_s * x;
	word   fst,end,n, real_end; 
	int    found = 0;

	if( g_is_partial_img ) 
	{  
		struct sfs_header_s *sfshdr0 =(struct sfs_header_s *)&base[SFS_BLK2POS(0)];
		sfshdr0->sfsbeg = to_little_endia_us(sfshdr0->sfsbeg);
		fst = sfshdr0->sfsbeg-1;
		g_partial_offset = sfshdr->sfsbeg - sfshdr0->sfsbeg;
		sfshdr0->sfsbeg = to_little_endia_us(sfshdr0->sfsbeg);
	}
	else
	{
		fst = sfshdr->sfsbeg-1;
		g_partial_offset = 0;
	}
	
	e=(struct sfs_encapsulated_s *)&base[SFS_BLK2POS(fst)];

	//end= real_end = fst+e->asize;
	end = fst+e->asize;
	real_end = fst+e->asize;
	e->magic = byte_swap_ulong(e->magic);

	//DEBUG_ERR("e->magic = 0x%x SFS_MAGIC_HDRECP = 0x%x",e->magic,SFS_MAGIC_HDRECP);
	if (e->magic!=SFS_MAGIC_HDRECP)
	{ 
		LOG((DBG_ERROR "Invalid encapsulation header\n")); 
		return -ERROR_INVALID_IMAGE;
	}
	e->magic = byte_swap_ulong(e->magic);
	// get the real image data size.

	for(n=fst;n<=real_end ;n++)
	{
		x =(struct sfs_end_s *) &base[SFS_BLK2POS(n)];
		//DEBUG_ERR("x->magic = 0x%x SFS_MAGIC_HDRPAD = 0x%x",x->magic,SFS_MAGIC_HDRPAD);
		x->magic = byte_swap_ulong(x->magic);
		if( x->magic == SFS_MAGIC_HDRPAD)
		{
			found = 1;
			end = n+1;
			break;
		}
		x->magic = byte_swap_ulong(x->magic);

	}

	if( found == 0) 
	{
		LOG((DBG_ERROR "Padded magic id not found!\n")); 
		return -ERROR_INVALID_IMAGE;
	}

	n=end-fst+1;	// SFS header+nr of 4K blocks in the partition

	if(g_update_mode==SFS_UPDATE_AUTO)
	{
		dword p;
		err_no = i2c_start_automode(base,end-fst,&p);
		
		part=(word)p;
		DEBUG_ERR(( "\nUpdating partition [%d]\n",part));  
		InitShowProgress( n);
		ShowProgress(2,0,err_no<0?I2C_ERR:I2C_OK,n);
	}
	else
	{
		// write the header, mark the header inactive (deleted)
		InitShowProgress( n);
		DEBUG_ERR(( "\nUpdating partition [%d]\n",part));  
		x=(struct sfs_end_s *)&base[SFS_BLK2POS(part)];
		x->magic=SFS_MAGIC_HDRDEL; 
		err_no = i2c_download_block(part,part+1,base,1,n);
		// write the partition
	}

	err_no = err_no ? err_no:i2c_download_block(fst,end,base,2,n);
	if( ERRNO_NO_MORE_DATA == err_no) err_no =ERRNO_NOERR;
	if( err_no < 0 ) return err_no;

	LOG((DBG_INFO "\n\nVerify CRC for partition %d...",part));

	if ((g_update_mode==SFS_UPDATE_AUTO)||(i2c_verify_image((word)part,(word)fst,(word)end,SFS_MAGIC_HEADER)==0))
	{
		LOG((DBG_INFO  "Good\n"));
		LOG((DBG_INFO "Mark partition %d as active...\n",part)); 
		if(g_update_mode==SFS_UPDATE_AUTO)
			LOG((DBG_INFO "Mark partition %d as inactive...\n",1-part)); 
		return 0;
	}
	else 
	{ 
		LOG((DBG_ERROR "Error\n"));
		return -ERROR_CRC_CHECK_ERROR;
	}
}

const char* const get_spi_mem_name(uint id)
{
	unsigned int i;
	for(i=0;i<sizeof(g_spiid_2_name_tlb)/sizeof(g_spiid_2_name_tlb[0]);
		i++)
	{
		if( id == g_spiid_2_name_tlb[i].id) 
			return g_spiid_2_name_tlb[i].name;
	}
	return "Unknown spi";
}

int i2c_download_image(const byte *img,uint siz)
{
	int err_no = ERRNO_NOERR;
	//  uint32_t magic;
	unsigned int update=0;
	dword id;
	dword status;
	word partition;
	g_i2c_blknr=0;
	do
	{
		BIF(i2c_ping(&id));
		LOG((DBG_INFO "\tSPI memory has ID   : %08xh => %s \n",id,get_spi_mem_name(id))); 
		BIF(i2c_read_status(&status));
		LOG((DBG_INFO "\tSPI memory status   : %08xh\n",status)); 
		LOG((DBG_INFO "\tImage file size     : %xh bytes\n",siz));

#if defined(_WINDOWS )
		for ( partition=0;partition<2;partition++)
		{
			i2c_read((word)partition,4,&magic,&id);
			LOG((DBG_INFO "\tpartition %d : %s\n",partition,SFS_MAGIC_HEADER == magic?"Active":"Inactive"));
		}
#endif

		if (1)
		{
			printf("\nErasing chip ...");
			//LOG((DBG_INFO "Erasing chip .. ")); 
			esase_flage =1;
			BIF(i2c_erase_chip(NULL));
			esase_flage =0;
			LOG((DBG_INFO "Done\n")); 
		}

		// Bootloader automatically determines which partition should be updated.
		// Both headers are compressed and sent to the bootloader. Host doesn't
		// make the decision which partition to update
		if(g_update_mode == SFS_UPDATE_AUTO)
		{
			BIF(i2c_download_partition(0,img));
		}
		else
		{
			update = g_update_mode;
			for (partition=0;partition<2&&err_no == ERRNO_NOERR;partition++)
			{
				if (update&(1<<partition))
				{
					BIF(i2c_download_partition(partition,img));
				}
			}
		}
	} while(0);

	return err_no;
}




int wait_for_loader()
{
	unsigned int ret;
	int err_no = ERRNO_NOERR;
	unsigned int   time_out_loop = TIME_OUT_MS/POLLING_INTERVAL_MS;
	uint32_t ready  = TRANSFER_STATE_STANDBY;

	if( g_bIsRenoDev )
	{
		ready  = TRANSFER_STATE_RENO_LOADER_READY;
	}


	/*
	# Look for reply at address 0. Flags that a transfer can be initiated
	*/
	for(;time_out_loop;time_out_loop--) 
	{ret= i2c_sub_read(0x00,&err_no);
		if( (ret == 0x80 || ret == 0xd9fb) && (err_no ==ERRNO_NOERR))
		{

			break;
		}
		
		sys_mdelay(POLLING_INTERVAL_MS);
	}
//	printf("ret = 0x%x  err_no = %d  \n",ret,err_no);
	if( time_out_loop == 0)
	{
		LOG((DBG_ERROR " aborting. timeout wating for device after %d ms\n",TIME_OUT_MS));
		err_no = -ERROR_WAITING_LOADER_TIMEOUT;
	}

	return err_no;
}

int TransferData( const unsigned char * image_data, unsigned int image_size)
{

	int err_no = ERRNO_NOERR;
	const unsigned char *pBlock;
	const unsigned char *pCur; 
	const unsigned char  *pBlockEnd = NULL;
	uint32_t  ulBlockCheckSum  = 0;
	uint32_t  ulBlockIndex     = 0;
	uint32_t  ulRemainder       = 0;
	uint32_t  nRetry           = 0;
	uint32_t  done             = 0;
	uint32_t  CRCReturned      = 0;
	struct i2c_xfer_reno_s          i2c_xfer_reno;
	struct i2c_xfer_s               i2c_xfer;
	int errno = 0;

	uint32_t  max_xfer_size =   IMG_BLOCK_SIZE;


	int  ReadErr = ERRNO_NOERR;
	//
	// Download FW image data.
	//
	if( g_bIsRenoDev )
	{
		max_xfer_size = IMG_BLOCK_SIZE_RENO;
	}
	
	InitShowProgress(image_size);
	ulBlockIndex=0;
	pBlock = image_data;
	ulRemainder = image_size;
	for(;;){

		uint32_t state = i2c_sub_read(0x00,&ReadErr); 
		//printf("%d:state  = 0x%x\n",__LINE__,state);
		if ( ReadErr !=ERRNO_NOERR )
		{
			ShowProgress(image_size-ulRemainder,0,I2C_ERR,image_size);
			err_no = ReadErr;
			break;
		}
		switch(state)
		{
		case TRANSFER_STATE_FATAL://# device->host: fatal error
			LOG((DBG_ERROR "\nFATAL error happened. aborting. \n"));
			err_no = -ERROR_STATE_FATAL;
			break;
		case TRANSFER_STATE_BAD_PACKET://# device->host: bad packet. retry
			{
				if( nRetry >= MAX_RETRIES )
				{
					i2c_sub_write(0x0000,TRANSFER_STATE_ABORT_TRANSFER);
					LOG((DBG_ERROR "\naborting. exceeded max number of retries \n"));
					ShowProgress(image_size-ulRemainder,0,I2C_ERR,image_size);
					err_no = -ERROR_LOAD_IMG_TIMEOUT;
					break;
				}
				else
				{
					/// re-try
					if( g_bIsRenoDev )
					{
						i2c_transmit((unsigned char*)&i2c_xfer_reno,sizeof(i2c_xfer_reno),
							(unsigned char*)pBlock,pBlockEnd-pBlock );
						ShowProgress(image_size-ulRemainder,0,I2C_RETRY,image_size);
					}
					else
					{

						i2c_transmit((unsigned char*)&i2c_xfer,sizeof(i2c_xfer),
							(unsigned char*)pBlock,pBlockEnd-pBlock );
						ShowProgress(image_size-ulRemainder,0,I2C_RETRY,image_size);

					}
					nRetry++;

				}
			}
			break;
		case TRANSFER_STATE_STANDBY://#  device->host: standing by for next packet.
		case TRANSFER_STATE_RENO_LOADER_READY:
			{
				int  *xfer_int;
				nRetry =0;
				if(pBlockEnd != 0)
				{
					ulRemainder = image_size - (pBlockEnd -image_data);
					pBlock = pBlockEnd;
					ulBlockIndex++;
				}

				if(ulRemainder == 0)  
				{   // No remainder data, write 'D' to address 0.
					ShowProgress(image_size,0,I2C_OK,image_size);
					//LOG((DBG_INFO "\nImage transfer completed successfully. \n"));
					i2c_sub_write(0x0000, TRANSFER_STATE_FILE_DONE);
					break;
				}

				pBlockEnd = pBlock+ ( ulRemainder > max_xfer_size ?max_xfer_size:ulRemainder);

				if( g_bIsRenoDev )
				{
					// get the checksum of this block data.
					ulBlockCheckSum = 0;
					for( pCur =pBlock;pCur!=pBlockEnd; pCur +=4)
					{
						ulBlockCheckSum+= from_little_endia_ul(*((uint32_t*)pCur));
					//	printf("pCur = 0x%x   pBlockEnd =0x%x  \n",*pCur,*pBlockEnd);
					}

					i2c_xfer_reno.xfer.xfer_flag    = TRANSFER_STATE_PACKET_READY;
					i2c_xfer_reno.xfer.block_index  = ulBlockIndex;
					i2c_xfer_reno.xfer.check_sum    = 0;
					i2c_xfer_reno.num_dword         = (pBlockEnd-pBlock) / 4;
					
					// get the checksum of xfer header.
					for( xfer_int =(int*)&i2c_xfer_reno;
						xfer_int !=((int*)&i2c_xfer_reno)+4; xfer_int++)
					{
						ulBlockCheckSum+= *xfer_int;
					}

					i2c_xfer_reno.xfer.check_sum  = ~ulBlockCheckSum;


					// conver the xfer to little endia
					for( xfer_int =(int*)&i2c_xfer_reno;
						xfer_int !=((int*)&i2c_xfer_reno)+4; xfer_int++)
					{
						*xfer_int = to_little_endia_ul(*xfer_int);
					}

					errno= i2c_transmit((unsigned char*)&i2c_xfer_reno,sizeof(i2c_xfer_reno),
						(unsigned char*)pBlock,pBlockEnd-pBlock );
					if(errno <0)
						return errno;
				}
				else
				{
					// get the checksum of this block data.
					ulBlockCheckSum = 0;
					for( pCur =pBlock;pCur!=pBlockEnd; pCur +=4)
					{
						ulBlockCheckSum+= from_little_endia_ul(*((uint32_t*)pCur));
					}

					i2c_xfer.xfer_flag   = TRANSFER_STATE_PACKET_READY;
					i2c_xfer.block_index = ulBlockIndex;
					i2c_xfer.check_sum   = ulBlockCheckSum;
					errno = i2c_transmit((unsigned char*)&i2c_xfer,sizeof(i2c_xfer),
						(unsigned char*)pBlock,pBlockEnd-pBlock );
					if(errno <0)
						return errno;

				}
				if (!(g_bIsRenoDev || g_is_dual_img) && (image_size-ulRemainder ==0) )
				{
					/* using old download firmare approach. this will take about few secondes
					for easing whole chip*/ 

					ShowProgress(image_size-ulRemainder,0,I2C_ERASE_CHIPS,image_size);
				} else{
					ShowProgress(image_size-ulRemainder,0,I2C_OK,image_size);

				}
			}
			break;
		case TRANSFER_STATE_TRANSFER_COMPLETE://#  device->host: transfer complete
			{
				if (g_bIsRenoDev || g_sfs_version >= _SFS_VERSION(0,3,0,0))
				{
					done =1;
				//	printf("%d: done = %d\n",__LINE__,done);
			//	printf("%d: done = %d g_sfs_version = 0x%x _SFS_VERSION(0,3,0,0)=  0x%x\n",__LINE__,done,g_sfs_version,_SFS_VERSION(0,3,0,0));
					break;
				}

				CRCReturned = i2c_sub_read(8,&ReadErr);
				if( ReadErr == ERRNO_NOERR)
				{
					if( CRCReturned != 0x53435243 )
					{
						ShowProgress(image_size,0,I2C_ERR,image_size);
						LOG((DBG_INFO "\nCRC error!\n"));
						err_no = -ERROR_CRC_CHECK_ERROR;
					}
					else
					{
						ShowProgress(image_size,0,I2C_OK,image_size);
						LOG((DBG_INFO "\nImage transfer completed successfully. \n"));
						done =1;
					}
				}
				else
				{
					ShowProgress(image_size,0,I2C_ERR,image_size);
					err_no = ReadErr;
				}

			}
			break;
		case TRANSFER_STATE_PACKET_READY:/*echo*/
		case TRANSFER_STATE_FILE_DONE:/*echo*/
			break;  
		default:
			err_no = -ERRNO_FAILED;
			break;
		}

		if( err_no !=ERRNO_NOERR || done ) {
			break;}
	}
	return err_no;
}

int download_image( const unsigned char * image_data, unsigned int image_size)
{
	if( g_bIsRenoDev )
	{
		if( g_legacy_device)
			return TransferData(image_data,image_size);
		else{
			return i2c_download_image(image_data,image_size);
			}
	}
	else
	{
		return TransferData(image_data,image_size);
	}
}

int download_boot_loader(const unsigned char *  loader_data, unsigned int num_byte)
{
	int  ReadErr = ERRNO_NOERR;
	volatile uint32_t       val = 0;

	unsigned int   time_out_loop = TIME_OUT_MS/POLLING_INTERVAL_MS;
	int err_no = ERRNO_NOERR;

	LOG((DBG_INFO "Waiting. Please reset the board\n"));
	//sets the Timeout setting to 5 seconds.
	time_out_loop = MANUAL_TIME_OUT_MS/POLLING_INTERVAL_MS;
//	set_gpio_value(20,1);
//	while(1);
	if( g_SetResetPinPtr )  
	{
		// System can reset the device prgorammatically. 
		g_SetResetPinPtr(g_pContextSetResetpin,0); //sets RESET PIN to low.
		sys_mdelay(20);
		g_SetResetPinPtr(g_pContextSetResetpin,1); //sets RESET PIN to high.
//	set_gpio_value(12,0);
//	sys_mdelay(20);
//	set_gpio_value(12,1);
	
	}else{
		DEBUG_ERR("g_SetResetPinPtr == NULL");
	}
	


	/*
	# Look for 'C' at address 0. Flags that a transfer can be initiated
	*/
#ifdef I2C_DISABLE_RESET 
	for(;;) 
#else
	for(;time_out_loop;time_out_loop--) 
#endif
	{
		/*if( (uint32_t)(i2c_sub_read(0x00,&ReadErr)  ==(uint32_t) 0X43) && ((int)ReadErr==(int)ERRNO_NOERR))*/
		val = i2c_sub_read(0x00,&ReadErr);
		if( val == 0X000043U)  //0X000043U
		{
			break;
		}
		/*sys_mdelay(POLLING_INTERVAL_MS);*/
	}
//		printf("%d:val = 0x%x\n",__LINE__,val);
	if( time_out_loop == 0)
	{
		LOG((DBG_ERROR " aborting. timeout wating for device after %d ms\n",
			(g_SetResetPinPtr?TIME_OUT_MS:MANUAL_TIME_OUT_MS)*POLLING_INTERVAL_MS));
		return -ERROR_WAITING_RESET_TIMEOUT;
	}

	/*
	# Delete the magic number to signal that the download just started
	# but hasn't finished yet
	*/
	i2c_sub_write(0x0000,0x0000); // to stop download sequence

	if( g_bIsRenoDev )
	{
		err_no = TransferData(loader_data,num_byte);
	}
	else
	{
		i2c_transmit(loader_data,num_byte,NULL,0);
	}
	return err_no;
}

int get_firmware_version(uint32_t verion[])
{
	int err_no ;

	uint32_t data,num,reply,i;

	if (verion == 0){
		err_no = -ERRNO_INVALID_PARAMETER;
		goto LEAVE;
	}

	err_no = i2c_sub_write(4,0xB32D2300);
	err_no = err_no<0?err_no:i2c_sub_write(0,0x0103000d);
	if (err_no < 0) goto LEAVE;
	for (i=0;i < 50;i++)
	{
		sys_mdelay(1);
		data = i2c_sub_read(0,&err_no);
		if (err_no < 0) goto LEAVE;
		num = data & 0xffff;
		reply = data>>31;
		if (num ==4 && reply)
		{
			goto GOT_VER;
		}
	}
	err_no = -ERROR_FW_NO_RESPONSE;
	goto LEAVE;

GOT_VER:
	for (i=0; i < num;i++) {
		verion[i] = i2c_sub_read(8+i*4,&err_no);
		if (err_no < 0) goto LEAVE;
	}
LEAVE:
	return err_no;
}

int verify_firmware_version() {

	int err_no = 0;
	uint32_t check_loop = READY_POOLING_LOOP_MANUALLY;
	uint32_t  fw_ver[4];
	uint32_t i;
	uint32_t val;
#ifdef SHOW_ELAPSED_TIME
	clock_t reboot_start = 0;
	clock_t reboot_end = 0;
#endif

#ifdef SHOW_ELAPSED_TIME
	reboot_start = clock();
#endif

	if( NULL == g_SetResetPinPtr ) {
		LOG((DBG_INFO "Please reset the device again...")); 
		check_loop = READY_POOLING_LOOP_MANUALLY;
	} else {
		LOG((DBG_INFO "Device rebooting...")); 
		check_loop = READY_POOLING_LOOP;
		// System can reset the device prgorammatically. 
		g_SetResetPinPtr(g_pContextSetResetpin,0); //sets RESET PIN to low.
		sys_mdelay(20);
		g_SetResetPinPtr(g_pContextSetResetpin,1); //sets RESET PIN to high.
	}

	val = 0;
	for (i=0;i<10000;i++ ) {
		LOG((DBG_INFO "."));
		sys_mdelay(READY_POOLING_INTERVAL);
		val = i2c_sub_read(0x04,&err_no);
		printf("\rval = 0x%x i = %d",val,i);
		if (BADBEFF == val) {
			break;
		}
	}

	if (BADBEFF != val ) {
		LOG((DBG_ERROR "\nFailed to reboot!\n"));
		system("reboot");
		return -ERROR_FW_CANNOT_BOOT;
	}

#ifdef SHOW_ELAPSED_TIME
	reboot_end = clock();
#endif
	LOG((DBG_INFO "\nDevice rebooted successfully!\n"));

	/*Check firmware version*/
	if (((uint32_t) -1) != g_firmware_version[0]) {
		LOG((DBG_INFO "Verify firmware version...."));
		err_no = get_firmware_version(fw_ver);
		if (err_no==0 && memcmp(fw_ver,g_firmware_version,sizeof(fw_ver)-sizeof(uint32_t)) ==0) {
			LOG((DBG_INFO "Good\n"));
		} else {
			if (err_no ==0) {
				LOG((DBG_ERROR "\nWarning: Dev FW ver [%d.%d.%d.%d] != SFS FW ver [%d.%d.%d.%d]\n",
					fw_ver[0], fw_ver[1], fw_ver[2], fw_ver[3],
					g_firmware_version[0], g_firmware_version[1], g_firmware_version[2], g_firmware_version[3]));
				err_no = -ERROR_FW_VER_INCORRECT;
			} else {
				LOG((DBG_INFO "Error: %d\n",err_no));
			}
		}
	} else {
		LOG((DBG_INFO "Read firmware version ...."));
		err_no =get_firmware_version(fw_ver);
		if (err_no==0) {
			LOG((DBG_INFO "[%d.%d.%d.%d]\n", fw_ver[0], 
				fw_ver[1], fw_ver[2], fw_ver[3]));
		} else {
			LOG((DBG_INFO "Error: %d\n",err_no));
		}
	}
	return err_no;
}
/*
* Download Firmware to Hudson.
* 
* PARAMETERS
*    
*    pBuffer             [in] - A potinter to a buffer for firmware update processing.
*    loader_data         [in] - A pointer to the input buffer that contains I2C bootloader data.
*    loader_size         [in] - The size of Bootloader data in bytes.
*    image_data          [in] - A pointer to the input buffer that contains Image data. 
*    image_size          [in] - The size of Image data in bytes.
*    slave_address       [in] - Specifies I2C chip address.
*    eDownLoadMode       [in] - Specifies the download mode, Could be the follwoing one of values.(Reno only) 
*                               0 :  auto 
*                               1 :  flash partition 0
*                               2 :  flash partition 1
*                               3 :  flash both partitions
*   boot_time_upgrade    [in] - 0 : flash image during boot time
*                               1 : flash image during firmware is running.
*   legacy_device        [in] - 0 : for reno devices
*                               1 : for hudson devices
* RETURN
*  
*    If the operation completes successfully, the return value is ERRNO_NOERR.
*    Otherwise, return ERRON_* error code. 
* 
* REMARKS
*  
*    You need to set up both I2cWrite and I2cWriteThenRead callback function by calling 
*    SetupI2cWriteMemCallback and SetupI2cReadMemCallback before you call this function.
*/
void test_rw()
{
		  #if 1
				int i,err_no;
				int k = 0x1234;
				unsigned char  p[4] = {0};
				unsigned char  p1[4] = {0x01,0x02,0x03,0x04};
				int j = 0;
				for (i	= 0;i<200;i = i + 4){
				printf("###########################\n");
				err_no = i2c_sub_write(0,k);
				printf("i2c_sub_write reg = 0x%x  val= 0x%x   ret = %d\n",i ,k,err_no);
				int val_read = -1;
				int err_no1 = 0;
				val_read = i2c_sub_read(0,&err_no1);
			//	printf("val_read= 0x%x\n",val_read);
				
				printf("p1beforei2c_sub_burst_write = 0x%x 0x%x 0x%x 0x%x\n",p1[0],p1[1],p1[2],p1[3]);
				i2c_sub_burst_write(j,4,p1);
				i2c_sub_burst_read(j,4,p);
				printf("pi2c_sub_burst_read = 0x%x 0x%x 0x%x 0x%x\n",p[0],p[1],p[2],p[3]);
				k = k + 1;
				p1[0] = p1[0] + 1;p1[1] = p1[1] + 1;p1[2] = p1[2] + 1;p1[3] = p1[3] + 1;
				j = j + 4;
				printf("j = 0x%x\n",j);
				//sys_mdelay(1000);
			
				}
#endif
	}


int DownloadFW(        void *                      pBuffer,
	const unsigned char *       loader_data,
	uint32_t               loader_size,
	const unsigned char *       image_data,
	uint32_t               image_size,
	unsigned char               slave_address,
	SFS_UPDATE_PARTITION        eDownLoadMode,
	int boot_time_upgrade,
	int legacy_device)
{
	int err_no = ERRNO_NOERR;

	struct sfs_header_s *header;    
	//	struct sfs_header_v300_s *header300;    

	int  ReadErr = ERRNO_NOERR;
	//    volatile uint32_t       val;
	unsigned int   time_out_loop = TIME_OUT_MS/POLLING_INTERVAL_MS;
	const int      little_endian = is_little_endian();

	g_bBootTimeUpgrade = boot_time_upgrade? 1:0;
	g_legacy_device = legacy_device;
//	g_bBootTimeUpgrade = 0;
//	*((dword *)loader_data)  = to_little_endia_ul(*((dword *)loader_data) );
	do{

		// Verfiy the byte order
#if __BYTE_ORDER != __LITTLE_ENDIAN
		if(little_endian) return ERROR_MIS_ENDIANNESS;
#else
		if(!little_endian) return ERROR_MIS_ENDIANNESS;
#endif

#if defined(DEBUG)
		signal (SIGSEGV, signal_mem_handler);
#endif 

		// check if the work buffer is set or not.
		if( NULL == pBuffer )
		{
			err_no = -ERRNO_INVALID_PARAMETER;
			LOG((DBG_ERROR "The buffer pointer is not set\n"));
			break;
		}

		// check if the i2c function is set or not.
		if( NULL == g_I2cReadMemPtr||
			NULL == g_I2cWriteMemPtr )
		{
			err_no = -ERRNO_I2CFUN_NOT_SET;
			LOG((DBG_ERROR "i2C function is not set.\n"));
			break;
		}

		/*make sure the buffer align to 4 bytes boundary*/
		//int    x = ALIGNMENT_SIZE;
		//x = ((size_t)pBuffer%ALIGNMENT_SIZE) ;
	//	unsigned char * p = NULL;
	//	p = malloc(72);
//		g_pRdBuffer = malloc(100);
//		g_pWrBuffer = malloc(100);

		g_pBuffer        = (uint8_t*)((uint8_t*)pBuffer + ALIGNMENT_SIZE-((size_t)pBuffer%ALIGNMENT_SIZE)) ;

		g_pWrBuffer      = g_pBuffer; 
		g_pRdBuffer      = g_pWrBuffer +MAX_WRITE_BUFFER; 
//		printf("ALIGNMENT_SIZE = %d sizeof(struct i2c_rtunnel_rpl_h_s) = %d \n",sizeof(long),sizeof(struct i2c_rtunnel_rpl_h_s));
//		printf("pBuffer = %d \n",(sizeof(struct i2c_verify_img_cmd_s) + sizeof(struct i2c_rtunnel_rpl_h_s) +sizeof(long)));
//		printf("g_pWrBuffer = %d  g_pRdBuffer = %d  MAX_WRITE_BUFFER = %d\n",sizeof((uint8_t*)g_pWrBuffer),sizeof((uint8_t*)g_pBuffer),MAX_WRITE_BUFFER);

		g_bChipAddress   = slave_address; 
		g_update_mode    = eDownLoadMode;

		if (g_legacy_device)
			header = (struct sfs_header_s *)image_data;
		else
		{
			if ((header = ParseImageHeader(image_data,&image_size,&g_is_partial_img))==NULL)
			{
				err_no = -ERROR_INVALID_IMAGE;
				LOG((DBG_ERROR "SFS image invalid.\n"));
				break;
			}
		}

		if( g_update_mode > SFS_UPDATE_BOTH )
		{
			err_no = -ERRNO_INVALID_PARAMETER;
			LOG((DBG_ERROR "Parameter is invalid.\n"));
			break;
		}
	//	header->version = (header->version >> 8);		
		g_sfs_version = header->version;
	//	g_sfs_version = (g_sfs_version >> 8);
		printf("g_sfs_version = 0x%x\n",g_sfs_version);
		if (g_bBootTimeUpgrade)
		{
			// If a bootloader isn't specified, look for a valid iflash.bin in the image. 
			if (loader_data == NULL)
			{
				if (header->iflash ==(dword)BLKNONE)
				{
					err_no = -ERRNO_INVALID_BOOTLOADER;
					LOG((DBG_ERROR "didn't find a valid iflash bootloader in image. aborting.\n"));
					break;
				}
				else
				{
					loader_data  = (unsigned char *)&image_data[header->iflash*SFS_BLOCKSIZE];
					loader_size = SFS_BLOCKSIZE;
					if ( g_bIsRenoDev )
						loader_size *= 4;
				}
			}

			// check file for a valid bootloader
			
			printf("loader_data = 0x%x SFS_MAGIC_BOOT =  0x%x  SFS_MAGIC_BOOT_H = 0x%x\n",*((dword *)loader_data),SFS_MAGIC_BOOT,SFS_MAGIC_BOOT_H);
			*((dword *)loader_data) = byte_swap_ulong(*((dword *)loader_data));
			if( *((dword *)loader_data) == SFS_MAGIC_BOOT)
			{
				g_bIsRenoDev =0;  // Boot loader for B0 board.
				LOG((DBG_INFO "Found boot loader for Hudson\n"));
			}
			else if ( *((dword *)loader_data) == SFS_MAGIC_BOOT_H)
			{
				g_bIsRenoDev =1;   // Boot loader is for Reno device.
				LOG((DBG_INFO "Found boot loader for Reno\n"));
			}
			else
			{
				if (g_bBootTimeUpgrade){
					err_no = -ERRNO_INVALID_BOOTLOADER;
					LOG((DBG_ERROR "didn't find a valid bootloader magic number. aborting.\n"));
					break;
				}
			}
		}
		else
		{
			g_bIsRenoDev = 1; 

		}

		if( !g_is_dual_img && ( g_update_mode !=SFS_UPDATE_PARTITION_0 ))
		{
			g_update_mode = SFS_UPDATE_PARTITION_0;
		}
		*((dword *)loader_data) = byte_swap_ulong(*((dword *)loader_data));
		// Dump the system environment 
		LOG((DBG_ERROR "Device Type          : %s \n", g_bIsRenoDev? "Reno":"Hudson"));
		LOG((DBG_ERROR "I2C Slave Address    : %02xh\n", g_bChipAddress));
		LOG((DBG_ERROR "    Max burst write  : %d bytes\n", g_cbMaxI2cWrite));
		LOG((DBG_ERROR "    Max burst read   : %d bytes\n", g_cbMaxI2cRead));
		LOG((DBG_ERROR "Endianness           : %s-Endian \n", little_endian? "Little":"Big"));
		LOG((DBG_ERROR "SFS version          : %d.%d.%d.%d \n", (uint8_t) (g_sfs_version>>24),
			(uint8_t) (g_sfs_version>>16),(uint8_t) (g_sfs_version>>8), (uint8_t) (g_sfs_version))); 
		if( ((uint32_t) -1) != g_firmware_version[0])
			LOG((DBG_INFO "Firmware version     : %d.%d.%d.%d \n", g_firmware_version[0],
			g_firmware_version[1],g_firmware_version[2],g_firmware_version[3]));
		//  LOG((DBG_INFO "Firmware version     : 
		if (g_bBootTimeUpgrade)
		{
			LOG((DBG_INFO "Boot loader size     : %xh bytes\n", loader_size));
			LOG((DBG_INFO "Image size           : %xh bytes\n", image_size));
		}

		if(!g_legacy_device){  
			LOG((DBG_INFO "Unpadded image       : %s \n", g_is_partial_img?"Yes":"No"));
			LOG((DBG_INFO "Is Dual Image        : %s \n", g_is_dual_img?"Yes":"No"));
			LOG((DBG_INFO "Update partition     : %s \n", update_mode_str[g_update_mode] ));
		}

		LOG((DBG_INFO "\n"));



		if (g_bBootTimeUpgrade==1)
		{
			LOG((DBG_INFO "Downloading boot loader\n"));

			err_no =  download_boot_loader(loader_data,loader_size); 
			if( err_no == 0) 
			{
				LOG((DBG_INFO "\nBootloader transfer completed successfully\n"));
			}
			else
			{
				LOG((DBG_ERROR "\nFailed to download boot loader\n"));
				break;
			}
		}
		else
		{
			// Tell the control app to switch to FW upgrade mode. 

			Command cmd = {0,CONTROL_APP_FW_UPGD,0,APP_ID('C','T','R','L')};
			i2c_sub_burst_write(4,4,&(((const unsigned char *)&cmd)[4]));
			i2c_sub_burst_write(0,4,&(((const unsigned char *)&cmd)[0]));

		}

		if( (err_no =wait_for_loader()) !=0) break ;

		*((dword *)image_data)  = byte_swap_ulong(*((dword *)image_data) );
	//	DEBUG_ERR("*((dword *)image_data) = 0x%x",*((dword *)image_data));

		LOG((DBG_INFO "\nDownloading image\n"));
		err_no = download_image(image_data,image_size);
		if(err_no  == 0) 
		{
			LOG((DBG_INFO "\nImage transfer completed successfully!\n"));
		}
		else
		{
			LOG((DBG_ERROR "\nFailed to download Image, error code = %d\n", err_no));
			break;
		}

		/*Check if the firmware version is correct or not*/
		err_no = verify_firmware_version();

	}while(0);
	// ShowProgress(0,2,0,0);
	return err_no;
}


// TODO: tweak the interval for reply bit polling and its timeout
#define REPLY_POLL_INTERVAL_MSEC     1
#define REPLY_POLL_TIMEOUT_MSEC   2000

void swap_i2cdata1(unsigned int *i2c_data)
{
	unsigned char tmp;
		int i;
	unsigned char *p = i2c_data;
	for (i = 0;i < 4;i++){
		DEBUG_ERR("p%d = 0x%x",i,i2c_data[i]);
		
	}
	tmp = p[4];
	p[4] = p[7];
	p[7] = tmp;

	tmp = p[5];
	p[5] = p[6];
	p[6] = tmp;

	
	i2c_data = p;

	for (i = 0;i < 4;i++){
		DEBUG_ERR("p%d = 0x%x",i,i2c_data[i]);
		
	}

}



void swap_i2cdata2(const unsigned char *pt,unsigned char *i2c_data)
{
	unsigned char tmp;
	int i;
	unsigned char *p = i2c_data;
	for (i = 0;i < 8;i++){
		DEBUG_ERR("i2c_data%d = 0x%x",i,i2c_data[i]);
	}

	for (i = 0;i < 8;i++){
		DEBUG_ERR("pt%d = 0x%x",i,pt[i]);
	}

	i2c_data[0] = pt[1];
	i2c_data[1] = pt[0];
	
	i2c_data[2] = (pt[3] << 1);
	i2c_data[3] = (pt[2] << 1);
	
	i2c_data[4] = pt[4];
	i2c_data[5] = pt[5];
	
	i2c_data[6] = pt[6];
	i2c_data[7] = pt[7];
	for (i = 0;i < 8;i++){
		DEBUG_ERR("i2c_data%d = 0x%x",i,i2c_data[i]);
	}


}



int SendCmdV (Command *cmd)
{
	int elapsed_ms = 0;
	int num_32b_words = cmd->num_32b_words;
	unsigned int *i2c_data = (unsigned int *)cmd;
	int size = num_32b_words + 2;
	int err_no;
	cmd->num_32b_words = (cmd->command_id&CMD_GET(0)) ? MAX_COMMAND_SIZE : num_32b_words;
	unsigned char *pt = NULL;
	unsigned char tmp;
	unsigned short *p16;
	unsigned char *p8 = cmd;
	int i;
#if 0
	int i;
	unsigned char *p8 = cmd;
	unsigned char tmp;
	unsigned short *p16;
	tmp = p8[0];
	p8[0] = p8[1];
	p8[1] = tmp;
	p16 = &p8[2];
	p16[0] = (p16[0]<<15) | (p16[0]>>1);
	tmp = p8[2];
	p8[2] = p8[3];
	p8[3] = tmp;
	tmp = p8[4];
	p8[4] = p8[7];
	p8[7] = tmp;
	tmp = p8[5];
	p8[5] = p8[6];
	p8[6] = tmp;	

	swap_i2cdata1(i2c_data);
	for (i = 0;i < 8;i++){
		DEBUG_ERR("p%d = 0x%x",i,i2c_data[i]);
		
	}
#endif	
	

	// write words 1 to N-1 , to addresses 4 to 4+4*N-1

	if( debugflag >=2)
	{
		LOG((DBG_INFO "SendCmd: update command buffer.\n"));
	}
	err_no =  i2c_sub_burst_write(4,(size-1)*4,(unsigned char*)&i2c_data[1]);
	if(err_no <0)
	{
		return -1;
	}

	// write word 0 to address 0
	if( debugflag >=2)
	{
		LOG((DBG_INFO "SendCmd: commit and trigger device to process command \n"));
	}
	err_no = i2c_sub_burst_write(0,4,(unsigned char*)&i2c_data[0]);
	if(err_no <0)
	{
		if( debugflag )
		{
			LOG((DBG_INFO "SendCmd: I2C ERROR\n"));
		}
		return -1;
	}

 
	if( debugflag >=2)
		LOG((DBG_INFO "SendCmd: Repeatedly check if device have done the command\n"));
	int flage = 1;

	while (elapsed_ms < REPLY_POLL_TIMEOUT_MSEC)
	{
		// only read the first word and check the reply bit
		err_no = i2c_sub_burst_read(0,4,(uint8_t*)&i2c_data[0]);
		if(err_no <0)
			return -1;

#if 1

	if (flage == 1){
		p8 = &i2c_data[0];
		tmp = p8[0];
		p8[0] = p8[1];
		p8[1] = tmp;
		tmp = p8[2];
		p8[2] = p8[3];
		p8[3] = tmp;
		p16 = &p8[2];
		p16[0] = (p16[0]>>15) | (p16[0]<<1);
		flage = 0;
	}	
#endif
	//	swap_i2cdata2(pt,i2c_data);


		if (cmd->reply==1){
			break;

		}
		sys_mdelay(REPLY_POLL_INTERVAL_MSEC);
		elapsed_ms += REPLY_POLL_INTERVAL_MSEC;
	}

	if (cmd->reply==1)
	{
		if( cmd->num_32b_words >0)
		{
			if( debugflag >=2)
				LOG((DBG_INFO "SendCmd: receiving the returned data from device \n"));
			pt = i2c_data;
			p8 = &pt[0];
			tmp = p8[0];
			p8[0] = p8[1];
			p8[1] = tmp;
			tmp = p8[2];
			p8[2] = p8[3];
			p8[3] = tmp;
			p16 = &p8[2];
			p16[0] = (p16[0]>>15) | (p16[0]<<1);
			
			tmp = p8[8];
			p8[8] =p8[11];
			p8[11] = tmp;
			tmp = p8[9];
			p8[9] =p8[10];
			p8[10] = tmp;
				
		//	i2c_data = pt;
			
			cmd->num_32b_words = byte_swap_ushort(cmd->num_32b_words);
			int write_addr = 0x08;
			if (cmd->num_32b_words == 0x1){
			pt = i2c_data;
			p8 = &pt[8];
			tmp = p8[9];
			p8[9] = p8[10];
			p8[10] = tmp;
			DEBUG_ERR("p8[10] = 0x%x p8[9] = 0x%x",p8[10],p8[9]);
			tmp = p8[8];
			p8[8] = p8[15];
			p8[15] = tmp;
			}
		//	write_addr = byte_swap_ulong(write_addr);
			err_no = i2c_sub_burst_read(8,cmd->num_32b_words*4,(uint8_t*)&i2c_data[2]);
			if(err_no <0)
				return -1;

#if 0
			p8 = &i2c_data[2];
			for(i = 0;i < cmd->num_32b_words;i ++)
			{
				tmp = p8[0];
				p8[0] = p8[3];
				p8[3] = tmp;
				tmp = p8[1];
				p8[1] = p8[2];
				p8[2] = tmp;
				p8 = p8 + 4;
			}	
#endif
		}
		if ( cmd->num_32b_words < 0 )
		{
			if( debugflag >=2)
				LOG((DBG_INFO "SendCmd: Failed, error = %d\n",cmd->num_32b_words ));
		}

		return (cmd->num_32b_words);
	}
	else
	{
		if( debugflag >=2)
			LOG((DBG_INFO "SendCmd: Time out, no responding from device.\n"));
	}
	return(-1);
}



