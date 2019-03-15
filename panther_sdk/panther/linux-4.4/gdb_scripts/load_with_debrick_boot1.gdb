file vmlinux
target remote localhost:3333
mon halt
restore vmlinux
set *(unsigned long *)0xbf005520=0x80000500
mon resume
set confirm off
quit
