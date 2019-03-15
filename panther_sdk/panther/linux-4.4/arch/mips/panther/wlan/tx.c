#include <tx.h>
#include <buffer.h>
#include <mac.h>
#include <mac_tables.h>
#include <mac_ctrl.h>
#include <os_compat.h>
#include <dragonite_main.h>
#include <resources.h>
#include <skb.h>
#include <wla_ctrl.h>
#include <linux/ktime.h>
#include <linux/skbuff.h>

#include "dragonite_common.h"
#include "mac_regs.h"
#include "beacon.h"

#define DRG_MAX_BUFFER_COUNT 300
#define PS_BUFFER_COUNT 32
#define PS_BUFFER_OVERFLOW(sp) (sp->tx_ps_buffer_skb_count > PS_BUFFER_COUNT)

#define MIN(x,y)  ((x) > (y)) ? (y) : (x)
#define MAX_BUFFER_PER_PACKET 9

#define cosim_delay_ns udelay

#if defined (DRAGONITE_TX_BHDR_CHECKER)
#define DRG_TX_BHDR_CHECKER_PERIOD 5 //sec
#define DRG_TX_BHDR_TIMEOUT_TOLERANT 10 //sec
#endif

#if defined (DRAGONITE_TX_HANG_CHECKER)
#define DRAGONITE_MAGIC_TX_SSN 0x900d
#define DRG_TX_HANG_CHECKER_PERIOD 1 //sec
#define DRG_TX_TOTAL_CMD_COUNT   ((ACQ_NUM - 1) * CMD_NUM)
#endif

static DEFINE_SPINLOCK(dragonite_acq_lock);
static DEFINE_SPINLOCK(dragonite_bhdr_lock);

static int tx_alloc_count;
u32 drg_tx_acq_delay_max_cnt = 8;
u32 drg_tx_acq_delay_interval = 1;

int drg_tx_protect = 0;
int drg_tx_force_filter_cck = 0;

#if defined (DRAGONITE_TX_BHDR_CHECKER)
extern u32 bhdr_record_t[BUFFER_HEADER_POOL_SIZE];
u32 drg_tx_bhdr_checker_prev_time = 0;
#endif

#if defined (DRAGONITE_TX_SKB_CHECKER)
u32 drg_tx_skb_checker_prev_time = 0;
#endif

#if defined (DRAGONITE_TX_HANG_CHECKER)
u16 drg_tx_done_prev_intr_times = 0;
u16 drg_tx_done_intr_times = 0;
u16 drg_tx_retry_prev_intr_times = 0;
u16 drg_tx_retry_intr_times = 0;
u16 drg_tx_switch_prev_intr_times = 0;
u16 drg_tx_switch_intr_times = 0;
u16 drg_tx_prev_ssn[ACQ_NUM - 1][CMD_NUM];
u32 drg_tx_hang_checker_prev_time = 0;
u64 drg_tx_hang_checker_prev_jiffies_time = 0;
#endif

u32 drg_tx_release_skb_queue_prev_time = 0;

int dragonite_force_txrate_idx = 0;

extern volatile u32 drg_wifi_recover;
extern int wifi_tx_enable;
extern u32 drg_wifi_sniffer;
int dragonite_tx_per_ac_max_buf=48;

static void tx_rate_adjust(struct ieee80211_tx_info *txi, struct skb_info *skbinfo);
static void tx_rate_retry_adjust(struct ieee80211_tx_info *txi);
static void tx_rate_fill(struct ieee80211_tx_info *txi, tx_descriptor *descr);

void acq_dump(acq *q)
{
   int i;
   volatile acqe* acqe;
   printk(KERN_EMERG "win_size %02x, queue_size %02x, queue_size_code %02x, flags %02x, rptr %d, wptr %d, qid %d, esn %d, qinfo %08x\n",
           q->win_size, q->queue_size, q->queue_size_code, q->flags, q->rptr, q->wptr, q->qid, q->esn, q->qinfo);

   for(i=0;i<q->queue_size;i++)
   {
      printk(KERN_EMERG "acqe[%d] : ", i);
      //acqe = ACQE(q, q->rptr+i);
      acqe = &q->acqe[i];
      printk(KERN_EMERG "bhr_h %d, pktlen %d, noa %d, try_cnt %d, mpdu %d, ps %d, bmap %d, own %d\n", 
             acqe->a.bhr_h, acqe->a.pktlen, acqe->a.noa, acqe->a.try_cnt, acqe->a.mpdu, acqe->a.ps, acqe->a.bmap, acqe->a.own);
   }

}

bool static acq_full(acq *q)
{
   u16 _wptr, _rptr;

   _wptr = q->wptr;
   _rptr = q->rptr;

   return ((_wptr >= _rptr) ? ((_wptr - _rptr) == q->queue_size) : (((u16) (_wptr + 4096) - _rptr) == q->queue_size));
}

#ifdef DRAGONITE_TX_BHDR_CHECKER
u32 bhdr_record_t[BUFFER_HEADER_POOL_SIZE];
int tx_bhdr_idx_low = -1;
int tx_bhdr_idx_high = -1;
#endif
buf_header* bhdr_get_first(MAC_INFO *info)
{
   volatile buf_header *bhdr;
   unsigned long irqflags;

   //BHDR_LOCK();
   spin_lock_irqsave(&dragonite_bhdr_lock, irqflags);

   if (info->sw_tx_bhdr_head >= 0)
   {
#if defined(DRAGONITE_TX_BHDR_CHECKER)
      if((info->sw_tx_bhdr_head > tx_bhdr_idx_high)
         || (info->sw_tx_bhdr_head < tx_bhdr_idx_low))
      {
         panic("bhdr_get_first: %d %d %d\n", info->sw_tx_bhdr_head, tx_bhdr_idx_low, tx_bhdr_idx_high);
      }
#endif

      bhdr = idx_to_bhdr(info, info->sw_tx_bhdr_head);
#if defined (DRAGONITE_TX_BHDR_CHECKER)
      bhdr_record_t[info->sw_tx_bhdr_head] = (jiffies / HZ);
#endif
      if (info->sw_tx_bhdr_head == info->sw_tx_bhdr_tail)
      {
         info->sw_tx_bhdr_head = -1;   /* the freelist is now empty */
         info->sw_tx_bhdr_tail = -1;
      }
      else
      {
         info->sw_tx_bhdr_head = bhdr->next_index;
      }

      bhdr->ep = 0;
      bhdr->next_index = 0;

      tx_alloc_count++;

      info->tx_buffer_header_alloc_count = tx_alloc_count;
      //BHDR_UNLOCK();
      spin_unlock_irqrestore(&dragonite_bhdr_lock, irqflags);

      return (buf_header *)bhdr;
   }
   else
   {
      //BHDR_UNLOCK();
      spin_unlock_irqrestore(&dragonite_bhdr_lock, irqflags);
      return NULL;
   }
}
void bhdr_insert_tail(MAC_INFO *info, int head, int tail)
{
   volatile buf_header *bhdr;
   int idx;
   unsigned long irqflags;

#if defined(DRAGONITE_TX_BHDR_CHECKER)
   for (idx = head;; idx = bhdr->next_index)
   {
      if((idx > tx_bhdr_idx_high)
         || (idx < tx_bhdr_idx_low))
      {
         panic("bhdr_insert_tail: %d %d %d %d %d\n", idx, head, tail, tx_bhdr_idx_low, tx_bhdr_idx_high);
      }
      bhdr = &info->buf_headers[idx];

      if ((idx == tail) || bhdr->ep)
         break;
   }
#endif

   //BHDR_LOCK();
   spin_lock_irqsave(&dragonite_bhdr_lock, irqflags);

#if 0 // duplicate free detection
   if((tail==-1)&&(info->sw_tx_bhdr_head>=0))
   {
      for (idx = info->sw_tx_bhdr_head;; idx = bhdr->next_index)
      {
         if(idx==head)
            panic("duplicate index in freelist %d\n", head);

         bhdr = &info->buf_headers[idx];

         if (idx == info->sw_tx_bhdr_tail)
            break;
      }
   }
#endif

#if defined (DRAGONITE_TX_BHDR_CHECKER)
   bhdr_record_t[head] = 0;
#endif
   if (info->sw_tx_bhdr_tail >= 0)
   {
      bhdr = idx_to_bhdr(info, info->sw_tx_bhdr_tail);
      bhdr->next_index = head;
      bhdr->ep = 0;
   }
   else
   {
      info->sw_tx_bhdr_head = head;
   }

   for (idx = head;; idx = bhdr->next_index)
   {
      bhdr = &info->buf_headers[idx];
      //bhdr->dptr = 0;

      tx_alloc_count--;

      info->tx_buffer_header_alloc_count = tx_alloc_count;

      if ((idx == tail) || bhdr->ep)
         break;
      bhdr->ep = 0;
   }

   bhdr->ep = 1;
   info->sw_tx_bhdr_tail = bhdr_to_idx(info, (buf_header *)bhdr);

   //BHDR_UNLOCK();
   spin_unlock_irqrestore(&dragonite_bhdr_lock, irqflags);
}
void cleanup_bhdr_on_descr(MAC_INFO *info, int bhdr_index)
{
   buf_header *bhdr;
   tx_descriptor *descr;

   bhdr = idx_to_bhdr(info, bhdr_index);

   descr = (tx_descriptor *) VIRTUAL_ADDR(bhdr->dptr);

   descr->bhr_h = 0;
}
int dragonite_tx_start(void)
{

   // enable all 6 ACQs
   MACREG_WRITE32(ACQ_EN, 0x03f);

   // enable CMD0/CMD1 switch interrupt
   MACREG_UPDATE32(ACQ_INTRM, 0x0, ACQ_INTRM_CMD_SWITCH);

   // enable ACQ done interrupt
   MACREG_UPDATE32(ACQ_INTRM, 0x0, ACQ_INTRM_TX_DONE);

   // enable retry fail interrupt
   MACREG_UPDATE32(ACQ_INTRM2, 0x0, ACQ_INTRM2_RETRY_FAIL);

   return 0;
}

// Reverse the step in dragonite_tx_start
int dragonite_tx_stop(void)
{

   // disable all 6 ACQs
   MACREG_WRITE32(ACQ_EN, 0x0);

   // disable CMD0/CMD1 switch interrupt
   MACREG_UPDATE32(ACQ_INTRM, ACQ_INTRM_CMD_SWITCH, ACQ_INTRM_CMD_SWITCH);

   // disable ACQ done interrupt
   MACREG_UPDATE32(ACQ_INTRM, ACQ_INTRM_TX_DONE, ACQ_INTRM_TX_DONE);

   // disable retry fail interrupt
   MACREG_UPDATE32(ACQ_INTRM2, ACQ_INTRM2_RETRY_FAIL, ACQ_INTRM2_RETRY_FAIL);
    
   return 0;
}

void setup_tx_acq(MAC_INFO *info)
{
   int i;

   for(i=0;i<ACQ_POOL_SIZE;i++)
   {
      info->acq_pool[i].next = &info->acq_pool[i+1];
   }
   info->acq_pool[ACQ_POOL_SIZE-1].next = NULL;

   for(i=0;i<ACQ_NUM;i++)
   {
      info->acq_hw_requested[i][0] = 0;
      info->acq_hw_requested[i][1] = 0;
   }

   info->acq_free_list = &info->acq_pool[0];

   for(i=0;i<ACQ_NUM;i++)
   {
      tx_mixed_acq_setup(&info->def_acq[i], i, ACQ_MAX_QSIZE);
      info->def_acq[i].flags |= ACQ_FLAG_DEFAULT_Q;
   }
   info->def_acq[BCQ_QID].flags |= ACQ_FLAG_LOCK;

   return;
}

void resetup_tx_acq(MAC_INFO *info)
{
   int i;
   acq *q;
   volatile acq *next;
   int queue_size;
   u32 queue_size_code;

   for(i=0;i<ACQ_NUM;i++)
   {
      info->acq_hw_requested[i][0] = 0;
      info->acq_hw_requested[i][1] = 0;
   }

   for(i=0;i<ACQ_NUM;i++)
   {
      q = &info->def_acq[i];

      /* backup acq attribute */
      next = q->next;
      queue_size = q->queue_size;
      queue_size_code = q->queue_size_code;

      memset((void *) q, 0, sizeof(struct __tag_acq));

      q->qid = i;
      q->next = next;
      q->queue_size = queue_size;
      q->queue_size_code = queue_size_code;

      info->def_acq[i].flags |= ACQ_FLAG_DEFAULT_Q;
   }
   info->def_acq[BCQ_QID].flags |= ACQ_FLAG_LOCK;

   return;
}

int init_tx_acq(MAC_INFO* info)
{
   info->def_acq = (acq*)mac_malloc(NULL, sizeof(acq) * ACQ_NUM, MAC_MALLOC_BZERO | MAC_MALLOC_UNCACHED);

   info->acq_pool = (acq*)mac_malloc(NULL, sizeof(acq) * ACQ_POOL_SIZE, MAC_MALLOC_BZERO | MAC_MALLOC_UNCACHED);

   setup_tx_acq(info);

   return 0;
}

void release_tx_acq(MAC_INFO *info)
{
   if(info->def_acq)
   {
      mac_free(info->def_acq);
      info->def_acq = NULL;
   }

   if(info->acq_pool)
   {
      mac_free(info->acq_pool);
      info->acq_pool = NULL;
   }
   info->acq_free_list = NULL;

   return;
}

int tid_to_qid(int tid)
{
   switch(tid)
   {
      case 0:
      case 3:
         return AC_BE_QID;
         break;
      case 1:
      case 2:
         return AC_BK_QID;
         break;
      case 4:
      case 5:
         return AC_VI_QID;
         break;
      case 6:
      case 7:
         return AC_VO_QID;
         break;
      default:
         return AC_LEGACY_QID;
         break;
   }

   return 4;

}

int rank_id_to_tid(int rank_id)
{
   switch(rank_id)
   {
      case 0:
         return 1;
      case 1:
         return 2;
      case 2:
         return 0;
      case 3:
         return 3;
      case 4:
         return 8;
      case 5:
         return 4;
      case 6:
         return 5;
      case 7:
         return 6;
      case 8:
         return 7;
      default:
         panic("rank_id_to_tid wrong !!");
         break;
   }
   return 0;
}
int mpdu_copy_to_buffer(MAC_INFO* info, mpdu *pkt, int use_eth_hdr, int *bhr_h, int *bhr_t)
{
   buf_header *bhdr, *bhdr_prev;
   u8 *dptr;
   int bhdr_idx[MAX_BUFFER_PER_PACKET];
   int i;
   int cur_offset, copy_length;
   int alloc_failure;
   int buf_count;

   bhdr_prev = NULL;

   /* allocate buffers */
   buf_count = ((pkt->length + TXDESCR_SIZE + (DEF_BUF_SIZE - 1)) / DEF_BUF_SIZE);
   if(buf_count > MAX_BUFFER_PER_PACKET)
   {
      panic("Unexpected packet length");
   }

   for(i=0;i<buf_count;i++)
      bhdr_idx[i] = -1;

   alloc_failure = 0;
   for(i=0;i<buf_count;i++)
   {
      bhdr = bhdr_get_first(info);

      if(bhdr)
      {
         bhdr_idx[i] = bhdr_to_idx(info, bhdr);
      }
      else
      {
         alloc_failure = 1;
         break;
      }
   }

   if(alloc_failure)
   {
      for(i=0;i<buf_count;i++)
      {
         if(bhdr_idx[i] != -1)
            bhdr_insert_tail(info, bhdr_idx[i], bhdr_idx[i]);
      }

      return -1;
   }

   // fill TX data
   cur_offset = 0;
   for(i=0;i<buf_count;i++)
   {
      bhdr = idx_to_bhdr(info, bhdr_idx[i]);

      dptr = (u8 *) UNCACHED_VIRTUAL_ADDR(bhdr->dptr);

      if(i==0)
      {
         bhdr->offset = TXDESCR_SIZE;
#if !defined(NEW_BUFH)
         bhdr->offset_h = 0;
#endif
         dptr += TXDESCR_SIZE;

         copy_length = MIN(pkt->length - cur_offset, DEF_BUF_SIZE - TXDESCR_SIZE);
         memcpy((void *)dptr, &pkt->data[cur_offset], copy_length);
         bhdr->len = copy_length;
         cur_offset += copy_length;

         *bhr_h = bhdr_idx[i];
      }
      else
      {
         bhdr_prev->next_index = bhdr_idx[i];
         bhdr->offset = 0;
#if !defined(NEW_BUFH)
         bhdr->offset_h = 0;
#endif

         copy_length = MIN(pkt->length - cur_offset, DEF_BUF_SIZE);
         memcpy((void *)dptr, &pkt->data[cur_offset], copy_length);
         bhdr->len = copy_length;
         cur_offset += copy_length;
      }

      if(i==(buf_count-1))
      {
         *bhr_t = bhdr_idx[i];
         bhdr->ep = 1;
      }

      bhdr_prev = bhdr;
   }

   return 0;
}
u16 tx_rate_encoding(int format, int ch_offset, int retry_count, int sgi, int rate)
{
   union __tx_rate
   {
      struct {
#if defined(__BIG_ENDIAN)
         u16 ch_offset:2;
         u16 txcnt:3;
         u16 sgi:1;
         u16 format:2;
         u16 ht:1;
         u16 rate:7;
#else
         u16 rate:7;
         u16 ht:1;
         u16 format:2;
         u16 sgi:1;
         u16 txcnt:3;
         u16 ch_offset:2;
#endif	 
      };
      struct {
         u16 encoded_rate;
      };

   } __attribute__((__packed__));

   union __tx_rate  txrate;

   txrate.encoded_rate = 0;

   txrate.ch_offset = ch_offset;
   txrate.txcnt = retry_count;
   txrate.sgi = sgi;
   txrate.format = format;
   txrate.rate = rate;

   if((format==FMT_HT_MIXED)||(format==FMT_HT_GF))
      txrate.ht = 1;

   return txrate.encoded_rate;
}

void incr_ac_count_safe(struct mac80211_dragonite_data *data, int qid)
{
   unsigned long irqflags;

   spin_lock_irqsave(&data->tx_per_ac_count_lock, irqflags);
   data->tx_per_ac_count[qid]++;
   spin_unlock_irqrestore(&data->tx_per_ac_count_lock, irqflags);
}
void decr_ac_count_safe(struct mac80211_dragonite_data *data, int qid)
{
   unsigned long irqflags;

   spin_lock_irqsave(&data->tx_per_ac_count_lock, irqflags);
   data->tx_per_ac_count[qid]--;
   spin_unlock_irqrestore(&data->tx_per_ac_count_lock, irqflags);
}

/* return 0 if the replace is success, return -1 if the packet is already a BAR */
int replace_tx_packet_with_bar(struct mac80211_dragonite_data *data, volatile acqe *acqe)
{
   MAC_INFO* info = data->mac_info;
   tx_descriptor *descr;
   buf_header *bhdr;
   u8 *pktdata;
   u32 ssn;
   u8 sta_addr[ETH_ALEN];
   int flag;
   sta_cap_tbl* captbl;

   flag = 0;

   bhdr = idx_to_bhdr(info, acqe->a.bhr_h);
   descr = (tx_descriptor *) UNCACHED_VIRTUAL_ADDR(bhdr->dptr);
   pktdata = (u8 *) UNCACHED_VIRTUAL_ADDR(bhdr->dptr + bhdr->offset);

   if(pktdata[0]==0x84)
      return -1;

   captbl = &info->sta_cap_tbls[descr->ds_idx];

   pktdata[0] = 0x84; /* IEEE80211_STYPE_BACK_REQ & IEEE80211_FTYPE_CTL */
   pktdata[1] = 0x00;
   pktdata[2] = 0x00;
   pktdata[3] = 0x00;

   dragonite_mac_lock();

   if(data->dragonite_bss_info[descr->bssid].dut_role == ROLE_STA)
   {
       wmac_addr_lookup_engine_find(sta_addr, captbl->addr_index, 0, flag | IN_DS_TBL | BY_ADDR_IDX);
   }
   else
   {
       wmac_addr_lookup_engine_find(sta_addr, captbl->addr_index, 0, flag | BY_ADDR_IDX);
   }

   dragonite_mac_unlock();

   memcpy(&pktdata[4], sta_addr, ETH_ALEN);
   memcpy(&pktdata[10], data->dragonite_bss_info[descr->bssid].MAC_ADDR, ETH_ALEN);

   ssn = (descr->sn + 1) % 0xffff;
   ssn = (ssn << 4);
   pktdata[16] = 0x04;     // compressed bitmap
   pktdata[17] = (descr->tid << 4);
   pktdata[18] = ssn & 0xff;
   pktdata[19] = ((ssn & 0xff00) >> 8);

   descr->wh = 1;
   descr->pkt_length = 20;
   descr->hdr_len = 14;
   descr->sec = 0;  
   descr->cipher_mode = CIPHER_TYPE_NONE; 

   descr->tx_rate[0] = tx_rate_encoding(FMT_11B, CH_OFFSET_20, 4, 0, CCK_1M);
   descr->tx_rate[1] = 0;
   descr->tx_rate[2] = 0;
   descr->tx_rate[3] = 0;

   acqe->a.mpdu = 1;
   acqe->a.try_cnt = 0;
   acqe->a.own = 1;

   return 0;
}

bool dragonite_station_power_saving(struct mac80211_dragonite_data *data, int sta_index)
{
   MAC_INFO* info = data->mac_info;
   if(sta_index >=32 || sta_index<0)
   {
      panic("DRAGONITE: ps station index wrong");
   }
   if(info->sta_ps_on & (0x1 << sta_index))
   {
      return true;
   }
   else
      return false;
}

void skb_list_add(volatile struct list_head *node, struct skb_list_head *queue)
{
   if((queue->qhead) != NULL)
   {
      list_add_tail((struct list_head *)node, (struct list_head *)(queue->qhead));
   }
   else
   {
      INIT_LIST_HEAD((struct list_head *)node);
      queue->qhead = node;
   }
   (queue->qcnt)++;
   return;
}

