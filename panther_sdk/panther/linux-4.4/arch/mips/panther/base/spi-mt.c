/*
 *  Copyright (c) 2018  Montage Inc.    All rights reserved.
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/spinlock.h>
#include <linux/workqueue.h>
#include <linux/interrupt.h>
#include <linux/errno.h>
#include <linux/slab.h>

#include <linux/spi/spi.h>
#include <asm/mach-panther/pmu.h>
#include <asm/mach-panther/pinmux.h>

/* NOTE: software based SPI is for reference only; it is fixed to use GPIO 17/18/19/20 */
#define ENABLE_HARDWARE_SPI
//#define HARDWARE_SPI_LOW_SPEED_SETTING

#define DRIVER_NAME     "spi-mt"
#define SPI_N_CHIPSEL   1

#if !defined(ENABLE_HARDWARE_SPI)
#define SPI_MISO_GPIO   17
#define SPI_MOSI_GPIO   18
#define SPI_SCK_GPIO    20
#undef NEED_SPIDELAY
#endif

struct spi_bitbang {
	struct mutex		lock;
	u8			busy;
	u8			use_dma;
	u8			flags;		/* extra spi->mode support */

	struct spi_master	*master;

	/* setup_transfer() changes clock and/or wordsize to match settings
	 * for this transfer; zeroes restore defaults from spi_device.
	 */
	int	(*setup_transfer)(struct spi_device *spi,
			struct spi_transfer *t);

	void	(*chipselect)(struct spi_device *spi, int is_on);
#define	BITBANG_CS_ACTIVE	1	/* normally nCS, active low */
#define	BITBANG_CS_INACTIVE	0

	/* txrx_bufs() may handle dma mapping for transfers that don't
	 * already have one (transfer.{tx,rx}_dma is zero), or use PIO
	 */
	int	(*txrx_bufs)(struct spi_device *spi, struct spi_transfer *t);

	/* txrx_word[SPI_MODE_*]() just looks like a shift register */
	u32	(*txrx_word[4])(struct spi_device *spi,
			unsigned nsecs,
			u32 word, u8 bits);
};

#define SPI_BITBANG_CS_DELAY	100

struct spi_bitbang_cs {
	unsigned	nsecs;	/* (clock cycle time)/2 */
	u32		(*txrx_word)(struct spi_device *spi, unsigned nsecs,
					u32 word, u8 bits);
	unsigned	(*txrx_bufs)(struct spi_device *,
					u32 (*txrx_word)(
						struct spi_device *spi,
						unsigned nsecs,
						u32 word, u8 bits),
					unsigned, struct spi_transfer *);
};

static unsigned bitbang_txrx_8(
	struct spi_device	*spi,
	u32			(*txrx_word)(struct spi_device *spi,
					unsigned nsecs,
					u32 word, u8 bits),
	unsigned		ns,
	struct spi_transfer	*t
) {
	unsigned		bits = t->bits_per_word;
	unsigned		count = t->len;
	const u8		*tx = t->tx_buf;
	u8			*rx = t->rx_buf;

	while (likely(count > 0)) {
		u8		word = 0;

		if (tx)
			word = *tx++;
		word = txrx_word(spi, ns, word, bits);
		if (rx)
			*rx++ = word;
		count -= 1;
	}
	return t->len - count;
}

static unsigned bitbang_txrx_16(
	struct spi_device	*spi,
	u32			(*txrx_word)(struct spi_device *spi,
					unsigned nsecs,
					u32 word, u8 bits),
	unsigned		ns,
	struct spi_transfer	*t
) {
	unsigned		bits = t->bits_per_word;
	unsigned		count = t->len;
	const u16		*tx = t->tx_buf;
	u16			*rx = t->rx_buf;

	while (likely(count > 1)) {
		u16		word = 0;

		if (tx)
			word = *tx++;
		word = txrx_word(spi, ns, word, bits);
		if (rx)
			*rx++ = word;
		count -= 2;
	}
	return t->len - count;
}

