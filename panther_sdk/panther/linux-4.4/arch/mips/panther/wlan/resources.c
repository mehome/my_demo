#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/skbuff.h>
#include <resources.h>
#include <linux/jiffies.h>
#include <linux/timer.h>
#include <linux/netdevice.h>

#include "dragonite_common.h"

#define MAX_DATA_FRAME_HEADERS_COUNT  (BUFFER_HEADER_POOL_SIZE+BEACON_Q_BUFFER_HEADER_POOL_SIZE+4)
static int alloc_buf_headers_count = 0;

struct wifi_sram_resources
{
    u8 __protect_guard_head[PROTECT_GUARD_COUNT];
    buf_header __buf_header[MAX_DATA_FRAME_HEADERS_COUNT]; 
    rx_descriptor __rx_descriptor[RX_DESCRIPTOR_COUNT];
    u8 __ext_sta_table[MAX_STA_NUM][10];
    u8 __ext_ds_table[MAX_DS_NUM][10];
    u8 __protect_guard_tail[PROTECT_GUARD_COUNT];
};

struct wifi_sram_resources* wifisram = (void*) 0xB0004000;

unsigned int wlan_driver_sram_size = sizeof(struct wifi_sram_resources);

void* wlan_driver_sram_base = &wifisram;

void resource_debug_dump(void)
{
    int i, j, dump_times = (wlan_driver_sram_size / 32) + 1; /* per 32 bytes a line */
    u32 *ptr;

    ptr = (u32 *) wifisram;

    printk("\nDRAGONITE: resource_debug_dump\n");

    for(i=0;i<dump_times;i++)
    {
        printk("0x%p: ", ptr+(i*8));
        for(j=0;j<8;j++)
        {
            printk("%08x ", *(ptr+(i*8)+j));
        }
        printk("\n");
    }
}

u8 *alloc_protect_guard_head(void)
{
    u8 *alloc;

    alloc = (u8 *) wifisram->__protect_guard_head;

    return (u8 *) UNCACHED_ADDR(alloc);  
}

u8 *alloc_protect_guard_tail(void)
{
    u8 *alloc;

    alloc = (u8 *) wifisram->__protect_guard_tail;

    return (u8 *) UNCACHED_ADDR(alloc);  
}

buf_header* alloc_buf_headers(int count)
{
    buf_header* alloc;

    if(0==alloc_buf_headers_count)
        dma_cache_wback_inv(CACHED_ADDR(wifisram), wlan_driver_sram_size);

    if ((alloc_buf_headers_count+count) > MAX_DATA_FRAME_HEADERS_COUNT)
    {
        ASSERT(0, "alloc_buf_headers failed\n");
    }

    alloc = &wifisram->__buf_header[alloc_buf_headers_count];

    alloc_buf_headers_count += count;

    return (buf_header*) UNCACHED_ADDR(alloc);
}

void free_buf_headers(int count)
{
    buf_header *free_head;

    if ((alloc_buf_headers_count - count) < 0)
    {
        ASSERT(0, "free_buf_headers failed\n");
    }
    alloc_buf_headers_count -= count;

    free_head = &(wifisram->__buf_header[alloc_buf_headers_count]);
    memset(free_head, 0, (count * sizeof(buf_header)));

    return;
}

rx_descriptor* alloc_rx_descriptors(int count)
{
    rx_descriptor* alloc;

    if (count > RX_DESCRIPTOR_COUNT)
    {
        ASSERT(0, "alloc_rx_descriptor failed\n");
    }

    alloc = wifisram->__rx_descriptor;

    return (rx_descriptor*) UNCACHED_ADDR(alloc);
}

void free_rx_descriptors(void)
{
    memset(wifisram->__rx_descriptor, 0, (RX_DESCRIPTOR_COUNT * sizeof(rx_descriptor)));
    return;
}

