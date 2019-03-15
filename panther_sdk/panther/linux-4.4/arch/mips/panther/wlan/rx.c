#include <linux/skbuff.h>
#include <math.h>

#include "dragonite_main.h"
#include "dragonite_common.h"
#include "mac.h"
#include "mac_regs.h"
#include "mac_ctrl.h"
#include "mac_tables.h"
#include "skb.h"
#include "tx.h"
#include "resources.h"
#include "panther_debug.h"
#include "rfc_comm.h"
#include "bb.h"
#include "snr.h"

#define AMPDU_HEADER_LENTH 4
#define LLC_HEADER_SIZE 6
#define HDR_3ADDR_SIZE sizeof(struct ieee80211_hdr_3addr)
#define QOS_HDR_SIZE sizeof(struct ieee80211_qos_hdr)
#define MAX_RX_PHY_INFO_STORAGE_SIZE 10

typedef struct rx_phy_info {
    unsigned char format;
    unsigned char snr_p1;
    unsigned char snr_p2;
    unsigned char rssi;
    unsigned char rate;
    unsigned char bandwidth;
    unsigned char offset;
} _rx_phy_info;

_rx_phy_info rx_phy_info_storage[MAX_RX_PHY_INFO_STORAGE_SIZE];
int rx_phy_info_storage_idx = 0;
unsigned long rx_phy_last_dump_jiffies;

int dragonite_rx_debug_dump=0;
int dragonite_rx_phy_info_dump=0;

extern u32 drg_wifi_recover;

void store_rx_phy_info(unsigned char format, unsigned char snr_p1, unsigned char snr_p2,
        unsigned char rssi, unsigned char rate, unsigned char bandwidth, unsigned char offset)
{
    rx_phy_info_storage[rx_phy_info_storage_idx].format = format;
    rx_phy_info_storage[rx_phy_info_storage_idx].snr_p1 = snr_p1;
    rx_phy_info_storage[rx_phy_info_storage_idx].snr_p2 = snr_p2;
    rx_phy_info_storage[rx_phy_info_storage_idx].rssi = rssi;
    rx_phy_info_storage[rx_phy_info_storage_idx].rate = rate;
    rx_phy_info_storage[rx_phy_info_storage_idx].bandwidth = bandwidth;
    rx_phy_info_storage[rx_phy_info_storage_idx].offset = offset;
    rx_phy_info_storage_idx++;
    rx_phy_info_storage_idx = rx_phy_info_storage_idx % MAX_RX_PHY_INFO_STORAGE_SIZE;
}

void dump_rx_rate_simply(unsigned char format, unsigned char rate, unsigned char bandwidth, unsigned char offset)
{
    if(bandwidth == 0)
        printk("20");
    else
        printk("40");

    if(offset == CH_OFFSET_20)
        printk("  MHz");
    else if(offset == CH_OFFSET_20U)
        printk("+ MHz");
    else if(offset == CH_OFFSET_20L)
        printk("- MHz");
    else
        printk("  MHz");

    if(format == FMT_NONE_HT)
        printk("  G   mode");
    else if(format == FMT_HT_MIXED)
        printk(" HT  mixed");
    else if(format == FMT_HT_GF)
        printk(" HT gfield");
    else if(format == FMT_11B)
        printk("  B   mode");

    if(format == FMT_11B)
    {
        if(rate == CCK_1M)
            printk(" 1M\n");
        else if(rate == CCK_2M)
            printk(" 2M\n");
        else if(rate == CCK_5_5M)
            printk(" 5.5M\n");
        else if(rate == CCK_11M)
            printk(" 11M\n");
        else if(rate == CCK_SHORT_PREAMBLE)
            printk(" short preamble\n");
    }

    if(format == FMT_NONE_HT)
    {
        if(rate == OFDM_6M)
            printk(" 6M\n");
        else if(rate == OFDM_9M)
            printk(" 9M\n");
        else if(rate == OFDM_12M)
            printk(" 12M\n");
        else if(rate == OFDM_18M)
            printk(" 18M\n");
        else if(rate == OFDM_24M)
            printk(" 24M\n");
        else if(rate == OFDM_36M)
            printk(" 36M\n");
        else if(rate == OFDM_48M)
            printk(" 48M\n");
        else if(rate == OFDM_54M)
            printk(" 54M\n");
    }

    if(format == FMT_HT_MIXED || format == FMT_HT_GF)
    {
        if(rate == MCS_0)
            printk(" 6.5M\n");
        else if(rate == MCS_1)
            printk(" 13M\n");
        else if(rate == MCS_2)
            printk(" 19.5M\n");
        else if(rate == MCS_3)
            printk(" 26M\n");
        else if(rate == MCS_4)
            printk(" 39M\n");
        else if(rate == MCS_5)
            printk(" 52M\n");
        else if(rate == MCS_6)
            printk(" 58.5M\n");
        else if(rate == MCS_7)
            printk(" 65M\n");
    }
}

