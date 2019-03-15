#ifndef __PANTHER_DEV_H__
#define __PANTHER_DEV_H__

#include <os_compat.h>

struct panther_dev {
	/* RF device */
	struct{
		u8 chip_ver;
		u8 power_level; /* RF power value */
		u8 if_type;
	}rf;
	u32 bb_reg_tbl;
	u32 usec;
	u32 usec_overflow;
};
extern struct panther_dev *ldev;
#endif
