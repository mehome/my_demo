#include <wla_cfg.h>
#include <os_compat.h>
#include <mac.h>
#include <mac_tables.h>
#include <linux/skbuff.h>
#include <mac_ctrl.h>
#include <dragonite_main.h>
#include <resources.h>
#include <packet.h>
#include <beacon.h>
#include <tx.h>
#include "panther_debug.h"
#include "dragonite_common.h"
#include "mac_regs.h"
#include "rf.h"

typedef union 
{
    struct
    {
        u16     ch_offset:2;
        u16     txcnt:3;
        u16     sgi:1;
        u16     format:2;
        u16     ht:1;
        u16     tx_rate:7;
    };
    u16     val;
} __attribute__((__packed__)) rate_config;
#define	EIO	5
#define _11B_1M		0x0
#define _OFDM_6M	0x0b
rate_config beacon_txrate;


void dragonite_beacon_setup(unsigned short beacon_interval, unsigned char dtim_period)
{
#define LMAC_DEFAULT_SLOT_TIME	1024
#define LMAC_DEFAULT_BEACON_TX_TIMEOUT	(30*1000)

//#define PRE_TBTT_TIME(beacon_interval)	((beacon_interval * 1) /3)

#define PRE_HW_TBTT_DEFAULT_TIME	0x10

	DRG_NOTICE(DRAGONITE_DEBUG_NETWORK_INFO_FLAG, "dragonite_beacon_setup beacon_interval %d, dtim_period %d\n", beacon_interval, dtim_period);

	MACREG_WRITE32(TS0_DTIM, dtim_period);
	
	if(beacon_interval <= 6)
	{
		MACREG_UPDATE32(GLOBAL_PRE_TBTT, GLOBAL_PRE_TBTT_DEFAULT_DIV_S(beacon_interval) & PRE_TBTT_INTERVAL_S, PRE_TBTT_INTERVAL_S);
		MACREG_WRITE32(TS0_BE, (beacon_interval << 16) | (((beacon_interval/2)&0xffff)));
	}
	else
	{
		MACREG_UPDATE32(GLOBAL_PRE_TBTT, GLOBAL_PRE_TBTT_DEFAULT_DIV_S(beacon_interval) & PRE_TBTT_INTERVAL_S, PRE_TBTT_INTERVAL_S);
		MACREG_WRITE32(TS0_BE, (beacon_interval << 16)|(((beacon_interval/3)&0xffff) - 1));
	}
}

void dragonite_beacon_start(unsigned long beacon_bitrate)
{
	DRG_DBG("dragonite_beacon_start\n");

	/*enable LMAC beacon timer*/
	MACREG_UPDATE32(LMAC_CNTL, LMAC_CNTL_BEACON_ENABLE, LMAC_CNTL_BEACON_ENABLE);

	MACREG_UPDATE32(BEACON_CONTROL, (beacon_bitrate << 8) | BEACON_CONTROL_ENABLE,
			BEACON_CONTROL_TX_IDLE | BEACON_CONTROL_RATE | BEACON_CONTROL_ENABLE);

	/*enable TS timer*/
	MACREG_WRITE32(TS0_CTRL, TS_ENABLE);
	
	MACREG_UPDATE32(TS_INT_MASK, 0 , PRE_TBTT_SW_TS0);
}

void dragonite_beacon_stop(void)
{
	DRG_DBG("dragonite_beacon_stop\n");

	MACREG_UPDATE32(TS_INT_MASK, 0x1, PRE_TBTT_SW_TS0);

	MACREG_WRITE32(TS0_CTRL, 0);

	MACREG_UPDATE32(BEACON_CONTROL, 0, BEACON_CONTROL_ENABLE);
	MACREG_UPDATE32(LMAC_CNTL, 0x0, LMAC_CNTL_BEACON_ENABLE);
}

static inline int bhdr_to_idx(MAC_INFO *info, buf_header *bhdr)
{
	return (bhdr - &info->buf_headers[0]);
}

