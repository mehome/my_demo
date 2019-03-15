#include <linux/kthread.h>
#include <linux/debugfs.h>
#include <linux/module.h>
#include <linux/ktime.h>
#include <dragonite_main.h>
#include <mac_tables.h>
#include <tx.h>

#include "panther_debug.h"

#define CLR_UNKNWN_PS_STAT_MSEC (1000)
extern unsigned long prev_recover_jiffies;
extern volatile u32 drg_wifi_recover;
extern int drg_ps_force_clr_cnt;

static int dragonite_kthread(void *priv)
{
    struct mac80211_dragonite_data *data = (struct mac80211_dragonite_data *) priv;
    int sta_idx;
    MAC_INFO *info = data->mac_info;
    u64 wakeup_t;

    do
    {
        if(drg_wifi_recover)
        {
            dragonite_wifi_recover(data);

            drg_wifi_recover = 0;
        }

        if(info->sta_ps_recover)
        {
            if(time_is_before_jiffies(prev_recover_jiffies + msecs_to_jiffies(CLR_UNKNWN_PS_STAT_MSEC)))
            {
                DRG_DBG("FORCE CLEAR nnknown ps state\n");

                dragonite_ps_lock();

                info->sta_ps_off_indicate |= info->sta_ps_recover;
                info->sta_ps_recover = 0;

                dragonite_ps_unlock();

                drg_ps_force_clr_cnt++;
            }
            else
            {
                for(sta_idx = 0; sta_idx < info->sta_idx_msb; sta_idx++)
                {
                    if(info->sta_ps_recover & (0x1UL << sta_idx))
                    {
                        if(0 != dragonite_tx_send_null_data(data, sta_idx))
                        {
                            DRG_WARN(DRAGONITE_DEBUG_DRIVER_WARN_FLAG, "kthread send null data failed\n");
                        }
                    }
                }
            }
        }

        dragonite_handle_power_saving_off(data);

        wakeup_t = jiffies;

        dragonite_tx_release_skb_queue(data, (u32) wakeup_t/HZ, false);

#if defined (DRAGONITE_TX_BHDR_CHECKER)
        dragonite_tx_bhdr_checker((u32) wakeup_t/HZ);
#endif

#if defined (DRAGONITE_TX_SKB_CHECKER)
        dragonite_tx_skb_checker(data, (u32) wakeup_t/HZ);
#endif

#if defined (DRAGONITE_TX_HANG_CHECKER)
        dragonite_tx_hang_checker(data, wakeup_t);
#endif

        if(drg_tx_stats.stats_enable)
        {
           if(!drg_tx_stats.stats_start)
           {   
               /* initial time and count */
               drg_tx_stats.stats_start = true;
               drg_tx_stats.jiffies_pre = jiffies;
               drg_tx_stats.tx_count_pre = drg_tx_stats.tx_count;
               drg_tx_stats.tx_done_count_pre = drg_tx_stats.tx_done_count;
           }
           if(time_after(jiffies, drg_tx_stats.jiffies_pre + drg_tx_stats.time_space))
           {
               DRG_DBG("Tx done delta/assign delta %d/%d, tx done/assign %d/%d, jiffies delta %ld\n", 
                      drg_tx_stats.tx_done_count - drg_tx_stats.tx_done_count_pre, drg_tx_stats.tx_count - drg_tx_stats.tx_count_pre, 
                      drg_tx_stats.tx_done_count, drg_tx_stats.tx_count, 
                      jiffies - drg_tx_stats.jiffies_pre);

               drg_tx_stats.jiffies_pre = jiffies;
               drg_tx_stats.tx_count_pre = drg_tx_stats.tx_count;
               drg_tx_stats.tx_done_count_pre = drg_tx_stats.tx_done_count;
           }
        }

        schedule_timeout_interruptible(HZ/10);

    }while(!kthread_should_stop());

    return 0;
}
int dragonite_kthread_init(struct mac80211_dragonite_data *data)
{
    int ret = 0;

    if(data->dragonited_task == NULL) {

        data->dragonited_task = kthread_run(dragonite_kthread, data, "dragonited");

        if(IS_ERR(data->dragonited_task)) {

            data->dragonited_task = NULL;
            ret = -ENOMEM;
        }
    }
    else
    {
        ret = -ENOMEM;
    }

    return ret;
}
void dragonite_kthread_cleanup(struct mac80211_dragonite_data *data)
{
    if(data->dragonited_task) {
        kthread_stop(data->dragonited_task);
    }
    data->dragonited_task = NULL;
}
