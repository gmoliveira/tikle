#!/bin/bash

HOSTS="dkv
passat
porsche"

for i in $HOSTS; do
	ssh $i "rm -rf /root/Desktop/gustavo*"
	scp $1 $i:
	ssh $i "tar -zxvf /root/gustavo.tar.gz -C /root/Desktop/"
#	ssh $i "rm -rf /root/Desktop/gustavo/screenshots/*"
done;