static unsigned bitbang_txrx_32(
	struct spi_device	*spi,
	u32			(*txrx_word)(struct spi_device *spi,
					unsigned nsecs,
					u32 word, u8 bits),
	unsigned		ns,
	struct spi_transfer	*t
) {
	unsigned		bits = t->bits_per_word;
	unsigned		count = t->len;
	const u32		*tx = t->tx_buf;
	u32			*rx = t->rx_buf;

	while (likely(count > 3)) {
		u32		word = 0;

		if (tx)
			word = *tx++;
		word = txrx_word(spi, ns, word, bits);
		if (rx)
			*rx++ = word;
		count -= 4;
	}
	return t->len - count;
}

static int spi_bitbang_setup_transfer(struct spi_device *spi, struct spi_transfer *t)
{
	struct spi_bitbang_cs	*cs = spi->controller_state;
	u8			bits_per_word;
	u32			hz;

	if (t) {
		bits_per_word = t->bits_per_word;
		hz = t->speed_hz;
	} else {
		bits_per_word = 0;
		hz = 0;
	}

	/* spi_transfer level calls that work per-word */
	if (!bits_per_word)
		bits_per_word = spi->bits_per_word;
	if (bits_per_word <= 8)
		cs->txrx_bufs = bitbang_txrx_8;
	else if (bits_per_word <= 16)
		cs->txrx_bufs = bitbang_txrx_16;
	else if (bits_per_word <= 32)
		cs->txrx_bufs = bitbang_txrx_32;
	else
		return -EINVAL;

	/* nsecs = (clock period)/2 */
	if (!hz)
		hz = spi->max_speed_hz;
	if (hz) {
		cs->nsecs = (1000000000/2) / hz;
		if (cs->nsecs > (MAX_UDELAY_MS * 1000 * 1000))
			return -EINVAL;
	}

	return 0;
}

static int spi_bitbang_setup(struct spi_device *spi)
{
	struct spi_bitbang_cs	*cs = spi->controller_state;
	struct spi_bitbang	*bitbang;

	bitbang = spi_master_get_devdata(spi->master);

	if (!cs) {
		cs = kzalloc(sizeof(*cs), GFP_KERNEL);
		if (!cs)
			return -ENOMEM;
		spi->controller_state = cs;
	}

	/* per-word shift register access, in hardware or bitbanging */
	cs->txrx_word = bitbang->txrx_word[spi->mode & (SPI_CPOL|SPI_CPHA)];
	if (!cs->txrx_word)
		return -EINVAL;

	if (bitbang->setup_transfer) {
		int retval = bitbang->setup_transfer(spi, NULL);
		if (retval < 0)
			return retval;
	}

	dev_dbg(&spi->dev, "%s, %u nsec/bit\n", __func__, 2 * cs->nsecs);

	/* NOTE we _need_ to call chipselect() early, ideally with adapter
	 * setup, unless the hardware defaults cooperate to avoid confusion
	 * between normal (active low) and inverted chipselects.
	 */

	/* deselect chip (low or high) */
	mutex_lock(&bitbang->lock);
	if (!bitbang->busy) {
		bitbang->chipselect(spi, BITBANG_CS_INACTIVE);
		ndelay(cs->nsecs);
	}
	mutex_unlock(&bitbang->lock);

	return 0;
}

static void spi_bitbang_cleanup(struct spi_device *spi)
{
	kfree(spi->controller_state);
}

static int spi_bitbang_bufs(struct spi_device *spi, struct spi_transfer *t)
{
	struct spi_bitbang_cs	*cs = spi->controller_state;
	unsigned		nsecs = cs->nsecs;

	return cs->txrx_bufs(spi, cs->txrx_word, nsecs, t);
}

static int spi_bitbang_prepare_hardware(struct spi_master *spi)
{
	struct spi_bitbang	*bitbang;

	bitbang = spi_master_get_devdata(spi);

	mutex_lock(&bitbang->lock);
	bitbang->busy = 1;
	mutex_unlock(&bitbang->lock);

	return 0;
}

static int spi_bitbang_transfer_one(struct spi_master *master,
				    struct spi_device *spi,
				    struct spi_transfer *transfer)
{
	struct spi_bitbang *bitbang = spi_master_get_devdata(master);
	int status = 0;

