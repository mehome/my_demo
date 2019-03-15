ARCH:=mips
SUBTARGET:=be
BOARDNAME:=Big Endian

# All OpenWRT features list, refer to scripts/metadata.pl
# arm_v broken audio display dt gpio pci pcie usb usbgadget pcmcia rtc
# squashfs jffs2$ jffs2_nand ext4 targz cpiogz ubifs fpu spe_fpu
# ramdisk powerpc64 nommu mips16 rfkill low_mem /nand
FEATURES:=ramdisk ubifs mips16
# FEATURES:=ramdisk jffs2_nand ubifs usb usbgadget

define Target/Description
	Build BE firmware images for Montage Panther board running in
	big-endian mode
endef