void dump_rx_phy_info(void)
{
    int idx;
    int rssi_total = 0;
    double snr_x, snr_y, snr_db, snr_x_total = 0, snr_y_total = 0;

    for(idx = 0; idx < MAX_RX_PHY_INFO_STORAGE_SIZE; idx++)
    {
        snr_x = (double)rx_phy_info_storage[idx].snr_p1 / 1024;
        snr_y = ((double)(((rx_phy_info_storage[idx].snr_p2 >> 5) & 0x07) + 8)/64) * pow(2, -1*(rx_phy_info_storage[idx].snr_p2 & 0x1F));
        snr_db = 10*log10(0.5 * (snr_x/snr_y - 1));

        dbg_double(RFC_DBG_TRUE, snr_db); printk(" / ");
        printk("%d / ", rx_phy_info_storage[idx].rssi);
        //printk("%02x / ", rx_phy_info_storage[idx].format);
        //printk("%02x\n", rx_phy_info_storage[idx].rate);
        dump_rx_rate_simply(rx_phy_info_storage[idx].format, rx_phy_info_storage[idx].rate, rx_phy_info_storage[idx].bandwidth, rx_phy_info_storage[idx].offset);
        snr_x_total += snr_x;
        snr_y_total += snr_y;
        rssi_total += rx_phy_info_storage[idx].rssi;
    }

    snr_x = snr_x_total/MAX_RX_PHY_INFO_STORAGE_SIZE;
    snr_y = snr_y_total/MAX_RX_PHY_INFO_STORAGE_SIZE;
    snr_db = 10*log10(0.5 * (snr_x/snr_y - 1));

    printk("avg ");
    dbg_double(RFC_DBG_TRUE, snr_db); printk(" / ");
    printk("%d\n", rssi_total/MAX_RX_PHY_INFO_STORAGE_SIZE);
    printk("======================================\n");
}