	if (bitbang->setup_transfer) {
		status = bitbang->setup_transfer(spi, transfer);
		if (status < 0)
			goto out;
	}

	if (transfer->len)
		status = bitbang->txrx_bufs(spi, transfer);

	if (status == transfer->len)
		status = 0;
	else if (status >= 0)
		status = -EREMOTEIO;

out:
	spi_finalize_current_transfer(master);

	return status;
}

static int spi_bitbang_unprepare_hardware(struct spi_master *spi)
{
	struct spi_bitbang	*bitbang;

	bitbang = spi_master_get_devdata(spi);

	mutex_lock(&bitbang->lock);
	bitbang->busy = 0;
	mutex_unlock(&bitbang->lock);

	return 0;
}

static void spi_bitbang_set_cs(struct spi_device *spi, bool enable)
{
	struct spi_bitbang *bitbang = spi_master_get_devdata(spi->master);

	/* SPI core provides CS high / low, but bitbang driver
	 * expects CS active
	 * spi device driver takes care of handling SPI_CS_HIGH
	 */
	enable = (!!(spi->mode & SPI_CS_HIGH) == enable);

	ndelay(SPI_BITBANG_CS_DELAY);
	bitbang->chipselect(spi, enable ? BITBANG_CS_ACTIVE :
			    BITBANG_CS_INACTIVE);
	ndelay(SPI_BITBANG_CS_DELAY);
}

static int spi_bitbang_start(struct spi_bitbang *bitbang)
{
	struct spi_master *master = bitbang->master;
	int ret;

	if (!master || !bitbang->chipselect)
		return -EINVAL;

	mutex_init(&bitbang->lock);

	if (!master->mode_bits)
		master->mode_bits = SPI_CPOL | SPI_CPHA | bitbang->flags;

	if (master->transfer || master->transfer_one_message)
		return -EINVAL;

	master->prepare_transfer_hardware = spi_bitbang_prepare_hardware;
	master->unprepare_transfer_hardware = spi_bitbang_unprepare_hardware;
	master->transfer_one = spi_bitbang_transfer_one;
	master->set_cs = spi_bitbang_set_cs;

	if (!bitbang->txrx_bufs) {
		bitbang->use_dma = 0;
		bitbang->txrx_bufs = spi_bitbang_bufs;
		if (!master->setup) {
			if (!bitbang->setup_transfer)
				bitbang->setup_transfer =
					 spi_bitbang_setup_transfer;
			master->setup = spi_bitbang_setup;
			master->cleanup = spi_bitbang_cleanup;
		}
	}

	/* driver may get busy before register() returns, especially
	 * if someone registered boardinfo for devices
	 */
	ret = spi_register_master(spi_master_get(master));
	if (ret)
		spi_master_put(master);

	return 0;
}

static void spi_bitbang_stop(struct spi_bitbang *bitbang)
{
	spi_unregister_master(bitbang->master);
}

#if defined(ENABLE_HARDWARE_SPI)

#if defined(HARDWARE_SPI_LOW_SPEED_SETTING)
#define SPI_INT_DLY   0xe
#define SPI_TXD_DELAY (0x1f << 1)
static unsigned long default_clkdiv = 0x40; // 120Mhz / 64 = 1.8Mhz
#else
#define SPI_INT_DLY   3
#define SPI_TXD_DELAY (1 << 1)
static unsigned long default_clkdiv = 10; // 120Mhz / 10 = 12Mhz
#endif

#define REG(base,offset) (*(volatile unsigned int*)(base+(offset)))
#define GSPI_BASE        0xBF002E00UL
#define GSPIREG(offset)  REG(GSPI_BASE,offset)