void skb_list_del(volatile struct list_head *node, struct skb_list_head *queue)
{
   if(node == (queue->qhead))
   {
      if(list_empty_careful((const struct list_head *)node))
      {
         queue->qhead = NULL;
      }
      else
      {
         queue->qhead = node->next;
      }
   }
   list_del((struct list_head *) node);
   (queue->qcnt)--;

   if(queue->qcnt < 0)
   {
      panic("DRAGONITE: skb_list_del wrong !!, queue count %d", queue->qcnt);
   }

   return;
}

static struct queue_param queue_params[QUEUE_INDEX_MAX] = {
   {.queue_name = "tx_sw_mixed_queue", .queue_located = QUEUE_FROM_DATA, .use_tid = false, .use_acqid = true },
   {.queue_name = "tx_hw_mixed_queue", .queue_located = QUEUE_FROM_DATA, .use_tid = false, .use_acqid = true },
   {.queue_name = "tx_sw_ps_mixed_queue", .queue_located = QUEUE_FROM_DATA, .use_tid = false, .use_acqid = true },
   {.queue_name = "tx_sw_gc_queue", .queue_located = QUEUE_FROM_VP, .use_tid = false, .use_acqid = true },
   {.queue_name = "tx_sw_ps_gc_queue", .queue_located = QUEUE_FROM_VP, .use_tid = false, .use_acqid = false },
   {.queue_name = "tx_hw_ps_gc_queue", .queue_located = QUEUE_FROM_VP, .use_tid = false, .use_acqid = false },
   {.queue_name = "tx_sw_queue", .queue_located = QUEUE_FROM_VP, .use_tid = true, .use_acqid = false },
   {.queue_name = "tx_sw_mgmt_queue", .queue_located = QUEUE_FROM_VP, .use_tid = true, .use_acqid = false },
   {.queue_name = "tx_hw_mgmt_queue", .queue_located = QUEUE_FROM_VP, .use_tid = true, .use_acqid = false },
   {.queue_name = "tx_sw_queue", .queue_located = QUEUE_FROM_SP, .use_tid = true, .use_acqid = false },
   {.queue_name = "tx_sw_single_queue", .queue_located = QUEUE_FROM_SP, .use_tid = true, .use_acqid = false },
   {.queue_name = "tx_hw_single_queue", .queue_located = QUEUE_FROM_SP, .use_tid = true, .use_acqid = false },
   {.queue_name = "tx_sw_ps_single_queue", .queue_located = QUEUE_FROM_SP, .use_tid = true, .use_acqid = false }
};

struct skb_list_head *idx_to_queue(struct mac80211_dragonite_data *data, struct dragonite_sta_priv *sp,
                                   struct dragonite_vif_priv *vp, u8 que_idx, u8 tid, u8 acqid)
{
   if(que_idx >= QUEUE_INDEX_MAX)
   {
      panic("queue_idx(%d) out of ragnge\n", que_idx);
   }
   if(queue_params[que_idx].use_tid && (tid >= (TID_MAX + 1)))
   {
      panic("tid(%d) out of range\n", tid);
   }
   if(queue_params[que_idx].use_acqid && (acqid >= (ACQ_NUM - 1)))
   {
      panic("acqid(%d) out of range\n", acqid);
   }

   if(queue_params[que_idx].queue_located == QUEUE_FROM_DATA)
   {
      if(data == NULL)
      {
         panic("data (null) with que_idx %d\n", que_idx);
      }
      switch(que_idx)
      {
         case SW_MIXED:
            return &(data->tx_sw_mixed_queue[acqid]);
         case HW_MIXED:
            return &(data->tx_hw_mixed_queue[acqid]);
         case SW_PS_MIXED:
            return &(data->tx_sw_ps_mixed_queue[acqid]);
         default:
            return NULL;
      }
   }
   else if(queue_params[que_idx].queue_located == QUEUE_FROM_VP)
   {
      if(vp == NULL)
      {
         panic("vp (null) with que_idx %d\n", que_idx);
      }
      switch(que_idx)
      {
         case VP_SW_GC:
            return &(vp->tx_sw_gc_queue[acqid]);
         case VP_SW_PS_GC:
            return &(vp->tx_sw_ps_gc_queue);
         case VP_HW_PS_GC:
            return &(vp->tx_hw_ps_gc_queue);
         case VP_SW:
            return &(vp->tx_sw_queue[tid]);
         case VP_SW_MGMT:
            return &(vp->tx_sw_mgmt_queue[tid]);
         case VP_HW_MGMT:
            return &(vp->tx_hw_mgmt_queue[tid]);
         default:
            return NULL;
      }
   }
   else if(queue_params[que_idx].queue_located == QUEUE_FROM_SP)
   {
      if(sp == NULL)
      {
         panic("sp (null) with que_idx %d\n", que_idx);
      }
      switch(que_idx)
      {
         case SP_SW:
            return &(sp->tx_sw_queue[tid]);
         case SP_SW_SINGLE:
            return &(sp->tx_sw_single_queue[tid]);
         case SP_HW_SINGLE:
            return &(sp->tx_hw_single_queue[tid]);
         case SP_SW_PS_SINGLE:
            return &(sp->tx_sw_ps_single_queue[tid]);
         default:
            return NULL;
      }
   }
   else
   {
      panic("undefined queue_located %d\n", queue_params[que_idx].queue_located);
   }
   return NULL;
}

void check_out_skb(struct list_head* node,
        struct mac80211_dragonite_data *data, struct dragonite_sta_priv *sp, struct dragonite_vif_priv *vp,
        u8 que_idx, u8 tid, u8 acqid, volatile int *total_cnt, bool cal_tx_per_ac_cnt,
        const char func_name[], int line_num)
{
   struct skb_list_head *queue;

   queue = idx_to_queue(data, sp, vp, que_idx, tid, acqid);
   if(queue == NULL)
   {
      panic("NULL queue\n");
   }

   if(queue->qhead)
   {
      skb_list_del(node, queue);
      if(total_cnt)
      {
         (*total_cnt)--;
      }

      if(cal_tx_per_ac_cnt)
      {
         if(data == NULL)
         {
            panic("data is needed\n");
         }
         if(queue_params[que_idx].use_tid)
         {
            decr_ac_count_safe(data, tid_to_qid(tid));
         }
         else if(queue_params[que_idx].use_acqid)
         {
            decr_ac_count_safe(data, acqid);
         }
         else /* MUTICAST MUST USE LEGACY */
         {
            decr_ac_count_safe(data, AC_LEGACY_QID);
         }
      }
   }
   else
   {
      panic("DRAGONITE: %s wrong called by %s at line %d\n", queue_params[que_idx].queue_name, func_name, line_num);
   }
}

#define SET_CHK_PARAMS(node, idx, cnt_ptr, cal_ac) do { \
   chk_skb     = node;      \
   chk_que_idx = idx;       \
   total_cnt   = cnt_ptr;   \
   cal_ac_cnt  = cal_ac;    \
   dbg_line    = __LINE__;  \
} while(0)
int dragonite_tx_schedule_done(struct mac80211_dragonite_data *data, acq *q)
{
   MAC_INFO* info = data->mac_info;
   struct dragonite_tx_descr *dragonite_descr;
   struct dragonite_vif_priv *vp;
   struct dragonite_sta_priv *sp;
   volatile acqe* sw_acqe;
   volatile acqe* acqe;
   tx_descriptor *descr;
   buf_header *bhdr;
   int tid = -1, qid = -1, sta_index = -1;

   struct list_head *chk_skb;
   u8 chk_que_idx = 0x7f;
   volatile int *total_cnt = NULL;
   bool cal_ac_cnt = false;
   int dbg_line = __LINE__;
   

   acqe = ACQE(q, q->rptr);

   sw_acqe = SWACQE(q, q->rptr);

   if(acqe->m.bhr_h != sw_acqe->m.bhr_h)
   {
      DRG_WARN(DRAGONITE_DEBUG_DRIVER_WARN_FLAG, "acqe->m.bhr_h %d != sw_acqe->m.bhr_h %d, recover it when dragonite_tx_schedule_done\n", acqe->m.bhr_h, sw_acqe->m.bhr_h);
      acqe->m.bhr_h = sw_acqe->m.bhr_h;
   }

   dragonite_txq_lock();

   if(acqe->m.bhr_h)
   {
      bhdr = idx_to_bhdr(info, acqe->m.bhr_h);
      descr = (tx_descriptor *) UNCACHED_VIRTUAL_ADDR(bhdr->dptr);
      dragonite_descr = (struct dragonite_tx_descr *) CACHED_ADDR((u32)descr - DRAGONITE_TXDESCR_SIZE);
      vp = (struct dragonite_vif_priv *) dragonite_descr->vp_addr;
      sp = (struct dragonite_sta_priv *) dragonite_descr->sp_addr;
   }
   else
   {
      panic("dragonite_tx_schedule_done, but acqe->m.bhr_h == 0");
   }

   /*dequeue hw queue*/
   if(q->qinfo & ACQ_SINGLE)//single queue
   {
      if(sp)
      {
         if(!dragonite_sta_valid(sp))
         {
            panic("dragonite sp invalid at %s %d , should not happed\n", __func__, __LINE__);
         }
      }
      else
      {
         panic("dragonite sp == NULL at %s %d , should not happed\n", __func__, __LINE__);
      }
      sta_index = (q->qinfo & ACQ_AIDX) >> 16;
      tid = (q->qinfo & ACQ_TID) >> 12;

      if(info->sta_ps_on & (0x1 << sta_index)) /* this station ps on */
      {
         DRG_WARN(DRAGONITE_DEBUG_DRIVER_WARN_FLAG, "ssn = %d\n", descr->sn);
         DRG_WARN(DRAGONITE_DEBUG_DRIVER_WARN_FLAG, "tx_sw_queue_count[%d][%d] %d\n", sta_index, tid, sp->tx_sw_queue[tid].qcnt);
         DRG_WARN(DRAGONITE_DEBUG_DRIVER_WARN_FLAG, "tx_sw_single_queue_count[%d][%d] %d\n", sta_index, tid, sp->tx_sw_single_queue[tid].qcnt);
         DRG_WARN(DRAGONITE_DEBUG_DRIVER_WARN_FLAG, "tx_hw_single_queue_count[%d][%d] %d\n", sta_index, tid, sp->tx_hw_single_queue[tid].qcnt);
         DRG_WARN(DRAGONITE_DEBUG_DRIVER_WARN_FLAG, "tx_sw_ps_single_queue_count[%d][%d] %d\n", sta_index, tid, sp->tx_sw_ps_single_queue[tid].qcnt);
         DRG_WARN(DRAGONITE_DEBUG_DRIVER_WARN_FLAG, "should not happen %s %d\n", __func__, __LINE__);
         panic("dragonite: should not happed !!");
      }
      else
      {
         SET_CHK_PARAMS(&dragonite_descr->group_list, SP_HW_SINGLE, NULL, COUNT_TX_PER_AC);
      }
   }
   else//mixed queue + mgmt queue + other's
   {
      qid = q->qid;

      if(qid == BCQ_QID)//BCQ
      {
         if(vp == NULL)
         {
            dragonite_txq_unlock();
            return 0;
         }
         SET_CHK_PARAMS(&dragonite_descr->group_list, VP_HW_PS_GC, NULL, COUNT_TX_PER_AC);
      }
      else
      {
         if(dragonite_descr->skbinfo_flags & SKB_INFO_FLAGS_MGMT_FRAME)
         {
            if(vp == NULL)
            {
               dragonite_txq_unlock();
               return 0;
            }
            sta_index = DRAGONITE_TX_MGMT_STATION_INDEX;
            tid = dragonite_descr->tid;
            SET_CHK_PARAMS(&dragonite_descr->group_list, VP_HW_MGMT, NULL, COUNT_TX_PER_AC);
         }
         else
         {
            if(acqe->m.mb == 1) /* multicast frame, detach from hw mixed queue*/
            {
               if(vp == NULL)
               {
                  dragonite_txq_unlock();
                  return 0;
               }
               SET_CHK_PARAMS(&dragonite_descr->group_list, HW_MIXED, NULL, COUNT_TX_PER_AC);
            }
            else /* unicast frame */
            {
               if(sp == NULL)
               {
                  dragonite_txq_unlock();
                  return 0;
               }

               if((info->sta_ps_on & (0x1 << acqe->m.aidx)) && 
                  (!(dragonite_descr->skbinfo_flags & (SKB_INFO_FLAGS_PS_POLL_RESPONSE | SKB_INFO_FLAGS_BAR_FRAME))))
               {
                  if(dragonite_descr->acqe_addr == 0)
                  {
                     SET_CHK_PARAMS(&dragonite_descr->group_list, SW_PS_MIXED, NULL, !COUNT_TX_PER_AC);
                     if(!(dragonite_descr->skbinfo_flags & SKB_INFO_FLAGS_SPECIAL_DATA_FRAME))
                     {
                        sp->tx_ps_buffer_skb_count--;
                     }
                  }
                  else
                  {
                     DRG_WARN(DRAGONITE_DEBUG_DRIVER_WARN_FLAG, "should not happen %s %d\n", __func__, __LINE__);
                     panic("dragonite: should not happed !!");
                  }
               }
               else
               {
                  if(dragonite_descr->acqe_addr == 0)
                  {
                     DRG_WARN(DRAGONITE_DEBUG_DRIVER_WARN_FLAG, "should not happen %s %d\n", __func__, __LINE__);
                     panic("dragonite: should not happed !!");
                  }
                  else
                  {
                     SET_CHK_PARAMS(&dragonite_descr->group_list, HW_MIXED, NULL, COUNT_TX_PER_AC);
                  }
               }
            }
         } 
      }
   }
   check_out_skb(chk_skb, data, sp, vp, chk_que_idx, (u8)tid, (u8)qid, total_cnt, cal_ac_cnt, __func__, dbg_line);
   /*dequeue sw queue include connected station and mgmt station*/
   if((dragonite_descr->sp_addr && !(dragonite_descr->skbinfo_flags & SKB_INFO_FLAGS_MULTICAST_FRAME)) || 
      (dragonite_descr->skbinfo_flags & SKB_INFO_FLAGS_MGMT_FRAME))
   {
      tid = dragonite_descr->tid;
      if(dragonite_descr->sp_addr)
      {
         sta_index = sp->stacap_index;
         SET_CHK_PARAMS(&dragonite_descr->private_list, SP_SW, &(data->tx_sw_queue_total_count), !COUNT_TX_PER_AC);
      }

      else
      {
         sta_index = DRAGONITE_TX_MGMT_STATION_INDEX;
         SET_CHK_PARAMS(&dragonite_descr->private_list, VP_SW, &(data->tx_sw_queue_total_count), !COUNT_TX_PER_AC);
      }
   }
   else//muticast queue or mixed queue
   {
      qid = tid_to_qid(dragonite_descr->tid); /* if qid == bcq id, need to transform from tid to qid*/
      
      SET_CHK_PARAMS(&dragonite_descr->private_list, VP_SW_GC, &(data->tx_sw_bmc_queue_total_count), !COUNT_TX_PER_AC);
   }
   check_out_skb(chk_skb, data, sp, vp, chk_que_idx, (u8)tid, (u8)qid, total_cnt, cal_ac_cnt, __func__, dbg_line);
   dragonite_txq_unlock();

   return 0;
}

static void dragonite_report_tx_status(struct mac80211_dragonite_data *data, struct sk_buff *skb, acq *q)
{
   MAC_INFO* info = data->mac_info;
   volatile acqe* acqe;
   struct ieee80211_tx_info *txi;
   int retry_counter = 0, i, j;
   tx_descriptor *descr;
   buf_header *bhdr;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 0, 0))
   u8 *status_attempts;
   u8 *status_fail;
#endif

   acqe = ACQE(q, q->rptr);
   txi = IEEE80211_SKB_CB(skb);
   bhdr = idx_to_bhdr(info, acqe->m.bhr_h);
   descr = (tx_descriptor *) UNCACHED_VIRTUAL_ADDR(bhdr->dptr);

   /* init status without rate */
   txi->status.ack_signal = 100;
   txi->status.ampdu_ack_len = 0;
   txi->status.ampdu_len = 0;

   if(acqe->m.bmap)
   {
      if(acqe->m.try_cnt == 0)
      {
         txi->status.rates[0].count = 1;
         txi->status.rates[1].idx = -1;
         txi->status.rates[1].count = 0;
      }
      else
      {
         retry_counter = acqe->m.try_cnt;
         retry_counter++;
         for (i = 0; i < IEEE80211_TX_MAX_RATES; i++)
         {
            if (retry_counter >= txi->status.rates[i].count) {
               retry_counter -= txi->status.rates[i].count;
            } else if (retry_counter > 0) {
               txi->status.rates[i].count = retry_counter;
               retry_counter = 0;
            } else {
               txi->status.rates[i].idx = -1;
               txi->status.rates[i].count = 0;
            }
         }
      }
      if (!(txi->flags & IEEE80211_TX_CTL_NO_ACK))
      {
         txi->flags |= IEEE80211_TX_STAT_ACK;
      }
      if (txi->flags & IEEE80211_TX_CTL_AMPDU)
      {
         /* only indicate rate adapter MPDU packet and AMPDU first packet */
         if(acqe->a.mpdu)
         {
            txi->flags &= ~(IEEE80211_TX_CTL_AMPDU);
         }
         else
         {
            /* only ampdu header packet needs to report tx status */
            if(descr->tx_status[0] != 0)
            {
               txi->flags |= IEEE80211_TX_STAT_AMPDU;

               txi->status.ampdu_ack_len = q->ampdu_len;
               txi->status.ampdu_len = q->ampdu_len;
               q->ampdu_len = 0;

               q->ampdu_len++;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 0, 0))
               status_attempts = (u8 *) &txi->status.status_driver_data[0];
               status_fail = (u8 *) &txi->status.status_driver_data[1];
#endif
               for(i = 0; i < IEEE80211_TX_MAX_RATES; i++)
               {
                  if(descr->tx_status[i] != 0)
                  {
                     for(j = 0; j < IEEE80211_TX_MAX_RATES; j++)
                     {
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 0, 0))
                        status_attempts[j] = (descr->tx_status[j] & 0xff00) >> 8;
                        status_fail[j] = (descr->tx_status[j] & 0x00ff);
#else
                        txi->status.attempts[j] = (descr->tx_status[j] & 0xff00) >> 8;
                        txi->status.fail[j] = (descr->tx_status[j] & 0x00ff);
#endif
                     }
                     break;
                  }
               }
            }
            else
            {
               q->ampdu_len++;
            }
         }
      }
      drg_check_out_tx_skb(data, skb, TX_SKB_SUCCESS);
   }
   else
   {
      if (txi->flags & IEEE80211_TX_CTL_AMPDU)
      {
         if(acqe->a.mpdu)
         {
            txi->flags &= ~(IEEE80211_TX_CTL_AMPDU);
         }
      }
      drg_check_out_tx_skb(data, skb, TX_SKB_FAIL);
   }
}

static void dragonite_acq_switch_handler(struct mac80211_dragonite_data *data, int acqid, int cmdid)
{
   MAC_INFO* info = data->mac_info;
   acq *q, *nextq;
   volatile acq *qtmp;

   q = (acq*) info->acq_hw_requested[acqid][cmdid];
   info->acq_hw_requested[acqid][cmdid] = 0;

   if(q == NULL)
   {
      DRG_WARN(DRAGONITE_DEBUG_DRIVER_WARN_FLAG, "q == NULL\n");
      return;
   }
   q->flags &= ~(ACQ_FLAG_CMD0_REQ|ACQ_FLAG_CMD1_REQ);
   if(ACQ_EMPTY(q))
   {
      q->flags &= ~ACQ_FLAG_ACTIVE;
   }

   if(q->flags & ACQ_FLAG_LOCK)
   {
      printf("Locked ACQ %p done\n", q);
   }

   q->qinfo = MACREG_READ32(ACQ_INFO(acqid,cmdid));

   nextq = NULL;
   qtmp = q;
   while(qtmp->next)
   {
      if(ACQ_FLAG_ACTIVE == (qtmp->next->flags & (ACQ_FLAG_CMD0_REQ|ACQ_FLAG_CMD1_REQ|ACQ_FLAG_ACTIVE)))
      {
         nextq = (acq *) qtmp->next;
         break;
      }

      qtmp = qtmp->next;
   }
   if(nextq==NULL)
   {
      qtmp = (volatile acq *) DEF_ACQ(acqid);

      while(qtmp!=q && qtmp!=NULL)
      {
         if(ACQ_FLAG_ACTIVE == (qtmp->flags & (ACQ_FLAG_CMD0_REQ|ACQ_FLAG_CMD1_REQ|ACQ_FLAG_ACTIVE)))
         {
            nextq = (acq *) qtmp;
            break;
         }
         qtmp = qtmp->next;
      }
   }
   if(nextq==NULL)
   {
      if(q->flags & ACQ_FLAG_ACTIVE)
         nextq = q;
   }

   if(nextq)
   {
#if defined (DRAGONITE_ACQ_BAN)
      if(nextq->flags & ACQ_FLAG_BAN)
      {
         acq_dump(nextq);
         panic("DRAGONITE: acq switch wrong !!");
      }
#endif
      if(cmdid==0)
         nextq->flags |= ACQ_FLAG_CMD0_REQ;
      else
         nextq->flags |= ACQ_FLAG_CMD1_REQ;

      info->acq_hw_requested[acqid][cmdid] = (u32) nextq;
      MACREG_WRITE32(ACQ_INFO(acqid, cmdid), nextq->qinfo);
      MACREG_WRITE32(ACQ_INFO2(acqid,cmdid), nextq->esn);
      MACREG_WRITE32(ACQ_BADDR(acqid, cmdid), (ACQ_REQ | (nextq->queue_size_code << 28) | (PHYSICAL_ADDR(&nextq->acqe[0])>>2)));
   }
   else
   {
      MACREG_WRITE32(ACQ_INFO(acqid, cmdid), 0);
      MACREG_WRITE32(ACQ_INFO2(acqid,cmdid), 0);
   }
}

