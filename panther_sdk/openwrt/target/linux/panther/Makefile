#
# Copyright (C) 2010-2011 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
include $(TOPDIR)/rules.mk

BOARD:=panther
BOARDNAME:=Panther board
CPU_TYPE:=interaptiv
SUBTARGETS:=le be
INITRAMFS_EXTRA_FILES:=

KERNEL_PATCHVER:=4.4.14

include $(INCLUDE_DIR)/target.mk

DEFAULT_PACKAGES += wpad-mini

$(eval $(call BuildTarget))
