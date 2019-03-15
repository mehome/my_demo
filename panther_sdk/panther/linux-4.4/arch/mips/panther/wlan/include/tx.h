#ifndef __TX_H__
#define __TX_H__

#include <packet.h>
#include <os_compat.h>
#include <mac.h>
#include <dragonite_main.h>
#include <skb.h>

int dragonite_tx_start(void);
int dragonite_tx_stop(void);
void setup_tx_acq(MAC_INFO* info);
void resetup_tx_acq(MAC_INFO *info);
int init_tx_acq(MAC_INFO* info);
void release_tx_acq(MAC_INFO *info);

#define ACQ_MAX_QSIZE 64

#define ACQ_FLAG_DEFAULT_Q       0x01
#define ACQ_FLAG_CMD0_REQ        0x02
#define ACQ_FLAG_CMD1_REQ        0x04
#define ACQ_FLAG_ACTIVE          0x08
#define ACQ_FLAG_LOCK            0x10     /* do not switch it before all of the queued TX is done */
#define ACQ_FLAG_DROP            0x20
#if defined (DRAGONITE_ACQ_BAN)
#define ACQ_FLAG_BAN             0x40
#endif

#define AC_BK_QID       0
#define AC_BE_QID       1
#define AC_VI_QID       2
#define AC_VO_QID       3
#define AC_LEGACY_QID   4
#define BCQ_QID         5

#define ACQ_AIDX_SHIFT 16

#define INVALID_SSN 0xffffU
#define IS_INVALID_SSN(_ssn) (_ssn == INVALID_SSN)

#define EMPTY_PKT_BUFH  0x3ff

#define ACQ_INTR_BIT(qid, cmdid)  (0x01UL << ((qid * 2) + cmdid))

#define NOT_USE_VIF_IDX (-1)
#define NOT_USE_QUE_IDX (-1)

#define CHECK_OUT_HW_QUE    true

int tid_to_qid(int tid);
#define DEF_ACQ(qid)    &info->def_acq[qid]
#define ACQE(q, ptr)    &q->acqe[ptr % q->queue_size]
#define SWACQE(q, ptr)    &q->acqe[(ptr % q->queue_size) + (q->queue_size)]

#define ACQ_EMPTY(q)    (q->wptr == q->rptr)

#define IS_MGMT_FRAME(_flags)   (_flags & SKB_INFO_FLAGS_MGMT_FRAME)
#define IS_SPECIAL_DATA_FRAME(_flags)   (_flags & SKB_INFO_FLAGS_SPECIAL_DATA_FRAME)
#define IS_MULTICAST_FRAME(_flags)   (_flags & SKB_INFO_FLAGS_MULTICAST_FRAME)

int mpdu_copy_to_buffer(MAC_INFO* info, mpdu *pkt, int use_eth_hdr, int *bhr_h, int *bhr_t);

u16 tx_rate_encoding(int format, int ch_offset, int retry_count, int sgi, int rate);

void skb_list_add(volatile struct list_head *node, struct skb_list_head *queue);
void skb_list_del(volatile struct list_head *node, struct skb_list_head *queue);

void dragonite_acq_intr_handler(struct mac80211_dragonite_data *data);

void tx_single_acq_setup(acq *acq, int acqid, int spacing, int max_length, int win_size, int queue_size, int aidx, int tid, int ssn);
void tx_single_acq_start(acq *acq, int ssn);
void tx_mixed_acq_setup(acq *acq, int acqid, int queue_size);
int tx_acq_kick(MAC_INFO *info, acq *q, u32 esn);
bool tx_acq_cmd_req(MAC_INFO *info, acq *q);
int tx_acq_detach_async(MAC_INFO *info, acq *q);
int tx_acq_detach(struct mac80211_dragonite_data *data, acq *q);
int tx_acq_attach(MAC_INFO *info, acq *q);
acq *tx_acq_alloc(MAC_INFO *info);
void tx_acq_free(MAC_INFO *info, acq *q);

