target remote localhost:3333
mon halt
set *(unsigned long *)0xa0000300=0x0
set *(unsigned long *)0xbf005520=0x0
mon reg pc 0xbfc00000
set *(unsigned long *)0xbf004820=0x01000001
mon resume
disconnect
shell echo -e "set \$startaddr=\c" > startaddr.gdb
shell mips-openwrt-linux-readelf -l vmlinuz  | grep "Entry point" | cut -b 13-22 >> startaddr.gdb
source startaddr.gdb
file ./vmlinuz
shell sleep 2
target remote localhost:3333
mon halt
load
set *(unsigned long *)0xbf005520=$startaddr
mon resume
set confirm off
quit
