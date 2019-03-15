#ifndef __PANTHER_BOOT_INFO_H__

#define BOOT_FROM_UART   0
#define BOOT_FROM_NOR    1
#define BOOT_FROM_NAND   2
#define BOOT_FROM_NAND_WITH_OTP 3
#define BOOT_FROM_SD     4

/* used for initializa linux cmdline*/
#define BAD_BLOCK_STR_SIZE 0x100
#define MAX_BAD_BLOCK_NUM 40

#define NAND_MAX_OOBSIZE        640
#define NAND_MAX_PAGESIZE       8192
                              
#endif // __PANTHER_BOOT_INFO_H__