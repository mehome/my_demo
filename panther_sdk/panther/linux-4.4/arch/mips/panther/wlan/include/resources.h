#ifndef __RESOURCES_H__
#define __RESOURCES_H__

#include <mac.h>
#include <mac_tables.h>
//#include <mac_sim_config.h>

#define DRAGONITE_MEMORY_CHECK

struct dragonite_malloc_meta
{
    u32 magic;
    u32 jiffies;            /* the jiffies when mac_free() got called for this memory resource */
    u32 size;
    u32 checksum;
    u32 flags;
    struct dragonite_malloc_meta *prev;
    struct dragonite_malloc_meta *next;
    u32 reserved[1];
};

void resource_debug_dump(void);
u8 *alloc_protect_guard_head(void);
u8 *alloc_protect_guard_tail(void);
buf_header* alloc_buf_headers(int count);
void free_buf_headers(int count);
rx_descriptor* alloc_rx_descriptors(int count);
void free_rx_descriptors(void);
void *alloc_ext_sta_table(void);
void free_ext_sta_table(void);
void *alloc_ext_ds_table(void);
void free_ext_ds_table(void);

#if defined(DRAGONITE_MEMORY_CHECK)
int mac_malloc_info(char *msg, int dump);
#else
int mac_malloc_info(char *msg);
#endif
void *mac_malloc(MAC_INFO* info, int size, int flags);
void mac_free(void *ptr);

#define DRAGONITE_MM_ALLOC_MAGIC       0x4D616C4CUL  /* MalL */
#define DRAGONITE_MM_FREE_MAGIC        0X46726545UL  /* FreE */
#define DRAGONITE_MM_INVALID           0x494E564CUL  /* INVL */

#define ___CACHED_ADDR(x) ( ((u32) (x) & 0x0FFFFFFFUL) | 0x80000000UL)

#endif /*__RESOURCES_H__*/
