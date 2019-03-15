#ifndef __OMNICFG_H__
#define __OMNICFG_H__

#define SC_LOCK_SPEC_CHAR               0x14
#define SC_LOCK_INTERVAL_LEN            0x10
#define SC_LOCK_LENGTH_BASE             400 // OMNICFG_CONFIG_LENGTH_BASE - 100
#define SC_FRAME_TIME_INTERVAL          18      // unit: ms

#define SC_LOCK_PHASE_DATA_NUMS         256
#define SC_LOCK_PHASE_BIT_MAP_NUMS      SC_LOCK_PHASE_DATA_NUMS/32

struct lock_phase_ta {
    unsigned char addr[6];
    unsigned int bit_map[SC_LOCK_PHASE_BIT_MAP_NUMS];

    struct lock_phase_ta *next;
};

struct try_lock_data {
    unsigned int frames[SC_LOCK_PHASE_DATA_NUMS];
    unsigned int timestamp[SC_LOCK_PHASE_DATA_NUMS];
    unsigned int start_pos;
    unsigned int next_pos;

    struct lock_phase_ta *ta_list;
};

int try_lock_channel(unsigned int data, unsigned char *addr, struct try_lock_data *lock_data, int *ret_state);
int free_lock_data(struct try_lock_data *lock_data);

#define OMNICFG_CONFIG_LENGTH_BASE    500
#define OMNICFG_CONFIG_SHIFT          2
#define OMNICFG_CONFIG_BLOCK_BYTES    8
#define OMNICFG_CONFIG_BLOCK_FRAMES   OMNICFG_CONFIG_BLOCK_BYTES*2 // one frame 4 bits
#define OMNICFG_CONFIG_BLOCK_NUMS     13
#define OMNICFG_CONFIG_LENGTH_MASK    0xff03    // SC info is only in bit[7:2]

#define SC_CONTROL_DATA             0x10 // BIT(4)
#define SC_EVEN_ODD                 0x20 // BIT(5)

#define SC_CTL_SSID_BLOCK_0         0x00
#define SC_CTL_SSID_BLOCK_1         0x01
#define SC_CTL_SSID_BLOCK_2         0x02
#define SC_CTL_SSID_BLOCK_3         0x03
#define SC_CTL_SSID_BLOCK_4         0x04
#define SC_CTL_SSID_BLOCK_5         0x05
#define SC_CTL_SSID_BLOCK_6         0x06
#define SC_CTL_SSID_BLOCK_7         0x07
#define SC_CTL_SSID_BLOCK_8         0x08
#define SC_CTL_SSID_BLOCK_9         0x09
#define SC_CTL_SSID_BLOCK_10        0x0A
#define SC_CTL_SSID_BLOCK_11        0x0B
#define SC_CTL_SSID_BLOCK_12        0x0C


struct sc_block_entry {
    unsigned char current_rx_count;
    unsigned char value[OMNICFG_CONFIG_BLOCK_FRAMES];
};

struct sc_rx_block {
    unsigned int block_map;
    unsigned char block_counter[OMNICFG_CONFIG_BLOCK_NUMS];
    struct sc_block_entry block[OMNICFG_CONFIG_BLOCK_NUMS];

    unsigned char rx_even[32];
    unsigned char rx_even_count;
    int current_rx_block;
    unsigned char pending_rx_data;
};

unsigned char* mimo_decoder(unsigned int data, struct sc_rx_block *blocks);

//#include <mac_ctrl.h>
//#include <wla_def.h>
#include <linux/jiffies.h>

// channel lock
static struct try_lock_data *lock_data=NULL;
//int len_low = SC_LOCK_LENGTH_BASE + SC_LOCK_SPEC_CHAR + 24; // 24: normal wifi header
//int len_high = len_low + 40 + SC_LOCK_INTERVAL_LEN*3; // 40 is a safe value
static int header_len_diff;

// decode
static unsigned char lock_mac_addr[6]; //={0xfful,0xfful,0xfful,0xfful,0xfful,0xfful};
static struct sc_rx_block *blocks=NULL;
#define SC_PRINT     printk
#define SC_MALLOC(x) kmalloc((x), GFP_ATOMIC)
#define SC_FREE      kfree

