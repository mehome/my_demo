target remote localhost:3333
mon halt
restore boot.img binary 0x81000000
mon resume
set confirm off
quit