void *alloc_ext_sta_table(void)
{
    u8 *alloc;

    alloc = (u8 *) wifisram->__ext_sta_table;

    return (void *)UNCACHED_ADDR(alloc);
}
void free_ext_sta_table(void)
{
    memset(wifisram->__ext_sta_table, 0, (MAX_STA_NUM * 10));
    return;
}
void *alloc_ext_ds_table(void)
{
    u8 *alloc;

    alloc = (u8 *) wifisram->__ext_ds_table;

    return (void *)UNCACHED_ADDR(alloc);  
}
void free_ext_ds_table(void)
{
    memset(wifisram->__ext_ds_table, 0, (MAX_DS_NUM * 10));
    return;
}
static int mac_alloc_skb_no_watermarks_warn = 1;
void *mac_alloc_skb_buffer(MAC_INFO* info, int size, int flags, u32* pskb)
{
    struct sk_buff * skb;

    skb = __dev_alloc_skb(size, GFP_ATOMIC);
    if(skb)
    {
        if((unsigned long)skb->data % L1_CACHE_BYTES)
        {
            panic("mac_alloc_skb_buffer(): skb data not cache aligned\n");
        }

        if(flags & MAC_MALLOC_UNCACHED)
        {
            dma_cache_wback_inv((unsigned long)skb->data, size);
        }

        *pskb = (u32) skb;
        return (void *)skb->data;
    }

    if(mac_alloc_skb_no_watermarks_warn)
    {
        printk(KERN_EMERG "dragonite_mm: alloc skb with no watermarks\n");
        mac_alloc_skb_no_watermarks_warn = 0;
    }
    skb = __dev_alloc_skb(size, GFP_ATOMIC | __GFP_MEMALLOC);
    if(skb)
    {
        if((unsigned long)skb->data % L1_CACHE_BYTES)
        {
            panic("mac_alloc_skb_buffer(): skb data not cache aligned\n");
        }

        if(flags & MAC_MALLOC_UNCACHED)
        {
            dma_cache_wback_inv((unsigned long)skb->data, size);
        }

        *pskb = (u32) skb;
        return (void *)skb->data;
    }

    *pskb = (u32) NULL;
    return NULL;
}

void mac_kfree_skb_buffer(u32 *pskb)
{
    struct sk_buff *skb = (struct sk_buff *)(*pskb);

    dev_kfree_skb_any(skb);
    return;
}

#if defined(DRAGONITE_MEMORY_CHECK)
u32 mm_total_chunks = 0;
u32 mm_total_alloc_size = 0;
u32 mm_malloc_count = 0;
u32 mm_free_count = 0;

static DEFINE_SPINLOCK(dragonite_mm_lock);
static struct dragonite_malloc_meta* alloc_mm_head = NULL;

static void mm_alloclist_insert_head(struct dragonite_malloc_meta* mm)
{
    mm->prev = NULL;

    if(alloc_mm_head)
    {
        mm->next = alloc_mm_head;
        alloc_mm_head->prev = mm;

        alloc_mm_head = mm;
    }
    else
    {
        mm->next = NULL;
        alloc_mm_head = mm;
    }
}

static void mm_alloclist_remove(struct dragonite_malloc_meta* mm)
{
    if(mm == alloc_mm_head)
        alloc_mm_head = mm->next;

    if(mm->prev)
        mm->prev->next = mm->next;

    if(mm->next)
        mm->next->prev = mm->prev;
}

#endif

#if defined(DRAGONITE_MEMORY_CHECK)
int mac_malloc_info(char *msg, int dump)
{
    unsigned long irqflags;
    struct dragonite_malloc_meta* mm;
    int i = 0;

    printk(KERN_DEBUG "dragonite_mm: %s total_chunks %d\n", msg, mm_total_chunks);

    if(dump)
    {
        spin_lock_irqsave(&dragonite_mm_lock, irqflags);

        mm = alloc_mm_head;

        while(mm)
        {
            printk("%d %08x %d %08x\n", ++i, (unsigned) &mm[1], mm->size, mm->flags);
            mm = mm->next;
        }

        spin_unlock_irqrestore(&dragonite_mm_lock, irqflags);
    }

    return mm_total_chunks;
}

#else
int mac_malloc_info(char *msg)
{
    return 0;
}

#endif

#if defined(DRAGONITE_MEMORY_CHECK)
void *___mac_malloc(MAC_INFO* info, int size, int flags)
#else
void *mac_malloc(MAC_INFO* info, int size, int flags)
#endif
{
    void *ptr;

    if(flags & MAC_MALLOC_ATOMIC)
    {
        if(flags & MAC_MALLOC_DMA)
            ptr = kmalloc(size, GFP_ATOMIC | GFP_DMA);
        else
            ptr = kmalloc(size, GFP_ATOMIC);
    }
    else
    {
        if(flags & MAC_MALLOC_DMA)
            ptr = kmalloc(size, GFP_KERNEL | GFP_DMA);
        else
            ptr = kmalloc(size, GFP_KERNEL);
    }


    if(ptr)
    {
        if(flags & MAC_MALLOC_BZERO)
            memset(ptr, 0, size);

        if(flags & MAC_MALLOC_UNCACHED)
        {
            dma_cache_wback_inv((unsigned long)ptr, size);
            return(void*) UNCAC_ADDR(ptr);
        }
        else
        {
            return(void*) ptr;
        }
    }
    else if(flags & MAC_MALLOC_ASSERTION)
    {
        panic("mac_malloc(): allocation failure, size %d\n", size);
    }


    return NULL;
}

