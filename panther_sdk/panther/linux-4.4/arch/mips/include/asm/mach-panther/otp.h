#ifndef __PANTHER_OTP_H__
#define __PANTHER_OTP_H__

#define REG(base,offset)                (((volatile unsigned int *)base)[offset >> 2])
#define OTP_REG_READ32(offset)         REG(PANTHER_REG_BASE, offset)
#define OTP_REG_WRITE32(offset, val)   REG(PANTHER_REG_BASE, offset) = (unsigned int)(val)
#define OTP_REG_UPDATE32(x, val, mask) do {                   \
    unsigned int newval;                                  \
    newval = *(volatile unsigned int*) (MI_BASE + (x));   \
    newval = (( newval & ~(mask) ) | ( (val) & (mask) )); \
    *(volatile unsigned int*)(MI_BASE + (x)) = newval;    \
} while(0)

/* OTP configuration */
#define OTP_CONFIG_BASE 32   // bytes
#define OTP_CONFIG_LEN 3     // bytes
#define OTP_ENABLE_SECURE_SHIFT        0
#define OTP_DISABLE_KEY_WRITE_SHIFT    1
#define OTP_KEY_ENCRYPT_SHIFT          2
#define OTP_ENABLE_FLASH_SHIFT         3
#define OTP_FLASH_BOOT_SELECT_SHIFT    4
#define OTP_UART_SD_BOOT_SEL_SHIFT     5
#define OTP_WATCHDOG_SHIFT             6
#define OTP_JTAG_SHIFT                 7
#define OTP_READ_CMD_DUMMY_SHIFT       8
#define OTP_READ_DATA_DUMMY_SHIFT      9
#define OTP_PAGE_SIZE_SHIFT           10
#define OTP_CHECK_BAD_BLOCK_SHIFT     12
#define OTP_UART_SD_BOOT_DISABLE_SHIFT 13
#define OTP_SD_CLK_INV_SHIFT          14
#define OTP_BOOT1_IMAGE_CHECK_SHIFT   15
#define OTP_DISABLE_BAD_BLOCK_CHECK_SHIFT  16
#define OTP_SD_CLK_DIV_SHIFT               17
#define OTP_FLASH_CLK_DIV_SHIFT            20
#define OTP_ONE_SECOND_WATCHDOG_SHIFT      23

#endif // __PANTHER_OTP_H__