int dragonite_tx_beacons(struct ieee80211_hw *hw, struct sk_buff *skb_list[])
{
	int i;
	int ret = -EIO;

	struct mac80211_dragonite_data *data = hw->priv;
	struct dragonite_vif_priv *vp;
	MAC_INFO* info = data->mac_info;

	ssq_tx_descr* beacon_descr = NULL;
	int beacon_tx_descr_index = 0;

	struct sk_buff *skb = NULL;

	buf_header* tx_hdr;
	int beacon_bh_index = 0;
	unsigned long irqflags;	

	/* Wait until HW idle if beacon queue is enable already */
	if((MACREG_READ32(BEACON_CONTROL) & BEACON_CONTROL_ENABLE))
	{
		/* wait till beacon HW is idle */
		if(!(BEACON_CONTROL_TX_IDLE & MACREG_READ32(BEACON_CONTROL)))
		{
			DRG_DBG("Waiting BEACON to idle, shall not happen! (status 0x%x, jiffies %ld)\n"
					, MACREG_READ32(BEACON_TX_STATUS), jiffies);
			DRG_DBG("skip one BEACON TX\n");
			return -EBUSY;
		}
	}

	/* reviewed and confirmed that no lock is required */
	//spin_lock_irqsave(&data->mac_info->dragonite_lock, irqflags);

	if(data->beacon_pending_tx_bitmap 
			&& (data->beacon_pending_tx_bitmap != (MACREG_READ32(BEACON_TX_STATUS) & BEACON_TX_STATUS_RESULT)))
	{
		DRG_DBG("BEACON TX failure, TX bitmap 0x%x, status 0x%x\n", data->beacon_pending_tx_bitmap
				,MACREG_READ32(BEACON_TX_STATUS));
	}

	data->beacon_pending_tx_bitmap = 0;

	if(!info->beacon_tx_descriptors)
	{
		DRG_DBG("no tx descriptors");
		ret = -EBUSY;
		return ret;
	}

	for(i=0;i<MAC_MAX_BSSIDS;i++)
	{
        vp = idx_to_vp(data, (u8) i);
		if(vp)
		{
			spin_lock_irqsave(&vp->vplock, irqflags);
			if(dragonite_vif_valid(vp))
			{
         		if(skb_list[i])
         		{
         			ASSERT( (beacon_tx_descr_index < info->beacon_tx_descr_count), "beacon_tx_descr_index overflow\n");
         			ASSERT( (beacon_bh_index < info->beacon_q_buffer_hdr_count), "beacon_bh_index overflow\n");

         			skb = skb_list[i];

         			if(data->dragonite_bss_info[i].beacon_tx_buffer_ptr==NULL)
         			{
						data->dragonite_bss_info[i].beacon_tx_buffer_ptr = (u32 *)mac_malloc(info, 1024 * sizeof(u32), MAC_MALLOC_ATOMIC | MAC_MALLOC_BZERO  | MAC_MALLOC_UNCACHED);

						if(data->dragonite_bss_info[i].beacon_tx_buffer_ptr == NULL)
						{
							goto fail;
						}
         			}

         			if(skb->len > 1024) 
         			{
         				DRG_WARN(DRAGONITE_DEBUG_DRIVER_WARN_FLAG, "DRAGONITE: Beacon length too large !!");
         				
         				memcpy((void *)UNCACHED_ADDR(data->dragonite_bss_info[i].beacon_tx_buffer_ptr), skb->data, 1024);

                    } 
                    else
                    {
						memcpy((void *)UNCACHED_ADDR(data->dragonite_bss_info[i].beacon_tx_buffer_ptr), skb->data, skb->len);

                    }
                                
         			beacon_descr = &info->beacon_tx_descriptors[beacon_tx_descr_index];
         			beacon_tx_descr_index++;
         			memset(beacon_descr, 0, sizeof(ssq_tx_descr));
         			beacon_descr->next_pointer = (u32) PHYSICAL_ADDR(&info->beacon_tx_descriptors[beacon_tx_descr_index]);

         			tx_hdr = &info->beacon_q_buf_headers[beacon_bh_index];
         			beacon_bh_index++;
         			memset(tx_hdr, 0, sizeof(buf_header));

         			tx_hdr->next_index = 0;

         			ASSERT( (( ((u32)data->dragonite_bss_info[i].beacon_tx_buffer_ptr) % 4) == 0), "mis-aligned beacon data");  	// try change hw->extra_tx_headroom if this one hit

         			tx_hdr->dptr = PHYSICAL_ADDR(data->dragonite_bss_info[i].beacon_tx_buffer_ptr);

         			//TODO tx_hdr->offset_h = 0;
         			tx_hdr->offset = 0;
         			tx_hdr->ep = 1;

					if(skb->len > 1024)
					{
						tx_hdr->len = 1024;
					}
					else
					{
						tx_hdr->len = skb->len;
					}

         			/* setting BEACON TX descr */
         			beacon_descr->tx_descr.bssid = i;		/* the bssid of skbs are input in-order */
         			//beacon_descr->tx_descr.ifs = MAC_IFS_DIFS;  /* hardcoded to PIFS in HW */
         			beacon_descr->tx_descr.ack_policy = MAC_ACK_POLICY_NOACK;
         			beacon_descr->tx_descr.dis_duration = 0;  /* enable duration */
         			beacon_descr->tx_descr.ins_ts = 1;
         			beacon_descr->tx_descr.bc = 1;            /* groupcast */

         			/* XXX: we shall use the rate specified by upper-layer */

         			//#if defined(BEACON_TXRATE_MANUAL_CONFIG)
         #if 1
         			if(beacon_txrate.val)
         			{
         				beacon_descr->tx_descr.u.beacon.ch_offset = beacon_txrate.ch_offset;
         				beacon_descr->tx_descr.u.beacon.format = beacon_txrate.format;
         			}
         			else
         #endif
         			{
         				/* sending on the 20Mhz primary channel for HT40+/HT40- */
         				beacon_descr->tx_descr.u.beacon.ch_offset = data->primary_ch_offset;

         				/* we fixed to use 11B 1Mhz / 11A 6Mhz for 2/5 GHz band currently */
         				if(data->curr_band == IEEE80211_BAND_2GHZ)
         				{    
         					beacon_descr->tx_descr.u.beacon.format = FMT_11B;
         				}
         				else
         				{    
         					beacon_descr->tx_descr.u.beacon.format = FMT_NONE_HT;
         				}
         			}

					if(skb->len > 1024)
					{
						beacon_descr->tx_descr.pkt_length = 1024;
					}
					else
					{
						beacon_descr->tx_descr.pkt_length = skb->len;
					}

         			beacon_descr->tx_descr.frame_header_head_index = bhdr_to_idx(info, tx_hdr);
         			data->beacon_pending_tx_bitmap |= (0x1 << i);
         		}
			}
fail:
			spin_unlock_irqrestore(&vp->vplock, irqflags);
		}
	}

	if(beacon_descr)
	{
		beacon_descr->tx_descr.eor = 1;

		MACREG_WRITE32(BEACON_TXDSC_ADDR, PHYSICAL_ADDR(&info->beacon_tx_descriptors[0]));

		if(!(MACREG_READ32(BEACON_CONTROL) & BEACON_CONTROL_ENABLE)) 
		{
			/* start the beacon queue if it is not started yet! */
			MACREG_UPDATE32(BEACON_CONTROL, BEACON_CONTROL_ENABLE, BEACON_CONTROL_ENABLE);
		}

		/* write 1 clear to IDLE bit, set HW to busy , trigger the HW */
		MACREG_WRITE32(BEACON_CONTROL, MACREG_READ32(BEACON_CONTROL));
	}

	ret = 0;

	//spin_unlock_irqrestore(&data->mac_info->dragonite_lock, irqflags);

	return ret;
}