#if defined(DRAGONITE_MEMORY_CHECK)
void *mac_malloc(MAC_INFO* info, int size, int flags)
{
    unsigned long irqflags;
    struct dragonite_malloc_meta *mm, *mm_caddr;

    if(sizeof(struct dragonite_malloc_meta) != L1_CACHE_BYTES)
        panic("malloc metadata is not cacheline aligned\n");

    mm = ___mac_malloc(info, (size + sizeof(struct dragonite_malloc_meta)), flags);

    if(NULL == mm)
        return NULL;

    mm_caddr = (struct dragonite_malloc_meta*) ___CACHED_ADDR(mm);

    mm_caddr->magic = DRAGONITE_MM_ALLOC_MAGIC;
    mm_caddr->jiffies = jiffies;
    mm_caddr->size = size;
    mm_caddr->flags = flags;

    spin_lock_irqsave(&dragonite_mm_lock, irqflags);

    mm_total_chunks++;
    mm_malloc_count++;
    mm_total_alloc_size += size;
    mm_alloclist_insert_head(mm_caddr);

    spin_unlock_irqrestore(&dragonite_mm_lock, irqflags);

#if defined(DRAGONITE_MM_DEBUG)
    printk("M %08x %d %08x\n", (unsigned) &mm[1], size, flags);
#endif

    return &mm[1];
}
#endif

#if defined(DRAGONITE_MEMORY_CHECK)
void ___mac_free(void *ptr)
#else
void mac_free(void *ptr)
#endif
{
    if(ptr)
    {
        if((unsigned long)ptr >= UNCAC_BASE)
            kfree(CAC_ADDR(ptr));
        else
            kfree(ptr);
    }
}

#if defined(DRAGONITE_MEMORY_CHECK)
void mac_free(void *ptr)
{
    unsigned long irqflags;
    struct dragonite_malloc_meta *mm, *mm_caddr;

    if(ptr == NULL)
        return;

    mm = (struct dragonite_malloc_meta*) (void *)(ptr - sizeof(struct dragonite_malloc_meta));

#if defined(DRAGONITE_MM_DEBUG)
    printk("F %08x\n", (unsigned) &mm[1]);
#endif

    mm_caddr = (struct dragonite_malloc_meta*) ___CACHED_ADDR(mm);

    if(mm_caddr->magic == DRAGONITE_MM_FREE_MAGIC)
        printk("mac_free(): double free detected, addr %p, flags %x, size %d\n", ptr, mm_caddr->flags, mm_caddr->size);

    if(mm_caddr->magic != DRAGONITE_MM_ALLOC_MAGIC)
        panic("mac_free(): wrong alloc magic (%x)\n", mm_caddr->magic);

    spin_lock_irqsave(&dragonite_mm_lock, irqflags);

    mm_alloclist_remove(mm_caddr);

    mm_total_chunks--;
    mm_total_alloc_size -= mm->size;
    mm_free_count++;

    mm_caddr->magic = DRAGONITE_MM_FREE_MAGIC;
    mm_caddr->jiffies = jiffies;
    mm_caddr->prev = (void *) DRAGONITE_MM_FREE_MAGIC;
    mm_caddr->next = (void *) DRAGONITE_MM_FREE_MAGIC;

    spin_unlock_irqrestore(&dragonite_mm_lock, irqflags);

    ___mac_free(mm);
}

#endif

unsigned int wla_current_time(void)
{
    return jiffies;
}

struct delayed_kfree_item {
    struct timer_list timer;
    unsigned long kfree_objp;
};

static void delayed_kfree_timer(unsigned long arg)
{
    struct delayed_kfree_item *dki = (struct delayed_kfree_item *) arg;
    printk(KERN_DEBUG "start delayed_kfree_timer\n");
    kfree((const void *) dki->kfree_objp);
    kfree((const void *) arg);
    printk(KERN_DEBUG "finish delayed_kfree_timer\n");
}

void kfree_delayed(const void *objp, unsigned long delay)
{
    struct delayed_kfree_item *dki;

    dki = (struct delayed_kfree_item *) kmalloc(sizeof(struct delayed_kfree_item), GFP_ATOMIC);
    if(NULL==dki)
    {
        printk(KERN_WARNING "dragonite_mm: kfree_delayed allocate timer failed\n");
        kfree(objp);
        return;
    }

    memset(dki, 0, sizeof(struct delayed_kfree_item));

    dki->kfree_objp = (unsigned long) objp;

    init_timer(&dki->timer);
    dki->timer.data = (unsigned long) dki;
    dki->timer.function = delayed_kfree_timer;
    dki->timer.expires = jiffies + delay;
    add_timer(&dki->timer);
}