/* TX RETRY FAIL INTR : ssn may stay at wrong places, so follow the following principles can be safe

1. bmap could be trust
2. bhr_h could not be trust (if tx ampdu all use the same try count, could be trust)
3. when ACQ_REQ pull up, make sure acq entry is correct

*/
u32 acqe_rdata;
static void dragonite_acq_retry_fail_handler(struct mac80211_dragonite_data *data, int acqid, int cmdid)
{
   MAC_INFO* info = data->mac_info;
   acq *q;
   volatile acqe* sw_acqe;
   volatile acqe* acqe;
   struct sk_buff *skb;
   u32 prev_rptr, acq_info, acq_baddr;
   tx_descriptor *descr;
   buf_header *bhdr;
   struct ieee80211_tx_info *txi;
   int idx;

   acq_baddr = MACREG_READ32(ACQ_BADDR(acqid,cmdid));
   if(acq_baddr & ACQ_REQ)
   {
      panic("acq baddr is still request %d", acq_baddr);
   }

   q = (acq *) (info->acq_hw_requested[acqid][cmdid]);

   prev_rptr = q->rptr;

   /* recycle and handle acq correctly */
   while(!ACQ_EMPTY(q))
   {
      acqe = ACQE(q, q->rptr);
      sw_acqe = SWACQE(q, q->rptr);
   
      if(acqe->m.bhr_h != sw_acqe->m.bhr_h)
      {
         DRG_ERROR(DRAGONITE_DEBUG_DRIVER_WARN_FLAG, 
                   "acqe->m.bhr_h %d != sw_acqe->m.bhr_h %d, recover it when retry failed\n", acqe->m.bhr_h, sw_acqe->m.bhr_h);
         acqe->m.bhr_h = sw_acqe->m.bhr_h;
      }
      if(acqe->a.bmap == 1)
      {
         dragonite_tx_schedule_done(data, q);

         skb = buffer_header_detach_skb(info, acqe->m.bhr_h);

         dragonite_report_tx_status(data, skb, q);
   
         bhdr_insert_tail(info, acqe->m.bhr_h, -1);
   
         memset((void *)acqe, 0, ACQE_SIZE);
         memset((void *)sw_acqe, 0, ACQE_SIZE);
         q->rptr++;
      }
      else
      {
         if(acqe->a.mpdu)
         {
            if(0 > replace_tx_packet_with_bar(data, acqe))
            {
               DRG_NOTICE(DRAGONITE_DEBUG_TX_FLAG, " I bmap:%d ps:%d try_cnt:%d own:%d\n", acqe->m.bmap, acqe->m.ps, acqe->m.try_cnt, acqe->m.own);
               dragonite_tx_schedule_done(data, q);
   
               skb = buffer_header_detach_skb(info, acqe->m.bhr_h);

               dragonite_report_tx_status(data, skb, q);
   
               bhdr_insert_tail(info, acqe->m.bhr_h, -1);
   
               memset((void *)acqe, 0, ACQE_SIZE);
               memset((void *)sw_acqe, 0, ACQE_SIZE);
               q->rptr++;
            }
            else
            {
               DRG_NOTICE(DRAGONITE_DEBUG_TX_FLAG, "Re-assign own bit (acqe %d) and redo with TX BAR\n", q->rptr);
               break;
            }
         }
         else
         {
            acqe->a.mpdu = 1;
            acqe->a.try_cnt = 0;
            acqe->a.own = 1;

            DRG_NOTICE(DRAGONITE_DEBUG_TX_FLAG, "ampdu retry fail, after that bhr_h %d, pktlen %d, noa %d, try_cnt %d, mpdu %d, ps %d, bmap %d, own %d\n",
                   acqe->a.bhr_h, acqe->a.pktlen, acqe->a.noa, acqe->a.try_cnt, acqe->a.mpdu, acqe->a.ps, acqe->a.bmap, acqe->a.own);

            bhdr = idx_to_bhdr(info, acqe->m.bhr_h);
            skb = buffer_header_peek_skb(info, acqe->m.bhr_h);
            descr = (tx_descriptor *) UNCACHED_VIRTUAL_ADDR(bhdr->dptr);
            txi = IEEE80211_SKB_CB(skb);

            DRG_NOTICE(DRAGONITE_DEBUG_TX_RETRY_RATE_FLAG, "origin rate %04x %04x %04x %04x\n",
                   descr->tx_rate[0], descr->tx_rate[1], descr->tx_rate[2], descr->tx_rate[3]);

            tx_rate_retry_adjust(txi);

            /* init descr->rate before fill */
            for(idx = 0; idx < 4; idx++)
            {
               descr->tx_rate[idx] = 0;
            }
            tx_rate_fill(txi, descr);

            DRG_NOTICE(DRAGONITE_DEBUG_TX_RETRY_RATE_FLAG, "retry  rate %04x %04x %04x %04x\n",
                   descr->tx_rate[0], descr->tx_rate[1], descr->tx_rate[2], descr->tx_rate[3]);

            /* read back to prevent REQUEST ACQ faster than MMU */
            acqe_rdata = *((volatile u32 *) acqe);
            break;
         }
      }
   }

   /* if acq info not sync, update it */
   if(prev_rptr != q->rptr)
   {
      acq_info = MACREG_READ32(ACQ_INFO(acqid,cmdid));
      acq_info = ((acq_info & ~0x0fff) | (q->rptr & 0xfff));
      MACREG_WRITE32(ACQ_INFO(acqid,cmdid), acq_info);
   }
}

void dragonite_acq_intr_handler(struct mac80211_dragonite_data *data)
{
   MAC_INFO* info = data->mac_info;
   int acqid, cmdid;
   acq *q;
   volatile acqe* sw_acqe;
   volatile acqe* acqe;
   u32 tx_acq_status, tx_acq_status2;
   u32 ssn, esn;
   buf_header *bhdr;
   struct sk_buff *skb;
   struct ieee80211_tx_info *txi;
   tx_descriptor *descr;
   unsigned long dragonite_acq_lock_irqflags;
   struct dragonite_sta_priv *sp;
   struct dragonite_tx_descr *dragonite_descr;
   struct ieee80211_hdr *wifi_hdr;
   int sta_idx;

   tx_acq_status = MACREG_READ32(ACQ_INTR);
   MACREG_WRITE32(ACQ_INTR, tx_acq_status);
   tx_acq_status2 = MACREG_READ32(ACQ_INTR2);
   MACREG_WRITE32(ACQ_INTR2, tx_acq_status2);

   for(acqid=0;acqid<ACQ_NUM;acqid++)
   {
      for(cmdid=0;cmdid<CMD_NUM;cmdid++)
      {
         if(tx_acq_status & ACQ_INTR_BIT(acqid,cmdid))
         {

#if defined (DRAGONITE_TX_HANG_CHECKER)
            drg_tx_done_intr_times++;
#endif
            q = (acq *) (info->acq_hw_requested[acqid][cmdid]);

            if(q == NULL)
            {
               DRG_WARN(DRAGONITE_DEBUG_DRIVER_WARN_FLAG, "q == NULL\n");
               continue;
            }
            ssn = MACREG_READ32(ACQ_INFO(acqid,cmdid)) & 0x0fff;

            while(q->rptr != ssn)
            {
               acqe = ACQE(q, q->rptr);
               sw_acqe = SWACQE(q, q->rptr);

               if(acqe->m.bhr_h != sw_acqe->m.bhr_h)
               {
                  DRG_ERROR(DRAGONITE_DEBUG_DRIVER_WARN_FLAG, 
                            "acqe->m.bhr_h %d != sw_acqe->m.bhr_h %d, recover it when tx done\n", acqe->m.bhr_h, sw_acqe->m.bhr_h);
                  acqe->m.bhr_h = sw_acqe->m.bhr_h;
               }
               if(acqe->m.own == 1 &&(q->qid != BCQ_QID))
               {
                  break;
               }

               if(acqe->m.bhr_h == EMPTY_PKT_BUFH)
               {
                  /* do nothing */
               }
               else if(acqe->m.bhr_h)
               {
                  drg_tx_stats.tx_done_count++;

                  skb = buffer_header_detach_skb(info, acqe->m.bhr_h);

                  txi = IEEE80211_SKB_CB(skb);
                  bhdr = idx_to_bhdr(info, acqe->m.bhr_h);
                  descr = (tx_descriptor *) UNCACHED_VIRTUAL_ADDR(bhdr->dptr);
                  dragonite_descr = (struct dragonite_tx_descr *) CACHED_ADDR((u32)descr - DRAGONITE_TXDESCR_SIZE);

                  if(info->sta_ps_recover)
                  {
                     if((dragonite_descr->skbinfo_flags & SKB_INFO_FLAGS_NULL_FUNCTION) && acqe->m.bmap
                        && (!acqe->m.ps))
                     {
                        ASSERT((data->dragonite_bss_info[descr->bssid].dut_role == ROLE_AP),
                               "dragonite_acq_intr_handler: handle null-data in STA BSS\n");
                        wifi_hdr = (struct ieee80211_hdr *)(skb->data);

                        dragonite_mac_lock();

                        sta_idx = wmac_addr_lookup_engine_find(wifi_hdr->addr1,0,0,0);

                        dragonite_mac_unlock();


                        if((sta_idx >= 0)&&(sta_idx < MAX_STA_NUM))
                        {
                           if(info->sta_ps_recover & (0x1UL << sta_idx))
                           {
                              dragonite_ps_lock();

                              info->sta_ps_recover &= ~(0x1UL << sta_idx);
                              info->sta_ps_off_indicate |= (0x1UL << sta_idx);

                              dragonite_ps_unlock();
                           }
                        }
                     }
                  }
                  dragonite_ps_lock();

                  if(dragonite_descr->skbinfo_flags & SKB_INFO_FLAGS_MGMT_FRAME)
                  {
                  }
                  else if(q->qid == BCQ_QID)
                  {
                  }
                  else if(acqe->m.mb == 0) /* unicast data frame */
                  {
                     if(q->qinfo & ACQ_SINGLE)
                     {
                        if((info->sta_ps_on & (0x1 << ((q->qinfo & ACQ_AIDX) >> 16))) || acqe->m.ps)
                        {
                            dragonite_descr->skbinfo_flags &= ~(SKB_INFO_FLAGS_IN_HW);
                            dragonite_ps_unlock();
                            cleanup_bhdr_on_descr(info, acqe->a.bhr_h);
                            goto step2;
                        }
                     }
                     else
                     {
                        if(((info->sta_ps_on & (0x1 << acqe->m.aidx)) || acqe->m.ps )
                           && (!(dragonite_descr->skbinfo_flags & (SKB_INFO_FLAGS_PS_POLL_RESPONSE | SKB_INFO_FLAGS_BAR_FRAME))))
                        {
                           if(info->sta_ps_on & (0x1 << acqe->m.aidx))
                           {
                              sp = idx_to_sp(data, (u8)acqe->m.aidx);
                              if(sp != NULL)
                              {   
                                 sp->tx_hw_pending_skb_count--;
                              }
                           }

                           if((acqe->m.bmap == 0) && (acqe->m.try_cnt != 0)) /* fail to send, re-enqueue */
                           {
                              dragonite_descr->skbinfo_flags &= ~(SKB_INFO_FLAGS_IN_HW);
                              dragonite_ps_unlock();
                              if(dragonite_descr->sp_addr == 0)
                              {
                                 goto step1;
                              }
                              cleanup_bhdr_on_descr(info, acqe->a.bhr_h);
                              goto step2;
                           }
                           else
                           {
                              /* success to send, done */
                           }
                        }                        
                     }
                  }
                  dragonite_tx_schedule_done(data, q);

                  dragonite_ps_unlock();
step1:
                  dragonite_report_tx_status(data, skb, q);
step2:
                  bhdr_insert_tail(info, acqe->m.bhr_h, -1);
               }
               memset((void *)acqe, 0, ACQE_SIZE);
               memset((void *)sw_acqe, 0, ACQE_SIZE);
               q->rptr++;
            }

            spin_lock_irqsave(&dragonite_acq_lock, dragonite_acq_lock_irqflags);
            if(ACQ_EMPTY(q))
            {
               q->flags &= ~ACQ_FLAG_ACTIVE;
            }
            spin_unlock_irqrestore(&dragonite_acq_lock, dragonite_acq_lock_irqflags);

            if(q->qinfo & ACQ_SINGLE)
            {
               dragonite_tx_schedule(data, NOT_USE_VIF_IDX, (q->qinfo & ACQ_AIDX) >> 16, (q->qinfo & ACQ_TID) >> 12);
            }
            else
            {
               if(q->qid == BCQ_QID)
               {
                  /* do not schedule*/
               }
               else
               {
                  dragonite_tx_schedule(data, NOT_USE_VIF_IDX, -1, q->qid);
               }
            }
         }
      }
   }

   if((tx_acq_status & ACQ_INTR_CMD_SWITCH) || (tx_acq_status2 & ACQ_INTR2_RETRY_FAIL))
   {
      for(acqid=0;acqid<ACQ_NUM;acqid++)
      {
         for(cmdid=0;cmdid<CMD_NUM;cmdid++)
         {
            if(tx_acq_status2 & (ACQ_INTR_BIT(acqid,cmdid)))
            {
#if defined (DRAGONITE_TX_HANG_CHECKER)
               drg_tx_retry_intr_times++;
#endif
               q = (acq *) (info->acq_hw_requested[acqid][cmdid]);

               if(q == NULL)
               {
                  DRG_WARN(DRAGONITE_DEBUG_DRIVER_WARN_FLAG, "q == NULL\n");
                  continue;
               }

               acqe = ACQE(q, q->rptr);
               sw_acqe = SWACQE(q, q->rptr);

               if(q->qid != acqid)
               {
                  panic("dragonite_acq_retry_fail_handler q->qid %d != acqid %d", q->qid, acqid);
               }

               ssn = MACREG_READ32(ACQ_INFO(acqid,cmdid)) & 0x0fff;
               esn = MACREG_READ32(ACQ_INFO2(acqid,cmdid)) & 0x0fff;

               DRG_NOTICE(DRAGONITE_DEBUG_TX_FLAG, " rptr %d, bmap:%d ps:%d noa:%d try_cnt:%d bhr_h:%d ssn %d esn %d\n",
                      q->rptr, acqe->m.bmap, acqe->m.ps, acqe->m.noa, acqe->m.try_cnt, acqe->m.bhr_h, ssn, esn);

               if(q->rptr != ssn)
               {
                  panic("DRAGONITE: tx retry fail, rptr != ssn, rptr %d ssn %d\n", q->rptr, ssn);
               }

               dragonite_ps_lock();
               if((info->sta_ps_on & (0x1 << ((q->qinfo & ACQ_AIDX) >> 16))) || acqe->m.ps)
               {
                  dragonite_ps_unlock();

                  if(acqe->m.bhr_h != sw_acqe->m.bhr_h)
                  {
                     DRG_ERROR(DRAGONITE_DEBUG_DRIVER_WARN_FLAG, 
                               "acqe->m.bhr_h %d != sw_acqe->m.bhr_h %d, recover it when tx done\n", acqe->m.bhr_h, sw_acqe->m.bhr_h);
                     acqe->m.bhr_h = sw_acqe->m.bhr_h;
                  }

                  cleanup_bhdr_on_descr(info, acqe->a.bhr_h);
                  buffer_header_detach_skb(info, acqe->m.bhr_h);
                  bhdr_insert_tail(info, acqe->m.bhr_h, -1);
                  memset((void *)acqe, 0, ACQE_SIZE);
                  memset((void *)sw_acqe, 0, ACQE_SIZE);

                  spin_lock_irqsave(&dragonite_acq_lock, dragonite_acq_lock_irqflags);

                  dragonite_acq_switch_handler(data, acqid, cmdid);

                  spin_unlock_irqrestore(&dragonite_acq_lock, dragonite_acq_lock_irqflags);
               }
               else
               {
                  dragonite_acq_retry_fail_handler(data, acqid, cmdid);

                  dragonite_ps_unlock();

                  spin_lock_irqsave(&dragonite_acq_lock, dragonite_acq_lock_irqflags);
                  /* if acq not empty, pull up acq request, else call dragonite_acq_switch_handler */
                  if(ACQ_EMPTY(q))
                  {
                     dragonite_acq_switch_handler(data, acqid, cmdid);
                  }
                  else
                  {
                     MACREG_UPDATE32(ACQ_BADDR(acqid,cmdid), ACQ_REQ, ACQ_REQ);
                  }

                  spin_unlock_irqrestore(&dragonite_acq_lock, dragonite_acq_lock_irqflags);
               }
            }
            else if(tx_acq_status & (ACQ_INTR_BIT(acqid,cmdid) << 16))
            {
#if defined (DRAGONITE_TX_HANG_CHECKER)
               drg_tx_switch_intr_times++;
#endif
               spin_lock_irqsave(&dragonite_acq_lock, dragonite_acq_lock_irqflags);

               dragonite_acq_switch_handler(data, acqid, cmdid);

               spin_unlock_irqrestore(&dragonite_acq_lock, dragonite_acq_lock_irqflags);
            }
         }
      }
   }
}

u32 tx_acq_ampdu_density_encoding(int ampdu_max_length)
{
   u32 density_code = 0;

   switch(ampdu_max_length)
   {
      case 8192:
         density_code = 0;
         break;
      case 16384:
         density_code = 1;
         break;
      case 32768:
         density_code = 2;
         break;
      case 65536:
         density_code = 3;
         break;
      default:
         panic("Unknown A-MPDU density");
         break;
   }

   return density_code;
}