void dma_cache_wback_inv_skb(struct sk_buff *skb)
{
    int mis_align, length;

    mis_align = ((unsigned long)skb->data % L1_CACHE_BYTES);
    length = skb->len + mis_align;
    length += (L1_CACHE_BYTES - (length % L1_CACHE_BYTES));

    dma_cache_wback_inv((unsigned long)skb->data - mis_align, length);
}
void dragonite_rx_ps_poll_handler(struct mac80211_dragonite_data *data)
{
    struct dragonite_sta_priv *sp;
    MAC_INFO* info = data->mac_info;
    int sta_idx, tid, ret;

    for(sta_idx = 0; sta_idx < info->sta_idx_msb; sta_idx++) {

        if(info->sta_ps_poll_request & (0x1 << sta_idx))
        {
            info->sta_ps_poll_request &= ~(0x1 << sta_idx);

            sp = idx_to_sp(data, (u8) sta_idx);
            if(sp == NULL)
            {
                continue;
            }
            for(tid = 0; tid < TID_MAX + 1; tid++) {

                if(sp->tx_sw_queue[TID_MAX - tid].qhead != NULL) {

                    ret = dragonite_tx_pspoll_response(data, sta_idx, TID_MAX - tid);

                    if(ret != 0)
                    {
                        dragonite_tx_send_null_data(data, sta_idx);
                    }
                    break;
                }
            }
        }
    }
}
#if defined (DRAGONITE_ENABLE_AIRKISS)
static int last_packet_len = 0;
static unsigned char LLC_HEADER[LLC_HEADER_SIZE] = {0xAA, 0xAA, 0x3, 0x0, 0x0, 0x0};
static void airkiss_replace_data(struct sk_buff *skb)
{
    struct ieee80211_hdr_3addr *wifi_hdr = (struct ieee80211_hdr_3addr *) skb->data;
    struct ieee80211_qos_hdr *wifi_qos_hdr;
    __le16 fc = wifi_hdr->frame_control;
    unsigned char receiver_addr[ETH_ALEN], transmitter_addr[ETH_ALEN];

    /* repair last frame and length */
    if(ieee80211_is_ack(fc))
    {
        if((last_packet_len - FCS_LEN) > skb->len)
        {
            skb_put(skb, last_packet_len - FCS_LEN - skb->len);

            wifi_hdr->frame_control = cpu_to_le16(IEEE80211_FCTL_TODS | IEEE80211_FTYPE_DATA);
            memcpy(receiver_addr, wifi_hdr->addr1, ETH_ALEN);
            memcpy(transmitter_addr, wifi_hdr->addr2, ETH_ALEN);
            memcpy(wifi_hdr->addr1, transmitter_addr, ETH_ALEN);
            memcpy(wifi_hdr->addr2, receiver_addr, ETH_ALEN);
            memcpy(wifi_hdr->addr3, transmitter_addr, ETH_ALEN);
            wifi_hdr->seq_ctrl = 0;

            memcpy(&skb->data[HDR_3ADDR_SIZE], LLC_HEADER, LLC_HEADER_SIZE);
        }
    }
    else if(ieee80211_is_back(fc))
    {
        if((last_packet_len - FCS_LEN - AMPDU_HEADER_LENTH) > skb->len)
        {
            skb_put(skb, last_packet_len - FCS_LEN - AMPDU_HEADER_LENTH - skb->len);

            wifi_qos_hdr = (struct ieee80211_qos_hdr *) skb->data;
            skb->len = last_packet_len - FCS_LEN - AMPDU_HEADER_LENTH;
            wifi_qos_hdr->frame_control = cpu_to_le16(IEEE80211_FCTL_TODS | IEEE80211_FTYPE_DATA | IEEE80211_STYPE_QOS_DATA);
            memcpy(receiver_addr, wifi_hdr->addr1, ETH_ALEN);
            memcpy(transmitter_addr, wifi_hdr->addr2, ETH_ALEN);
            memcpy(wifi_hdr->addr1, transmitter_addr, ETH_ALEN);
            memcpy(wifi_hdr->addr2, receiver_addr, ETH_ALEN);
            memcpy(wifi_hdr->addr3, transmitter_addr, ETH_ALEN);
            wifi_qos_hdr->seq_ctrl = 0;
            wifi_qos_hdr->qos_ctrl = 0;

            memcpy(&skb->data[QOS_HDR_SIZE], LLC_HEADER, LLC_HEADER_SIZE);
        }
    }
}
static int dragonite_rx_airkiss_pass_skb(struct mac80211_dragonite_data *data, volatile rx_descriptor* rxdescr, rx_packet_descriptor* pktdescr, struct sk_buff *skb)
{
	struct ieee80211_hdr *wifi_hdr;
	struct ieee80211_rx_status rx_status;
	bool rx_drop = false;			/* rx_drop will be on if this RX packet is not going to report to upper-layers */
        __le16 fc;
        int hdrlen;

        if(skb->len > 1000 || skb->len < 10)
        {
            last_packet_len = 0;
            rx_drop = true;
        }
        else
        {
            /* MIMO packet */
            if(rxdescr->fsc_err_sw)
            {
                last_packet_len = skb->len;
                rx_drop = true;
            }
            else
            {
                wifi_hdr = (struct ieee80211_hdr *) skb->data;
                hdrlen = ieee80211_hdrlen(wifi_hdr->frame_control);
                fc = wifi_hdr->frame_control;

                if(hdrlen == 0)
                {
                    rx_drop = true;
                }
                else if(ieee80211_is_ack(fc) || ieee80211_is_back(fc))
                {
                    if(last_packet_len)
                    {
                        /* do replace header and change length*/
                        airkiss_replace_data(skb);
                    }
                    else
                    {
                        rx_drop = true;
                    }
                }
                last_packet_len = 0;
            }
        }

        if(rx_drop)
	{
		dev_kfree_skb_any(skb);
	}
	else
	{
		memset(&rx_status, 0, sizeof(rx_status));

		wifi_hdr = (struct ieee80211_hdr *) skb->data;

		if(!ieee80211_has_protected(wifi_hdr->frame_control))
		{
			rx_status.flag |= (RX_FLAG_MMIC_STRIPPED | RX_FLAG_DECRYPTED | RX_FLAG_IV_STRIPPED);
		}
		if(pktdescr->sec_status == SEC_STATUS_TKIP_MIC_ERR)
		{
			rx_status.flag |= RX_FLAG_MMIC_ERROR;
		}
		rx_status.freq = data->curr_center_freq;
		rx_status.band = data->curr_band;
		rx_status.rate_idx = 0;

		memcpy(IEEE80211_SKB_RXCB(skb), &rx_status, sizeof(rx_status));

		ieee80211_rx_irqsafe(data->hw, skb);
	}

	return 0;
}
#endif