void mac_config_tbtt_intr(u32 tsf_idx, u32 beacon_interval)
{
   if(beacon_interval <= 6) 
   {
      MACREG_UPDATE32(GLOBAL_PRE_TBTT, (GLOBAL_PRE_TBTT_DEFAULT_DIV(beacon_interval) << 16) & PRE_TBTT_INTERVAL, PRE_TBTT_INTERVAL);
      MACREG_WRITE32(TS_BE(tsf_idx), (beacon_interval << 16)|((((beacon_interval)/2)&0xffff)));
   }
   else
   {
      MACREG_UPDATE32(GLOBAL_PRE_TBTT, (GLOBAL_PRE_TBTT_DEFAULT_DIV(beacon_interval) << 16) & PRE_TBTT_INTERVAL, PRE_TBTT_INTERVAL);
      MACREG_WRITE32(TS_BE(tsf_idx), (beacon_interval << 16)|((((beacon_interval)/3)&0xffff) - 1));
   }

   MACREG_UPDATE32(TS_INT_MASK, 0, PRE_TBTT_HW_TS3);
}

void dragonite_cfg_start_tsync(u32 tsf_idx, u32 beacon_interval, u32 dtim_interval, u32 timeout)
{
	DRG_NOTICE(DRAGONITE_DEBUG_NETWORK_INFO_FLAG, "start to sync timer tsf_idx %d, beacon_interval %d, dtim_interval %d, timeout %d\n",
			   tsf_idx , beacon_interval , dtim_interval , timeout);
   if(timeout == 0)	/* add for IBSS mode to fill atim window (TU) */
      timeout = ((beacon_interval/3) & 0xffff) - 1;

   //MACREG_UPDATE32(TS_INT_MASK, mask, mask);
   MACREG_UPDATE32(TS_ERROR_INT_MASK, 0, (0x01 << (tsf_idx)));

   /* JH modify beacon sync window to entire TBTT. So we resume original duration of timeout(1/3 TBTT). */
   MACREG_WRITE32(TS_BE(tsf_idx), ((beacon_interval << 16)|(timeout)));

   //MACREG_WRITE32(TS_DTIM(tsf_idx), dtim_interval);

   /* preset/reset TSF counter */
#if 1
   MACREG_WRITE64(TS_NEXT_TBTT(tsf_idx), (TEST_TSF_START * tsf_idx));
   MACREG_WRITE64(TS_O(tsf_idx), (TEST_TSF_START * tsf_idx) - (beacon_interval/2));
#else
   MACREG_WRITE64(TS_NEXT_TBTT(tsf_idx), 0x0ULL);
   MACREG_WRITE64(TS_O(tsf_idx), 0x0ULL);
#endif

   MACREG_WRITE32(TS_INACC_LIMIT(tsf_idx), 10);

   //bss_beacon_interval[tsf_idx] = beacon_interval;

   /* clear receive & loss counter for client mode check ap alive */
   MACREG_WRITE32(TS_BEACON_COUNT(tsf_idx), 0);

   mac_config_tbtt_intr(tsf_idx, beacon_interval);

   MACREG_WRITE32(TS_CTRL(tsf_idx), TS_ENABLE);
}