u32 tx_acq_size_encoding(int size)
{
   u32 size_code = 0;

   switch(size)
   {
      case 8:
         size_code = 0;
         break;
      case 16:
         size_code = 1;
         break;
      case 32:
         size_code = 2;
         break;
      case 64:
         size_code = 3;
         break;
      default:
         panic("Unsupport ACQ WIN/QUEUE SIZE");
         break;
   }

   return size_code;
}
void tx_single_acq_setup(acq *acq, int acqid, int spacing, int max_length, int win_size, int queue_size, int aidx, int tid, int ssn)
{
   u32 win_size_code;
   u32 queue_size_code;
   u32 density_code;

   memset((void *) acq, 0, sizeof(struct __tag_acq));
   acq->qid = acqid;

   win_size_code = tx_acq_size_encoding(win_size);
   queue_size_code = tx_acq_size_encoding(queue_size);
   density_code = tx_acq_ampdu_density_encoding(max_length);

   acq->queue_size = queue_size;
   acq->queue_size_code = queue_size_code;
   acq->qinfo = ACQ_SINGLE | (spacing << 28) | (density_code << 26) | (win_size_code << 24)
                  | (aidx << 16) | (tid << 12) | ssn;
   acq->ampdu_len = 0;
}
void tx_single_acq_start(acq *acq, int ssn)
{
   DRG_NOTICE(DRAGONITE_DEBUG_TX_FLAG, "change single q ssn to %d\n", ssn);
   acq->qinfo = (acq->qinfo & 0xfffff000UL) | ssn;
   acq->rptr = ssn;
   acq->wptr = ssn;
}
void tx_mixed_acq_setup(acq *acq, int acqid, int queue_size)
{
   u32 queue_size_code;

   memset((void *) acq, 0, sizeof(struct __tag_acq));
   acq->qid = acqid;

   queue_size_code = tx_acq_size_encoding(queue_size);

   acq->queue_size = queue_size;
   acq->queue_size_code = queue_size_code;
}
int tx_acq_kick(MAC_INFO *info, acq *q, u32 esn)
{
   int qid, cmdid, ret = 0;
   unsigned long irqflags;

   spin_lock_irqsave(&dragonite_acq_lock, irqflags);

#if defined (DRAGONITE_ACQ_DELAY_KICK)
   q->tx_count = 0;
#endif

#if defined (DRAGONITE_ACQ_BAN)
   if(q->flags & ACQ_FLAG_BAN)
   {
      acq_dump(q);
      spin_unlock_irqrestore(&dragonite_acq_lock, irqflags);
      panic("DRAGONITE: acq handle wrong !!");
   }
#endif

   if(q->flags & ACQ_FLAG_DROP)
   {
      ret = -1;
      goto exit;
   }

   qid = q->qid;
   q->flags |= ACQ_FLAG_ACTIVE;
   q->esn = (u16) esn;

   if(q->flags & ACQ_FLAG_CMD0_REQ)
   {
      MACREG_WRITE32(ACQ_INFO2(qid,0), esn);
      goto exit;
   }
   else if(q->flags & ACQ_FLAG_CMD1_REQ)
   {
      MACREG_WRITE32(ACQ_INFO2(qid,1), esn);
      goto exit;
   }

   cmdid = -1;
   if((info->acq_hw_requested[qid][0])==0)
   {
      q->flags |= ACQ_FLAG_CMD0_REQ;
      cmdid = 0;
   }
   else if((info->acq_hw_requested[qid][1])==0)
   {
      q->flags |= ACQ_FLAG_CMD1_REQ;
      cmdid = 1;
   }

   if(cmdid>=0)
   {
         info->acq_hw_requested[qid][cmdid] = (u32) q;

         MACREG_WRITE32(ACQ_INFO(qid, cmdid), q->qinfo);
         MACREG_WRITE32(ACQ_INFO2(qid,cmdid), esn);
         if(q->flags & ACQ_FLAG_LOCK)
            MACREG_WRITE32(ACQ_BADDR(qid, cmdid), (ACQ_REQ | ACQ_REQ_LOCK | ((q->queue_size_code) << 28) | (PHYSICAL_ADDR(&q->acqe[0])>>2)));
         else
            MACREG_WRITE32(ACQ_BADDR(qid, cmdid), (ACQ_REQ | ((q->queue_size_code) << 28) | (PHYSICAL_ADDR(&q->acqe[0])>>2)));
   }

exit:
   spin_unlock_irqrestore(&dragonite_acq_lock, irqflags);

   return ret;
}
bool tx_acq_cmd_req(MAC_INFO *info, acq *q)
{
   unsigned long irqflags;
   int ret;

   spin_lock_irqsave(&dragonite_acq_lock, irqflags);
   if(q->flags & (ACQ_FLAG_CMD0_REQ | ACQ_FLAG_CMD1_REQ))
   {
      ret = 1;
   }
   else
   {
      ret = 0;
   }

   spin_unlock_irqrestore(&dragonite_acq_lock, irqflags);

   return ret;
}
int tx_acq_detach_async(MAC_INFO *info, acq *q)
{
   acq *def_acq;
   unsigned long irqflags;

   /* default Q is always attached */
   if(q->flags & ACQ_FLAG_DEFAULT_Q)
      return -1;

   //ACQ_LOCK();
   spin_lock_irqsave(&dragonite_acq_lock, irqflags);
   if(q->flags & ACQ_FLAG_ACTIVE)
   {
      q->flags &= ~ACQ_FLAG_ACTIVE;
   }
   // queue busy
   if(q->flags & (ACQ_FLAG_CMD0_REQ | ACQ_FLAG_CMD1_REQ))
   {
      //ACQ_UNLOCK();
      spin_unlock_irqrestore(&dragonite_acq_lock, irqflags);

      def_acq = DEF_ACQ(q->qid);
      tx_acq_kick(info, def_acq, def_acq->esn);
   }
   else
   {
      //ACQ_UNLOCK();
      spin_unlock_irqrestore(&dragonite_acq_lock, irqflags);
   }

   return 0;
}
int tx_acq_detach(struct mac80211_dragonite_data *data, acq *q)
{
   int qid;
   volatile acq *cur_acq;
   acq *def_acq;
   int try_count = 0;
   unsigned long irqflags;
   MAC_INFO *info = data->mac_info;

   /* default Q is always attached */
   if(q->flags & ACQ_FLAG_DEFAULT_Q)
      return -1;

Retry:
   //ACQ_LOCK();
   spin_lock_irqsave(&dragonite_acq_lock, irqflags);

   q->flags |= ACQ_FLAG_DROP;

   if(q->flags & ACQ_FLAG_ACTIVE)
   {
      q->flags &= ~ACQ_FLAG_ACTIVE;
   }

   if(try_count > 20)
   {
      DRG_WARN(DRAGONITE_DEBUG_DRIVER_WARN_FLAG, "DRAGONITE: ACQ force detach, acq_hw_requested[%d] [CMD0]%x [CMD1]%x\n", 
             q->qid, info->acq_hw_requested[q->qid][0], info->acq_hw_requested[q->qid][1]);

      acq_dump(q);

      drg_wifi_recover = 1;
   }
   else if(q->flags & (ACQ_FLAG_CMD0_REQ | ACQ_FLAG_CMD1_REQ))
   {
      //ACQ_UNLOCK();
      spin_unlock_irqrestore(&dragonite_acq_lock, irqflags);

      def_acq = DEF_ACQ(q->qid);
      tx_acq_kick(info, def_acq, def_acq->esn);

      try_count++;

      schedule_timeout_interruptible(HZ/10);

      goto Retry;
   }

   qid = q->qid;
   cur_acq = &info->def_acq[qid];

   while(cur_acq->next)
   {
      if(cur_acq->next == q)
      {
         cur_acq->next = q->next;
         break;
      }

      cur_acq = cur_acq->next;
   }

   q->next = NULL;

   //ACQ_UNLOCK();
   spin_unlock_irqrestore(&dragonite_acq_lock, irqflags);

   return 0;
}
int tx_acq_attach(MAC_INFO *info, acq *q)
{
   int qid;
   volatile acq *cur_acq;
   unsigned long irqflags;

   /* default Q is always attached */
   if(q->flags & ACQ_FLAG_DEFAULT_Q)
      return -1;

   qid = q->qid;
   cur_acq = &info->def_acq[qid];

   //ACQ_LOCK();
   spin_lock_irqsave(&dragonite_acq_lock, irqflags);


   while(cur_acq->next)
      cur_acq = cur_acq->next;

   cur_acq->next = q;

   //ACQ_UNLOCK();
   spin_unlock_irqrestore(&dragonite_acq_lock, irqflags);

   return 0;
}

acq *tx_acq_alloc(MAC_INFO *info)
{  
   acq *q;
   unsigned long irqflags;

   //ACQ_LOCK();
   spin_lock_irqsave(&dragonite_acq_lock, irqflags);

   q = (acq *) info->acq_free_list;
   if(q)
   {
      info->acq_free_list = q->next;
   }

   //ACQ_UNLOCK();
   spin_unlock_irqrestore(&dragonite_acq_lock, irqflags);

   return q;
}
void tx_acq_free(MAC_INFO *info, acq *q)
{
   unsigned long irqflags;
   volatile acq* cur_acq;
   //ACQ_LOCK();
   spin_lock_irqsave(&dragonite_acq_lock, irqflags);

   if(q)
   {
      cur_acq = info->acq_free_list;
      while(cur_acq->next)
      {
         cur_acq = cur_acq->next;
      }
      cur_acq->next = q;
   }

   //ACQ_UNLOCK();
   spin_unlock_irqrestore(&dragonite_acq_lock, irqflags);
}
static void tx_descriptor_fill(MAC_INFO *info, struct sk_buff *skb, tx_descriptor *descr, u32 flags, 
                            struct skb_info *skbinfo, struct dragonite_vif_priv *vp, struct dragonite_sta_priv *sp)
{
   int cipher_mode = CIPHER_TYPE_NONE;
   sta_cap_tbl* captbl;
   int sta_index;

   memset((void *)descr, 0, sizeof(tx_descriptor));

   if(sp)
   {
      sta_index = sp->stacap_index;
   }
   else
   {
      sta_index = -1;
   }

   if((sta_index >= 0) && (sta_index < MAX_STA_NUM))
   {
      captbl = &info->sta_cap_tbls[sta_index];

      cipher_mode = captbl->cipher_mode;

      descr->wep_def = captbl->wep_defkey;
      descr->ds_idx = sta_index;
   }
   if(skbinfo->flags & SKB_INFO_FLAGS_MULTICAST_FRAME)
   {
      descr->bc = 1;
      descr->ack = 1;

      cipher_mode = vp->bssinfo->cipher_mode;

      /* force multicast frame(CCMP and TKIP) use software en/decrypt to work around bcq handle PN wrong */
      if((cipher_mode==CIPHER_TYPE_TKIP) || (cipher_mode==CIPHER_TYPE_CCMP))
      {
         skbinfo->flags |= SKB_INFO_FLAGS_FORCE_SECURITY_OFF;
      }
   }
#if defined(CONFIG_MAC80211_MESH) && defined(DRAGONITE_MESH_TX_GC_HW_ENCRYPT)
   if((skbinfo->flags & SKB_INFO_FLAGS_MULTICAST_FRAME) && (vp->type == NL80211_IFTYPE_MESH_POINT))
   {
      descr->bssid = vp->bssid_idx + MAC_EXT_BSSIDS;
   }
   else
   {
      descr->bssid = vp->bssid_idx;
   }
#else
   descr->bssid = vp->bssid_idx;
#endif

   if(skbinfo->flags & SKB_INFO_FLAGS_INSERT_TIMESTAMP)
   {
      descr->ts = 1;
   }
   descr->tid = skbinfo->tid;
   descr->pkt_length = skb->len;
   descr->key_idx = sta_index;

   if((cipher_mode != CIPHER_TYPE_NONE) && (skbinfo->flags & SKB_INFO_FLAGS_DATA_FRAME)) 
   {
      if(skbinfo->flags & SKB_INFO_FLAGS_FORCE_SECURITY_OFF)
      {
         descr->cipher_mode = CIPHER_TYPE_NONE;
      }
      else
      {
         descr->sec = 1;
         descr->cipher_mode = cipher_mode;
      }
   }
   descr->sn = skbinfo->ssn;
   
   descr->wh = 1;

   if(skbinfo->flags & SKB_INFO_FLAGS_QOS_FRAME)
   {
      descr->qos = 1;
   }

   if(drg_tx_protect)
   {
      descr->rts = 1;
   }

   descr->hdr_len = skbinfo->header_length;
   
   if((cipher_mode==CIPHER_TYPE_TKIP) || (cipher_mode==CIPHER_TYPE_CCMP))
   {
      if(sp)
      {
         descr->rekey_id = sp->rekey_id;

         descr->tsc[1] = 0;
         descr->tsc[0] = (u64) sp->tsc[0];
         descr->tsc[0] = (u64) (descr->tsc[0] << 32) | sp->tsc[1];

         sp->tsc[1] += 1;
         if(sp->tsc[1]==0)
         {
            sp->tsc[0] += 1;
         }
      }
      else
      {
         descr->rekey_id = vp->rekey_id;

         descr->tsc[1] = 0;
         descr->tsc[0] = (u64) vp->tsc[0];
         descr->tsc[0] = (u64) (descr->tsc[0] << 32) | vp->tsc[1];

         vp->tsc[1] += 1;
         if(vp->tsc[1]==0)
         {
            vp->tsc[0] += 1;
         }
      }
   }
}

struct sk_buff *buffer_header_detach_skb(MAC_INFO *info, int bhr_idx)
{
   struct sk_buff *skb;

#if defined(DRAGONITE_TX_BHDR_CHECKER)
   if((bhr_idx > tx_bhdr_idx_high) || (bhr_idx < tx_bhdr_idx_low))
   {
      panic("buffer_header_detach_skb: %d %d %d\n", bhr_idx, tx_bhdr_idx_low, tx_bhdr_idx_high);
   }
#endif

   skb = (struct sk_buff *) info->swpool_bh_idx_to_skbs[bhr_idx];
   info->swpool_bh_idx_to_skbs[bhr_idx] = 0;

   if(skb==NULL)
      panic("buffer_header_detach_skb: NULL skb %d %p\n", bhr_idx, skb);

   return skb;
}

struct sk_buff *buffer_header_peek_skb(MAC_INFO *info, int bhr_idx)
{
   struct sk_buff *skb;

#if defined(DRAGONITE_TX_BHDR_CHECKER)
   if((bhr_idx > tx_bhdr_idx_high) || (bhr_idx < tx_bhdr_idx_low))
   {
      panic("buffer_header_peek_skb: %d %d %d\n", bhr_idx, tx_bhdr_idx_low, tx_bhdr_idx_high);
   }
#endif

   skb = (struct sk_buff *) info->swpool_bh_idx_to_skbs[bhr_idx];
   if(skb==NULL)
      panic("buffer_header_peek_skb: NULL skb %d %p\n", bhr_idx, skb);

   return skb;
}

int skb_attach_buffer_header(MAC_INFO *info, struct sk_buff *skb, int *bhr_h)
{
   buf_header *bhdr, *bhdr_prev;
   int cur_offset;
   int alloc_failure;
   int bhdr_idx = -1;
   int mis_align;

   bhdr_prev = NULL;

   alloc_failure = 0;

   bhdr = bhdr_get_first(info);

   if(bhdr)
   {
      bhdr_idx = bhdr_to_idx(info, bhdr);
   }
   else
   {
      alloc_failure = 1;
   }
   
   if(alloc_failure)
   {
      return -1;
   }


   cur_offset = 0;

   bhdr = idx_to_bhdr(info, bhdr_idx);

   if(info->swpool_bh_idx_to_skbs[bhdr_idx]!=0)
      panic("skb_attach_buffer_header %d %x\n", bhdr_idx, info->swpool_bh_idx_to_skbs[bhdr_idx]);

   if(skb==NULL)
      panic("skb_attach_buffer_header: attach null skb\n");

   info->swpool_bh_idx_to_skbs[bhdr_idx] = (u32) skb;

   mis_align = ((unsigned long)(skb->data - TXDESCR_SIZE) % L1_CACHE_BYTES);

   bhdr->dptr = (u32) PHYSICAL_ADDR((skb->data - TXDESCR_SIZE) - mis_align);

   bhdr->offset = TXDESCR_SIZE + mis_align;

   bhdr->len = skb->len;

   *bhr_h = bhdr_idx;    

   bhdr->ep = 1;

   return 0;
}

static void tx_rate_seletion(s8 index, int *fmt_val, int *rate_index)
{
   switch(index)
   {
      case 0:
         *fmt_val = FMT_11B;
         *rate_index = CCK_1M;
         break;
      case 1:
         *fmt_val = FMT_11B;
         *rate_index = CCK_2M;
         break;
      case 2:
         *fmt_val = FMT_11B;
         *rate_index = CCK_5_5M;
         break;
      case 3:
         *fmt_val = FMT_11B;
         *rate_index = CCK_11M;
         break;
      case 4:
         *fmt_val = FMT_NONE_HT;
         *rate_index = OFDM_6M;
         break;
      case 5:
         *fmt_val = FMT_NONE_HT;
         *rate_index = OFDM_9M;
         break;
      case 6:
         *fmt_val = FMT_11B;
         *rate_index = OFDM_12M;
         break;
      case 7:
         *fmt_val = FMT_NONE_HT;
         *rate_index = OFDM_18M;
         break;
      case 8:
         *fmt_val = FMT_NONE_HT;
         *rate_index = OFDM_24M;
         break;
      case 9:
         *fmt_val = FMT_NONE_HT;
         *rate_index = OFDM_36M;
         break;
      case 10:
         *fmt_val = FMT_NONE_HT;
         *rate_index = OFDM_48M;
         break;
      case 11:
         *fmt_val = FMT_NONE_HT;
         *rate_index = OFDM_54M;
         break;
      default:
         *fmt_val = FMT_11B;
         *rate_index = CCK_1M;
         break;
   }
}
#define PANTHER_TX_RATE_TRY_COUNT 7
static void tx_rate_adjust(struct ieee80211_tx_info *txi, struct skb_info *skbinfo)
{
   int i, try_count = PANTHER_TX_RATE_TRY_COUNT;
   bool force_mpdu = false, last_rate = false;

   if(skbinfo->flags & SKB_INFO_FLAGS_PAE_FRAME)
   {
      skbinfo->flags |= SKB_INFO_FLAGS_FORCE_MPDU_ON_BA_SESSION;

      txi->control.rates[0].flags &= ~(IEEE80211_TX_RC_MCS | IEEE80211_TX_RC_GREEN_FIELD | IEEE80211_TX_RC_SHORT_GI);
      txi->control.rates[0].idx = 0; //use CCK 1M
      txi->control.rates[0].count = try_count;
      txi->control.rates[1].idx = -1;
      txi->control.rates[1].count = 0;

      goto finish;
   }

   if(drg_tx_force_filter_cck)
   {
      if(!(txi->control.rates[0].flags & IEEE80211_TX_RC_MCS))
      {
         force_mpdu = true;
         skbinfo->flags |= SKB_INFO_FLAGS_FORCE_MPDU_ON_BA_SESSION;
      }
   }
   else
   {
      /* prevent use non MCS rate to A-mpdu */
      for(i = 0; i < IEEE80211_TX_MAX_RATES; i++)
      {
         if((txi->control.rates[i].idx == -1)
            || (try_count == 0))
         {
            break;
         }
         if(!(txi->control.rates[i].flags & IEEE80211_TX_RC_MCS))
         {
            force_mpdu = true;
            skbinfo->flags |= SKB_INFO_FLAGS_FORCE_MPDU_ON_BA_SESSION;
         }
      }
   }

   /* need to use MCS rate when not force mpdu */
   for(i = 0; i < IEEE80211_TX_MAX_RATES; i++)
   {
redo:
      if((txi->control.rates[i].idx == -1)
         || (try_count == 0))
      {
         break;
      }
      if(i == (IEEE80211_TX_MAX_RATES - 1)) /* last rate */
      {
         last_rate = true;
      }
      if(!force_mpdu &&
         (!(txi->control.rates[i].flags & IEEE80211_TX_RC_MCS)))
      {
         if(!last_rate)
         {
            memmove(&txi->control.rates[i], &txi->control.rates[i+1], sizeof(struct ieee80211_tx_rate) * ((IEEE80211_TX_MAX_RATES - 1) - i));
            txi->control.rates[IEEE80211_TX_MAX_RATES - 1].idx = -1;
            txi->control.rates[IEEE80211_TX_MAX_RATES - 1].count = 0;
   
            goto redo;  
         }
         else
         {
            break;
         }
      }
      if(txi->control.rates[i].count > try_count)
      {
         txi->control.rates[i].count = try_count;
      }
      try_count -= txi->control.rates[i].count;
   }
   if(!force_mpdu && (try_count > 0))
   {
      txi->control.rates[i-1].count += try_count;
      txi->control.rates[i].idx = -1;
      txi->control.rates[i].count = 0;
   }
   else
   {
      if(!last_rate)
      {
         txi->control.rates[i].idx = -1;
         txi->control.rates[i].count = 0;
      }
   }

finish:

   return;
}

static void tx_rate_retry_adjust(struct ieee80211_tx_info *txi)
{
   int i;

   /* before adjust rate, backup original rate to driver data for report minstrel */
   memcpy(&txi->status.status_driver_data[0], &txi->control.rates[0], sizeof(struct ieee80211_tx_rate) * IEEE80211_TX_MAX_RATES);

   for(i = 0; i < IEEE80211_TX_MAX_RATES; i++)
   {
      /* find the last rate */
      if(txi->control.rates[i].idx == -1)
      {
         /* choose the best probabilty rate at the tail and fix retry count to max */
         memcpy(&txi->control.rates[0], &txi->control.rates[i-1], sizeof(struct ieee80211_tx_rate));
         txi->control.rates[0].count = 1;

         /* use CCK series to make best probabilty */
         txi->control.rates[1].idx = 3;
         txi->control.rates[1].count = 3;
         txi->control.rates[1].flags = 0;

         txi->control.rates[2].idx = 0;
         txi->control.rates[2].count = 3;
         txi->control.rates[2].flags = 0;

         txi->control.rates[3].idx = -1;
         txi->control.rates[3].count = 0;
         break;
      }
   }
}

static void tx_rate_fill(struct ieee80211_tx_info *txi, tx_descriptor *descr)
{
   int i, format, ch_offset, sgi, rate_idx;

   for(i = 0; i < IEEE80211_TX_MAX_RATES; i++)
   {
      if(txi->control.rates[i].idx < 0)
      {
         break;
      }
      if(txi->control.rates[i].flags & IEEE80211_TX_RC_40_MHZ_WIDTH)
      {
         ch_offset = CH_OFFSET_40;
      }
      else
      {
         ch_offset = CH_OFFSET_20;
      }
      if(txi->control.rates[i].flags & IEEE80211_TX_RC_MCS)
      {
         if(txi->control.rates[i].flags & IEEE80211_TX_RC_SHORT_GI)
         {
            sgi = 1;
         }
         else
         {
            sgi = 0;
         }
         if(txi->control.rates[i].flags & IEEE80211_TX_RC_GREEN_FIELD)
         {
            format = FMT_HT_GF;
         }
         else
         {
            format = FMT_HT_MIXED;
         }
         descr->tx_rate[i] = tx_rate_encoding(format, ch_offset, txi->control.rates[i].count, sgi, txi->control.rates[i].idx);
      }
      else
      {
         tx_rate_seletion(txi->control.rates[i].idx, &format, &rate_idx);

         if((txi->control.rates[i].flags & IEEE80211_TX_RC_USE_SHORT_PREAMBLE)
            && (format == FMT_11B))
         {
            rate_idx |= CCK_SHORT_PREAMBLE;
         }
         descr->tx_rate[i] = tx_rate_encoding(format, ch_offset, txi->control.rates[i].count, 0, rate_idx);
      }
   }

   if(dragonite_force_txrate_idx > 0)
   {
      if(dragonite_force_txrate_idx >=1 && dragonite_force_txrate_idx <= 12)
      {
         tx_rate_seletion(dragonite_force_txrate_idx - 1, &format, &rate_idx);
      }
      else if(dragonite_force_txrate_idx >=13 && dragonite_force_txrate_idx <= 20)
      {
         format = FMT_HT_MIXED;
         rate_idx = dragonite_force_txrate_idx - 13;
      }
      else
      {
         return;
      }
      for(i = 0; i < IEEE80211_TX_MAX_RATES; i++)
          descr->tx_rate[i] = tx_rate_encoding(format, CH_OFFSET_20, txi->control.rates[i].count, 0, rate_idx);
   }
}

void dragonite_tx_swq_dump(struct mac80211_dragonite_data *data, struct list_head *lptr)
{
   struct list_head *hptr;
   int i=0;

   dragonite_txq_lock();

   hptr = lptr;
   while(lptr)
   {
      printk("%p->", lptr);
      if(lptr->next == hptr)
      {
         printk("head\n");
         break;
      }
      else
      {
         lptr = lptr->next;
      }
      i++;
      if(i!=0 && (i%8==0))
      {
         printk("\n");
      }
      
   }
   dragonite_txq_unlock();

}

