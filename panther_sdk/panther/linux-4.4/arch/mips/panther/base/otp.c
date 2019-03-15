/*=============================================================================+
|                                                                              |
| Copyright 2016                                                               |
| Montage Technology, Inc. All rights reserved.                                |
|                                                                              |
+=============================================================================*/
/*!
*   \file sflash_controller.c
*   \brief c main entry
*   \author Montage
*/

/*=============================================================================+
| Included Files                                                               |
+=============================================================================*/
#include <asm/mach-panther/panther.h>
#include <asm/mach-panther/pdma.h>
#include <asm/mach-panther/otp.h>

#ifdef OTP_DEBUG_ENABLE
#define sf_log(fmt, args...) printk(fmt, ##args)
#else
#define sf_log(fmt, args...)
#endif

/*=============================================================================+
| Define                                                                       |
+=============================================================================*/
#define AXI_CLK_MHZ  230
#define PGMEN_USEC   10

#define OTP_BYTE_SIZE         256
#define OTP_BIT_SIZE          (8 * OTP_BYTE_SIZE)
#define OTP_ADDR_SHIFT_BIT    8

#define OTP_REG      0x00001810
//	    #define OTP_CLK_PLUSE_WIDTH   0x22c00000ul  //  bit[31:21]=11'h116
    #define OTP_CLK_PLUSE_WIDTH   (478 << 21)
    #define OTP_CLK_PLUSE_GEN     0x00100000
    #define OTP_READ_MODE_EN      0x00080000
    #define OTP_ADDR_VALID_BIT    0x000007ff
    #define OTP_DATA_VALID_BIT    0x000000ff

#define OTP_AVDD_REG 0x00001814
    #define OTP_AES_KEY_WRITE_PROTECT 0x00000010
    #define OTP_PROGRAM_MODE_EN       0x00000004
    #define OTP_CLK_PLUSE_WIDTH_BIT11 0x00000002
    #define OTP_AVDD_EN               0x00000001

#if defined(CONFIG_FPGA)
// not doing the delay to speedup testing time, as the OTP is not the real one in FPGA mode
#undef OTP_CLK_PLUSE_WIDTH
#define OTP_CLK_PLUSE_WIDTH       0x003000000UL
#undef OTP_CLK_PLUSE_WIDTH_BIT11
#define OTP_CLK_PLUSE_WIDTH_BIT11 0x0
#endif

/*=============================================================================+
| Variables                                                                    |
+=============================================================================*/
// Global variables
unsigned long otp_config;


int efuse_read(u8 *bufp, u32 index_start, u32 element_num)
{
    u32 i, index_max;
    u32 reg_data;

#if 1 // defined(CONFIG_FPGA)
    OTP_REG_WRITE32(OTP_AVDD_REG, 0);
#endif

    index_max = ((index_start + element_num) > OTP_BYTE_SIZE) ? (OTP_BYTE_SIZE) : (index_start + element_num);
    reg_data = OTP_READ_MODE_EN | OTP_CLK_PLUSE_WIDTH;
    OTP_REG_WRITE32(OTP_REG, reg_data);
    OTP_REG_UPDATE32(OTP_AVDD_REG, 0, OTP_CLK_PLUSE_WIDTH_BIT11);
    for (i = index_start; index_max > i; i++)
    {
        // Set address
        reg_data &= ~(OTP_ADDR_VALID_BIT << OTP_ADDR_SHIFT_BIT);
        reg_data |= ((i & OTP_ADDR_VALID_BIT) << OTP_ADDR_SHIFT_BIT);
        OTP_REG_WRITE32(OTP_REG, reg_data);
        // Enable clock generation
        //wait_for_period(1);
        reg_data |= OTP_CLK_PLUSE_GEN;
        OTP_REG_WRITE32(OTP_REG, reg_data);
        // Wait for data being ready
        while (reg_data & OTP_CLK_PLUSE_GEN)
        {
            reg_data = OTP_REG_READ32(OTP_REG);
        }

        bufp[(i - index_start)] = (u8) (reg_data & OTP_DATA_VALID_BIT);
    }

    return (int) (i - index_start);
}

