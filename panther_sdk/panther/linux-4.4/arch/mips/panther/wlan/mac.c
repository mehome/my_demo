#include "dragonite_main.h"
#include "dragonite_common.h"
#include "panther_debug.h"
#include "mac_ctrl.h"
#include "mac.h"
#include "tx.h"
#include "resources.h"

ssq_tx_descr* alloc_ssq_tx_descriptors(int count)
{
    return (ssq_tx_descr*) mac_malloc(NULL, sizeof(ssq_tx_descr) * count, MAC_MALLOC_BZERO | MAC_MALLOC_UNCACHED); 
}

int set_mac_info_parameters(MAC_INFO* info)
{
    info->sta_tbl_count = 0;
    info->ds_tbl_count = 0;

    info->buffer_hdr_count = BUFFER_HEADER_POOL_SIZE;

    info->beacon_q_buffer_hdr_count = BEACON_Q_BUFFER_HEADER_POOL_SIZE;

    /* the swpool_rx_buffer_hdr_count should be less then swpool_buffer_hdr_count 
       it is the value to chain linklist in SW buffer header pool for RX path
     */
    info->rx_buffer_hdr_count = info->buffer_hdr_count / 2;  

    info->rx_descr_count = RX_DESCRIPTOR_COUNT;
    info->beacon_tx_descr_count = BEACON_TX_DESCRIPTOR_COUNT;

    info->tx_descr_index = 0;
    info->rx_descr_index = 0;

    info->fastpath_bufsize = DEF_BUF_SIZE;

    info->sw_path_bufsize = DEF_BUF_SIZE;

    info->sta_ps_recover = 0;

    info->sta_ampdu_recover = false;

    return 0;
}

#define ALLOC_SKB   (true)
void setup_rx_buf_headers(MAC_INFO *info, bool alloc_skb)
{
    int i;
    u32 skb;

    /* 
        chain buffer headers from element[0] to element[info->rx_buffer_hdr_count-1]
        this linklist is used for HW to store receive frame (RX to software path)           
     */
    ASSERT(info->rx_buffer_hdr_count, "zero info->rx_buffer_hdr_count\n");
    for (i=0;i<info->rx_buffer_hdr_count;i++) {
        info->buf_headers[i].next_index = i + 1;
        info->buf_headers[i].offset = 0;
        info->buf_headers[i].ep = 0;

        /* allocate buffer referenced by this buffer header */
        if(alloc_skb)
        {
            info->buf_headers[i].dptr = (u32) PHYSICAL_ADDR(mac_alloc_skb_buffer(info, info->sw_path_bufsize, MAC_MALLOC_UNCACHED, &skb));
            if(!skb)
            {
                panic("dragonite: setup rx buffer fail");
            }
            //MACDBG("swpool buffer %d, data ptr %x\n", i, info->buf_headers[i].dptr);
	    info->swpool_bh_idx_to_skbs[i] = skb;
        }
    }
    info->buf_headers[i-1].next_index = 0;
    info->buf_headers[i-1].ep = 1;

    info->rx_freelist_head_index = 0;
    info->rx_freelist_tail_index = i-1;

    return;
}

int setup_protect_guard(MAC_INFO* info)
{
    memset((void *) info->protect_guard_head, 0, sizeof(u8) * PROTECT_GUARD_COUNT);
    memset((void *) info->protect_guard_tail, 0, sizeof(u8) * PROTECT_GUARD_COUNT);

    return 0;
}

