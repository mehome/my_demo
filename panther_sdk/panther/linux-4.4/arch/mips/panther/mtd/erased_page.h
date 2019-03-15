#ifndef __ERASED_PAGE_H__
#define __ERASED_PAGE_H__

#define AES_BLOCK_SIZE 16

extern unsigned char __erased_page_data_first[];
extern unsigned char __erased_page_data_next[];

void load_erased_page_data_from_boot_cmdline(void);

static inline int is_erased_page(unsigned char *data, int size)
{
  int i;
  if(memcmp(__erased_page_data_first, &data[0], AES_BLOCK_SIZE))
     return 0;
  for(i=AES_BLOCK_SIZE;i<size;i+=AES_BLOCK_SIZE)
  {
     if(memcmp(__erased_page_data_next, &data[i], AES_BLOCK_SIZE))
        return 0;
  }
  return 1;
}

#endif // __ERASED_PAGE_H__