void otp_read_config(void)
{
#ifdef IPL
    u8 _otp_config[OTP_CONFIG_LEN];
    efuse_read(_otp_config, OTP_CONFIG_BASE, OTP_CONFIG_LEN);
    // reorder bytes to support bi-endian
    otp_config = (_otp_config[2] << 16) | (_otp_config[1] << 8) | _otp_config[0];
#else
    u8 _otp_config[OTP_CONFIG_LEN];

    otp_config = *((volatile unsigned long *)0xbf005520UL);
    if(0==(otp_config & 0x80000000))
    {
        efuse_read(_otp_config, OTP_CONFIG_BASE, OTP_CONFIG_LEN);
        // reorder bytes to support bi-endian
        otp_config = (_otp_config[2] << 16) | (_otp_config[1] << 8) | _otp_config[0];
    }
    sf_log("OTP_CONFIG: %08x\n", otp_config);
#endif
}

unsigned long get_otp_config(void)
{
    return otp_config;
}

int otp_parse_config(int shift_val)
{
    int ret;

    switch (shift_val)
    {
        case OTP_ENABLE_SECURE_SHIFT:
        case OTP_DISABLE_KEY_WRITE_SHIFT:
        case OTP_KEY_ENCRYPT_SHIFT:
        case OTP_ENABLE_FLASH_SHIFT:
        case OTP_FLASH_BOOT_SELECT_SHIFT:
        case OTP_UART_SD_BOOT_SEL_SHIFT:
        case OTP_WATCHDOG_SHIFT:
        case OTP_JTAG_SHIFT:
        case OTP_READ_CMD_DUMMY_SHIFT:
        case OTP_READ_DATA_DUMMY_SHIFT:
        case OTP_CHECK_BAD_BLOCK_SHIFT:
        case OTP_UART_SD_BOOT_DISABLE_SHIFT:
        case OTP_SD_CLK_INV_SHIFT:
        case OTP_BOOT1_IMAGE_CHECK_SHIFT:
        case OTP_DISABLE_BAD_BLOCK_CHECK_SHIFT:
        case OTP_ONE_SECOND_WATCHDOG_SHIFT:
            ret = (otp_config >> shift_val) & 0x1;
            break;
        case OTP_PAGE_SIZE_SHIFT:
            ret = (otp_config >> shift_val) & 0x3;
            break;
        case OTP_SD_CLK_DIV_SHIFT:
            ret = (otp_config >> shift_val) & 0x7;
            break;
        case OTP_FLASH_CLK_DIV_SHIFT:
            ret = (otp_config >> shift_val) & 0x7;
            break;
        default:
            ret = 0;
    }

    return ret;
}

int otp_get_boot_type(void)
{
    int boot_type;

#ifdef IPL
    // read pin strapping
    unsigned int pin_strap;
    pin_strap = (*(volatile unsigned long *)(PIN_STRAP_REG_ADDR) >> 1) & 0x1;
    
    if (otp_parse_config(OTP_ENABLE_FLASH_SHIFT))
    {
        if((pin_strap == 1) && (0==otp_parse_config(OTP_UART_SD_BOOT_DISABLE_SHIFT)))
        {
            if (otp_parse_config(OTP_UART_SD_BOOT_SEL_SHIFT))
                boot_type = BOOT_FROM_UART;
            else
                boot_type = BOOT_FROM_SD;
        }
        else
        {
            if (otp_parse_config(OTP_FLASH_BOOT_SELECT_SHIFT))
            {
                boot_type = BOOT_FROM_NAND_WITH_OTP;
            }
            else
            {
                boot_type = BOOT_FROM_NOR;
            }
        }
    }
    else
    {
        if (pin_strap == 1)
        {
            boot_type = BOOT_FROM_NAND;
        }
        else
        {
            boot_type = BOOT_FROM_NOR;
        }
    }
#else
    boot_type = *((volatile unsigned long *)0xbf005524UL);
#endif

    return boot_type;
}