int dragonite_tx_skb(struct mac80211_dragonite_data *data, struct sk_buff *skb, struct skb_info *skbinfo, struct dragonite_vif_priv *vp, struct dragonite_sta_priv *sp);
int dragonite_tx_get_ssn(struct dragonite_vif_priv *vp, struct dragonite_sta_priv *sp, struct skb_info *skbinfo);

int skb_attach_buffer_header(MAC_INFO *info, struct sk_buff *skb, int *bhr_h);
struct sk_buff *buffer_header_detach_skb(MAC_INFO *info, int bhr_idx);
struct sk_buff *buffer_header_peek_skb(MAC_INFO *info, int bhr_idx);

void check_hw_queue(struct mac80211_dragonite_data *data);

int dragonite_tx_send_ba_request(struct mac80211_dragonite_data *data, int sta_index, u16 tid, u16 ssn);
int dragonite_tx_send_null_data(struct mac80211_dragonite_data *data, int sta_index);
int dragonite_tx_pspoll_response(struct mac80211_dragonite_data *data, int sta_index, int tid);
void dragonite_tx_bcq_handler(struct mac80211_dragonite_data *data, int bss_index);
#if defined (DRAGONITE_TX_BHDR_CHECKER)
void dragonite_tx_bhdr_checker(u32 wakeup_t);
extern int tx_bhdr_idx_low;
extern int tx_bhdr_idx_high;
#endif
#if defined (DRAGONITE_TX_SKB_CHECKER)
void dragonite_tx_skb_checker(struct mac80211_dragonite_data *data, u32 wakeup_t);
#endif
#if defined (DRAGONITE_TX_HANG_CHECKER)
void dragonite_init_tx_hang_checker(struct mac80211_dragonite_data *data);
void dragonite_tx_hang_checker(struct mac80211_dragonite_data *data, u64 wakeup_jiffies);
#endif
void dragonite_tx_release_skb_queue(struct mac80211_dragonite_data *data, u32 wakeup_t, bool force);
void dragonite_tx_ps_buffer_adjust(struct mac80211_dragonite_data *data, int sta_index);
void dragonite_tx_bmcq_to_bcq(struct mac80211_dragonite_data *data, int bss_index);
void dragonite_tx_bmcq_to_mixq(struct mac80211_dragonite_data *data, int bss_index);
void dragonite_tx_swq_to_psq(struct mac80211_dragonite_data *data, int sta_index);
void dragonite_tx_swq_to_gpq(struct mac80211_dragonite_data *data, int sta_index);
void dragonite_tx_swq_dump(struct mac80211_dragonite_data *data, struct list_head *lptr);
void dragonite_tx_ampdu_q_detach(struct mac80211_dragonite_data *data, int sta_idx, int tid);
void dragonite_tx_cleanup_pending_skb(struct mac80211_dragonite_data *data, bool check_out_hw_que);
void dragonite_tx_detach_vif_node(struct mac80211_dragonite_data *data, struct dragonite_vif_priv *vp);
void dragonite_tx_detach_node(struct mac80211_dragonite_data *data, struct dragonite_sta_priv *sp);
void dragonite_tx_schedule(struct mac80211_dragonite_data *data, int vif_idx, int swq_idx, int swq_id);
bool dragonite_has_tx_pending(struct mac80211_dragonite_data *data);

void acq_dump(acq *q);

struct dragonite_tx_descr 
{
    struct list_head private_list;
    struct list_head group_list;
#if defined (DRAGONITE_TX_SKB_CHECKER)
    struct list_head skb_list;
#endif
    u32     skb_addr;
    u32     skbinfo_flags;

    u32     vp_addr;
    u32     sp_addr;
    u32     acqe_addr;
    u32     tid;
    u32     ts_tstamp;
    u32     free_tstamp;
};
#define DRAGONITE_TXDESCR_SIZE 64
#define DRAGONITE_TX_MGMT_STATION_INDEX 32
#define DRG_TX_SKB_DELAY_FREE_PERIOD 2 //sec
#define DRG_TX_SKB_DELAY_FREE_TIME 5 //sec
#endif // __TX_H__