#if defined(DRAGONITE_MLME_DEBUG)
struct beacon_rx_data
{
    unsigned long jiffies;
    unsigned char rssi;
    char snr;
    unsigned short seqnum;
    unsigned int acc_rx_count;
    unsigned long long timestamp;
};
struct beacon_rx_data beacon_rx_history[BEACON_RX_HISTORY_DEPTH];
static unsigned int acc_rx_count = 0;
static int rx_beacon_history_idx = 0;
static int beacon_history_dump_count;

static void beacon_history_dump(void)
{
    int i = BEACON_RX_HISTORY_DEPTH;
    int idx = rx_beacon_history_idx;

    printk(KERN_CRIT "Beacon RX histroy (%ld)\n", jiffies);
    while(i-->0)
    {
        printk(KERN_CRIT "B(%ld) %d %d %d %d %llx\n",
                  beacon_rx_history[idx].jiffies
                  , beacon_rx_history[idx].rssi
                  , beacon_rx_history[idx].snr
                  , beacon_rx_history[idx].seqnum
                  , beacon_rx_history[idx].acc_rx_count
                  , beacon_rx_history[idx].timestamp);
        idx = (idx+1) % BEACON_RX_HISTORY_DEPTH;
    }
}

void dragonite_notify_beacon_rx(struct sk_buff *skb, rx_packet_descriptor* pktdescr)
{
    beacon_rx_history[rx_beacon_history_idx].jiffies = jiffies;
    beacon_rx_history[rx_beacon_history_idx].rssi = pktdescr->rssi;
    beacon_rx_history[rx_beacon_history_idx].snr = snr_calc(pktdescr->snr, pktdescr->snr_p1, pktdescr->snr_p2);
    beacon_rx_history[rx_beacon_history_idx].seqnum = *((unsigned short *) &skb->data[22]) >> 4;
    beacon_rx_history[rx_beacon_history_idx].timestamp = *((unsigned long long *) &skb->data[24]);
    beacon_rx_history[rx_beacon_history_idx].acc_rx_count = acc_rx_count;
    rx_beacon_history_idx = (rx_beacon_history_idx+1) % BEACON_RX_HISTORY_DEPTH;

    if(beacon_history_dump_count > 0)
    {
        beacon_history_dump();
        beacon_history_dump_count--;
    }
}
#endif