int setup_buf_headers(MAC_INFO* info)
{
    int i;

    setup_rx_buf_headers(info, ALLOC_SKB);
    /* 
        chain buffer headers from element[info->rx_buffer_hdr_count] to  element[buffer_hdr_count-1]
        this linklist is used as freelist for SW to allocate buffer headers in TX path
     */
    ASSERT(info->buffer_hdr_count > info->rx_buffer_hdr_count, "no free buffer headers available for TX freelist\n");
    for (i=info->rx_buffer_hdr_count;i<info->buffer_hdr_count;i++) {
        info->buf_headers[i].next_index = i + 1;
        info->buf_headers[i].offset = 0;
        info->buf_headers[i].ep = 0;
        info->buf_headers[i].dptr = 0;
    }
    info->buf_headers[i-1].next_index = 0;
    info->buf_headers[i-1].ep = 1;

    info->sw_tx_bhdr_head = info->rx_buffer_hdr_count;
    info->sw_tx_bhdr_tail = i-1;
#if defined(DRAGONITE_TX_BHDR_CHECKER)
    tx_bhdr_idx_low = info->rx_buffer_hdr_count;
    tx_bhdr_idx_high = i-1;
#endif

    return 0;
}

void reset_rx_buf_header(MAC_INFO *info)
{
    int i;

    ASSERT(info->rx_buffer_hdr_count, "info->rx_buffer_hdr_count is zero before reset_buf_header\n");
    for (i=0; i < info->rx_buffer_hdr_count; i++)
    {
        info->buf_headers[i].next_index = 0;
        info->buf_headers[i].offset = 0;
        info->buf_headers[i].ep = 0;

        /* free buffer referenced by this buffer header */
        mac_kfree_skb_buffer(&(info->swpool_bh_idx_to_skbs[i]));
        info->swpool_bh_idx_to_skbs[i] = 0;
        info->buf_headers[i].dptr = 0;
    }

    info->rx_freelist_head_index = 0;
    info->rx_freelist_tail_index = 0;

    return;
}

void reset_buf_header(MAC_INFO *info)
{
    int i;

    reset_rx_buf_header(info);

    ASSERT(info->buffer_hdr_count > info->rx_buffer_hdr_count,
           "buffer_hdr_count or rx_buffer_hdr_count might be reset before reset_buf_header\n");
    for (i=info->rx_buffer_hdr_count; i < info->buffer_hdr_count; i++)
    {
        info->buf_headers[i].next_index = 0;
        info->buf_headers[i].offset = 0;
        info->buf_headers[i].ep = 0;
        info->buf_headers[i].dptr = 0;
    }

    info->sw_tx_bhdr_head = -1;
    info->sw_tx_bhdr_tail = -1;
#if defined(DRAGONITE_TX_BHDR_CHECKER)
    tx_bhdr_idx_low = -1;
    tx_bhdr_idx_high = -1;
#endif

    return;
}

int init_protect_guard(MAC_INFO* info)
{
    info->protect_guard_head = alloc_protect_guard_head();
    info->protect_guard_tail = alloc_protect_guard_tail();

    ASSERT(info->protect_guard_head, "NULL info->protect_guard_head\n");
    ASSERT(info->protect_guard_tail, "NULL info->protect_guard_tail\n");

    setup_protect_guard(info);

    return 0;
}

void release_protect_guard(MAC_INFO *info)
{
    if(info->protect_guard_head)
    {
        info->protect_guard_head = NULL;
    }

    if(info->protect_guard_tail)
    {
        info->protect_guard_tail = NULL;
    }

    return;
}

int init_buf_headers(MAC_INFO* info)
{
    info->buf_headers = alloc_buf_headers(info->buffer_hdr_count);
    info->beacon_q_buf_headers = alloc_buf_headers(info->beacon_q_buffer_hdr_count);

    ASSERT(info->buf_headers, "NULL info->buf_headers\n");
    ASSERT(info->beacon_q_buf_headers, "NULL info->hwpool_buf_headers\n");

    info->swpool_bh_idx_to_skbs = (u32 *)mac_malloc(info, info->buffer_hdr_count * sizeof(u32), MAC_MALLOC_BZERO);
    if(NULL==info->swpool_bh_idx_to_skbs)
        return -1;

    setup_buf_headers(info);

    return 0;
}
void release_buffer_headers(MAC_INFO* info)
{
	reset_buf_header(info);

	if(info->swpool_bh_idx_to_skbs)
	{
		mac_free((void *) info->swpool_bh_idx_to_skbs);
		info->swpool_bh_idx_to_skbs = NULL;
	}
	if(info->beacon_q_buf_headers)
	{
		free_buf_headers(info->beacon_q_buffer_hdr_count);
		info->beacon_q_buf_headers = NULL;
	}
	if(info->buf_headers)
	{
		free_buf_headers(info->buffer_hdr_count);
		info->buf_headers = NULL;
	}

	return;
}