#define LE_WLAN_BA_TID      0x00f0ul
#define LE_WLAN_BA_TID_S    4
#define LE_WLAN_BA_SSN_L    0xf000ul
#define LE_WLAN_BA_SSN_L_S  12
#define LE_WLAN_BA_SSN      0x00fful
#define LE_WLAN_BA_SSN_S    4
#define WLAN_BA_SSN_FIELD   0x0ffful

#define	WLAN_ADDR_LEN			6

#define	WLAN_FC_TYPE_CTRL			0x0400	// 0x01
#define	WLAN_FC_TYPE_DATA			0x0800	// 0x02
                                            // 
#define	WLAN_FC_SUBTYPE_BA				0x9000	//	0x9
#define	WLAN_FC_SUBTYPE_ACK				0xd000	//	0xd
#define	WLAN_FC_SUBTYPE_QOS				0x8000	//	0x8
                                               
#define FRAME_CONTROL_FIELD \
        u16             subtype:4;              \
        u16             type:2;                 \
        u16      ver:2; \
        u16             order:1;        \
        u16             protected:1;    \
        u16             moredata:1;             \
        u16             pwr_mgt:1;              \
        u16             retry:1;                \
        u16             morefrag:1;             \
        u16             from_ds:1;              \
        u16             to_ds:1;                \

#define QOS_CONTROL_FIELD \
        u8              amsdu:1; \
        u8              ack_policy:2; \
        u8              eosp:1; \
        u8              tid:4;  \
        u8              txop_dur;

struct wlan_hdr {
        FRAME_CONTROL_FIELD
        u16             dur;
        u8              addr1[WLAN_ADDR_LEN];
        u8              addr2[WLAN_ADDR_LEN];
        u8              addr3[WLAN_ADDR_LEN];
        u16             seq_frag;
} __attribute__ ((packed));

struct wlan_qoshdr {
        FRAME_CONTROL_FIELD
        u16             dur;
        u8              addr1[WLAN_ADDR_LEN];
        u8              addr2[WLAN_ADDR_LEN];
        u8              addr3[WLAN_ADDR_LEN];
        u16             seq_frag;
        QOS_CONTROL_FIELD
} __attribute__ ((packed));

struct wlan_ctrl_hdr {
        FRAME_CONTROL_FIELD
        u16             dur;
        u8              addr1[WLAN_ADDR_LEN];
        u8              addr2[WLAN_ADDR_LEN];
} __attribute__ ((packed));

struct wlan_ba
{
    struct wlan_ctrl_hdr    hdr;
    u16                     bactl;
    u16                     baseqctl;
} __attribute__ ((packed));

struct ieee80211_ctrl_ba
{
    __le16 frame_control;
    __le16 duration;
    u8 ra[6];
    u8 ta[6];
    __le16 ba_ctrl;
    __le16 start_seq_num;
    u8 bitmap[0];
} __attribute__ ((packed));

unsigned int current_timestamp(void)
{
    unsigned int milliseconds;

    milliseconds=(unsigned int)(jiffies + 30000);

    return milliseconds;
}

#define OMNICFG_BASELEN   200

struct omnicfg 
{
    u8 bss_desc;
    u8 state;
    u8 decoder;
    u8 fixchan;                     //      fixed chan section
    u8 channel_index;
    u8 cur_chan;            //      current channel
    u8 num_of_channel;
    u8 channel_below_first; //      channel bellow or above
    u8 swupdwn;                     //      need to above/below chan
    u8 macmatch;            //  mac match bit field
    u32 datalen;            //      ssid len
    u32 datacnt;
    u32 offset;                     //      actually len = pktlen - offset
    
    u8 ta[6];
    
    u32 valid_pkt_count;
    u32 valid_pkt_count_prev;
    u32 signature_count;
    u32 signature_count_prev;
    unsigned long next_decode_status_check_time;
    
    u8 black_list_count;
    u8 black_list_channel_num;
    
    unsigned long total;
    unsigned long chantime;

    u8 org_channel;
    u8 org_bandwidth;
};


#endif // __OMNICFG_H__