/* dragonite_tx_send_ba_request use must in dragonite_ps_off_ready_lock */
int dragonite_tx_send_ba_request(struct mac80211_dragonite_data *data, int sta_index, u16 tid, u16 ssn)
{
   struct sk_buff *skb;
   struct ieee80211_bar *bar;
   struct skb_info skbinfo;
   struct ieee80211_tx_info *txi;
   int ret, bssid;
   struct dragonite_vif_priv *vp;
   struct dragonite_sta_priv *sp;
   u16 bar_control = 0;

   ret = 0;
   skbinfo.flags = 0;

   skb = dev_alloc_skb(data->hw->extra_tx_headroom + sizeof(*bar));
   if (!skb) {
      ret = -ENOMEM;
      return ret;
   }

   skb_reserve(skb, data->hw->extra_tx_headroom);

   bar = (struct ieee80211_bar *)skb_put(skb, sizeof(*bar));

   memset(bar, 0, sizeof(*bar));

   bar->frame_control = cpu_to_le16(IEEE80211_FTYPE_CTL | IEEE80211_STYPE_BACK_REQ);

   bar_control |= (u16)IEEE80211_BAR_CTRL_ACK_POLICY_NORMAL;
   bar_control |= (u16)IEEE80211_BAR_CTRL_CBMTID_COMPRESSED_BA;
   bar_control |= (u16)(tid << IEEE80211_BAR_CTRL_TID_INFO_SHIFT);
   bar->control = cpu_to_le16(bar_control);
   bar->start_seq_num = cpu_to_le16(ssn << 4);

   bssid = data->mac_info->sta_cap_tbls[sta_index].bssid;

   vp = idx_to_vp(data, (u8)bssid);
   sp = idx_to_sp(data, (u8)sta_index);

   if((vp==NULL) || (sp==NULL))
   {
      goto fail;
   }
   memcpy(bar->ra, sp->mac_addr, ETH_ALEN);
   memcpy(bar->ta, data->dragonite_bss_info[bssid].MAC_ADDR, ETH_ALEN);

   skbinfo.ssn = 0;
   skbinfo.header_length = 14;

   skbinfo.tid = tid;

   skbinfo.flags |= SKB_INFO_FLAGS_FORCE_SECURITY_OFF;
   skbinfo.flags |= SKB_INFO_FLAGS_USE_ORIGINAL_SEQUENCE_NUMBER;
   skbinfo.flags |= SKB_INFO_FLAGS_BAR_FRAME;

   txi = IEEE80211_SKB_CB(skb);
   memset(txi, 0 , sizeof(struct ieee80211_tx_info));

   txi->control.rates[0].idx = 0; /* CCK 1M */
   txi->control.rates[0].count = 4;
   txi->control.rates[1].idx = -1;

   ret = dragonite_tx_skb(data, skb, &skbinfo, vp, sp);
fail:
   if(ret < 0) {
      dev_kfree_skb_any(skb);
      ret = -EBUSY;
   }

   return ret;
}
int dragonite_tx_send_null_data(struct mac80211_dragonite_data *data, int sta_index)
{
   struct sk_buff *skb;
   struct ieee80211_hdr *hdr;
   struct skb_info skbinfo;
   struct ieee80211_tx_info *txi;
   int ret;
   int bssid;
   struct dragonite_vif_priv *vp;
   struct dragonite_sta_priv *sp;

   ret = 0;
   skbinfo.flags = 0;
   skb = dev_alloc_skb(data->hw->extra_tx_headroom + sizeof(struct ieee80211_hdr));
   if (!skb) {
      ret = -ENOMEM;
      return ret;
   }

   skb_reserve(skb, data->hw->extra_tx_headroom);

   hdr = (void *) skb_put(skb, sizeof(*hdr) - ETH_ALEN);

   hdr->frame_control = cpu_to_le16(IEEE80211_FTYPE_DATA |
					 IEEE80211_STYPE_NULLFUNC | IEEE80211_FCTL_FROMDS);
   hdr->duration_id = cpu_to_le16(0);

   bssid = data->mac_info->sta_cap_tbls[sta_index].bssid;

   vp = idx_to_vp(data, (u8)bssid);
   sp = idx_to_sp(data, (u8)sta_index);

   if((vp==NULL) || (sp==NULL))
   {
      ret = -1;
      goto fail;
   }
	memcpy(hdr->addr1, sp->mac_addr, ETH_ALEN);
   memcpy(hdr->addr2, data->dragonite_bss_info[bssid].MAC_ADDR, ETH_ALEN);
   memcpy(hdr->addr3, data->dragonite_bss_info[bssid].MAC_ADDR, ETH_ALEN);

#if defined(CONFIG_MAC80211_MESH) || defined(DRAGONITE_SUPPORT_POWERSAVE_IN_STATION_MODE)
   if(0 > dragonite_parse_tx_skb(data, skb, &skbinfo, NULL))
#else
   if(0 > dragonite_parse_tx_skb(data, skb, &skbinfo))
#endif
   {
      dev_kfree_skb_any(skb);
      ret = -EINVAL;
		return ret;
   }
   skbinfo.flags |= SKB_INFO_FLAGS_MGMT_FRAME;
   skbinfo.flags |= SKB_INFO_FLAGS_NULL_FUNCTION;

   txi = IEEE80211_SKB_CB(skb);
   memset(txi, 0 , sizeof(struct ieee80211_tx_info));

   txi->control.rates[0].idx = 0; /* CCK 1M */
   txi->control.rates[0].count = 3;
   txi->control.rates[1].idx = 0; /* CCK 1M */
   txi->control.rates[1].count = 3;
   txi->control.rates[2].idx = -1;
   
   ret = dragonite_tx_skb(data, skb, &skbinfo, vp, 0);
fail:
 	if(ret!=0) {
	    dev_kfree_skb_any(skb);
       ret = -ENOMEM;
	}

   return ret;
}
int dragonite_tx_pspoll_response(struct mac80211_dragonite_data *data, int sta_index, int tid)
{
   struct dragonite_tx_descr *dragonite_descr;
   volatile struct list_head *lptr, *gptr;
   volatile acqe* acq_entry;
   int ret, qid = 0;
   struct dragonite_sta_priv *sp;
   bool tx_schdl = false;
   u32 skbinfo_flags;

   ret = 0;

   dragonite_ps_lock();

   /* if station is not in ps state, clear ps poll handle bit and do nothing */
   if(0 ==(data->mac_info->sta_ps_on & (0x1 << sta_index)))
   {
      goto fail2;
   }

   dragonite_txq_lock();

   sp = idx_to_sp(data, (u8)sta_index);
   if(sp==NULL)
   {
      goto fail;
   }
   if(sp->tx_sw_queue[tid].qhead != NULL)
   {
      lptr = sp->tx_sw_queue[tid].qhead;
      dragonite_descr = (struct dragonite_tx_descr *) lptr;
      gptr = (volatile struct list_head *) &dragonite_descr->group_list;
      acq_entry = (volatile acqe *) dragonite_descr->acqe_addr;
      skbinfo_flags = dragonite_descr->skbinfo_flags;

      if(skbinfo_flags & SKB_INFO_FLAGS_IN_HW)
      {
         ret = -1;
      }
      else
      {
         tx_schdl = true;
         dragonite_descr->skbinfo_flags |= SKB_INFO_FLAGS_PS_POLL_RESPONSE;

         if(!list_empty_careful((const struct list_head *) lptr))
         {
            dragonite_descr->skbinfo_flags |= SKB_INFO_FLAGS_PS_POLL_MORE_DATA;
         }
         if(dragonite_descr->skbinfo_flags & SKB_INFO_FLAGS_USE_SINGLE_QUEUE)/* single queue */
         {
            DRG_DBG("move from tx_sw_ps_single_queue then kick\n");
            if(sp->tx_sw_ps_single_queue[tid].qhead != NULL)
            {
               skb_list_del(gptr, &(sp->tx_sw_ps_single_queue[tid]));

               dragonite_descr->skbinfo_flags &= ~(SKB_INFO_FLAGS_USE_SINGLE_QUEUE);

               //qid = tid_to_qid(tid);
               qid = AC_VO_QID;/* use high priority queue */

               incr_ac_count_safe(data, qid);

               skb_list_add(gptr, &(data->tx_sw_mixed_queue[qid]));

               if(!(skbinfo_flags & SKB_INFO_FLAGS_SPECIAL_DATA_FRAME))
               {
                  sp->tx_ps_buffer_skb_count--;
               }
               else
               {
                  DRG_WARN(DRAGONITE_DEBUG_DRIVER_WARN_FLAG, "should not happen %s %d\n", __func__, __LINE__);
                  panic("single queue should not has SKB_INFO_FLAGS_SPECIAL_DATA_FRAME");
               }
            }
            else
            {
               DRG_WARN(DRAGONITE_DEBUG_DRIVER_WARN_FLAG, "DRAGONITE: dragonite_tx_pspoll_response, but tx_sw_ps_single_queue something wrong !!\n");
               panic("should not happen !!");
            }
         }
         else
         {
            DRG_DBG("move from tx_sw_ps_mixed_queue then kick\n");
            qid = tid_to_qid(tid);
            if(data->tx_sw_ps_mixed_queue[qid].qhead != NULL)
            {
               skb_list_del(gptr, &(data->tx_sw_ps_mixed_queue[qid]));

               qid = AC_VO_QID; /* use high priority queue */

               incr_ac_count_safe(data, qid);

               skb_list_add(gptr, &(data->tx_sw_mixed_queue[qid]));

               if(!(skbinfo_flags & SKB_INFO_FLAGS_SPECIAL_DATA_FRAME))
               {
                  sp->tx_ps_buffer_skb_count--;
               }
            }
            else
            {
               DRG_WARN(DRAGONITE_DEBUG_DRIVER_WARN_FLAG, "tx_sw_ps_mixed_queue something wrong !!\n");
               panic("should not happen !!");
            }
         }
      }
   }
   else
   {
      ret = -1;
   }
fail:
   dragonite_txq_unlock();
fail2:
   dragonite_ps_unlock();

   if(tx_schdl)
   {
      dragonite_tx_schedule(data, NOT_USE_VIF_IDX, -1, qid);
   }
   return ret;
}
void dragonite_tx_bcq_handler(struct mac80211_dragonite_data *data, int bss_index)
{
	struct dragonite_vif_priv *vp;
	u32 ts0_dtim;
	int qid;

	if(data->dragonite_bss_info[bss_index].dut_role != ROLE_AP)
	{
		return;
	}
	if(data->dragonite_bss_info[bss_index].dtim_count == 0) {

		ts0_dtim = MACREG_READ32(TS0_DTIM);

		if((ts0_dtim & DTIM_COUNT) != 0)
		{
			MACREG_WRITE32(TS0_DTIM, (ts0_dtim & ~(DTIM_COUNT)));

			DRG_WARN(DRAGONITE_DEBUG_DRIVER_WARN_FLAG, "dtim count out of sync, correct it\n");
		}

		vp = idx_to_vp(data, (u8)bss_index);
      if(vp==NULL)
      {
         return;
      }

		if(data->mac_info->bss_ps_status[bss_index] == DRG_BSS_PS_OFF)
		{
         if(data->mac_info->bcq_status[bss_index] == DRG_BCQ_ENABLE)
         {
            /* if hw bcq not done yet, do not change bss ps status */
            if(vp->tx_hw_ps_gc_queue.qhead != NULL)
            {
               DRG_WARN(DRAGONITE_DEBUG_DRIVER_WARN_FLAG, "bss no.%d bcq not done completed !!\n", bss_index);
            }
            else
            {
               dragonite_tx_bmcq_to_mixq(data, bss_index);
   
               data->mac_info->bcq_status[bss_index] = DRG_BCQ_DISABLE;

               DRG_NOTICE(DRAGONITE_DEBUG_PS_FLAG, "bss no.%d disable to use bcq\n", bss_index);

               for(qid = 0; qid < ACQ_NUM - 1; qid++){
                   if(data->tx_sw_mixed_queue[qid].qhead != NULL) {
                       dragonite_tx_schedule(data, NOT_USE_VIF_IDX, -1, qid);
                   }
               }
            }
         }
		}
      else if(data->mac_info->bss_ps_status[bss_index] == DRG_BSS_PS_ON)
		{
			/* if hw bcq not done yet, do not enqueue bcq */
         if(vp->tx_hw_ps_gc_queue.qhead != NULL)
         {
            DRG_WARN(DRAGONITE_DEBUG_DRIVER_WARN_FLAG, "bss no.%d bcq not done completed !!\n", bss_index);
         }
			else if(vp->tx_sw_ps_gc_queue.qhead != NULL) {

				dragonite_tx_schedule(data, (u8)bss_index, -2, NOT_USE_QUE_IDX);
			}
		}
	}
}

#if defined (DRAGONITE_TX_BHDR_CHECKER)
void dragonite_tx_bhdr_checker(u32 wakeup_t)
{
    int i;

    if((wakeup_t - drg_tx_bhdr_checker_prev_time) < DRG_TX_BHDR_CHECKER_PERIOD)
    {
        return;
    }
    else
    {
        drg_tx_bhdr_checker_prev_time = wakeup_t;
    }

    /* check bhdr owner */
    for(i = (BUFFER_HEADER_POOL_SIZE/2); i < BUFFER_HEADER_POOL_SIZE; i++) {

        if(bhdr_record_t[i]==0) {
        }
        else if((wakeup_t - bhdr_record_t[i]) > DRG_TX_BHDR_TIMEOUT_TOLERANT) {
            DRG_WARN(DRAGONITE_DEBUG_DRIVER_WARN_FLAG, "No.%d bhdr takes so long , current time %d, get bhdr time %d !!\n", i, wakeup_t, bhdr_record_t[i]);
        }
    }
}
#endif

#if defined (DRAGONITE_TX_SKB_CHECKER)
void dragonite_tx_skb_checker(struct mac80211_dragonite_data *data, u32 wakeup_t)
{
    if((wakeup_t - drg_tx_skb_checker_prev_time) < DRG_TX_SKB_CHECKER_PERIOD)
    {
        return;
    }
    else
    {
        drg_tx_skb_checker_prev_time = wakeup_t;
    }
    skb_list_trace_timeout(data, wakeup_t);
}
#endif

#if defined (DRAGONITE_TX_HANG_CHECKER)
void dragonite_init_tx_hang_checker(struct mac80211_dragonite_data *data)
{
   int acqid, cmdid;

   for(acqid = 0; acqid < ACQ_NUM - 1; acqid++){

		for(cmdid = 0; cmdid < CMD_NUM; cmdid++) {

			drg_tx_prev_ssn[acqid][cmdid] = DRAGONITE_MAGIC_TX_SSN;
		}
	}

}

void dragonite_tx_hang_checker(struct mac80211_dragonite_data *data, u64 wakeup_jiffies)
{
   int acqid, cmdid, tx_cmd_idle_cnt = 0;
   bool tx_hang = true;
   u16 ssn, esn, cur_ssn;
   u16 drg_tx_ssn[ACQ_NUM - 1][CMD_NUM], drg_tx_esn[ACQ_NUM - 1][CMD_NUM];
   u32 wakeup_t = (u32) wakeup_jiffies/HZ;
   u64 wakeup_jiffies_diff;

#if defined(DRAGONITE_POWERSAVE_AGGRESSIVE)
   dragonite_pm_lock_lock();
   if(data->poweroff)
   {
      goto exit;
   }
#endif

   if(wakeup_jiffies > drg_tx_hang_checker_prev_jiffies_time)
   {
      wakeup_jiffies_diff = wakeup_jiffies - drg_tx_hang_checker_prev_jiffies_time;
   }
   else
   {
      wakeup_jiffies_diff = drg_tx_hang_checker_prev_jiffies_time - wakeup_jiffies;
   }
   if(((wakeup_t - drg_tx_hang_checker_prev_time) < DRG_TX_HANG_CHECKER_PERIOD)
      || (wakeup_jiffies_diff < (DRG_TX_HANG_CHECKER_PERIOD * HZ)))
   {
       goto exit;
   }
   else
   {
       drg_tx_hang_checker_prev_time = wakeup_t;
       drg_tx_hang_checker_prev_jiffies_time = wakeup_jiffies;
   }

   for(acqid = 0; acqid < ACQ_NUM - 1; acqid++)
   {
      for(cmdid = 0; cmdid < CMD_NUM; cmdid++)
      {
         ssn = MACREG_READ32(ACQ_INFO(acqid,cmdid)) & 0x0fff;
         esn = MACREG_READ32(ACQ_INFO2(acqid,cmdid)) & 0x0fff;

         if(ssn == esn)
         {
            cur_ssn = DRAGONITE_MAGIC_TX_SSN;
         }
         else
         {
            cur_ssn = ssn;
         }

         if(drg_tx_prev_ssn[acqid][cmdid] != cur_ssn)
         {
            tx_hang = false;
         }
         if(cur_ssn == DRAGONITE_MAGIC_TX_SSN)
         {
            tx_cmd_idle_cnt++;
         }
         /* update ssn */
         drg_tx_prev_ssn[acqid][cmdid] = cur_ssn;
         drg_tx_ssn[acqid][cmdid] = ssn;
         drg_tx_esn[acqid][cmdid] = esn;
      }
   }

#if defined(DRAGONITE_POWERSAVE_AGGRESSIVE)
   dragonite_pm_lock_unlock();
#endif

   if(tx_hang 
      && (tx_cmd_idle_cnt < DRG_TX_TOTAL_CMD_COUNT) 
      && (drg_tx_done_prev_intr_times == drg_tx_done_intr_times)
      && (drg_tx_retry_prev_intr_times == drg_tx_retry_intr_times)
      && (drg_tx_switch_prev_intr_times == drg_tx_switch_intr_times))
   {
      DRG_ERROR(DRAGONITE_DEBUG_DRIVER_WARN_FLAG, "TX hang, recover wifi !!\n");
      DRG_NOTICE(DRAGONITE_DEBUG_DRIVER_WARN_FLAG, "[acqid][cmdid]   ssn   esn [acqid][cmdid]   ssn   esn\n");
      DRG_NOTICE(DRAGONITE_DEBUG_DRIVER_WARN_FLAG, "[  0  ][  0  ]  %04x  %04x [  0  ][  1  ]  %04x  %04x\n",
                 drg_tx_ssn[0][0], drg_tx_esn[0][0], drg_tx_ssn[0][1], drg_tx_esn[0][1]);
      DRG_NOTICE(DRAGONITE_DEBUG_DRIVER_WARN_FLAG, "[  1  ][  0  ]  %04x  %04x [  1  ][  1  ]  %04x  %04x\n",
                 drg_tx_ssn[1][0], drg_tx_esn[1][0], drg_tx_ssn[1][1], drg_tx_esn[1][1]);
      DRG_NOTICE(DRAGONITE_DEBUG_DRIVER_WARN_FLAG, "[  2  ][  0  ]  %04x  %04x [  2  ][  1  ]  %04x  %04x\n",
                 drg_tx_ssn[2][0], drg_tx_esn[2][0], drg_tx_ssn[2][1], drg_tx_esn[2][1]);
      DRG_NOTICE(DRAGONITE_DEBUG_DRIVER_WARN_FLAG, "[  3  ][  0  ]  %04x  %04x [  3  ][  1  ]  %04x  %04x\n",
                 drg_tx_ssn[3][0], drg_tx_esn[3][0], drg_tx_ssn[3][1], drg_tx_esn[3][1]);
      DRG_NOTICE(DRAGONITE_DEBUG_DRIVER_WARN_FLAG, "[  4  ][  0  ]  %04x  %04x [  4  ][  1  ]  %04x  %04x\n",
                 drg_tx_ssn[4][0], drg_tx_esn[4][0], drg_tx_ssn[4][1], drg_tx_esn[4][1]);

      /* recover wifi */
      drg_wifi_recover = 1;
   }
   else if(tx_hang
           && (tx_cmd_idle_cnt < DRG_TX_TOTAL_CMD_COUNT))
   {
      DRG_WARN(DRAGONITE_DEBUG_DRIVER_WARN_FLAG, "wifi tx hang, but tx intr still working\n");
      DRG_NOTICE(DRAGONITE_DEBUG_DRIVER_WARN_FLAG, "prev tx done intr times %d, current intr times %d !!\n", drg_tx_done_prev_intr_times, drg_tx_done_intr_times);
      DRG_NOTICE(DRAGONITE_DEBUG_DRIVER_WARN_FLAG, "prev tx retry intr times %d, current intr times %d !!\n", drg_tx_retry_prev_intr_times, drg_tx_retry_intr_times);
      DRG_NOTICE(DRAGONITE_DEBUG_DRIVER_WARN_FLAG, "prev tx switch intr times %d, current intr times %d !!\n", drg_tx_switch_prev_intr_times, drg_tx_switch_intr_times);
   }

   drg_tx_done_prev_intr_times = drg_tx_done_intr_times;
   drg_tx_retry_prev_intr_times = drg_tx_retry_intr_times;
   drg_tx_switch_prev_intr_times = drg_tx_switch_intr_times;

   return;

exit:
#if defined(DRAGONITE_POWERSAVE_AGGRESSIVE)
   dragonite_pm_lock_unlock();
#endif
   return;
}
#endif

void dragonite_tx_release_skb_queue(struct mac80211_dragonite_data *data, u32 wakeup_t, bool force)
{
	volatile struct list_head *lptr;
	struct dragonite_tx_descr *dragonite_descr;
	int bhdr_index;
	tx_descriptor *descr;
	struct sk_buff *skb;

   if(((wakeup_t - drg_tx_release_skb_queue_prev_time) < DRG_TX_SKB_DELAY_FREE_PERIOD)
      && !force)
   {
      return;
   }

   drg_tx_release_skb_queue_prev_time = wakeup_t;

	dragonite_txq_lock();

	while(data->tx_sw_free_queue[0].qhead != NULL)
	{
		lptr = data->tx_sw_free_queue[0].qhead;
		dragonite_descr = (struct dragonite_tx_descr *) lptr;
		descr = (tx_descriptor *) VIRTUAL_ADDR((u32)dragonite_descr + DRAGONITE_TXDESCR_SIZE);

      if((wakeup_t - dragonite_descr->free_tstamp < DRG_TX_SKB_DELAY_FREE_TIME)
         && !force)
      {
         break;
      }

		skb_list_del(lptr, data->tx_sw_free_queue);

		if(descr->bhr_h)
		{
			bhdr_index = descr->bhr_h;
			cleanup_bhdr_on_descr(data->mac_info, bhdr_index);

			buffer_header_detach_skb(data->mac_info, bhdr_index);
			bhdr_insert_tail(data->mac_info, bhdr_index, -1);
		}
		skb = (struct sk_buff *) dragonite_descr->skb_addr;
		drg_check_out_tx_skb(data, skb, TX_SKB_FAIL);
	}

	dragonite_txq_unlock();
}