#if !defined(IPL)
static u8 pgm_byte_data[OTP_BYTE_SIZE];
int efuse_write(u8 *bufp, u32 index_start, u32 element_num)
{
    u32 addr, bit_offset, byte_offset, index_max, i;
    u32 reg_data;
    u8 tmp_data;
    int ret;
    //u64 pgm_start_time, pgm_end_time;

    // Initial pgm_byte_data
    for (i = 0; i < OTP_BYTE_SIZE; i++)
    {
        pgm_byte_data[i] = 0;
    }

    index_max = ((index_start + element_num) > OTP_BYTE_SIZE) ? (OTP_BYTE_SIZE) : (index_start + element_num);
    if (index_max > index_start)
    {
        // Do not program the programed bit
        for (i = index_start; i < index_max; i++)
        {
            ret = efuse_read(&tmp_data, i, 1);
            if (ret > 0)
            {
                pgm_byte_data[i-index_start] = bufp[i-index_start] & (~tmp_data);
            }
            else
            {
                return -1;
            }
        }
    }
    else
    {
        return -1;
    }

#if 0
    for (i = index_start; i < index_max; i++)
    {
        sf_log("%04d:%02x\n", i, pgm_byte_data[i]);
    }
#endif
    // Disable read enable
    reg_data = OTP_CLK_PLUSE_WIDTH;
    OTP_REG_WRITE32(OTP_REG, reg_data);
    OTP_REG_UPDATE32(OTP_AVDD_REG, 0, OTP_CLK_PLUSE_WIDTH_BIT11);
    // Enable AVDD
    //get_sim_time(&pgm_start_time);
    OTP_REG_WRITE32(OTP_AVDD_REG, OTP_AVDD_EN);
    //wait_for_period(1000);
    OTP_REG_WRITE32(OTP_AVDD_REG, OTP_PROGRAM_MODE_EN | OTP_AVDD_EN);
    reg_data = OTP_CLK_PLUSE_WIDTH;

    // obyte_data[i] = {odata[896+i],odata[768+i],odata[640+i],odata[512+i],odata[384+i],odata[256+i],odata[128+i],odata[i]};
#if 0
    for (i = 0; OTP_BIT_SIZE > i; i++)
    {
        bit_offset = i / OTP_BYTE_SIZE;
        byte_offset = i % OTP_BYTE_SIZE;

        // Program when data bit doesn't equal to 0
        if (write_byte_data[byte_offset] & (1 << bit_offset))
        {
            // Set address
            reg_data &= ~(OTP_ADDR_VALID_BIT << OTP_ADDR_SHIFT_BIT);
            reg_data |= ((i & OTP_ADDR_VALID_BIT) << OTP_ADDR_SHIFT_BIT);
            // Enable clock generation
            reg_data |= OTP_CLK_PLUSE_GEN;
            OTP_REG_WRITE32(OTP_REG, reg_data);
            // Wait for program done
            while (reg_data & OTP_CLK_PLUSE_GEN)
            {
                reg_data = OTP_REG_READ32(OTP_REG);
            }
        }
    }
#endif

#if defined(CONFIG_FPGA)
    for (byte_offset = index_start; index_max > byte_offset; byte_offset++)
    {
        for (bit_offset = 0; 8 > bit_offset; bit_offset++)
        {
            addr = bit_offset + (8 * byte_offset);
#else
    for (bit_offset = 0; 8 > bit_offset; bit_offset++)
    {
        for (byte_offset = index_start; index_max > byte_offset; byte_offset++)
        {
            addr = (bit_offset * OTP_BYTE_SIZE) + byte_offset;
#endif
            // Check address = [0, 2047]
            if (OTP_BIT_SIZE <= addr)
            {
                sf_log("!!!Address not valid %x\n", addr);
#if defined(SIM)
                cosim_stop();
#else
                return 0;
#endif
            }

            // Program when data bit doesn't equal to 0
            if (pgm_byte_data[(byte_offset - index_start)] & (1 << bit_offset))
            {
                // Set address
                reg_data &= ~(OTP_ADDR_VALID_BIT << OTP_ADDR_SHIFT_BIT);
                reg_data |= ((addr & OTP_ADDR_VALID_BIT) << OTP_ADDR_SHIFT_BIT);
                OTP_REG_WRITE32(OTP_REG, reg_data);
                // Enable clock generation
                //wait_for_period(1);
                reg_data |= OTP_CLK_PLUSE_GEN;
                OTP_REG_WRITE32(OTP_REG, reg_data);
                // Wait for program done
                while (reg_data & OTP_CLK_PLUSE_GEN)
                {
                    reg_data = OTP_REG_READ32(OTP_REG);
                }
            }
        }
    }
    // Disable program
    OTP_REG_WRITE32(OTP_AVDD_REG, OTP_AVDD_EN);
    reg_data = OTP_CLK_PLUSE_WIDTH;
    OTP_REG_WRITE32(OTP_REG, reg_data);
    //wait_for_period(1000);
    // Disable AVDD
    OTP_REG_WRITE32(OTP_AVDD_REG, 0);
    //get_sim_time(&pgm_end_time);

#if 0
    time_gap(8, pgm_start_time, pgm_end_time);
    if (1000000000 < (pgm_end_time - pgm_start_time))
    {
        sf_log("!!!Programing time exceed 1s\n");
        cosim_stop();
    }
#endif

    return (int)(byte_offset-index_start);
}

void check_result(u8 *write_byte_data, u8 *read_byte_data, int element_num)
{
    int i;
    for (i = 0; i < element_num; i++)
    {
        if (write_byte_data[i] != read_byte_data[i])
        {
            sf_log("offset %d, !!! Matched Error, expected data = %02x, read data = %02x\n"
                   ,i
                   ,write_byte_data[i]
                   ,read_byte_data[i]);
            //cosim_stop();
        }
    }
    return;
}

#endif

#if 0
//static u8 write_byte_data[OTP_BYTE_SIZE];
//static u8 read_byte_data[OTP_BYTE_SIZE];
void main(void)
{
    int i, ret;
    u16 read_offset, read_len, write_offset, write_len;
    static u8 write_byte_data[OTP_BYTE_SIZE];
    static u8 read_byte_data[OTP_BYTE_SIZE];

    sf_log("Simulation program start...\n");

    //sf_log("Read register %x: %08x\n", OTP_REG, OTP_REG_READ32(OTP_REG));

    // Initial write_byte_data and read_byte_data
    for (i = 0; OTP_BYTE_SIZE > i; i++)
    {
        write_byte_data[i] = 0;
        read_byte_data[i] = 0;
    }

    read_offset = 0;
    read_len = 33;
    // Check the initial OTP value
    ret = efuse_read(read_byte_data, read_offset, read_len);
    if (ret > 0)
    {
        check_result(write_byte_data, read_byte_data, ret);
    }
    else
    {
        sf_log("!!! read no data\n");
        cosim_stop();
    }

    for (i = 0; OTP_BYTE_SIZE > i; i++)
    {
        write_byte_data[i] = 0xff;
    }
#if 0
    for (i = 1; OTP_BYTE_SIZE > i; i++)
    {
        sf_log("%04d : %02x", i, write_byte_data[i]);
    }
#endif

    write_offset = 0;
    write_len = 33;
    // Program OTP
    ret = efuse_write(write_byte_data, write_offset, write_len);
    if (ret <= 0)
    {
        sf_log("!!! write no data\n");
        cosim_stop();
    }

    read_offset = write_offset;
    read_len = write_len;
    // Check the OTP value after program
    ret = efuse_read(read_byte_data, read_offset, read_len);
    if (ret > 0)
    {
        check_result(write_byte_data, read_byte_data, ret);
    }
    else
    {
        sf_log("!!! read no data\n");
        cosim_stop();
    }

    sf_log("Simulation program done\n");

    cosim_set_test_passed();
    cosim_stop();
}
#endif
