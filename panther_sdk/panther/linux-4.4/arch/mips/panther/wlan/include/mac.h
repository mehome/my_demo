#ifndef __MAC_H__
#define __MAC_H__

#include <linux/spinlock.h>

#include "dragonite_common.h"
#include "mac_tables.h"

#define ETH_ALEN    6

#define REKEY_TYPE_PKEY 0
#define REKEY_TYPE_GKEY 1

typedef enum {
	DRG_BSS_PS_OFF,
	DRG_BSS_PS_ON,
} DRG_BSS_PS_STATUS;

typedef enum {
	DRG_BCQ_DISABLE,
	DRG_BCQ_ENABLE,
} DRG_BCQ_STATUS;

struct __bss_info
{
    int timer_index;
    int beacon_interval;
    int dtim_period;
    bool enable_beacon;
    unsigned int dut_role;
    unsigned char BSSID[ETH_ALEN];
    unsigned char MAC_ADDR[ETH_ALEN];
    unsigned char WAN_MAC_ADDR[ETH_ALEN];
    unsigned long LAN_IP_ADDR;
    unsigned long WAN_IP_ADDR;
    int AP_STA_NUM;
    int ap_stacap_idx;
    int ap_dstable_idx;
    int cipher_mode;
    int ps_stat;
    bool occupy;
    int dtim_count;
    u32 *beacon_tx_buffer_ptr;

    u8 associated_bssid[ETH_ALEN] __attribute__ ((aligned (4)));
    
};

typedef struct {

    sta_cap_tbl* sta_cap_tbls;                      /* STA station capability tables */
    
    //int rate_tbl_count;
    //sta_cap_tbl* rate_tbls;                       /* STA station capability tables next page for rate selection */

    cipher_key* group_keys;                         /* pointer to group/def keys array */
    cipher_key* private_keys;                       /* pointer to private/pair keys array */

    int buffer_hdr_count;                  /* total number of buffer headers in SW pool */
    int rx_buffer_hdr_count;               /* total number of buffer headers in SW pool used for RX linklist */
    volatile buf_header *buf_headers;       /* array of allocated buffer headers; Software path pool (set base address to hardware)*/

    volatile int tx_buffer_header_alloc_count;
    u32 *swpool_bh_idx_to_skbs;                     /* the corresponding struct sk_buff * */
    u32 *swpool_bh_idx_to_alloc_skb_data;

    spinlock_t drg_mac_lock;
    unsigned long drg_mac_lock_irqflags;

    volatile int rx_freelist_head_index;                     /* index of free list head of buf_header for RX usage */
    volatile int rx_freelist_tail_index;
    volatile int sw_tx_bhdr_head;                     /* index of free list head of buf_header for TX usage */
    volatile int sw_tx_bhdr_tail;

    int beacon_q_buffer_hdr_count;                /* static headers for HW beacon queue */
    buf_header *beacon_q_buf_headers;
    int beacon_tx_descr_count; 
    ssq_tx_descr *beacon_tx_descriptors;

    int rx_descr_count;                             /* static allocated rx_descriptor count */
    volatile rx_descriptor *rx_descriptors;                  /* array of static allocated rx_descriptors */

    int tx_descr_index;                             /* next/first free tx descriptor index */
    int rx_descr_index;                             /* next unhandled rx descriptor index */

#define ACQ_NUM   6
#define CMD_NUM   2

#define ACQ_POOL_SIZE 32

    acq* def_acq;
    acq* acq_pool;
    u32 acq_hw_requested[ACQ_NUM][CMD_NUM];
    volatile acq* acq_free_list;

    int fastpath_bufsize;                           /* frame buffer size on fastpath */
    int sw_path_bufsize;

    u8 *ext_sta_table;
    u8 *ext_ds_table;

	u8 sta_tbl_count;
	u8 ds_tbl_count;

    u8 *protect_guard_head;
    u8 *protect_guard_tail;

    u32 sta_idx_bitmap;
    volatile int sta_idx_msb;
    volatile bool sta_ampdu_recover;

    volatile u32 sta_ps_on;
    volatile u32 sta_ps_off_indicate;
    volatile u32 sta_ps_recover;
    volatile u32 sta_ps_poll_request;

    spinlock_t drg_ps_lock;
    unsigned long drg_ps_lock_irqflags;

    volatile int bcq_status[MAX_BSSIDS];
    volatile int bss_ps_status[MAX_BSSIDS];

    spinlock_t drg_bss_ps_lock;
    unsigned long drg_bss_ps_lock_irqflags;
    
} MAC_INFO;

#define TXDESCR_SIZE  sizeof(tx_descriptor)
extern MAC_INFO mac_info;
extern MAC_INFO *info;

int set_mac_info_parameters(MAC_INFO* info);
int init_station_cap_tables(MAC_INFO* info);
int init_key_tables(MAC_INFO* info);

int init_buf_headers(MAC_INFO* info);
void release_buffer_headers(MAC_INFO* info);
int mac_set_pairwise_key(int sta_id, cipher_key *key);
int mac_set_group_key(int bss_idx, cipher_key *key);
void mac_invalidate_ampdu_scoreboard_cache(int sta_index, int tid);

buf_header* alloc_buf_headers(int count);

#define MAC_MALLOC_CACHED       0x00000000
#define MAC_MALLOC_UNCACHED     0x00000001
#define MAC_MALLOC_BZERO        0x00000002      /* bzero the allocated memory */
#define MAC_MALLOC_ATOMIC       0x00000004      /* use atomic memory pool     */
#define MAC_MALLOC_ASSERTION    0x00000008      /* assert when the malloc is failed */
#define MAC_MALLOC_DMA          0x00000010      /* allocate in the DMA zone (<16Mbyes) */

#define ROLE_NONE   0
#define ROLE_AP     1
#define ROLE_STA    2
#define ROLE_IBSS   3
#define ROLE_P2PC   4
#define ROLE_P2PGO  5

void *mac_alloc_skb_buffer(MAC_INFO* info, int size, int flags, u32* pskb);
void mac_kfree_skb_buffer(u32 *pskb);
void *buf_alloc(void);
void mac_free(void *ptr);
int init_transmit_descriptors(MAC_INFO* info);
void release_descriptors(MAC_INFO* info);

MAC_INFO* dragonite_mac_init(void);
void dragonite_mac_release(MAC_INFO* info);

buf_header* mac_tx_freelist_get_first(MAC_INFO* info, int *index, int blocking);
int mac_tx_freelist_insert_tail(MAC_INFO* info, int index);
tx_descriptor* get_free_tx_descr(MAC_INFO* info, int blocking);
tx_descriptor* get_free_eth_tx_descr(MAC_INFO* info, int blocking);
int poll_eth_tx_return_descr(MAC_INFO* info, tx_descriptor* eth_tx_r_descr);
int mac_trigger_tx(void);
int mac_eth_trigger_tx(void);
int mac_ssq_trigger_tx(void);

#endif // __MAC_H__