void dragonite_tx_ps_buffer_adjust(struct mac80211_dragonite_data *data, int sta_index)
{
	int flush_count, i, tid , bhdr_index;
	volatile struct list_head *lptr, *gptr;
	volatile struct list_head *head, *next;
	u32 skbinfo_flags;
	struct dragonite_tx_descr *dragonite_descr;
	tx_descriptor *descr;
	struct dragonite_sta_priv *sp;
	struct sk_buff *skb;

	dragonite_txq_lock();

	if(sta_index >=MAX_STA_NUM || sta_index<0)
	{
		panic("DRAGONITE: ps_buffer_adjust wrong sta_index");
	}
	sp = idx_to_sp(data, (u8)sta_index);
	if(sp==NULL)
	{
		goto done;
	}

	if(PS_BUFFER_OVERFLOW(sp))
	{
		flush_count = sp->tx_ps_buffer_skb_count - PS_BUFFER_COUNT;

		for(i=0;i<TID_MAX+1;i++)/* choose low priority first */
		{
			tid = rank_id_to_tid(i);
			head = NULL;
			lptr = sp->tx_sw_queue[tid].qhead;
			while(sp->tx_sw_queue[tid].qhead != NULL)
			{
				next = lptr->next;

				dragonite_descr = (struct dragonite_tx_descr *) lptr;
				gptr = (volatile struct list_head *) &dragonite_descr->group_list;
				descr = (tx_descriptor *) VIRTUAL_ADDR((u32)dragonite_descr + DRAGONITE_TXDESCR_SIZE);
				skbinfo_flags = dragonite_descr->skbinfo_flags;

				if(flush_count==0)
				{
					goto done;
				}
				if(skbinfo_flags & (SKB_INFO_FLAGS_PS_POLL_RESPONSE | SKB_INFO_FLAGS_SPECIAL_DATA_FRAME | SKB_INFO_FLAGS_BAR_FRAME))
				{
					/*do not drop*/
					if(head == NULL)
						head = lptr;
				}
				else if(skbinfo_flags & SKB_INFO_FLAGS_USE_SINGLE_QUEUE)
				{
					/* de q */
					skb_list_del(gptr, &(sp->tx_sw_ps_single_queue[tid]));
					skb_list_del(lptr, &(sp->tx_sw_queue[tid]));

					data->tx_sw_queue_total_count--;
					sp->tx_ps_buffer_skb_count--;
					flush_count--;
					/* SKB_INFO_FLAGS_PS_BUFFER_FULL move to free skb list */
					if(skbinfo_flags & SKB_INFO_FLAGS_IN_HW)
					{
						dragonite_descr->free_tstamp = (jiffies/HZ);
						skb_list_add(lptr, data->tx_sw_free_queue); /* delay free skb */
					}
					else
					{
						/* cleanup bhdr and skb */
						if(descr->bhr_h)
						{
							bhdr_index = descr->bhr_h;
							cleanup_bhdr_on_descr(data->mac_info, bhdr_index);

							buffer_header_detach_skb(data->mac_info, bhdr_index);
							bhdr_insert_tail(data->mac_info, bhdr_index, -1);
						}
						skb = (struct sk_buff *) dragonite_descr->skb_addr;
						drg_check_out_tx_skb(data, skb, TX_SKB_FAIL);
					}
				}
				else /*MIXED Q*/
				{
					if(skbinfo_flags & SKB_INFO_FLAGS_IN_HW)
					{
						/* will done later, can not drop */
						goto done;
					}
					else
					{
						/* de q */
						skb_list_del(gptr, &(data->tx_sw_ps_mixed_queue[tid_to_qid(tid)]));
						skb_list_del(lptr, &(sp->tx_sw_queue[tid]));

						data->tx_sw_queue_total_count--;
						sp->tx_ps_buffer_skb_count--;
						flush_count--;

						/* cleanup bhdr and skb */
						if(descr->bhr_h)
						{
							bhdr_index = descr->bhr_h;
							cleanup_bhdr_on_descr(data->mac_info, bhdr_index);

                     buffer_header_detach_skb(data->mac_info, bhdr_index);
							bhdr_insert_tail(data->mac_info, bhdr_index, -1);
						}
						skb = (struct sk_buff *) dragonite_descr->skb_addr;
						drg_check_out_tx_skb(data, skb, TX_SKB_FAIL);
					}
				}
				if(next == head)
				{
					break;
				}
				lptr = next;
			}
		}
	}
done:
	dragonite_txq_unlock();
}
void dragonite_tx_bmcq_to_bcq(struct mac80211_dragonite_data *data, int bss_index)
{
   struct dragonite_tx_descr *dragonite_descr;
   volatile struct list_head *lptr, *gptr;
   volatile acqe* acq_entry;
   int i, qid;
   u32 skbinfo_flags; 
   struct dragonite_vif_priv *vp;

   dragonite_txq_lock();

   if(bss_index >= MAX_BSSIDS || bss_index<0)
   {
      panic("DRAGONITE: dragonite_tx_bmcq_to_bcq wrong bss_index");
   }
   vp = idx_to_vp(data, (u8)bss_index);
   if(vp==NULL)
   {
      goto done;
   }
   for(i=0;i<ACQ_NUM - 1;i++)
   {
      if(vp->tx_sw_gc_queue[i].qhead != NULL)
      {
         lptr = vp->tx_sw_gc_queue[i].qhead;
         do
         {
            dragonite_descr = (struct dragonite_tx_descr *) lptr;
            gptr = (volatile struct list_head *) &dragonite_descr->group_list;
            acq_entry = (volatile acqe *) dragonite_descr->acqe_addr;
            skbinfo_flags = dragonite_descr->skbinfo_flags;

            qid = i;
            if(acq_entry)/* check in hw queue or not*/
            {
               goto next;
            }
            else if(data->tx_sw_mixed_queue[qid].qhead != NULL)
            {
               skb_list_del(gptr, &(data->tx_sw_mixed_queue[qid]));
            }
            else
            {
               goto next;
            }
            skb_list_add(gptr, &(vp->tx_sw_ps_gc_queue));
            
next:
            lptr = lptr->next;
         }while(lptr != vp->tx_sw_gc_queue[i].qhead);
      }
   }
done:
   dragonite_txq_unlock();
}
void dragonite_tx_bmcq_to_mixq(struct mac80211_dragonite_data *data, int bss_index)
{
   struct dragonite_tx_descr *dragonite_descr;
   volatile struct list_head *lptr, *gptr;
   volatile acqe* acq_entry;
   int i, qid;
   u32 skbinfo_flags;
   struct dragonite_vif_priv *vp;

   dragonite_txq_lock();

   if(bss_index >= MAX_BSSIDS || bss_index<0)
   {
      panic("DRAGONITE: dragonite_tx_bmcq_to_bcq wrong bss_index");
   }
   vp = idx_to_vp(data, (u8)bss_index);
   if(vp==NULL)
   {
      goto Exit;
   }
   for(i=0;i<ACQ_NUM - 1;i++)
   {
      if(vp->tx_sw_gc_queue[i].qhead != NULL)
      {
         lptr = vp->tx_sw_gc_queue[i].qhead;
         do
         {
            dragonite_descr = (struct dragonite_tx_descr *) lptr;
            gptr = (volatile struct list_head *) &dragonite_descr->group_list;
            acq_entry = (volatile acqe *) dragonite_descr->acqe_addr;
            skbinfo_flags = dragonite_descr->skbinfo_flags;

            qid = i;
            if(acq_entry)/* check in hw queue or not*/
            {
               goto next;
            }
            else if(vp->tx_sw_ps_gc_queue.qhead != NULL)
            {
               skb_list_del(gptr, &(vp->tx_sw_ps_gc_queue));
            }
            else
            {
               goto Exit;
            }
            skb_list_add(gptr, &(data->tx_sw_mixed_queue[qid]));
            
next:
            lptr = lptr->next;
         }while(lptr != vp->tx_sw_gc_queue[i].qhead);
      }
   }
Exit:
   dragonite_txq_unlock();
}
void dragonite_tx_swq_to_psq(struct mac80211_dragonite_data *data, int sta_index)
{
   struct dragonite_tx_descr *dragonite_descr;
   volatile struct list_head *lptr, *gptr;
   volatile acqe* acq_entry;
   tx_descriptor *descr;
   int i, qid;
   u32 skbinfo_flags;
   struct dragonite_sta_priv *sp;

   dragonite_txq_lock();

   if(sta_index >=MAX_STA_NUM || sta_index<0)
   {
      panic("DRAGONITE: dragonite_tx_swq_to_psq wrong sta_index");
   }
   sp = idx_to_sp(data, (u8)sta_index);
   if(sp==NULL)
   {
      goto done;
   }
   sp->tx_hw_pending_skb_count = 0; /* initialize pending hw count */

   sp->tx_ps_buffer_skb_count = 0; /* initialize ps buffer count, must in dragonite_sw_queue_lock*/

   for(i=0;i<TID_MAX+1;i++)
   {
      if(sp->tx_sw_queue[i].qhead != NULL)
      {
         lptr = sp->tx_sw_queue[i].qhead;
         do
         {
            dragonite_descr = (struct dragonite_tx_descr *) lptr;
            gptr = (volatile struct list_head *) &dragonite_descr->group_list;
            acq_entry = (volatile acqe *) dragonite_descr->acqe_addr;
            skbinfo_flags = dragonite_descr->skbinfo_flags;
            descr = (tx_descriptor *) VIRTUAL_ADDR((u32)dragonite_descr + DRAGONITE_TXDESCR_SIZE);

            if(skbinfo_flags & (SKB_INFO_FLAGS_PS_POLL_RESPONSE | SKB_INFO_FLAGS_BAR_FRAME))
            {
               /* do nothing */
            }
            else if(skbinfo_flags & SKB_INFO_FLAGS_USE_SINGLE_QUEUE)/* single queue */
            {
               if(IS_INVALID_SSN(PREV_AGGR_SSN(sp, i)))
               {
                  PREV_AGGR_SSN(sp, i) = descr->sn;
               }
               if(acq_entry)/* check in hw queue or not*/
               {
                  if(sp->tx_hw_single_queue[i].qhead == NULL)
                  {
                     DRG_WARN(DRAGONITE_DEBUG_DRIVER_WARN_FLAG, "should not happen %s %d\n", __func__, __LINE__);
                     panic("dragonite: should not happed !!");
                  }
                  else//if(acq_entry->m.ps || acq_entry->m.own)/* meet ps mode */
                  {
                     skb_list_del(gptr, &(sp->tx_hw_single_queue[i]));
                     dragonite_descr->acqe_addr = 0;
                  }
               }
               else
               {
                  if(sp->tx_sw_single_queue[i].qhead == NULL)
                  {
                     goto next;
                  }
                  else
                  {
                     skb_list_del(gptr, &(sp->tx_sw_single_queue[i]));
                  }
               }

               decr_ac_count_safe(data, tid_to_qid(dragonite_descr->tid));
               /* add in ps single queue */
               skb_list_add(gptr, &(sp->tx_sw_ps_single_queue[i]));

               if(!(skbinfo_flags & SKB_INFO_FLAGS_SPECIAL_DATA_FRAME))
               {
                  sp->tx_ps_buffer_skb_count++;
               }
               else
               {
                  DRG_WARN(DRAGONITE_DEBUG_DRIVER_WARN_FLAG, "should not happen %s %d\n", __func__, __LINE__);
                  panic("single queue should not has SKB_INFO_FLAGS_SPECIAL_DATA_FRAME");
               }
            }
            else/* mixed queue */
            {
               qid = tid_to_qid(i);
               if(acq_entry)/* check in hw queue or not*/
               {
                  if(data->tx_hw_mixed_queue[qid].qhead == NULL)
                  {
                     DRG_WARN(DRAGONITE_DEBUG_DRIVER_WARN_FLAG, "should not happen %s %d\n", __func__, __LINE__);
                     panic("dragonite: should not happed !!");
                  }
                  else //if(acq_entry->m.ps || acq_entry->m.own)/* meet ps mode */
                  {
                     skb_list_del(gptr, &(data->tx_hw_mixed_queue[qid]));
                     dragonite_descr->acqe_addr = 0;
                  }
               }
               else
               {
                  if(data->tx_sw_mixed_queue[qid].qhead == NULL)
                  {
                     goto next;
                  }
                  else
                  {
                     skb_list_del(gptr, &(data->tx_sw_mixed_queue[qid]));
                  }
               }
               decr_ac_count_safe(data, tid_to_qid(dragonite_descr->tid));

               skb_list_add(gptr, &(data->tx_sw_ps_mixed_queue[qid]));

               if(dragonite_descr->skbinfo_flags & SKB_INFO_FLAGS_IN_HW)
               {
                  sp->tx_hw_pending_skb_count++;
               }
               if(!(skbinfo_flags & SKB_INFO_FLAGS_SPECIAL_DATA_FRAME))
               {
                  sp->tx_ps_buffer_skb_count++;
               }
            }
next:
            lptr = lptr->next;
         }while(lptr != sp->tx_sw_queue[i].qhead);
      }

      PREV_AGGR_SSN(sp, i) = INVALID_SSN;

      if(SINGLE_Q(sp, i))
      {
         if(IS_INVALID_SSN(PREV_AGGR_SSN(sp, i)))
         {
            PREV_AGGR_SSN(sp, i) = AGGR_SSN(sp, i);
         }
      }
   }
done:

   dragonite_txq_unlock();
}
void dragonite_tx_swq_to_gpq(struct mac80211_dragonite_data *data, int sta_index)
{
   struct dragonite_tx_descr *dragonite_descr;
   volatile struct list_head *lptr, *gptr;
   volatile struct list_head *head, *next;
   volatile acqe* acq_entry;
   int i, qid;
   int per_ac_allow_count[ACQ_NUM - 1], per_ac_drop_count[ACQ_NUM - 1], per_ac_ps_buffer_count[ACQ_NUM - 1];
   u32 skbinfo_flags;
   struct dragonite_sta_priv *sp;
   int bhdr_index;
   tx_descriptor *descr;
   struct sk_buff *skb;

   dragonite_txq_lock();

   if(sta_index >=MAX_STA_NUM || sta_index<0)
   {
      panic("DRAGONITE: dragonite_tx_swq_to_gpq wrong sta_index");
   }
   sp = idx_to_sp(data, (u8)sta_index);
   if(sp==NULL)
   {
      goto done;
   }
   for(i=0;i<ACQ_NUM - 1;i++)
   {
      per_ac_ps_buffer_count[i] = 0;

      if(dragonite_tx_per_ac_max_buf >= data->tx_per_ac_count[i])
      {
         per_ac_allow_count[i] = dragonite_tx_per_ac_max_buf - data->tx_per_ac_count[i];
      }
      else
      {
         per_ac_allow_count[i] = 0;
      }
   }
   for(i=0;i<TID_MAX+1;i++)
   {
      per_ac_ps_buffer_count[tid_to_qid(i)] += sp->tx_sw_queue[i].qcnt;
   }
   for(i=0;i<ACQ_NUM - 1;i++)
   {
      if(per_ac_allow_count[i] >= per_ac_ps_buffer_count[i])
      {
         per_ac_drop_count[i] = 0;
      }
      else
      {
         per_ac_drop_count[i] = per_ac_ps_buffer_count[i] - per_ac_allow_count[i];
      }
   }
   for(i=0;i<TID_MAX+1;i++)
   {
      qid = tid_to_qid(i);
      head = NULL;
      lptr = sp->tx_sw_queue[i].qhead;
      while(sp->tx_sw_queue[i].qhead != NULL)
      {
         next = lptr->next;
         dragonite_descr = (struct dragonite_tx_descr *) lptr;
         gptr = (volatile struct list_head *) &dragonite_descr->group_list;
         acq_entry = (volatile acqe *) dragonite_descr->acqe_addr;
         skbinfo_flags = dragonite_descr->skbinfo_flags;
         descr = (tx_descriptor *) VIRTUAL_ADDR((u32)dragonite_descr + DRAGONITE_TXDESCR_SIZE);


         if(skbinfo_flags & (SKB_INFO_FLAGS_PS_POLL_RESPONSE | SKB_INFO_FLAGS_BAR_FRAME))
         {
            if(head == NULL)
               head = lptr;
            goto next;
         }

         if(skbinfo_flags & SKB_INFO_FLAGS_USE_SINGLE_QUEUE)/* single queue */
         {
            if(acq_entry)/* check in hw queue or not*/
            {
               DRG_WARN(DRAGONITE_DEBUG_DRIVER_WARN_FLAG, "should not happen %s %d\n", __func__, __LINE__);
               panic("dragonite: should not happed !!");
            }
            else
            {
               skb_list_del(gptr, &(sp->tx_sw_ps_single_queue[i]));
            }

            /* clear bhdr on descr and cleanup bhdr if descr->bhdr still hold */
            dragonite_descr->skbinfo_flags &= ~(SKB_INFO_FLAGS_IN_HW);
            if(descr->bhr_h)
            {
               bhdr_index = descr->bhr_h;
               cleanup_bhdr_on_descr(data->mac_info, bhdr_index);

               buffer_header_detach_skb(data->mac_info, bhdr_index);
               bhdr_insert_tail(data->mac_info, bhdr_index, -1);
            }

            if(!(dragonite_descr->skbinfo_flags & SKB_INFO_FLAGS_SPECIAL_DATA_FRAME))
            {
               sp->tx_ps_buffer_skb_count--;

               if(per_ac_drop_count[qid] > 0)
               {
                  per_ac_drop_count[qid]--;
                  skb_list_del(lptr, &(sp->tx_sw_queue[i]));
                  data->tx_sw_queue_total_count--;
                  skb = (struct sk_buff *) dragonite_descr->skb_addr;
                  //dev_kfree_skb(skb);
                  drg_check_out_tx_skb(data, skb, TX_SKB_FAIL);
                  goto next;
               }
            }
            else
            {
               DRG_WARN(DRAGONITE_DEBUG_DRIVER_WARN_FLAG, "should not happen %s %d\n", __func__, __LINE__);
               panic("single queue should not has SKB_INFO_FLAGS_SPECIAL_DATA_FRAME");
            }

            incr_ac_count_safe(data, qid);

            skb_list_add(gptr, &(sp->tx_sw_single_queue[i]));
            if(head == NULL)
               head = lptr;
            
         }
         else/* mixed queue */
         {
            if(acq_entry)/* check in hw queue or not*/
            {
               DRG_WARN(DRAGONITE_DEBUG_DRIVER_WARN_FLAG, "should not happen %s %d\n", __func__, __LINE__);
               panic("dragonite: should not happed !!");
            }
            else
            {
               skb_list_del(gptr, &(data->tx_sw_ps_mixed_queue[qid]));
            }

            if(!(dragonite_descr->skbinfo_flags & SKB_INFO_FLAGS_SPECIAL_DATA_FRAME))
            {
               sp->tx_ps_buffer_skb_count--;
               if(per_ac_drop_count[qid] > 0)
               {
                  per_ac_drop_count[qid]--;
                  skb_list_del(lptr, &(sp->tx_sw_queue[i]));
                  data->tx_sw_queue_total_count--;
                  skb = (struct sk_buff *) dragonite_descr->skb_addr;
                  //dev_kfree_skb(skb);
                  drg_check_out_tx_skb(data, skb, TX_SKB_FAIL);
                  goto next;
               }
            }
            incr_ac_count_safe(data, qid);

            skb_list_add(gptr, &(data->tx_sw_mixed_queue[qid]));
            if(head == NULL)
               head = lptr;
            
         }
next:
         if(next == head)
         {
             break;
         }
         lptr = next;
      }
      
   }
done:
   dragonite_txq_unlock();
}

