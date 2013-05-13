#!/bin/sh
HOSTS="10.0.0.1
10.0.0.2
10.0.0.3
10.0.0.4"

for i in $HOSTS;
do
	if [ $1 == "limpa" ]; then
		ssh root@$i "rmmod pie_core; dmesg -c" &
	else
		ssh root@$i "insmod /root/Desktop/com_log/pie/pie_core.ko" &
	fi
done