void dragonite_notify_beacon_loss(int beacon_loss_count)
{
#if defined(DRAGONITE_MLME_DEBUG)
    printk(KERN_CRIT "Beacon loss %d (%ld)\n", beacon_loss_count, jiffies);
    beacon_history_dump();
    beacon_history_dump_count = 3;
#endif
}

void dragonite_notify_reset_ap_probe(void)
{
#if defined(DRAGONITE_TRIGGER_RECOVERY_ON_BEACON_LOSS)
    printk(KERN_CRIT "Beacon loss recovery (%ld)\n", jiffies);
    drg_wifi_recover = 1;
#endif
}

static int dragonite_rx_forward_skb(struct mac80211_dragonite_data *data, rx_packet_descriptor* pktdescr, struct sk_buff *skb)
{
	struct ieee80211_hdr *wifi_hdr;
	struct skb_info skbinfo;
	struct ieee80211_rx_status rx_status;
	bool rx_drop = false;			/* rx_drop will be on if this RX packet is not going to report to upper-layers */
	MAC_INFO *info = data->mac_info;

	skbinfo.flags = 0;

	if(0 == dragonite_parse_rx_skb(data, skb, &skbinfo))
	{

	}
	else /* frame format or information wrong */
	{
		DRG_ERROR(DRAGONITE_DEBUG_DRIVER_WARN_FLAG, "rx frame format or information wrong, drop it !!\n");
		dragonite_dump_rx_skb(skb);
		rx_drop = true;
	}

	if((pktdescr->sec_status == SEC_STATUS_SEC_MISMATCH) && (skbinfo.flags != SKB_INFO_FLAGS_PAE_FRAME))
	{
		/* if security mismatch and also not PAE frame, drop it */
		DRG_ERROR(DRAGONITE_DEBUG_DRIVER_WARN_FLAG, "rx security mismatch, drop it !!\n");
		dragonite_dump_rx_skb(skb);
	}

	if(info->sta_ampdu_recover && (skbinfo.flags & SKB_INFO_FLAGS_DATA_FRAME))
	{
                /* drop data frame after mac recover and before delba done */
		rx_drop = true;
	}

	if(info->sta_ps_poll_request)
	{
		dragonite_rx_ps_poll_handler(data);
	}

	if(dragonite_rx_phy_info_dump)
	{
		/*
		 * X = SNR_part1[7:0]/1024
		 * Y = (SNR_part2[7:5]+8)/64 *2^(- SNR_part2[4:0])
		 * SNR_dB = 10*log10( 0.5*(X/Y-1) )
		*/
		store_rx_phy_info(pktdescr->format, pktdescr->snr_p1, pktdescr->snr_p2, pktdescr->rssi, pktdescr->rate, pktdescr->cbw, data->primary_ch_offset);

		if(jiffies - 100 > rx_phy_last_dump_jiffies)
		{
			dump_rx_phy_info();
			rx_phy_last_dump_jiffies = jiffies;
		}
	}

#if defined(DRAGONITE_MLME_DEBUG)
	if((skbinfo.flags & SKB_INFO_FLAGS_BEACON_FRAME)
        && (pktdescr->hit & HIT_DS))
	{
		dragonite_notify_beacon_rx(skb, pktdescr);
		//rx_drop = true;   // for beacon loss emulation
	}
	else
	{
		acc_rx_count++;
	}
#endif

	if(rx_drop)
	{
		dev_kfree_skb_any(skb);
	}
	else
	{
		memset(&rx_status, 0, sizeof(rx_status));

		wifi_hdr = (struct ieee80211_hdr *) skb->data;

		if(!ieee80211_has_protected(wifi_hdr->frame_control))
		{
			rx_status.flag |= (RX_FLAG_MMIC_STRIPPED | RX_FLAG_DECRYPTED | RX_FLAG_IV_STRIPPED);
		}
		if(pktdescr->sec_status == SEC_STATUS_TKIP_MIC_ERR)
		{
			rx_status.flag |= RX_FLAG_MMIC_ERROR;
		}
		if(pktdescr->format==FMT_HT_MIXED)
		{
			rx_status.flag |= RX_FLAG_HT;
		}
		rx_status.freq = data->curr_center_freq;
		rx_status.band = data->curr_band;
		rx_status.signal = pktdescr->rssi;

		memcpy(IEEE80211_SKB_RXCB(skb), &rx_status, sizeof(rx_status));

		ieee80211_rx_irqsafe(data->hw, skb);
	}

	return 0;
}