void dragonite_tx_ampdu_q_detach(struct mac80211_dragonite_data *data, int sta_idx, int tid)
{
   struct dragonite_tx_descr *dragonite_descr;
   volatile struct list_head *lptr;
   struct sk_buff *skb;
   struct dragonite_sta_priv *sp;
   tx_descriptor *descr;

   dragonite_txq_lock();
   
      sp = idx_to_sp(data, (u8) sta_idx);
      if(sp==NULL)
      {
         panic("DRAGONITE: unexpect error, sp == NULL, dragonite_tx_ampdu_q_detach failed !!");
      }

      if(sp->tx_sw_single_queue[tid].qhead != NULL)
      {
         /* remove sw single queue element */
         while((sp->tx_sw_single_queue[tid].qhead != NULL) && (sp->tx_sw_queue[tid].qhead != NULL))
         {
            lptr = sp->tx_sw_single_queue[tid].qhead;
            dragonite_descr = (struct dragonite_tx_descr *) VIRTUAL_ADDR((u32)lptr - sizeof(struct list_head));
            descr = (tx_descriptor *) VIRTUAL_ADDR((u32)dragonite_descr + DRAGONITE_TXDESCR_SIZE);
            skb_list_del(lptr, &(sp->tx_sw_single_queue[tid]));

            lptr = (volatile struct list_head *) &dragonite_descr->private_list;
            skb_list_del(lptr, &(sp->tx_sw_queue[tid]));
            data->tx_sw_queue_total_count--;

            decr_ac_count_safe(data, tid_to_qid(tid));

            if(descr->bhr_h)
            {
               buffer_header_detach_skb(data->mac_info, descr->bhr_h);
               bhdr_insert_tail(data->mac_info, descr->bhr_h, -1);
            }

            skb = (struct sk_buff *) dragonite_descr->skb_addr;

            drg_check_out_tx_skb(data, skb, TX_SKB_FAIL);
         }
      }
      if(sp->tx_sw_ps_single_queue[tid].qhead != NULL)
      {
         /* remove sw ps single queue element */
         while((sp->tx_sw_ps_single_queue[tid].qhead != NULL) && (sp->tx_sw_queue[tid].qhead != NULL))
         {
            lptr = sp->tx_sw_ps_single_queue[tid].qhead;
            dragonite_descr = (struct dragonite_tx_descr *) VIRTUAL_ADDR((u32)lptr - sizeof(struct list_head));
            descr = (tx_descriptor *) VIRTUAL_ADDR((u32)dragonite_descr + DRAGONITE_TXDESCR_SIZE);
            skb_list_del(lptr, &(sp->tx_sw_ps_single_queue[tid]));

            lptr = (volatile struct list_head *) &dragonite_descr->private_list;
            skb_list_del(lptr, &(sp->tx_sw_queue[tid]));
            data->tx_sw_queue_total_count--;

            if(descr->bhr_h)
            {
               buffer_header_detach_skb(data->mac_info, descr->bhr_h);
               bhdr_insert_tail(data->mac_info, descr->bhr_h, -1);
            }

            skb = (struct sk_buff *) dragonite_descr->skb_addr;

            drg_check_out_tx_skb(data, skb, TX_SKB_FAIL);
         }
      }
      if(sp->tx_hw_single_queue[tid].qhead != NULL)
      {
         /* remove hw single queue element */
         while((sp->tx_hw_single_queue[tid].qhead != NULL) && (sp->tx_sw_queue[tid].qhead != NULL))
         {
            lptr = sp->tx_hw_single_queue[tid].qhead;
            dragonite_descr = (struct dragonite_tx_descr *) VIRTUAL_ADDR((u32)lptr - sizeof(struct list_head));
            descr = (tx_descriptor *) VIRTUAL_ADDR((u32)dragonite_descr + DRAGONITE_TXDESCR_SIZE);
            skb_list_del(lptr, &(sp->tx_hw_single_queue[tid]));

            lptr = (volatile struct list_head *) &dragonite_descr->private_list;
            skb_list_del(lptr, &(sp->tx_sw_queue[tid]));
            data->tx_sw_queue_total_count--;

            decr_ac_count_safe(data, tid_to_qid(tid));

            if(descr->bhr_h)
            {
               buffer_header_detach_skb(data->mac_info, descr->bhr_h);
               bhdr_insert_tail(data->mac_info, descr->bhr_h, -1);
            }

            skb = (struct sk_buff *) dragonite_descr->skb_addr;

            drg_check_out_tx_skb(data, skb, TX_SKB_FAIL);
         }
      }

   dragonite_txq_unlock();
}

void dragonite_tx_cleanup_pending_skb(struct mac80211_dragonite_data *data, bool check_out_hw_que)
{
   int i;
   acq *q;
   volatile acqe* sw_acqe;
   volatile acqe* acqe;
   struct sk_buff *skb;
   MAC_INFO* info = data->mac_info;

   for(i=0;i<ACQ_NUM;i++)
   {
      q = &info->def_acq[i];
      while(!ACQ_EMPTY(q))
      {
         acqe = ACQE(q, q->rptr);
         sw_acqe = SWACQE(q, q->rptr);

         if(acqe->m.bhr_h != sw_acqe->m.bhr_h)
         {
            DRG_WARN(DRAGONITE_DEBUG_DRIVER_WARN_FLAG, "acqe->m.bhr_h %d != sw_acqe->m.bhr_h %d, recover it when tx done\n", acqe->m.bhr_h, sw_acqe->m.bhr_h);
            acqe->m.bhr_h = sw_acqe->m.bhr_h;
         }
         if(acqe->m.bhr_h == EMPTY_PKT_BUFH)
         {
         }
         else if(acqe->m.bhr_h)
         {
            skb = buffer_header_detach_skb(info, acqe->m.bhr_h);
   
            if(check_out_hw_que)
            {
               dragonite_tx_schedule_done(data, q);
            }

            //dev_kfree_skb(skb);
            drg_check_out_tx_skb(data, skb, TX_SKB_FAIL);

            bhdr_insert_tail(info, acqe->m.bhr_h, -1);
         }
         memset((void *)acqe, 0, ACQE_SIZE);
         memset((void *)sw_acqe, 0, ACQE_SIZE);

         q->rptr++;
      }
   }
}

static void dragonite_tx_skb_lock(bool acquire_ps_lock, bool multicast_frame)
{
   if(acquire_ps_lock)
   {
      if(multicast_frame)
      {
         dragonite_bss_ps_lock();
      }
      else
      {
         dragonite_ps_lock();
      }
   }
   dragonite_txq_lock();
}

static void dragonite_tx_skb_unlock(bool acquire_ps_lock, bool multicast_frame)
{
   dragonite_txq_unlock();

   if(acquire_ps_lock)
   {
      if(multicast_frame)
      {
         dragonite_bss_ps_unlock();
      }
      else
      {
         dragonite_ps_unlock();
      }
   }
}

void dragonite_tx_detach_vif_node(struct mac80211_dragonite_data *data, struct dragonite_vif_priv *vp)
{
   struct dragonite_tx_descr *dragonite_descr;
   volatile struct list_head *lptr, *gptr;
   bool is_ps = false;
   u32 skbinfo_flags;
   volatile acqe* acq_entry;
   struct sk_buff *skb;
   int i, qid;

   /* get lock */
   dragonite_tx_skb_lock(true, false);

   for(i=0;i<TID_MAX+1;i++)/* detach unicast frame and mgmt frame*/
   {
      qid = tid_to_qid(i);
      while((lptr = vp->tx_sw_queue[i].qhead) != NULL)
      {
         dragonite_descr = (struct dragonite_tx_descr *) lptr;
         gptr = (volatile struct list_head *) &dragonite_descr->group_list;
         acq_entry = (volatile acqe *) dragonite_descr->acqe_addr;
         skbinfo_flags = dragonite_descr->skbinfo_flags;
         skb = (struct sk_buff *) dragonite_descr->skb_addr;

         /* deq pri swq */
         skb_list_del(lptr, &(vp->tx_sw_queue[i]));
         data->tx_sw_queue_total_count--;

         decr_ac_count_safe(data, qid);

         if(acq_entry)
         {
            dragonite_descr->vp_addr = 0;
            if(skbinfo_flags & SKB_INFO_FLAGS_MGMT_FRAME)
            {
               /* deq gp hw q*/
               skb_list_del(gptr, &(vp->tx_hw_mgmt_queue[i]));
            }
            else
            {
               panic("dragonite_tx_detach_vif_node: in acq entry should not happed!!!");
            }
         }
         else
         {
            if(skbinfo_flags & SKB_INFO_FLAGS_MGMT_FRAME)
            {
               /* deq gp hw q*/
               skb_list_del(gptr, &(vp->tx_sw_mgmt_queue[i]));
            }
            else
            {
               panic("dragonite_tx_detach_vif_node: should not happed!!!");
            }
            //dev_kfree_skb(skb);
            drg_check_out_tx_skb(data, skb, TX_SKB_FAIL);
         }
      }
   }
   dragonite_tx_skb_unlock(true, false);

   dragonite_tx_skb_lock(true, true);

   if(data->mac_info->bcq_status[vp->bssid_idx]==DRG_BCQ_ENABLE)/* is bss any station ps mode*/
   {
      is_ps = true;
   }
   for(i=0;i<ACQ_NUM-1;i++)
   {
      /* detach by bmcq*/
      while((lptr = vp->tx_sw_gc_queue[i].qhead) != NULL)
      {
         dragonite_descr = (struct dragonite_tx_descr *) lptr;
         gptr = (volatile struct list_head *) &dragonite_descr->group_list;
         acq_entry = (volatile acqe *) dragonite_descr->acqe_addr;
         skbinfo_flags = dragonite_descr->skbinfo_flags;
         skb = (struct sk_buff *) dragonite_descr->skb_addr;

         /* deq pri swq */
         skb_list_del(lptr, &(vp->tx_sw_gc_queue[i]));
         data->tx_sw_bmc_queue_total_count--;

         decr_ac_count_safe(data, i);

         if(acq_entry)
         {
            dragonite_descr->vp_addr = 0;
            /* if broadcast frame still in hw, deq from mixq first, then psq */
            if(data->tx_hw_mixed_queue[i].qhead != NULL)
            {
               skb_list_del(gptr, &(data->tx_hw_mixed_queue[i]));
            }
            else
            {
               skb_list_del(gptr, &(vp->tx_hw_ps_gc_queue));
            }
         }
         else
         {
            if(is_ps)
            {
               /* deq gp hw q*/
               skb_list_del(gptr, &(vp->tx_sw_ps_gc_queue));
            }
            else
            {
               /* deq gp hw q*/
               skb_list_del(gptr, &(data->tx_sw_mixed_queue[i]));
            }
            //dev_kfree_skb(skb);
            drg_check_out_tx_skb(data, skb, TX_SKB_FAIL);
         }
      }
   }

   dragonite_tx_skb_unlock(true, true);
}
void dragonite_tx_detach_node(struct mac80211_dragonite_data *data, struct dragonite_sta_priv *sp)
{
   struct dragonite_tx_descr *dragonite_descr;
   volatile struct list_head *lptr, *gptr;
   volatile struct list_head *head, *next;
   bool is_ps = false;
   u32 skbinfo_flags;
   volatile acqe* acq_entry;
   struct sk_buff *skb;
   int i, qid;

   dragonite_tx_skb_lock(true, false);

   if(dragonite_station_power_saving(data, sp->stacap_index))
   {
      is_ps = true;
   }
   for(i=0;i<TID_MAX+1;i++)
   {
      head = NULL;
      lptr = sp->tx_sw_queue[i].qhead;
      while(sp->tx_sw_queue[i].qhead != NULL)
      {
         next = lptr->next;
         dragonite_descr = (struct dragonite_tx_descr *) lptr;
         gptr = (volatile struct list_head *) &dragonite_descr->group_list;
         acq_entry = (volatile acqe *) dragonite_descr->acqe_addr;
         skbinfo_flags = dragonite_descr->skbinfo_flags;
         skb = (struct sk_buff *) dragonite_descr->skb_addr;

         if(skbinfo_flags & SKB_INFO_FLAGS_USE_SINGLE_QUEUE)
         {
             if(head == NULL)
             {
                 head = lptr;
             }
             goto NEXT;
         }
         /* deq pri swq */
         skb_list_del(lptr, &(sp->tx_sw_queue[i]));
         data->tx_sw_queue_total_count--;

         qid = tid_to_qid(i);

         if(skbinfo_flags & SKB_INFO_FLAGS_PS_POLL_RESPONSE)
         {
            /* ps poll response use AC_VO_QID q */
            qid = AC_VO_QID;
         }

         if(acq_entry)
         {
            decr_ac_count_safe(data, qid);
            /* deq gp hw q*/
            skb_list_del(gptr, &(data->tx_hw_mixed_queue[qid]));
         }
         else
         {

            if(skbinfo_flags & (SKB_INFO_FLAGS_PS_POLL_RESPONSE | SKB_INFO_FLAGS_BAR_FRAME))
            {
               decr_ac_count_safe(data, qid);
               /* deq gp mix q*/
               skb_list_del(gptr, &(data->tx_sw_mixed_queue[qid]));
            }
            else
            {
               if(is_ps)
               {
                  /* deq gp ps misxq */
                  skb_list_del(gptr, &(data->tx_sw_ps_mixed_queue[qid]));
               }
               else
               {
                  decr_ac_count_safe(data, qid);
                  /* deq gp misxq */
                  skb_list_del(gptr, &(data->tx_sw_mixed_queue[qid]));
               }
            }
         }
         if(skbinfo_flags & SKB_INFO_FLAGS_IN_HW)
         {
            dragonite_descr->sp_addr = 0;
         }
         else
         {
            //dev_kfree_skb(skb);
            drg_check_out_tx_skb(data, skb, TX_SKB_FAIL);
         }
NEXT:
         if(next == head)
         {
             break;
         }
         lptr = next;
      }
   }

   dragonite_tx_skb_unlock(true, false);
}

#if defined (DRAGONITE_BEACON_CHECKER)
extern u64 pre_ts_o_val_64;
#endif
void dragonite_tx_schedule(struct mac80211_dragonite_data *data, int vif_idx, int swq_idx, int swq_id)
{
   MAC_INFO *info = data->mac_info;
   struct dragonite_tx_descr *dragonite_descr;
   struct dragonite_vif_priv *vp;
   struct dragonite_sta_priv *sp;
   struct sk_buff *skb;
   u32 skbinfo_flags;
   tx_descriptor *descr;
   volatile struct list_head *lptr;
   struct ieee80211_tx_info *txi;
   int bhr_h, qid;
   buf_header *bhdr;
   acq* q = NULL;
   volatile acqe* sw_acqe;
   volatile acqe* acqe;
   struct ieee80211_hdr *wifi_hdr;
   bool use_bcq = false;
   struct skb_list_head *skb_sw_queue, *skb_hw_queue;
   unsigned long irqflags;
#if defined (DRAGONITE_BEACON_CHECKER)
   u64 val_64;
#endif

   dragonite_txq_lock();

   if((wifi_tx_enable == 0) || (drg_wifi_sniffer == 1))
   {
      goto exit;
   }

   if(swq_idx == -1) /* MIXED Q */
   {
      while(data->tx_sw_mixed_queue[swq_id].qhead)
      {
         lptr = data->tx_sw_mixed_queue[swq_id].qhead;
         dragonite_descr = (struct dragonite_tx_descr *) VIRTUAL_ADDR((u32)lptr - sizeof(struct list_head));
         skb = (struct sk_buff *) dragonite_descr->skb_addr;
         skbinfo_flags = dragonite_descr->skbinfo_flags;
         if(dragonite_descr->skbinfo_flags & SKB_INFO_FLAGS_IN_HW) /* still in hw , tx not done yet*/
         {
            break;
         }
         q = DEF_ACQ(swq_id);
         if(!acq_full(q))
         {
            if(0 > skb_attach_buffer_header(info, skb, &bhr_h))
            {
               break;
            }
            else
            {
               /*dequeue sw linklist head*/
               lptr = data->tx_sw_mixed_queue[swq_id].qhead;
               skb_list_del(lptr, &(data->tx_sw_mixed_queue[swq_id]));
               /*enqueue hw linklist tail*/
               skb_list_add(lptr, &(data->tx_hw_mixed_queue[swq_id]));
               /*fill acq info then kick*/
               bhdr = idx_to_bhdr(info, bhr_h);
               descr = (tx_descriptor *) VIRTUAL_ADDR(bhdr->dptr);
               descr->bhr_t = bhr_h;
               descr->bhr_h = bhr_h;

               if(skbinfo_flags & SKB_INFO_FLAGS_PS_POLL_MORE_DATA)
               {
                  wifi_hdr = (struct ieee80211_hdr *) skb->data;
                  wifi_hdr->frame_control |= cpu_to_le16(IEEE80211_FCTL_MOREDATA);
               }

               dma_cache_wback_inv((unsigned long) CACHED_ADDR(bhdr->dptr), bhdr->offset + skb->len);

               acqe = ACQE(q, q->wptr);
               dragonite_descr->acqe_addr = (u32) acqe;
               dragonite_descr->skbinfo_flags |= SKB_INFO_FLAGS_IN_HW;
               memset((void *)acqe, 0, ACQE_SIZE);
               acqe->m.own = 1;
               acqe->m.bmap = 0;
               acqe->m.try_cnt = 0;
               acqe->m.bhr_h = bhr_h;
               acqe->m.tid = descr->tid;

               sw_acqe = SWACQE(q, q->wptr);
               memset((void *)sw_acqe, 0, ACQE_SIZE);
               sw_acqe->m.bhr_h = bhr_h;

               if(skbinfo_flags & SKB_INFO_FLAGS_PS_POLL_RESPONSE)
               {
                  acqe->m.ps = 1; /* force to send */
               }

               if(dragonite_descr->sp_addr) /* data frame */
               {
                  sp = (struct dragonite_sta_priv *) dragonite_descr->sp_addr;
                  acqe->m.aidx = sp->stacap_index;
               }
               if(dragonite_descr->skbinfo_flags & SKB_INFO_FLAGS_MULTICAST_FRAME) /* data frame with muticast */
               {
                  vp = (struct dragonite_vif_priv *) dragonite_descr->vp_addr;
                  acqe->m.mb = 1;
                  acqe->m.aidx = vp->bssid_idx;
               }
               q->wptr++;
               tx_acq_kick(info, q, q->wptr);
            }
         }
         else{
            break;
         }
      }
   }
   else if(swq_idx == -2) /* BCQ */
   {
      vp = idx_to_vp(data, vif_idx);
      if(vp==NULL)
      {
         goto bcq_done;
      }
      while(vp->tx_sw_ps_gc_queue.qhead)
      {
         lptr = vp->tx_sw_ps_gc_queue.qhead;
         dragonite_descr = (struct dragonite_tx_descr *) VIRTUAL_ADDR((u32)lptr - sizeof(struct list_head));
         skb = (struct sk_buff *) dragonite_descr->skb_addr;
         qid = tid_to_qid(dragonite_descr->tid);
#if defined (DRAGONITE_TX_BCQ_TAIL_DROP)
         if(vp->tx_hw_ps_gc_queue.qcnt > DRAGONITE_TX_BCQ_QSIZE_LIMIT)
         {
            if((vp->tx_sw_ps_gc_queue.qhead != NULL) && (vp->tx_sw_gc_queue[qid].qhead != NULL))
            {
               skb_list_del(lptr, &(vp->tx_sw_ps_gc_queue));

               lptr = (volatile struct list_head *) &dragonite_descr->private_list;
               skb_list_del(lptr, &(vp->tx_sw_gc_queue[qid]));
               data->tx_sw_bmc_queue_total_count--;

               decr_ac_count_safe(data, qid);

               skb = (struct sk_buff *) dragonite_descr->skb_addr;

               drg_check_out_tx_skb(data, skb, TX_SKB_FAIL);
            }
            else
            {
               panic("DRAGONITE: Tail drop bc buffer wrong !!!");
            }
            continue;
         }
#endif
         q = DEF_ACQ(BCQ_QID);
         if(!acq_full(q))
         {
            if(0 > skb_attach_buffer_header(info, skb, &bhr_h))
            {
               goto bcq_done;
            }
            else
            {
               /*dequeue sw linklist head*/
               lptr = vp->tx_sw_ps_gc_queue.qhead;

               skb_list_del(lptr, &(vp->tx_sw_ps_gc_queue));
               /*enqueue hw linklist tail*/
               skb_list_add(lptr, &(vp->tx_hw_ps_gc_queue));
               /*fill bcq info then kick*/
               bhdr = idx_to_bhdr(info, bhr_h);
              
               descr = (tx_descriptor *) VIRTUAL_ADDR(bhdr->dptr);
               descr->bhr_t = bhr_h;
               descr->bhr_h = bhr_h;
#if defined (DRAGONITE_TX_BCQ_TAIL_DROP)
               if((vp->tx_sw_ps_gc_queue.qhead != NULL) && (vp->tx_hw_ps_gc_queue.qcnt < DRAGONITE_TX_BCQ_QSIZE_LIMIT))
#else
               if((vp->tx_sw_ps_gc_queue.qhead != NULL) && (vp->tx_hw_ps_gc_queue.qcnt < ACQ_MAX_QSIZE))
#endif
               {
                  wifi_hdr = (struct ieee80211_hdr *) skb->data;
                  wifi_hdr->frame_control |= cpu_to_le16(IEEE80211_FCTL_MOREDATA);
               }

               dma_cache_wback_inv((unsigned long) CACHED_ADDR(bhdr->dptr), bhdr->offset + skb->len);

               acqe = ACQE(q, q->wptr);
               dragonite_descr->acqe_addr = (u32) acqe;
               memset((void *)acqe, 0, ACQE_SIZE);

               acqe->m.mb = 1;
               acqe->m.aidx = vif_idx;
               acqe->m.ps = 1;
               acqe->m.own = 1;
               acqe->m.bmap = 0;
               acqe->m.try_cnt = 0;
               acqe->m.bhr_h = bhr_h;

               sw_acqe = SWACQE(q, q->wptr);
               memset((void *)sw_acqe, 0, ACQE_SIZE);
               sw_acqe->m.bhr_h = bhr_h;

               q->wptr++;
               use_bcq = true;
            }
         }
         else{
            goto bcq_done;
         }
      }
bcq_done:
      if(use_bcq)
      {
         acqe = ACQE(q, q->wptr);
         memset((void *)acqe, 0, ACQE_SIZE);

         acqe->m.mb = 1;
         acqe->m.aidx = vif_idx;
         acqe->m.ps = 1;
         acqe->m.own = 1;
         acqe->m.bmap = 0;
         acqe->m.try_cnt = 0;
         acqe->m.bhr_h = EMPTY_PKT_BUFH;

         sw_acqe = SWACQE(q, q->wptr);
         memset((void *)sw_acqe, 0, ACQE_SIZE);
         sw_acqe->m.bhr_h = EMPTY_PKT_BUFH;
         q->wptr++;
#if defined (DRAGONITE_BEACON_CHECKER)
         if(pre_ts_o_val_64)
         {
            val_64 = MACREG_READ64(TS_O(0));
            if((val_64 - pre_ts_o_val_64) > BCQ_PROCESS_TIME)
            {
               DRG_WARN(DRAGONITE_DEBUG_DRIVER_WARN_FLAG, "bcq enqueue too slow\n");
            }
         }
#endif
         DRG_NOTICE(DRAGONITE_DEBUG_TX_FLAG, "use bcq and kick !!\n");
         tx_acq_kick(info, q, q->wptr);
      }
   }
   else
   {
      if(swq_idx == DRAGONITE_TX_MGMT_STATION_INDEX)
      {
         if((vif_idx < 0)||(vif_idx >= MAX_BSSIDS))
         {
            panic("!!!Error, vif_idx out of range\n");
         }
         vp = idx_to_vp(data, (u8)vif_idx);
         if(vp==NULL)
         {
            goto exit;;
         }
         skb_sw_queue = &(vp->tx_sw_mgmt_queue[swq_id]);
         skb_hw_queue = &(vp->tx_hw_mgmt_queue[swq_id]);
      }
      else
      {
         if((swq_idx < 0)||(swq_idx >= MAX_STA_NUM))
         {
            panic("!!!Error, sw_queue_index out of range\n");
         }
         sp = idx_to_sp(data, (u8) swq_idx);
         if(sp==NULL)
         {
            goto exit;
         }
         skb_sw_queue = &(sp->tx_sw_single_queue[swq_id]);
         skb_hw_queue = &(sp->tx_hw_single_queue[swq_id]);
      }
      while(skb_sw_queue->qhead)
      {
         lptr = skb_sw_queue->qhead;
         dragonite_descr = (struct dragonite_tx_descr *) VIRTUAL_ADDR((u32)lptr - sizeof(struct list_head));
         skb = (struct sk_buff *) dragonite_descr->skb_addr;
         descr = (tx_descriptor *) VIRTUAL_ADDR((u32)dragonite_descr + DRAGONITE_TXDESCR_SIZE);
         txi = IEEE80211_SKB_CB(skb);
         if(dragonite_descr->skbinfo_flags & SKB_INFO_FLAGS_IN_HW) /* still in hw , tx not done yet*/
         {
            break;
         }
         if(dragonite_descr->sp_addr)
         {
            sp = (struct dragonite_sta_priv *) dragonite_descr->sp_addr;

            spin_lock_irqsave(&sp->splock, irqflags);

            q = SINGLE_Q(sp, swq_id);

            if(q) /* use single queue*/
            {
               if((q->wptr % q->queue_size) != (descr->sn % q->queue_size))
               {
                  DRG_ERROR(DRAGONITE_DEBUG_DRIVER_WARN_FLAG, 
                            "Wrong sequence number q->wptr %% q->queue_size = %d, descr->sn %% q->queue_size = %d, DRAGONITE decide it early %d\n", 
                            (q->wptr % q->queue_size), ((descr->sn) % q->queue_size), descr->sn);
                  acq_dump(q);

                  spin_unlock_irqrestore(&sp->splock, irqflags);

                  panic("DRAGONITE: should not happened !!");
               }
               else
               {   
                  spin_unlock_irqrestore(&sp->splock, irqflags);
               }
            }
            else
            {
               spin_unlock_irqrestore(&sp->splock, irqflags);

               break;
            }
         }
         else
         {
            qid = tid_to_qid(dragonite_descr->tid);
            q = DEF_ACQ(qid);
         }
         if(!acq_full(q))
         {
            if(0 > skb_attach_buffer_header(info, skb, &bhr_h))
            {
               break;
            }
            else
            {
               /*dequeue sw linklist head*/
               lptr = skb_sw_queue->qhead;
               skb_list_del(lptr, skb_sw_queue);
               /*enqueue hw linklist tail*/
               skb_list_add(lptr, skb_hw_queue);
               /*fill acq info then kick*/
               bhdr = idx_to_bhdr(info, bhr_h);
               descr = (tx_descriptor *) VIRTUAL_ADDR(bhdr->dptr);
               descr->bhr_t = bhr_h;
               descr->bhr_h = bhr_h;

               dma_cache_wback_inv((unsigned long) CACHED_ADDR(bhdr->dptr), bhdr->offset + skb->len);

               acqe = ACQE(q, q->wptr);
               dragonite_descr->acqe_addr = (u32) acqe;
               dragonite_descr->skbinfo_flags |= SKB_INFO_FLAGS_IN_HW;
               memset((void *)acqe, 0, ACQE_SIZE);
               acqe->a.own = 1;

               sw_acqe = SWACQE(q, q->wptr);
               memset((void *)sw_acqe, 0, ACQE_SIZE);
               sw_acqe->m.bhr_h = bhr_h;

               if(dragonite_descr->sp_addr)
               {
                  if((txi->flags & IEEE80211_TX_CTL_AMPDU) && !(dragonite_descr->skbinfo_flags & SKB_INFO_FLAGS_FORCE_MPDU_ON_BA_SESSION))
                  {
                     acqe->a.mpdu = 0;
                  }
                  else
                  {
                     acqe->a.mpdu = 1;
                  }
                  acqe->a.try_cnt = 0;
                  acqe->a.pktlen = skb->len - descr->hdr_len;
                  acqe->a.bhr_h = bhr_h;
                  q->wptr++;

#if defined (DRAGONITE_ACQ_DELAY_KICK)
                  if(q->tx_count >= drg_tx_acq_delay_max_cnt)
                  {
                     hrtimer_cancel(&q->tx_acq_timer);
                     tx_acq_kick(info, q, (descr->sn+1) % 4096);
                  }
                  else
                  {
                     hrtimer_cancel(&q->tx_acq_timer);

                     spin_lock_irqsave(&dragonite_acq_lock, irqflags);
                     q->tx_count++;
                     spin_unlock_irqrestore(&dragonite_acq_lock, irqflags);

                     hrtimer_start(&q->tx_acq_timer,
                                   ktime_set(0, 1000 * drg_tx_acq_delay_interval),  /* after 1 us */
                                    HRTIMER_MODE_REL);
                  }
#else
                  if(0 > tx_acq_kick(info, q, (descr->sn+1) % 4096))
                  {
                     /* ampdu tear down, no need to free buffer header, dragonite_tx_ampdu_q_detach will handle */
                     break;
                  }
#endif
               }
               else
               {
                  acqe->m.bhr_h = bhr_h;
                  acqe->m.tid = swq_id;
                  acqe->m.aidx = 0;
                  acqe->m.ps = 1;
                  if(dragonite_descr->skbinfo_flags & SKB_INFO_FLAGS_MULTICAST_FRAME)
                  {
                     acqe->m.mb = 1;
                  }
                  q->wptr++;
                  tx_acq_kick(info, q, q->wptr);
               }
            }
         }
         else
         {
            break;
         }
      }
   }
exit:
   dragonite_txq_unlock();
}