/* SPI bank register */
#define DFIFO          0x000
#define CH0_BAUD       0x100
#define  SPI_DLYSHFT      28    /* shift for delay field */
#define CH0_MODE_CFG   0x104
#define  SPI_DUMYH        (1<<31)       /* high impedance for dummy */
#define  SPI_CSRTRN       (1<<30)       /* cs retrun to default automatically */
#define  SPI_BYTEPKG      (1<<29)       /* pack data unit at byte */
#define  SPI_WTHSHFT      24    /* shift for data width field */
#define  SPI_CLDSHFT      18    /* shift for CLK delay field */
#define  SPI_CSDSHFT      16    /* shift for CS delay field */
#define  SPI_DBYTE        (1<<29)       /* data unit: byte */
#define  SPI_CPHA_WR      (1<<14)       /* cpha, data write */
#define  SPI_CFG_CPOL     (1<<13)       /* cpol */
#define  SPI_LSB          (1<<12)       /* least significant bit first */
#define  SPI_CPHA_RD      (1<<11)       /* cpha, data read */
#define  SPI_DPACK        (1<<10)       /* enable data package */
#define  SPI_HBYTE        (1<<9)        /* high byte first */
#define  SPI_FSHMODE      (1<<8)        /* change to flash mode */
#define CH1_BAUD       0x108
#define CH1_MODE_CFG   0x10C
#define CH2_BAUD       0x110
#define CH2_MODE_CFG   0x114
#define CH3_BAUD       0x118
#define CH3_MODE_CFG   0x11C
#define SPI_TC         0x120
#define SPI_CTRL       0x124
#define  SPI_C1NSHFT      30    /* shift for cmd1 io num field */
#define  SPI_C1LSHFT      24    /* shift for cmd1 len field */
#define  SPI_C0NSHFT      22    /* shift for cmd0 io num field */
#define  SPI_C0LSHFT      16    /* shift for cmd0 len field */
#define  SPI_DNTSHFT      14    /* shift for data io num field */
#define  SPI_DULSHFT      3     /* shift for dummy len field */
#define  SPI_DIRSHFT      1     /* shift for direction field */
#define  SPI_TRIGGER      (1<<0)        /* kick */
#define FIFO_CFG       0x128
#define SMPL_MODE      0x12C
#define PIN_MODE       0x130
#define  SPI_IO3SHFT       6    /* shift for io3 direction field */
#define  SPI_IO2SHFT       4    /* shift for io2 direction field */
#define  SPI_IO1SHFT       2    /* shift for io1 direction field */
#define  SPI_IO0SHFT       0    /* shift for io0 direction field */
#define  SPI_PININ         0    /* in direction */
#define  SPI_PINOUT        1    /* out direction */
#define  SPI_PINDIN        2    /* in direction at duplex mode, in/out direction at half-duplex mode */
#define  SPI_PINDOUT       3    /* out direction at duplex mode, in/out direction at half-duplex mode */
#define PIN_CTRL       0x134
#define  SPI_3WMODE       (1<<16)       /* 3 wire mode */
#define  SPI_03SHFT        6    /* shift for MOSI3 setting field */
#define  SPI_02SHFT        4    /* shift for MOSI2 setting field */
#define  SPI_01SHFT        2    /* shift for MOSI1 setting field */
#define  SPI_00SHFT        0    /* shift for MOSI0 setting field */
#define  SPI_OUTLOW        1    /* output low level */
#define  SPI_OUTHGH        2    /* output high level */
#define CH_MUX         0x138
#define  SPI_IO1CS3       (1<<2)        /* use io1 as channel3's cs */
#define  SPI_IO2CS2       (1<<1)        /* use io2 as channel2's cs */
#define  SPI_IO3CS1       (1<<1)        /* use io3 as channel1's cs */
#define STA            0x140
#define  SPI_BUSY         (1<<16)       /* spi busy state */
#define  SPI_TCSHFT        8    /* shift for TXD FIFO count state field */
#define  SPI_RCSHFT        0    /* shift for RXD FIFO count state field */
#define INT_STA        0x144
#define CMD_FIFO       0x148

static unsigned long ch_baud_cfg_base;
static unsigned long ch_mode_cfg_base;

void gspi_check_finish(void)
{
    u32 ahb_read_data;

    ahb_read_data = GSPIREG(STA);
	//printk("%x\n", ahb_read_data);

	while (ahb_read_data & SPI_BUSY)
    {
        ahb_read_data = GSPIREG(STA);
		//printk("%x\n", ahb_read_data);
    }
}