static struct sk_buff* retrieve_skb_from_rx_descr(struct mac80211_dragonite_data *data, rx_descriptor* rxdescr, rx_packet_descriptor* pktdescr)
{
	MAC_INFO* info = data->mac_info;
	int head_index;
	int tail_index;
	struct sk_buff *skb = NULL;
	int skb_offset;
	u32 ndptr;
	int defrag_offset, fragment_len;          /* two variable for copying fragment packet */
	struct sk_buff *frag_skb;
	volatile buf_header* buffer_hdr;
	struct ieee80211_hdr *wifi_hdr;
	unsigned char *dptr;
    bool dragonite_rx_error_handle = false;

	rx_packet_descriptor *pdescr;

	head_index = rxdescr->frame_header_head_index;
	tail_index = rxdescr->frame_header_tail_index;

	buffer_hdr = &info->buf_headers[head_index];

	skb_offset = buffer_hdr->offset;

	dptr = (unsigned char *) VIRTUAL_ADDR(((unsigned long)buffer_hdr->dptr));

	memcpy(pktdescr, dptr, sizeof(rx_packet_descriptor));

	pdescr = pktdescr;

	if(head_index == tail_index)
	{
		skb = (struct sk_buff *) info->swpool_bh_idx_to_skbs[head_index];
		if((0x80000000UL > ((u32) skb)) || (( (u32) skb) == 0))
			panic("retrieve_skb_from_rx_descr(): wrong rx skb buffer ptr %p, rxdescr %p\n", skb, rxdescr);

		/* check for a possibly error condition, as wifi header is larger than 32bytes */
		if(skb_offset < 8)
		{
			DRG_ERROR(DRAGONITE_DEBUG_DRIVER_WARN_FLAG, 
                                  "I possibly wrong wifi header length, header offset %d, len %d\n", skb_offset, buffer_hdr->len);
            dragonite_rx_error_handle = true;
            goto exit;
		}

		if((pdescr->packet_len + skb_offset) >= DEF_BUF_SIZE)
		{
			DRG_ERROR(DRAGONITE_DEBUG_DRIVER_WARN_FLAG, "wrong skb packet len %d\n", pdescr->packet_len);
			dragonite_rx_error_handle = true;
			goto exit;
		}

		if((pdescr->packet_len + skb_offset) >= DEF_BUF_SIZE)
		{
			DRG_ERROR(DRAGONITE_DEBUG_DRIVER_WARN_FLAG, "wrong skb packet len %d\n", pdescr->packet_len);
			dragonite_rx_error_handle = true;
			goto exit;
		}

		skb_put(skb, pdescr->packet_len + skb_offset);
		skb_pull(skb, skb_offset);
	}
	else
	{
		ndptr = (u32) mac_alloc_skb_buffer(info, pdescr->packet_len, 0, (u32 *)&skb);  /* this SKB is not UNCACHED */
	
		if(skb)
		{
			skb_put(skb, pdescr->packet_len);
			defrag_offset = 0;

			while(1)
			{
				if(head_index >= info->rx_buffer_hdr_count)
					panic("retrieve_skb_from_rx_descr(): wrong rx buffer header index %d (%d), rxdescr %p\n", head_index, info->rx_buffer_hdr_count, rxdescr);
	
				frag_skb = (struct sk_buff *) info->swpool_bh_idx_to_skbs[head_index];
		
				if((0x80000000UL > ((u32) frag_skb)) || (( (u32) frag_skb) == 0))
					panic("retrieve_skb_from_rx_descr(): wrong rx skb buffer ptr %p, rxdescr %p\n", frag_skb, rxdescr);
		
				buffer_hdr = &info->buf_headers[head_index];
				skb_offset = buffer_hdr->offset;

				if(defrag_offset == 0)
				{
					if(skb_offset < 8)
					{
						DRG_ERROR(DRAGONITE_DEBUG_DRIVER_WARN_FLAG, 
                                                          "II possibly wrong wifi header length, header offset %d, len %d\n", skb_offset, buffer_hdr->len);
                        dragonite_rx_error_handle = true;
                        goto exit;
					}
				}
				else
				{
					if(skb_offset != 0)
					{
						DRG_ERROR(DRAGONITE_DEBUG_DRIVER_WARN_FLAG, "wrong skb offset in fragment frame\n");
                        dragonite_rx_error_handle = true;
                        goto exit;
					}
				}
	
				if(buffer_hdr->len > (pdescr->packet_len - defrag_offset))
				{
					fragment_len = pdescr->packet_len - defrag_offset;
				}
				else
				{
					fragment_len = buffer_hdr->len;
				}

				if(fragment_len)
					memcpy(&skb->data[defrag_offset], &frag_skb->data[skb_offset], fragment_len);

				defrag_offset += fragment_len;

				if(head_index == rxdescr->frame_header_tail_index)
                {    
					break;
				}

				if(buffer_hdr->ep)
				{
					DRG_ERROR(DRAGONITE_DEBUG_DRIVER_WARN_FLAG, "inconsistent rx buffer header chain, insufficient packet data\n");
					break;
				}

				head_index = buffer_hdr->next_index;
			}

			if(defrag_offset!=pdescr->packet_len)
			{
				DRG_ERROR(DRAGONITE_DEBUG_DRIVER_WARN_FLAG, "wrong pkt length %d %d\n", pdescr->packet_len, defrag_offset);
                dragonite_rx_error_handle = true;
                goto exit;
			}

			if(skb->len>=2)
			{
				wifi_hdr = (struct ieee80211_hdr *) skb->data;
				wifi_hdr->frame_control &= cpu_to_le16(~IEEE80211_FCTL_MOREFRAGS);
			}
		}
	}