static bool dragonite_private_valid(struct dragonite_vif_priv *vp, struct dragonite_sta_priv *sp)
{
   if(sp != NULL)
   {
      if(!dragonite_sta_valid(sp))
      {
         goto unvalid;
      }
   }
   if(vp != NULL)
   {
      if(!dragonite_vif_valid(vp))
      {
         goto unvalid;
      }
   }
   return true;

unvalid:
   return false;
}

static bool over_ac_limit(struct mac80211_dragonite_data *data, int qid)
{
   if(data->tx_per_ac_count[qid] > dragonite_tx_per_ac_max_buf)
   {
      return true;
   }
   else
   {
      return false;
   }
}

static bool dragonite_tx_idle(struct mac80211_dragonite_data *data, struct dragonite_sta_priv *sp, struct skb_info *skbinfo)
{
   bool ret = true;

   if(IS_MGMT_FRAME(skbinfo->flags) || IS_SPECIAL_DATA_FRAME(skbinfo->flags))
   {
      /* CAN NOT DROP */
   }
   else if(over_ac_limit(data, tid_to_qid(skbinfo->tid)))
   {
      ret = false;
   }

   /* if buffer count more than DRG_MAX_BUFFER_COUNT, DROP it */
   if((data->tx_sw_queue_total_count + data->tx_sw_bmc_queue_total_count) > DRG_MAX_BUFFER_COUNT)
   {
      DRG_WARN(DRAGONITE_DEBUG_DRIVER_WARN_FLAG, "buffer count more than %d pkts, drop it !!\n", DRG_MAX_BUFFER_COUNT);
      ret = false;
   }

   return ret;
}

void incr_ssn(u16 *ssn)
{
   if(*ssn < 4095)
   {
      (*ssn)++;
   }
   else if(*ssn == 4095)
   {
      *ssn = 0;
   }
   else
   {
      panic("DRAGONITE: incr_ssn wrong");
   }
}

int dragonite_tx_get_ssn(struct dragonite_vif_priv *vp, struct dragonite_sta_priv *sp, struct skb_info *skbinfo)
{
   u16 ssn = INVALID_SSN;
   unsigned long irqflags;

   if(sp != NULL)
   {
      if(!(skbinfo->flags & (SKB_INFO_FLAGS_MGMT_FRAME | SKB_INFO_FLAGS_MULTICAST_FRAME)))
      {
         if(dragonite_sta_valid(sp))
         {
            spin_lock_irqsave(&sp->splock, irqflags);

            if(IS_AMPDU(sp))
            {
               if(SINGLE_Q(sp, skbinfo->tid) != NULL) // check AMPDU setup or not
               {
                  skbinfo->flags |= SKB_INFO_FLAGS_USE_SINGLE_QUEUE;
                  ssn = AGGR_SSN(sp, skbinfo->tid);
                  incr_ssn(&sp->drg_tx_aggr.ssn[skbinfo->tid]);
               }
            }
            if(IS_INVALID_SSN(ssn))
            {
               ssn = NORMAL_SSN(sp);
               incr_ssn(&sp->ssn);
            }

            spin_unlock_irqrestore(&sp->splock, irqflags);
         }
      }
   }
   else if(vp != NULL)
   {
      if(dragonite_vif_valid(vp))
      {
         spin_lock_irqsave(&vp->vplock, irqflags);

         ssn = NORMAL_SSN(vp);
         incr_ssn(&vp->ssn);

         spin_unlock_irqrestore(&vp->vplock, irqflags);
      }
   }
   return ssn;
}

int dragonite_tx_skb(struct mac80211_dragonite_data *data, struct sk_buff *skb, struct skb_info *skbinfo, struct dragonite_vif_priv *vp, struct dragonite_sta_priv *sp)
{
   MAC_INFO *info = data->mac_info;
   int mis_align, sta_index = -1, tid = 0, qid = 0;
   tx_descriptor *descr;
   struct list_head *lptr;
   struct dragonite_tx_descr *dragonite_descr;
   struct ieee80211_tx_info *txi = IEEE80211_SKB_CB(skb);
   bool acquire_ps_lock = (skbinfo->flags & SKB_INFO_FLAGS_BAR_FRAME) ? 0 : 1;
   bool ps_mode = false, use_bcq = false;
   struct skb_list_head *skb_queue;
   u16 ssn = 0;

   drg_tx_stats.tx_count++;

   if(!dragonite_tx_idle(data, sp, skbinfo))
   {
      goto drop;
   }

   dragonite_tx_skb_lock(acquire_ps_lock, IS_MULTICAST_FRAME(skbinfo->flags));

   if(!dragonite_private_valid(vp, sp) || (wifi_tx_enable == 0) || (drg_wifi_sniffer == 1))
   {
      dragonite_tx_skb_unlock(acquire_ps_lock, IS_MULTICAST_FRAME(skbinfo->flags));

      goto drop;
   }

   if(!(skbinfo->flags & SKB_INFO_FLAGS_USE_ORIGINAL_SEQUENCE_NUMBER))
   {
      ssn = dragonite_tx_get_ssn(vp, sp, skbinfo);
      if(!IS_INVALID_SSN(ssn))
      {
         skbinfo->ssn = ssn;
         /* replace on skb->data sequence number */
         dragonite_tx_replace_ssn(skb, ssn);
      }
      else
      {
         /* use mac80211 originally assigned sequence number */
      }
   }

   /* align->*------------------------------------------------ */
   /*        | list head  | tx descr | mis_align | skb->data  |*/
   /*        |  64btyes   |  48btyes | 0~32bytes |            |*/
   /*        ------------------------------------------------- */
   
   /*Filled tx descriptor*/
   mis_align =  ((unsigned long)(skb->data - TXDESCR_SIZE) % L1_CACHE_BYTES);
   descr = (tx_descriptor *) VIRTUAL_ADDR(((u32) skb->data - TXDESCR_SIZE) - mis_align);

   tx_descriptor_fill(info, skb, descr, 0, skbinfo, vp, sp);

   tx_rate_adjust(txi, skbinfo);

   tx_rate_fill(txi, descr);

   DRG_NOTICE(DRAGONITE_DEBUG_TX_RATE_FLAG, "retry rate %04x %04x %04x %04x\n",
              descr->tx_rate[0], descr->tx_rate[1], descr->tx_rate[2], descr->tx_rate[3]);

   dragonite_descr = (struct dragonite_tx_descr *) VIRTUAL_ADDR((u32)descr - DRAGONITE_TXDESCR_SIZE);
   dragonite_descr->skb_addr = (u32) skb;
   dragonite_descr->vp_addr = (u32) vp;
   dragonite_descr->acqe_addr = 0;
   if(sp)
   {
      dragonite_descr->sp_addr = (u32) sp;
   }
   else
   {
      dragonite_descr->sp_addr = 0;
   }
   dragonite_descr->skbinfo_flags = (u32) skbinfo->flags;

   if(skbinfo->flags & SKB_INFO_FLAGS_BAR_FRAME)
   {
      skbinfo->tid = EDCA_HIGHEST_PRIORITY_TID;
   }
   dragonite_descr->tid = skbinfo->tid;

   drg_check_in_tx_skb(data, skb);

   /* enqueue in private list*/
   lptr = (struct list_head *) &dragonite_descr->private_list;

   tid = dragonite_descr->tid;
   if(dragonite_descr->skbinfo_flags & SKB_INFO_FLAGS_MGMT_FRAME) /* No.33 station (mgmt station) */
   {
      dragonite_descr->sp_addr = 0;
      sta_index = DRAGONITE_TX_MGMT_STATION_INDEX;
      skb_list_add(lptr, &(vp->tx_sw_queue[tid]));
      data->tx_sw_queue_total_count++;
   }
   else /* data frame or bufferable mgmt frame*/
   {
      if(sp && !(dragonite_descr->skbinfo_flags & SKB_INFO_FLAGS_MULTICAST_FRAME))
      {
         sta_index = sp->stacap_index;
         skb_list_add(lptr, &(sp->tx_sw_queue[tid]));
         data->tx_sw_queue_total_count++;
      }
      else if(dragonite_descr->skbinfo_flags & SKB_INFO_FLAGS_MULTICAST_FRAME)/* broadcast and muticast frame */
      {
         qid = tid_to_qid(tid);
         skb_list_add(lptr, &(vp->tx_sw_gc_queue[qid]));
         data->tx_sw_bmc_queue_total_count++;
      }
      else /* other packet, without MGMT frame, Muticast frame, Connected station; Solu: use MGMT queue */
      {

         dragonite_descr->sp_addr = 0;
         dragonite_descr->skbinfo_flags |= SKB_INFO_FLAGS_MGMT_FRAME;

         sta_index = DRAGONITE_TX_MGMT_STATION_INDEX;
         skb_list_add(lptr, &(vp->tx_sw_queue[tid]));
         data->tx_sw_queue_total_count++;
      }
   }

   /* enqueue in group list*/
   lptr = (struct list_head *) &dragonite_descr->group_list;

   if(dragonite_descr->skbinfo_flags & SKB_INFO_FLAGS_MGMT_FRAME)
   {
   }
   else if(sp && !(skbinfo->flags & SKB_INFO_FLAGS_MULTICAST_FRAME))
   {
      if(dragonite_station_power_saving(data, sta_index))
      {
         if(!(dragonite_descr->skbinfo_flags & SKB_INFO_FLAGS_BAR_FRAME))
         {
            ps_mode = true;
         }
      }
   }
   else if(skbinfo->flags & SKB_INFO_FLAGS_MULTICAST_FRAME)
   {
      if(info->bcq_status[vp->bssid_idx]==DRG_BCQ_ENABLE)
      {
         use_bcq = true;
      }
   }

   if(dragonite_descr->skbinfo_flags & (SKB_INFO_FLAGS_MGMT_FRAME | SKB_INFO_FLAGS_USE_SINGLE_QUEUE))
   {
      if(ps_mode)
      {
         if(sta_index == DRAGONITE_TX_MGMT_STATION_INDEX)
         {
            panic("Error sta_index %d when power saving\n", sta_index);
         }
         /* move to ps single q*/
         skb_list_add(lptr, &(sp->tx_sw_ps_single_queue[tid]));
         if(!(dragonite_descr->skbinfo_flags & SKB_INFO_FLAGS_SPECIAL_DATA_FRAME))
         {
            sp->tx_ps_buffer_skb_count++;
         }
         else
         {
            DRG_WARN(DRAGONITE_DEBUG_DRIVER_WARN_FLAG, "should not happen %s %d\n", __func__, __LINE__);
            panic("single queue should not has SKB_INFO_FLAGS_SPECIAL_DATA_FRAME");
         }
      }
      else
      {
         if(sta_index == DRAGONITE_TX_MGMT_STATION_INDEX)
         {
            skb_queue = &(vp->tx_sw_mgmt_queue[tid]);
         }
         else
         {
            skb_queue = &(sp->tx_sw_single_queue[tid]);
         }

         incr_ac_count_safe(data, tid_to_qid(skbinfo->tid));

         skb_list_add(lptr, skb_queue);  
      }
   }
   else
   {
      if(ps_mode)
      {
         /* move to ps mixed q*/
         qid = tid_to_qid(tid);
         skb_list_add(lptr, &(data->tx_sw_ps_mixed_queue[qid]));
         if(!(dragonite_descr->skbinfo_flags & SKB_INFO_FLAGS_SPECIAL_DATA_FRAME))
         {
            sp->tx_ps_buffer_skb_count++;
         }
      }
      else if(use_bcq)
      {
         incr_ac_count_safe(data, tid_to_qid(skbinfo->tid));
         /* move to ps bc q*/
         skb_list_add(lptr, &(vp->tx_sw_ps_gc_queue));
      }
      else
      {
         qid = tid_to_qid(tid);

         incr_ac_count_safe(data, qid);

         skb_list_add(lptr, &(data->tx_sw_mixed_queue[qid]));
      }
   }

   dragonite_tx_skb_unlock(acquire_ps_lock, IS_MULTICAST_FRAME(skbinfo->flags));

   if(ps_mode)
   {
      dragonite_tx_ps_buffer_adjust(data, sta_index);
   }

   if(!ps_mode && !use_bcq)
   {
      if(dragonite_descr->skbinfo_flags & (SKB_INFO_FLAGS_MGMT_FRAME | SKB_INFO_FLAGS_USE_SINGLE_QUEUE))
      {
         dragonite_tx_schedule(data, vp->bssid_idx, sta_index, tid);
      }
      else
      {
         dragonite_tx_schedule(data, NOT_USE_VIF_IDX, -1, qid);
      }
   }

   return 0;

drop:
   return -TX_SKB_DROP;
}

char err_msg[64], err_msg2[64];
void assert_not_empty_queue(struct mac80211_dragonite_data *data, struct dragonite_sta_priv *sp,
                            struct dragonite_vif_priv *vp, u8 que_idx, u8 tid, u8 acqid)
{
    struct skb_list_head *queue;

    queue = idx_to_queue(data, sp, vp, que_idx, tid, acqid);
    ASSERT((queue != NULL),"assert_not_empty_queue: queue is NULL\n");

    if(queue_params[que_idx].use_tid)
    {
        sprintf(err_msg, "check_hw_queue: %s[%d] is not NULL\n", queue_params[que_idx].queue_name, tid); 
        sprintf(err_msg2, "check_hw_queue: %s[%d] is not empty\n", queue_params[que_idx].queue_name, tid); 
    }
    else if(queue_params[que_idx].use_acqid)
    {
        sprintf(err_msg, "check_hw_queue: %s[%d] is not NULL\n", queue_params[que_idx].queue_name, acqid); 
        sprintf(err_msg2, "check_hw_queue: %s[%d] is not empty\n", queue_params[que_idx].queue_name, acqid); 
    }
    else
    {
        sprintf(err_msg, "check_hw_queue: %s is not NULL\n", queue_params[que_idx].queue_name); 
        sprintf(err_msg2, "check_hw_queue: %s is not empty\n", queue_params[que_idx].queue_name); 
    }
    ASSERT(((queue->qhead) == NULL), err_msg);
    ASSERT(((queue->qcnt) <= 0), err_msg2);
}

void check_hw_queue(struct mac80211_dragonite_data *data)
{
    int idx, id;
    struct dragonite_vif_priv *vp;

    dragonite_txq_lock();
    for(id = 0; id < 5; id++)
    {
        assert_not_empty_queue(data, NULL, NULL, SW_MIXED, -1, id);
        assert_not_empty_queue(data, NULL, NULL, HW_MIXED, -1, id);
        assert_not_empty_queue(data, NULL, NULL, SW_PS_MIXED, -1, id);
    }
    for(idx = 0; idx < MAC_MAX_BSSIDS; idx++)
    {
        vp = idx_to_vp(data, idx);
        if(vp == NULL)
        {
            continue;
        }

        for(id = 0; id < (ACQ_NUM - 1); id++)
        {
            assert_not_empty_queue(NULL, NULL, vp, VP_SW_GC, -1, id);
        }
        assert_not_empty_queue(NULL, NULL, vp, VP_SW_PS_GC, -1, -1);
        assert_not_empty_queue(NULL, NULL, vp, VP_HW_PS_GC, -1, -1);
        for(id = 0; id < (TID_MAX + 1); id++)
        {
            assert_not_empty_queue(NULL, NULL, vp, VP_SW, id, -1);
            assert_not_empty_queue(NULL, NULL, vp, VP_SW_MGMT, id, -1);
            assert_not_empty_queue(NULL, NULL, vp, VP_HW_MGMT, id, -1);
        }
    }
    ASSERT(((data->mac_info) != NULL), "check_hw_queue: info is NULL\n");
    dragonite_txq_unlock();

    return;
}

bool dragonite_has_tx_pending(struct mac80211_dragonite_data *data)
{
   bool pending;

   dragonite_txq_lock();

   if(data->tx_sw_queue_total_count + data->tx_sw_bmc_queue_total_count > 0)
   {
      pending = true;
   }
   else
   {
      pending = false;
   }

   dragonite_txq_unlock();

   return pending;
}