u32 gspi_read_write_data(unsigned int nsecs, unsigned int cpol, unsigned int cpha, u32 word, u8 bits)
{
	u32 word2;
	u32 ch_mode = (ch_mode_cfg_base | ((bits-1) << SPI_WTHSHFT));
	unsigned int clkdiv = (nsecs * 2) * 120 / 1000;   /* SPI base clock rate is 120Mhz */

	GSPIREG(CH0_BAUD) = (ch_baud_cfg_base | clkdiv);
	GSPIREG(CH1_BAUD) = (ch_baud_cfg_base | clkdiv);

	if(cpol)
		ch_mode |= SPI_CFG_CPOL;

	if(cpha)
		ch_mode |= (SPI_CPHA_WR | SPI_CPHA_RD);

	GSPIREG(CH0_MODE_CFG) = ch_mode;
	GSPIREG(CH1_MODE_CFG) = ch_mode;

	GSPIREG(SPI_TC) = 1;
	GSPIREG(SPI_CTRL) = ( (0x03 << 9) | (0x03 << SPI_DIRSHFT) | SPI_TRIGGER );

	GSPIREG(DFIFO) = word;

	gspi_check_finish();

	word2 = GSPIREG(DFIFO); // & 0xff;

	//if((word!=0xff)||(word2!=0xff))
	//	printk("TX %02x RX %02x\n", word, word2);

	return word2;
}

void gspi_master_init(void)
{
#if 0
	// IO pad driving
	*(volatile unsigned long *)0xbf004a2c = 0xffffffffUL;
	*(volatile unsigned long *)0xbf004a30 = 0x00ffffffUL;
#endif

#if 0 // shall be irrelevant to GSPI (2nd SPI controller); only applicable to SPI-NAND/SPI-NOR (1st SPI controller)
	// clear sf_cs_sel to default value
	REG_UPDATE32(GPIO_SEL5_REG, 0, SF_CS_SEL_MASK);
#endif

	ch_baud_cfg_base = (SPI_INT_DLY << SPI_DLYSHFT);
	ch_mode_cfg_base = (SPI_DUMYH | SPI_CSRTRN | SPI_BYTEPKG | 
							  (3 << SPI_CLDSHFT) | (3 << SPI_CSDSHFT) | SPI_TXD_DELAY);

	GSPIREG(CH0_BAUD) = (ch_baud_cfg_base | default_clkdiv);
	GSPIREG(CH1_BAUD) = (ch_baud_cfg_base | default_clkdiv);

	GSPIREG(CH0_MODE_CFG) = ch_mode_cfg_base;
	GSPIREG(CH1_MODE_CFG) = ch_mode_cfg_base;

	GSPIREG(PIN_CTRL) = ( (SPI_OUTHGH << SPI_03SHFT) | (SPI_OUTHGH << SPI_02SHFT) );
	GSPIREG(PIN_MODE) = ( (SPI_PINOUT << SPI_IO0SHFT) | (SPI_PININ << SPI_IO1SHFT) );
}
#endif

struct spi_mt_platform_data {
	unsigned	sck;
	unsigned long	mosi;
	unsigned long	miso;

	u16		num_chipselect;
};

/*
 * This bitbanging SPI master driver should help make systems usable
 * when a native hardware SPI engine is not available, perhaps because
 * its driver isn't yet working or because the I/O pins it requires
 * are used for other purposes.
 *
 * platform_device->driver_data ... points to spi_gpio
 *
 * spi->controller_state ... reserved for bitbang framework code
 * spi->controller_data ... holds chipselect GPIO
 *
 * spi->master->dev.driver_data ... points to spi_gpio->bitbang
 */

struct spi_mt {
	struct spi_bitbang		bitbang;
	struct spi_mt_platform_data	pdata;
	struct platform_device		*pdev;
	unsigned long			cs_gpios[16];
};

/*----------------------------------------------------------------------*/

static inline struct spi_mt *__pure
spi_to_spi_mt(const struct spi_device *spi)
{
	const struct spi_bitbang	*bang;
	struct spi_mt			*spi_mt;

	bang = spi_master_get_devdata(spi->master);
	spi_mt = container_of(bang, struct spi_mt, bitbang);
	return spi_mt;
}

