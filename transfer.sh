#!/bin/bash

if [ $# -ne 1 ] 
then
	echo "usage $0: <name of sd card>"
	exit
fi

make clean all
cp kernel.img /media/anas/$1
sync
