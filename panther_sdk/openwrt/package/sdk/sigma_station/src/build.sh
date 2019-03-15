#!/bin/bash

export PATH=/usr/lib/lightdm/lightdm:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin:/usr/games

make clean; 
make all &&
sudo cp dut/wfa_dut /usr/sbin/ &&
sudo cp scripts/*.sh /usr/local/sbin/
