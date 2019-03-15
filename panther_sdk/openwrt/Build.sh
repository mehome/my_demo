#!/bin/sh
if [ $1 = ist ];then
	for i in $2
	do
	make $i/clean	
	if [ $3 = VVV ];then
		make $i/install -j1 V=s || return -1
	else
		make $i/install || return -1
	fi
	done
elif [ $1 = all ];then
	make clean
	./install_host_tools.sh
	make target/linux/install &
	sleep 3
	make
elif [ $1 = pkg ];then
	for i in $2
	do
	make package/$i/clean
	if [ $3= VVV ];then
		make package/$i/install -j1 V=s || return -1
	else
		make package/$i/install || return -1
	fi	
	done
elif [ $1 = cpkg ];then
	make target/linux/clean
	
else
#	make $1/clean	
	if [ $3 = VVV ];then
		make $1/$2 -j1 V=s
	else
		make $1/$2
	fi
fi