static inline struct spi_mt_platform_data *__pure
spi_to_pdata(const struct spi_device *spi)
{
	return &spi_to_spi_mt(spi)->pdata;
}

#if !defined(ENABLE_HARDWARE_SPI)

/* this is #defined to avoid unused-variable warnings when inlining */
#define pdata		spi_to_pdata(spi)

static inline void setsck(const struct spi_device *spi, int is_on)
{
	gpio_set_value_cansleep(SPI_SCK_GPIO, is_on);
}

static inline void setmosi(const struct spi_device *spi, int is_on)
{
	gpio_set_value_cansleep(SPI_MOSI_GPIO, is_on);
}

static inline int getmiso(const struct spi_device *spi)
{
	return !!gpio_get_value_cansleep(SPI_MISO_GPIO);
}

#undef pdata

static inline void spidelay(unsigned nsecs)
{
#ifdef NEED_SPIDELAY
	if (unlikely(nsecs))
		ndelay(nsecs);
#endif /* NEED_SPIDELAY */
}

static inline u32
bitbang_txrx_be_cpha0(struct spi_device *spi,
		unsigned nsecs, unsigned cpol, unsigned flags,
		u32 word, u8 bits)
{
	/* if (cpol == 0) this is SPI_MODE_0; else this is SPI_MODE_2 */
	//u32 oldword = word;
	u32 oldbit = (!(word & (1<<(bits-1)))) << 31;
	/* clock starts at inactive polarity */
	for (word <<= (32 - bits); likely(bits); bits--) {

		/* setup MSB (to slave) on trailing edge */
		if ((flags & SPI_MASTER_NO_TX) == 0) {
			if ((word & (1 << 31)) != oldbit) {
				setmosi(spi, word & (1 << 31));
				oldbit = word & (1 << 31);
			}
		}
		spidelay(nsecs);	/* T(setup) */

		setsck(spi, !cpol);
		spidelay(nsecs);

		/* sample MSB (from slave) on leading edge */
		word <<= 1;
		if ((flags & SPI_MASTER_NO_RX) == 0)
			word |= getmiso(spi);
		setsck(spi, cpol);
	}

	//if((oldword!=0xff)||(word!=0xff))
	//	printk("TX %08x RX %08x\n", oldword, word);
	return word;
}

static inline u32
bitbang_txrx_be_cpha1(struct spi_device *spi,
		unsigned nsecs, unsigned cpol, unsigned flags,
		u32 word, u8 bits)
{
	/* if (cpol == 0) this is SPI_MODE_1; else this is SPI_MODE_3 */

	u32 oldbit = (!(word & (1<<(bits-1)))) << 31;
	/* clock starts at inactive polarity */
	for (word <<= (32 - bits); likely(bits); bits--) {

		/* setup MSB (to slave) on leading edge */
		setsck(spi, !cpol);
		if ((flags & SPI_MASTER_NO_TX) == 0) {
			if ((word & (1 << 31)) != oldbit) {
				setmosi(spi, word & (1 << 31));
				oldbit = word & (1 << 31);
			}
		}
		spidelay(nsecs); /* T(setup) */

		setsck(spi, cpol);
		spidelay(nsecs);

		/* sample MSB (from slave) on trailing edge */
		word <<= 1;
		if ((flags & SPI_MASTER_NO_RX) == 0)
			word |= getmiso(spi);
	}
	return word;
}
#endif

static u32 spi_mt_txrx_word_mode0(struct spi_device *spi,
		unsigned nsecs, u32 word, u8 bits)
{
#if defined(ENABLE_HARDWARE_SPI)
	return gspi_read_write_data(nsecs, 0, 0, word, bits);
#else
	return bitbang_txrx_be_cpha0(spi, nsecs, 0, 0, word, bits);
#endif
}

static u32 spi_mt_txrx_word_mode1(struct spi_device *spi,
		unsigned nsecs, u32 word, u8 bits)
{
#if defined(ENABLE_HARDWARE_SPI)
	return gspi_read_write_data(nsecs, 0, 1, word, bits);
#else
	return bitbang_txrx_be_cpha1(spi, nsecs, 0, 0, word, bits);
#endif
}