	if(dragonite_rx_debug_dump)
	{
		dragonite_dump_rx_skb(skb);
	}
exit:
    if(dragonite_rx_error_handle)
    {
        DRG_ERROR(DRAGONITE_DEBUG_DRIVER_WARN_FLAG, "RX somthing wrong, recover wifi !!\n");
        resource_debug_dump();
        mac_kfree_skb_buffer((u32 *)skb);
        skb = NULL;

        /* recover wifi */
        drg_wifi_recover = 1;
    }

	return skb;
}

static inline struct sk_buff *dragonite_rx_replace_buffer_and_retrieve_skb(struct mac80211_dragonite_data *data, rx_descriptor* rxdescr, rx_packet_descriptor* pktdescr)
{
	struct sk_buff *skb = NULL;
	struct sk_buff *nskb;
	u32 ndptr;

	if(rxdescr->frame_header_head_index != rxdescr->frame_header_tail_index)
	{
		/* the routine will defragment these skbs and give us a new skb */
		skb = retrieve_skb_from_rx_descr(data, rxdescr, pktdescr);

        if(skb)
        {
            dma_cache_wback_inv_skb(skb);
        }
        else
        {
            DRG_EMERG(DRAGONITE_DEBUG_SYSTEM_FLAG, "alloc skb failed\n");
        }
	}
	else
	{
		/* caution: !!! the operation should be atomic */
		ndptr = (u32) mac_alloc_skb_buffer(data->mac_info, data->mac_info->sw_path_bufsize, MAC_MALLOC_UNCACHED, (u32 *)&nskb);

		if(nskb)
		{
			skb = retrieve_skb_from_rx_descr(data, rxdescr, pktdescr);

			data->mac_info->buf_headers[rxdescr->frame_header_head_index].dptr =   (u32) PHYSICAL_ADDR(ndptr);
			data->mac_info->swpool_bh_idx_to_skbs[rxdescr->frame_header_head_index] = (u32) nskb;
		}
		else
		{
			/* malloc failure: just drop the packet received */
			DRG_EMERG(DRAGONITE_DEBUG_SYSTEM_FLAG, "alloc skb failed\n");
		}
	}
	return skb;
}

