
/*
 * @file mtdm_types.h
 *
 * @Author  Frank Wang
 * @Date    2015-10-06
 */

#ifndef _MTDM_TYPES_H_
#define _MTDM_TYPES_H_

#define BUF_LEN_16      16
#define BUF_LEN_32      32
#define BUF_LEN_64      64
#define BUF_LEN_128     128
#define BUF_LEN_256     256
#define BUF_LEN_512     512
#define BUF_LEN_1024    1024
#define BUF_LEN_2048    2048
#define BUF_LEN_4096    4096
#define BUF_LEN_8192    8192

#ifndef  NULL
#define  NULL   ((void*)0)
#endif

#define YES     1
#define NO      0
#define NotUsed (-1)

#define success 1
#define fail    0

#ifndef TRUE
#define TRUE    1
#endif

#ifndef FALSE
#define FALSE   0
#endif

#define BUF_LEN_4    4
#define BUF_LEN_8    8
#define BUF_LEN_16   16
#define BUF_LEN_32   32
#define BUF_LEN_64   64
#define BUF_LEN_128  128
#define BUF_LEN_256  256
#define BUF_LEN_512  512
#define BUF_LEN_1024 1024
#define BUF_LEN_2048 2048
#define BUF_LEN_4096 4096
#define BUF_LEN_8192 8192

#define IBUF_LEN     4   /* for cdb INT */
#define PBUF_LEN     16  /* for cdb IP */
#define MBUF_LEN     18  /* for cdb MAC */
#define SBUF_LEN     512 /* for cdb STR */
#define USBUF_LEN    BUF_LEN_32  /* for cdb small STR */
#define SSBUF_LEN    BUF_LEN_64  /* for cdb small STR */
#define MSBUF_LEN    BUF_LEN_128 /* for cdb middle STR */
#define LSBUF_LEN    BUF_LEN_512 /* for cdb large STR */


#define s8           char
#define u8           unsigned char
#define s16          short
#define u16          unsigned short
#define s32          int
#define u32          unsigned int

/* interface */
typedef enum
{
    INFC_KEY = 0x0100,
    INFC_CMD = 0x0101,
    INFC_DAT = 0x0102,

    INFC_MAX = 0x01ff
} IfCode;

/* return code */
typedef enum
{
    RETC_ERROR = 0x0200,
    RETC_OK,

    RETC_MAX = 0x02ff
} RtCode; 

typedef struct
{
    union {
        IfCode ifc;
        RtCode rtc;
    };
    int opc;
    int arg;
    unsigned int len;
}PHdr;

#define KEY_DEBUG 0

/* key code */
typedef enum
{
    OPC_KEY_MIN             = 0x0300,
    OPC_KEY_SHORT_A         = 0x0300,
    OPC_KEY_SHORT_B         = 0x0301,
    OPC_KEY_SHORT_C         = 0x0302,
    OPC_KEY_SHORT_D         = 0x0303,
    OPC_KEY_LONG_A          = 0x0304,
    OPC_KEY_LONG_B          = 0x0305,
    OPC_KEY_LONG_C          = 0x0306,
    OPC_KEY_LONG_D          = 0x0307,
    OPC_KEY_HWHEEL_RIGHT    = 0x0308,
    OPC_KEY_HWHEEL_LEFT     = 0x0309,

    OPC_KEY_VIRTUAL_A       = 0x03f0,
    OPC_KEY_MAX             = 0x03ff
} OpCodeKey;

/* cmd code */
typedef enum
{
    OPC_CMD_MIN             = 0x0400,
    OPC_CMD_ALSA            = 0x0400,
    OPC_CMD_WDK                     ,
    OPC_CMD_USB                     ,
    OPC_CMD_LCD                     ,
    OPC_CMD_BOOT                    ,
#ifdef CONFIG_PACKAGE_libdbus    
    OPC_CMD_DBUS                    ,
#endif    
    OPC_CMD_AVAHI                   ,
    OPC_CMD_CROND                   ,
    OPC_CMD_CDB                     ,
    OPC_CMD_TELNET                  ,
    OPC_CMD_UARTD					,
    OPC_CMD_POSTPLAYLIST			,
    OPC_CMD_ALEXA			,
    OPC_CMD_OCFG                    ,
    OPC_CMD_DONE                    ,
    OPC_CMD_SYSCTL                  ,
    OPC_CMD_UMOUNT                  ,

    OPC_CMD_LOG             = 0x0420,
    OPC_CMD_OP                      ,
    OPC_CMD_VLAN                    ,
    OPC_CMD_LAN                     ,
    OPC_CMD_LAN6                    ,
    OPC_CMD_WL                      ,
    OPC_CMD_WAN                     ,
    OPC_CMD_FW                      ,
    OPC_CMD_NAT                     ,
    OPC_CMD_ROUTE                   ,
    OPC_CMD_DDNS                    ,
    OPC_CMD_DLNA                    ,
    OPC_CMD_HTTP                    ,
    OPC_CMD_DNS                     ,
    OPC_CMD_SMB                     ,
    OPC_CMD_FTP                     ,
    OPC_CMD_PRN                     ,
    OPC_CMD_QOS                     ,
    OPC_CMD_SYS                     ,
    OPC_CMD_ANTIBB                  ,
    OPC_CMD_IGMP                    ,
    OPC_CMD_LLD2                    ,
    OPC_CMD_UPNP                    ,
    OPC_CMD_RA                      ,
    OPC_CMD_OMICFG                  ,

    OPC_CMD_COMMIT          = 0x0480,
    OPC_CMD_SAVE                    ,
    OPC_CMD_SHUTDOWN                ,
    OPC_CMD_WANIPUP                 ,
    OPC_CMD_WANIPDOWN               ,
    OPC_CMD_OCFGARG                 ,
    OPC_CMD_SLEEP                   ,
#ifdef SPEEDUP_WLAN_CONF
    OPC_CMD_WLHUP                   ,
#endif

    OPC_CMD_MAX             = 0x04ff
} OpCodeCmd;
#define OPC_CMD_IDX(x)    ((x) - OPC_CMD_MIN)

/* cmd arg */
typedef enum
{
    OPC_CMD_ARG_MIN         = 0,
    OPC_CMD_ARG_UNKNOWN     = 0,
    OPC_CMD_ARG_BOOT,
    OPC_CMD_ARG_START,
    OPC_CMD_ARG_STOP,
    OPC_CMD_ARG_RESTART,
    OPC_CMD_ARG_DBG,
    OPC_CMD_ARG_UNDBG,
    OPC_CMD_ARG_MAX
} OpCodeCmdArg;


/* data code */
typedef enum
{
    OPC_DAT_MIN             = 0x0500,
    OPC_DAT_INFO            = 0x0500,

    OPC_DAT_MAX             = 0x05ff
} OpCodeData;
#define OPC_DAT_IDX(x)    ((x) - OPC_DAT_MIN)

/* data arg */
typedef enum
{
    OPC_DAT_ARG_UNKNOWN     = 0,
} OpCodeDataArg;


typedef struct
{
    PHdr head;
    char *data;
} MtdmPkt;

#define MTDMSOCK "/var/run/mtdm.sock"
#define MTDM_CLIENT_SOCK "/var/run/mtdm.client_"




#endif