static u32 spi_mt_txrx_word_mode2(struct spi_device *spi,
		unsigned nsecs, u32 word, u8 bits)
{
#if defined(ENABLE_HARDWARE_SPI)
	return gspi_read_write_data(nsecs, 1, 0, word, bits);
#else
	return bitbang_txrx_be_cpha0(spi, nsecs, 1, 0, word, bits);
#endif
}

static u32 spi_mt_txrx_word_mode3(struct spi_device *spi,
		unsigned nsecs, u32 word, u8 bits)
{
#if defined(ENABLE_HARDWARE_SPI)
	return gspi_read_write_data(nsecs, 1, 1, word, bits);
#else
	return bitbang_txrx_be_cpha1(spi, nsecs, 1, 0, word, bits);
#endif
}

/*----------------------------------------------------------------------*/

static void spi_mt_chipselect(struct spi_device *spi, int is_active)
{
	struct spi_mt *spi_mt = spi_to_spi_mt(spi);
	unsigned long cs = spi_mt->cs_gpios[spi->chip_select];

#if !defined(ENABLE_HARDWARE_SPI)
	/* set initial clock polarity */
	if (is_active)
		setsck(spi, spi->mode & SPI_CPOL);
#endif

	/* SPI is normally active-low */
	gpio_set_value_cansleep(cs, (spi->mode & SPI_CS_HIGH) ? is_active : !is_active);
}

static int spi_mt_setup(struct spi_device *spi)
{
	unsigned long		cs;
	int			status = 0;
	struct spi_mt		*spi_mt = spi_to_spi_mt(spi);

	cs = spi_mt->cs_gpios[spi->chip_select];

	if (!spi->controller_state) {
		status = gpio_request(cs, dev_name(&spi->dev));
		if (status)
			return status;
		status = gpio_direction_output(cs,
				!(spi->mode & SPI_CS_HIGH));
	}
	if (!status) {
		status = spi_bitbang_setup(spi);
	}

	if (status) {
		if (!spi->controller_state)
			gpio_free(cs);
	}
	return status;
}

static void spi_mt_cleanup(struct spi_device *spi)
{
	struct spi_mt *spi_mt = spi_to_spi_mt(spi);
	unsigned long cs = spi_mt->cs_gpios[spi->chip_select];

	gpio_free(cs);
	spi_bitbang_cleanup(spi);
}

#if !defined(ENABLE_HARDWARE_SPI)
static int spi_mt_alloc(unsigned pin, const char *label, bool is_in)
{
	int value;

	value = gpio_request(pin, label);
	if (value == 0) {
		if (is_in)
			value = gpio_direction_input(pin);
		else
			value = gpio_direction_output(pin, 0);
	}
	return value;
}

static int spi_mt_request(struct spi_mt_platform_data *pdata,
			    const char *label, u16 *res_flags)
{
	int value;

	/* NOTE:  SPI_*_GPIO symbols may reference "pdata" */

	value = spi_mt_alloc(SPI_MOSI_GPIO, label, false);
	if (value)
		goto done;

	value = spi_mt_alloc(SPI_MISO_GPIO, label, true);
	if (value)
		goto free_mosi;

	value = spi_mt_alloc(SPI_SCK_GPIO, label, false);
	if (value)
		goto free_miso;

	goto done;

free_miso:
	gpio_free(SPI_MISO_GPIO);
free_mosi:
	gpio_free(SPI_MOSI_GPIO);
done:
	return value;
}
#endif