#if defined (DRAGONITE_BEACON_CHECKER)
u64 pre_ts_o_val_64 = 0;
#endif
void dragonite_tsf_intr_handler(struct mac80211_dragonite_data *data)
{
   u32 ts_status, ts_err_status, bcn_interval_32;
   struct dragonite_vif_priv *vp;
   int i, idx;
#if defined (DRAGONITE_BEACON_CHECKER)
   u32 beacon_interval;
#endif

	union {
		u64 val_64;
		u32	val_32[2];
	} ts_o, ts_o_temp, ts_o_final;

   ts_status = MACREG_READ32(TS_INT_STATUS);
   MACREG_WRITE32(TS_INT_STATUS, ts_status);
   ts_err_status = MACREG_READ32(TS_ERROR_INT_STATUS);
   MACREG_WRITE32(TS_ERROR_INT_STATUS, ts_err_status);
   //printk(KERN_DEBUG "====> ts_status %08x ts_err_status %08x\n", ts_status, ts_err_status);

   ts_o.val_64 = MACREG_READ64(TS_O(0));
   //printk("ts_0 HL %08x %08x\n", ts_o.val_32[0], ts_o.val_32[1]);
   if(PRE_TBTT_SW & ts_status)
   {
       if(ts_status & PRE_TBTT_SW_TS0) /* Because AP must be TSF0 */
       {
	       for(i=0;i<MAX_BSSIDS;i++)
	       {
		       if(data->dragonite_bss_info[i].dut_role == ROLE_AP)//should be change, for any AP
		       {
#if defined (DRAGONITE_BEACON_CHECKER)
				   beacon_interval = ((MACREG_READ32(TS0_BE) & 0xFFFF0000UL) >> 16);
				   if(pre_ts_o_val_64)/* check ts_o and tsf interrupt on time or not */
				   {
					   if((ts_o.val_64 - pre_ts_o_val_64) > (TBTT_INTERVAL(beacon_interval) + TBTT_INTERRUPT_REACTIVE_TOLERANT))
					   {
						   DRG_WARN(DRAGONITE_DEBUG_DRIVER_WARN_FLAG, "tsf interrupt reactive too slow %llu\n", ts_o.val_64 - pre_ts_o_val_64);
					   }
				   }
				   pre_ts_o_val_64 = ts_o.val_64;
#endif
			       dragonite_beacon((unsigned long)data->hw);
			       break;
		       }
	       }
       }
	   
#if 0
      for(i=0;i<MAX_BSSIDS;i++)
      {
         if(( 0x01 << i ) & ts_status)
         {
            if(tsf_mode[i]==TSF_MODE_STA)
            {
               dump_ts_o();
               printf("STA PRE-TBTT, TSF-%d\n", i);
            }
            else
            {
               dump_ts_o();

               if(tsf_mode[i]==TSF_MODE_IBSS)
               {
                  printf("IBSS PRE-TBTT, TSF-%d\n", i);

                  printf("IBSS beacon rx %d, tx %d\n", MACREG_READ32(GLOBAL_BEACON)>>24, (MACREG_READ32(GLOBAL_BEACON)>>16) & 0xff);

                  /* set next beacon backoff period */
                  MACREG_UPDATE32(GLOBAL_BEACON, TEST_TSF_IBSS_BACKOFF, BEACON_BACKOFF);
               }
               else
               {
                  printf("AP PRE-TBTT, TSF-%d\n", i);
               }

               /* AP mode: PRE-TBTT interrupt for Beacon TX */
               if(!(MACREG_READ32(BEACON_CONTROL) & BEACON_CONTROL_TX_IDLE))
               {
                  MACREG_WRITE32(TS_INT_STATUS, ts_status);

                  printf("Beacon is busy, skip this TBTT???\n");
               }

               dump_ts_o();
               printf("Trigger AP Beacon TX\n");

               /* write 1 clear to IDLE bit, set HW to busy , trigger the HW */
               MACREG_WRITE32(BEACON_CONTROL, MACREG_READ32(BEACON_CONTROL));
            }
         }
      }
#endif
   }
   if(PRE_TBTT_HW & ts_status)
   {
       if(ts_status & PRE_TBTT_HW_TS3) /* Because STA must be TSF3 */
       {
#if defined (DRAGONITE_SUPPORT_POWERSAVE_IN_STATION_MODE)
	       DRG_NOTICE(DRAGONITE_DEBUG_PS_FLAG, "preTBTT\n");
	       dragonite_pm_lock_lock();

	       dragonite_ps_wakeup(data);
	       data->ps_flags |= PS_WAIT_FOR_BEACON;
	       data->ps_flags &= ~PS_WAIT_FOR_MULTICAST;

	       dragonite_pm_lock_unlock();
#endif
       }
   }

    if (TS_ERR0_MASK & ts_err_status)
    {
        for (i=0;i<MAX_BSSIDS;i++)
        {
            if (( 0x01 << i ) & ts_err_status)
            {
                ASSERT((i != 0), "TSF0 should not happen TS ERR\n");
                for (idx=0;idx<MAX_BSSIDS;idx++)
                {
                    vp = idx_to_vp(data, (u8) idx);
                    if (vp)
                    {
                        if (vp->bssinfo->timer_index == i)
                        {
                            if (vp->ts_err_mute)
                            {
                                if (time_is_before_jiffies(vp->ts_err_jiffies))
                                {
                                    vp->ts_err_mute = false;
                                }
                            }
                            else
                            {
                                if (vp->ts_err_count < TS_ERROR_LIMIT)
                                {
                                    vp->ts_err_count++;
                                }
                                else
                                {
                                    vp->ts_err_count = 0;
                                    vp->ts_err_mute = true;
                                    vp->ts_err_jiffies = jiffies + TS_ERROR_MUTE_TIME;
                                }
                            }
                            break;
                        }
                    }
                    else
                    {
                        continue;
                    }
                }
                if (idx>=MAX_BSSIDS)
                    continue;

                ts_o.val_64 = MACREG_READ64(TS_O(i));

                TSF_WARN(vp, "RX TSF out-of-range, TSF-%d, %08x %08x\n", i, ts_o.val_32[0], ts_o.val_32[1]);

                bcn_interval_32 = (data->dragonite_bss_info[idx].beacon_interval * TIME_UNIT); /* TSF0 never happen, and STA mode timer index is BSS index plus one */
                ts_o.val_64 = (ts_o.val_64 + (bcn_interval_32 - 1));
#ifdef __BIG_ENDIAN
                ts_o_temp.val_64 = ((((ts_o.val_32[0] % bcn_interval_32) * ((UINT_MAX % bcn_interval_32) + 1)) % bcn_interval_32)
                                    + (ts_o.val_32[1] % bcn_interval_32)) % bcn_interval_32;
#else
                ts_o_temp.val_64 = ((((ts_o.val_32[1] % bcn_interval_32) * ((UINT_MAX % bcn_interval_32) + 1)) % bcn_interval_32)
                                    + (ts_o.val_32[0] % bcn_interval_32)) % bcn_interval_32;
#endif
                ts_o_final.val_64 = ts_o.val_64 - ts_o_temp.val_64;

                MACREG_WRITE64(TS_NEXT_TBTT(i), ts_o_final.val_64);

                TSF_WARN(vp, "ADJUST next TBTT to %08x %08x\n", ts_o_final.val_32[0], ts_o_final.val_32[1]);
            }
        }
    }

   if(TS_NOA_START_MASK & ts_status)
   {
	   DRG_DBG("TS_NOA_START_MASK\n");
	   #if 0
	   
      for(i=0;i<MAX_BSSIDS;i++)
      {
         if(( 0x01 << (i + 24) ) & ts_status)
         {
            ts_o = MACREG_READ64(TS_O(i));
            ts_noa = MACREG_READ32(TS_NOA_START(i));

            printf("TSF(%d): NoA start intr @ %08x %08x, TS_NOA_START_LOW %08x\n", 
                   i, (u32)(ts_o >> 32), (u32)(ts_o), (u32) ts_noa);

            if((((u32)(ts_noa + NOA_INTR_SW_PROCESSING_DELAY_MAX)) < ((u32) ts_o))
               || (((u32)(ts_noa)) > ((u32) ts_o)))
            {
               cosim_panic("Wrong NoA start interrupt");
            }
         }
      }
	  #endif
   }
#if 0
   if(TS_NOA_END_MASK & ts_err_status)
   {
      for(i=0;i<MAX_BSSIDS;i++)
      {
         if(( 0x01 << (i + 8) ) & ts_err_status)
         {
            ts_o = MACREG_READ64(TS_O(i));
            ts_noa = MACREG_READ32(TS_NOA_END(i));

            printf("TSF(%d): NoA end intr @ %08x %08x, TS_NOA_END_LOW %08x\n", 
                   i, (u32)(ts_o >> 32), (u32)(ts_o), (u32) ts_noa);

            if((((u32)(ts_noa + NOA_INTR_SW_PROCESSING_DELAY_MAX)) < ((u32) ts_o))
               || (((u32)(ts_noa)) > ((u32) ts_o)))
            {
               cosim_panic("Wrong NoA end interrupt");
            }
         }
      }
   }
#endif
}

