#
# Copyright (C) 2010 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/image.mk
ifeq ($(CONFIG_MTD_BLOCK_SIZE_NOR),y)
	# Define for NOR flash
	UBIFS_OPTS = -F -m 256 -e 65024 -c 600 #-j 730112
	UBI_OPTS = -m 256 -p 64KiB -O 256
#       UBIFS_OPTS = -F -m 1 -e 65408 -c 200 #-j 730112
#       UBI_OPTS = -m 1 -p 64KiB -O 64
	BLOCK_SIZE = 128k
	PAGE_SIZE = 256
	SKIP_SIZE = 131072
else
	ifeq ($(CONFIG_MTD_BLOCK_SIZE_128K),y)
		UBIFS_OPTS = -F -m 2048 -e 124KiB -c 1580 -U -j 5840896
		UBI_OPTS = -m 2048 -p 128KiB -O 2048
		BLOCK_SIZE = 128k
		PAGE_SIZE = 2048
		SKIP_SIZE = 131072
	else
		# UBI options for page size = 4K
		UBIFS_OPTS = -F -m 4096 -e 248KiB -c 1580 -U -j 5840896
		UBI_OPTS = -m 4096 -p 256KiB -O 4096
		BLOCK_SIZE = 256k
		PAGE_SIZE = 4096
		SKIP_SIZE = 262144
	endif
endif

define get_kernel_entry
0x$(shell $(TARGET_CROSS)nm $(1) 2>/dev/null | grep " kernel_entry" | cut -f1 -d ' ')
endef

define CompressLzma
  $(STAGING_DIR_HOST)/bin/lzma e $(1) -lc1 -lp2 -pb2 $(2)
endef

define CompressGzip
	gzip -9n -c $(1) > $(2)
endef

define MkuImage
	mkimage -A mips -O linux -T kernel -a 0x80100000 -C $(1) $(2) \
		-e $(call get_kernel_entry,$(KDIR)/vmlinux.debug) -n 'MIPS OpenWrt Linux-$(LINUX_VERSION)' \
		-d $(3) $(4)
endef

define imgname
	$(BIN_DIR)/combined-uzImage-$(1)
endef

define secure_imgname
	$(BIN_DIR)/secure-combined-uzImage-$(1)
endef

define msign_bin
	#$(TOPDIR)/../panther/tools/msign/msign
	$(TOPDIR)/staging_dir/host/bin/msign
endef

define msign_key
	$(TOPDIR)/../panther/tools/msign/key
endef

ifdef CONFIG_SYS_SUPPORTS_ZBOOT
define Image/Prepare
endef

define Image/BuildKernel
	cp $(KERNEL_BUILD_DIR)/linux-$(LINUX_VERSION)/arch/mips/boot/uzImage $(BIN_DIR)/uzImage
endef
else
define Image/Prepare
	cp $(KDIR)/vmlinux.elf $(BIN_DIR)/$(IMG_PREFIX)-vmlinux.elf
	$(call CompressLzma,$(KDIR)/vmlinux,$(KDIR)/vmlinux.bin.lzma)
	$(call MkuImage,lzma,,$(KDIR)/vmlinux.bin.lzma,$(KDIR)/uImage.lzma)
	$(call CompressGzip,$(KDIR)/vmlinux,$(KDIR)/vmlinux.bin.gz)
	$(call MkuImage,gzip,,$(KDIR)/vmlinux.bin.gz,$(KDIR)/uImage.gz)
endef

define Image/BuildKernel
	cp $(KDIR)/uImage.lzma $(BIN_DIR)/$(IMG_PREFIX)-uImage-lzma
	cp $(KDIR)/uImage.gz $(BIN_DIR)/$(IMG_PREFIX)-uImage-gzip
endef
endif

define Image/Build/squashfs
	$(call prepare_generic_squashfs,$(KDIR)/root.squashfs)
endef

define Image/Build/Initramfs
	cp $(KDIR)/vmlinux-initramfs.elf $(BIN_DIR)/$(IMG_PREFIX)-vmlinux-initramfs.elf
	cp $(KDIR)/vmlinux-initramfs $(BIN_DIR)/$(IMG_PREFIX)-vmlinux-initramfs.bin
endef

define Image/Build
	$(call Image/Build/$(1))
	dd if=$(KDIR)/root.$(1) of=$(BIN_DIR)/$(IMG_PREFIX)-root.$(1) bs=$(BLOCK_SIZE) conv=sync

	# add combined image header before uImage header, disable now
	#dd if=/dev/zero of=$(BIN_DIR)/kernel-image bs=128 count=1 conv=sync
	#cat $(BIN_DIR)/uzImage >> $(BIN_DIR)/kernel-image
	dd if=$(BIN_DIR)/uzImage of=$(BIN_DIR)/kernel-image bs=$(BLOCK_SIZE) conv=sync
	-sh $(TOPDIR)/scripts/combined-image.sh \
		$(BIN_DIR)/kernel-image \
		$(BIN_DIR)/$(IMG_PREFIX)-root.$(1) \
		$(call imgname,$(1)) \
		$(BLOCK_SIZE)

	$(call msign_bin) -M -s $(call msign_key) -i $(call imgname,$(1)) -o \
		$(call secure_imgname,$(1)) -b $(PAGE_SIZE) -e $(SKIP_SIZE)
	-sh $(TOPDIR)/scripts/combined-image-md5.sh \
		$(call secure_imgname,$(1)) \
		$(BLOCK_SIZE)

	$(call msign_bin) -I -s $(call msign_key) -i $(call imgname,$(1))
	$(call msign_bin) -I -s $(call msign_key) -i $(call secure_imgname,$(1))

endef

$(eval $(call BuildImage))
