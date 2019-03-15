#!/bin/sh
echo "usb 0" > /proc/soc/power
ew af0050a0 ffff44ff
mdio 1 4 4461
mdio 1 0 b10
#echo "320 120" > /proc/soc/clk