static int spi_mt_probe(struct platform_device *pdev)
{
	int				status;
	struct spi_master		*master;
	struct spi_mt			*spi_mt;
	struct spi_mt_platform_data	*pdata;
	u16 master_flags = 0;
	int num_devices;
	int pinmux_group;
	int i;

#if defined(ENABLE_HARDWARE_SPI)
	pinmux_group = gspi_pinmux_selected();
	if(!pinmux_group)
		return -ENXIO;
#else
	pinmux_group = 1;
#endif

	pdata = dev_get_platdata(&pdev->dev);

	num_devices = SPI_N_CHIPSEL;

#if defined(ENABLE_HARDWARE_SPI)
	status = 0;
#else
	status = spi_mt_request(pdata, dev_name(&pdev->dev), &master_flags);
	if (status < 0)
		return status;
#endif

	master = spi_alloc_master(&pdev->dev, sizeof(*spi_mt) +
					(sizeof(unsigned long) * num_devices));
	if (!master) {
		status = -ENOMEM;
		goto gpio_free;
	}
	spi_mt = spi_master_get_devdata(master);
	platform_set_drvdata(pdev, spi_mt);

	spi_mt->pdev = pdev;
	if (pdata)
		spi_mt->pdata = *pdata;

	master->bits_per_word_mask = SPI_BPW_RANGE_MASK(1, 32);   // SPI_BPW_MASK(8);
	master->flags = master_flags;
	master->bus_num = pdev->id;
	master->num_chipselect = num_devices;
	master->setup = spi_mt_setup;
	master->cleanup = spi_mt_cleanup;

	spi_mt->bitbang.master = master;
	spi_mt->bitbang.chipselect = spi_mt_chipselect;

	spi_mt->bitbang.txrx_word[SPI_MODE_0] = spi_mt_txrx_word_mode0;
	spi_mt->bitbang.txrx_word[SPI_MODE_1] = spi_mt_txrx_word_mode1;
	spi_mt->bitbang.txrx_word[SPI_MODE_2] = spi_mt_txrx_word_mode2;
	spi_mt->bitbang.txrx_word[SPI_MODE_3] = spi_mt_txrx_word_mode3;
	spi_mt->bitbang.setup_transfer = spi_bitbang_setup_transfer;
	spi_mt->bitbang.flags = SPI_CS_HIGH;

	if(pinmux_group==1)
		spi_mt->cs_gpios[0] = 19;
	else if(pinmux_group==2)
		spi_mt->cs_gpios[0] = 7;

	status = spi_bitbang_start(&spi_mt->bitbang);
	if (status < 0) {
gpio_free:
#if !defined(ENABLE_HARDWARE_SPI)
		gpio_free(SPI_MISO_GPIO);
		gpio_free(SPI_MOSI_GPIO);
		gpio_free(SPI_SCK_GPIO);
#endif
		spi_master_put(master);
	}

#if defined(ENABLE_HARDWARE_SPI)
	if(status==0)
		gspi_master_init();
#else
	if(status==0)
	{
		pinmux_pin_func_gpio(SPI_SCK_GPIO);
		pinmux_pin_func_gpio(SPI_MOSI_GPIO);
		pinmux_pin_func_gpio(SPI_MISO_GPIO);
	}
#endif

	if(status==0)
	{
		/* we use software controlled GPIO to generate SPI_CS signal */
		for(i=0;i<SPI_N_CHIPSEL;i++)
		{
			pinmux_pin_func_gpio(spi_mt->cs_gpios[i]);
		}
	}

	return status;
}

static int spi_mt_remove(struct platform_device *pdev)
{
	struct spi_mt			*spi_mt;
	struct spi_mt_platform_data	*pdata;

	spi_mt = platform_get_drvdata(pdev);
	pdata = dev_get_platdata(&pdev->dev);

	/* stop() unregisters child devices too */
	spi_bitbang_stop(&spi_mt->bitbang);

#if !defined(ENABLE_HARDWARE_SPI)
	gpio_free(SPI_MISO_GPIO);
	gpio_free(SPI_MOSI_GPIO);
	gpio_free(SPI_SCK_GPIO);
#endif

	spi_master_put(spi_mt->bitbang.master);

	return 0;
}

static struct platform_driver spi_mt_driver = {
	.driver = {
		.name	= DRIVER_NAME,
	},
	.probe		= spi_mt_probe,
	.remove		= spi_mt_remove,
};
module_platform_driver(spi_mt_driver);

MODULE_DESCRIPTION("Montage SPI master driver");
MODULE_LICENSE("GPL");

