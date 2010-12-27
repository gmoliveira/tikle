#!/bin/bash

HOSTS="10.0.0.1
10.0.0.2
10.0.0.3
10.0.0.4"

for i in $HOSTS; do
	ssh root@$i "export DISPLAY=:0.0; xterm	-e '/root/Desktop/com_log/ping/ping.sh ${1}'" &
done;
/bin/sleep 3 
pie/parser/dispatcher pie/parser/ping.tkl