void setup_rx_transmit_descriptors(MAC_INFO *info)
{
    int i;
    //unsigned char *dptr;

    /* assign ownership of all RX descriptors to hardware */
    for (i=0;i<info->rx_descr_count;i++) {
        //memset(&info->rx_descriptors[i], 0xff, sizeof(rx_descriptor));
        //dptr = (unsigned char *)&info->rx_descriptors[i];
        //printk(KERN_EMERG "dptr = %08x ", *((u32 *)dptr));
        info->rx_descriptors[i].own = 1;
        info->rx_descriptors[i].eor = 0;
    }
    info->rx_descriptors[i-1].eor = 1;      /* indicate the HW it is the last descriptors */

    return;
}

/* alloc & init TX/RX descriptors */
int init_transmit_descriptors(MAC_INFO* info)
{
    /* init rx descriptors pool */

    info->rx_descriptors = alloc_rx_descriptors(info->rx_descr_count);
    ASSERT(info->rx_descriptors, "alloc_rx_descriptors failed\n");

    setup_rx_transmit_descriptors(info);

    if(info->beacon_tx_descr_count)
    {
        info->beacon_tx_descriptors = alloc_ssq_tx_descriptors(info->beacon_tx_descr_count);
        ASSERT(info->beacon_tx_descriptors, "beacon_tx_descriptors allocation failed\n");
    }



    return 0;
}

void release_descriptors(MAC_INFO* info)
{
    if(info->rx_descriptors)
    {
        free_rx_descriptors();
        info->rx_descriptors = NULL;
    }
    if(info->beacon_tx_descriptors)
    {
        mac_free((void *) info->beacon_tx_descriptors);
        info->beacon_tx_descriptors = NULL;
    }
}

int init_station_cap_tables(MAC_INFO* info)
{
    sta_cap_tbl* cap_tbls;

    cap_tbls = mac_malloc(info, sizeof(sta_cap_tbl) * MAX_STA_NUM,
                           MAC_MALLOC_BZERO | MAC_MALLOC_UNCACHED);
    if(cap_tbls) 
    {
        info->sta_cap_tbls = cap_tbls;
        return 0;
    }

    return -ENOMEM;
}

void release_station_cap_tables(MAC_INFO* info)
{
    if(info->sta_cap_tbls)
    {
        mac_free(info->sta_cap_tbls);
        info->sta_cap_tbls = NULL;
    }
}

int init_key_tables(MAC_INFO* info)
{
#if defined(CONFIG_MAC80211_MESH) && defined(DRAGONITE_MESH_TX_GC_HW_ENCRYPT)
    info->group_keys = mac_malloc(info, sizeof(cipher_key) * (MAC_MAX_BSSIDS + MAC_EXT_BSSIDS), MAC_MALLOC_BZERO | MAC_MALLOC_UNCACHED);
#else
    info->group_keys = mac_malloc(info, sizeof(cipher_key) * MAC_MAX_BSSIDS, MAC_MALLOC_BZERO | MAC_MALLOC_UNCACHED);
#endif
    if(NULL == info->group_keys)
        return -ENOMEM;

    info->private_keys = mac_malloc(info, sizeof(cipher_key) * MAX_STA_NUM, MAC_MALLOC_BZERO | MAC_MALLOC_UNCACHED);
    if(NULL == info->private_keys)
        return -ENOMEM;

    return 0;
}
void release_key_tables(MAC_INFO* info)
{
    if(info->group_keys)
    {
        mac_free(info->group_keys);
        info->group_keys = NULL;
    }

    if(info->private_keys)
    {
        mac_free(info->private_keys);
        info->private_keys = NULL;
    }
}

