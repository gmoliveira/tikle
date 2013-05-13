#!/bin/bash

HOSTS="10.0.0.1
10.0.0.3
10.0.0.4
10.0.0.2"

for i in $HOSTS; do
	ssh root@$i "rmmod pie_core"
	ssh root@$i "insmod /root/Desktop/com_log/pie/pie_core.ko"
done;