extern struct wifi_sram_resources* wifisram;
extern unsigned int wlan_driver_sram_size;
#if defined (DRAGONITE_ISR_CHECK)
extern volatile u32 drg_rx_count;
#endif
int dragonite_process_rx_queue(struct mac80211_dragonite_data *data)
{
    MAC_INFO* info = data->mac_info;
	volatile rx_descriptor* rxdescr;
    unsigned long pkt_descr[(sizeof(rx_packet_descriptor)+4)/4];
    rx_packet_descriptor *pktdescr = (rx_packet_descriptor *) &pkt_descr[0];
	struct sk_buff *skb;
    bool dragonite_rx_error_handle = false;
    int i;

	i = info->rx_descr_index;
	while (info->rx_descriptors[i].own == 0)
	{
#if defined (DRAGONITE_ISR_CHECK)
        drg_rx_count++;
#endif
        skb = NULL;

        rxdescr = &info->rx_descriptors[i];

        if((u32) rxdescr > UNCACHED_ADDR((u32)wifisram + wlan_driver_sram_size) ||  (u32) rxdescr < UNCACHED_ADDR((u32)wifisram))
        {
            DRG_ERROR(DRAGONITE_DEBUG_DRIVER_WARN_FLAG, "Wrong rxdescr\n");
            dragonite_rx_error_handle = true;
            goto exit;
        }

		if((rxdescr->frame_header_head_index >= info->rx_buffer_hdr_count) 
				|| (rxdescr->frame_header_tail_index >= info->rx_buffer_hdr_count))
		{
			DRG_ERROR(DRAGONITE_DEBUG_DRIVER_WARN_FLAG, "Wrong software rx buffer header index H:%d T:%d (%d), rxdescr %p\n", 
					rxdescr->frame_header_head_index, rxdescr->frame_header_tail_index, info->rx_buffer_hdr_count, rxdescr);
            dragonite_rx_error_handle = true;
            goto exit;
		}

		if(data->started)
		{
			skb = dragonite_rx_replace_buffer_and_retrieve_skb(data, (rx_descriptor *) rxdescr, pktdescr);
		}

		while (MACREG_READ32(MAC_FREE_CMD) & MAC_FREE_CMD_BUSY)
                    ;
		MACREG_WRITE32(MAC_FREE_PTR,
				((info->rx_descriptors[i].frame_header_head_index << 16)
				 | info->rx_descriptors[i].frame_header_tail_index));

		MACREG_WRITE32(MAC_FREE_CMD,  MAC_FREE_CMD_SW_BUF | MAC_FREE_CMD_KICK);

		if(skb)
		{
#if defined (DRAGONITE_ENABLE_AIRKISS)
                    if(__airkiss_filter_mode)
                        dragonite_rx_airkiss_pass_skb(data, rxdescr, pktdescr, skb);
                    else
                        dragonite_rx_forward_skb(data, pktdescr, skb);
#else
                    dragonite_rx_forward_skb(data, pktdescr, skb);
#endif
		}

		info->rx_descriptors[i].own = 1;
		i = (i + 1) % info->rx_descr_count;
		info->rx_descr_index = i;

		MACREG_WRITE32(CPU_RD_SWDES_IND, 1);     /* kick RX queue logic as it might be pending on get free RX descriptor to write */
	}

    dragonite_handle_power_saving(data);

exit:
    if(dragonite_rx_error_handle)
    {
        DRG_ERROR(DRAGONITE_DEBUG_DRIVER_WARN_FLAG, "RX somthing wrong, recover wifi !!\n");
        resource_debug_dump();

        /* recover wifi */
        drg_wifi_recover = 1;
    }
	return 0;
}