int init_hw_addr_lookup_tables(MAC_INFO* info)
{
    info->ext_sta_table = alloc_ext_sta_table();

    info->ext_ds_table = alloc_ext_ds_table();

    memset(info->ext_sta_table, 0 , MAX_STA_NUM * 10);
    memset(info->ext_ds_table, 0 , MAX_DS_NUM * 10);

    return 0;
}
void release_hw_addr_lookup_tables(MAC_INFO *info)
{
    if(info->ext_sta_table)
    {
        free_ext_sta_table();
        info->ext_sta_table = NULL;
    }

    if(info->ext_ds_table)
    {
        free_ext_ds_table();
        info->ext_ds_table = NULL;
    }
    return;
}

MAC_INFO* dragonite_mac_init(void)
{
	MAC_INFO* info = NULL;

	info = mac_malloc(NULL, sizeof(MAC_INFO), MAC_MALLOC_BZERO);

	if(info)
	{
		reset_mac_registers();

		set_mac_info_parameters(info);

		init_station_cap_tables(info);

        init_protect_guard(info);

		init_buf_headers(info);

		init_transmit_descriptors(info);

        init_tx_acq(info);

        init_key_tables(info);

        init_hw_addr_lookup_tables(info);

		mac_program_addr_lookup_engine(info);

		program_mac_registers(info);
	}
	else
    {    
        DRG_ERROR(DRAGONITE_DEBUG_SYSTEM_FLAG, "Mac alloc failed\n");
    }

	return info;
}

void dragonite_mac_release(MAC_INFO* info)
{
    if(info)
    {
        release_hw_addr_lookup_tables(info);

        release_key_tables(info);

        release_tx_acq(info);

        release_descriptors(info);
            
        release_buffer_headers(info);

        release_protect_guard(info);

        release_station_cap_tables(info);

        mac_free(info);
    }
}
extern int wifi_tx_enable;
void recover_vif(struct mac80211_dragonite_data *data)
{
    int idx, vif_cnt;
    struct dragonite_vif_priv *vp;
    struct __bss_info *bss_info;

    for(idx = 0, vif_cnt = 0; idx < MAC_MAX_BSSIDS; idx++)
    {
        vp = idx_to_vp(data, (u8) idx);
        if(vp == NULL)
        {
            continue;
        }
        wifi_tx_enable++;
        vif_cnt++;
        bss_info = vp->bssinfo;
        mac_program_bssid(bss_info->MAC_ADDR, vp->bssid_idx, bss_info->dut_role, bss_info->timer_index);
    }
    ASSERT((vif_cnt == data->bssid_count), "recover_vif: The recover vif num not equal to bssid_count\n");

    mac_apply_default_wmm_ap_parameters();

    return;
}

void recover_sta(struct mac80211_dragonite_data *data)
{
    u8 idx;

    for(idx = 0; idx < MAX_STA_NUM; idx++)
    {
        dragonite_recover_station(data, idx);
    }

    return;
}

void dragonite_mac_recover(struct mac80211_dragonite_data *data)
{
    data->mac_info->sta_tbl_count = 0;
    data->mac_info->ds_tbl_count = 0;
    data->mac_info->tx_descr_index = 0;
    data->mac_info->rx_descr_index = 0;

    reset_mac_registers();

    setup_protect_guard(data->mac_info);

    setup_rx_buf_headers(data->mac_info, !ALLOC_SKB);

    setup_rx_transmit_descriptors(data->mac_info);

    resetup_tx_acq(data->mac_info);

    mac_program_addr_lookup_engine(data->mac_info);

    program_mac_registers(data->mac_info);

    recover_vif(data);

    recover_sta(data);

    data->mac_info->sta_ampdu_recover = true;

    return;
}
