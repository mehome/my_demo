#ifndef __OTP_H__
#define __OTP_H__

enum {
    OTP_ERR_OK = 0,
    OTP_ERR_OPEN_DEV = -99,
    OTP_ERR_MMAP,
    OTP_ERR_ID_NOT_FOUND,
    OTP_ERR_NOT_INIT,
    OTP_ERR_PARAM,
    OTP_ERR_READ_FAILED,
};

enum
{
    OTP_MIN         = 1,
    OTP_TXVGA       = 1,
    OTP_FOFS        = 2,
    OTP_TXP_DIFF    = 3,
    OTP_MAC_ADDR    = 4,
    OTP_MAX         = 31,
};

#define OTP_EMPTY_VALUE         0x00
#define OTP_OVERWRITTEN_VALUE   0xFF

#define OTP_DATA_TYPE_NUM   8
#define OTP_ID_MASK         0x1F
#define OTP_DATALEN_MASK    0xE0
#define OTP_DATALEN_SHIFT   5

#define OTP_START_INDEX     36

#define OTP_LEN_CUSTOM      0
#define OTP_LEN_FOFS        1
#define OTP_LEN_TXP_DIFF    2
#define OTP_LEN_MAC         6
#define OTP_LEN_TXVGA       7
#define RESERVED_LEN        100

#define OTP_DEV_PATH        "/dev/uio0"
#define OTP_BYTE_SIZE       256
#define OTP_ADDR_SHIFT_BIT  8
#define OTP_REG_OFFSET      (0x00000810 / 4)
    // The register should be set with 0x4B0 with PBUS clock 120MHz.
    #define OTP_CLK_PLUSE_WIDTH   (0x4B0 << 21)
    #define OTP_CLK_PLUSE_GEN     0x00100000
    #define OTP_READ_MODE_EN      0x00080000
    #define OTP_ADDR_VALID_BIT    0x000007ff
    #define OTP_DATA_VALID_BIT    0x000000ff

#define OTP_AVDD_REG_OFFSET (0x00000814 / 4)
    #define OTP_AES_KEY_WRITE_PROTECT 0x00000010
    #define OTP_PROGRAM_MODE_EN       0x00000004
    #define OTP_CLK_PLUSE_WIDTH_BIT11 0x00000002
    #define OTP_AVDD_EN               0x00000001

struct otpdata {
    unsigned char *txvga_gain;
    unsigned char *fofs;
    unsigned char *txp_diff;
    unsigned char *mac_addr;
};

int get_otp_data(struct otpdata *data);

#endif