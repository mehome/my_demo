shell echo -e "set \$startaddr=\c" > startaddr.gdb
shell mips-openwrt-linux-readelf -l vmlinuz  | grep "Entry point" | cut -b 13-22 >> startaddr.gdb
source startaddr.gdb
file ./vmlinuz
target remote localhost:3333
mon halt
restore ./vmlinuz
set *(unsigned long *)0xbf005520=$startaddr
mon resume
set confirm off
quit
