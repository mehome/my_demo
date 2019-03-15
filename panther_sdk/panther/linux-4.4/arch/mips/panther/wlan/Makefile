#
#	Copyright (c) 2013	Montage Inc.	All rights reserved. 
#
#EXTRA_CFLAGS += -Werror
#NOSTDINC_FLAGS += -I$(src)/../wla/include/
NOSTDINC_FLAGS += $(LINUXINCLUDE)

NOSTDINC_FLAGS += -I$(src)/include/
MATH_OBJS = $(patsubst %, math/%, e_asin.o e_log10.o e_log.o e_pow.o e_rem_pio2.o e_sqrt.o k_cos.o k_rem_pio2.o k_sin.o k_tan.o s_ceil.o s_copysign.o s_cos.o s_fabs.o s_finite.o \
                        s_floor.o s_isnan.o s_scalbn.o s_sin.o s_tan.o)
TEST_OBJS = dragonite_main.o \
	mac.o \
	rx.o \
	tx.o \
	skb.o \
	beacon.o \
	resources.o \
	station.o \
	mac_ctrl.o \
	cipher.o \
	lib_test.o \
        rf.o \
        bb.o \
        rfc_comm.o \
        rfc_panther.o \
        rfc_new.o \
        rfac.o \
        rfac_rfc_patch.o \
        panther_rf.o \
        panther_rfac.o \
	ip301.o \
        complex.o \
	debugfs.o \
	procfs.o \
	kthread.o \
	bridge.o \
	mat.o \
	omnicfg.o \
	bbdump.o \
	performance.o \
	snr.o \
	mt_driver_Panther.o

LIBGCC_OBJS =  libgcc/_mul_sf.o libgcc/_mul_df.o libgcc/_addsub_df.o \
               libgcc/_div_df.o libgcc/_ge_df.o libgcc/_gt_df.o libgcc/_le_df.o \
               libgcc/_lt_df.o libgcc/_df_to_si.o libgcc/_si_to_df.o \
               libgcc/_thenan_sf.o libgcc/_thenan_df.o libgcc/_unpack_sf.o \
               libgcc/_unpack_df.o libgcc/_pack_sf.o libgcc/_pack_df.o  \
               libgcc/_eq_df.o libgcc/_fixunsdfsi.o libgcc/_fpcmp_parts_df.o libgcc/_floatunsidf.o

obj-$(CONFIG_PANTHER_WLAN)   += $(MATH_OBJS)

obj-$(CONFIG_PANTHER_WLAN)   += $(LIBGCC_OBJS)

obj-$(CONFIG_PANTHER_WLAN)   += $(TEST_OBJS)

#obj-$(CONFIG_PANTHER) += dragonite_main.o
#obj-$(CONFIG_PANTHER) += mac80211_hwsim.o
