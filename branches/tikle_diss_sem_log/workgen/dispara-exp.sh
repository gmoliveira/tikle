#!/bin/bash

HOSTS="dkv
passat
porsche
fusca"

if [ $1 == "draw" ]; then
	for i in $HOSTS; do
#		if [ $i == "ferrari" ]; then
#			ssh root@$i "export DISPLAY=:0.0; xterm -geometry 80x24+0+350 -e '/root/Desktop/gustavo/workgen/draw/draw.sh com_falha'" &
#		else
		ssh root@$i "export DISPLAY=:0.0; xterm	-e '/root/Desktop/gustavo/workgen/draw/draw.sh com_falha'" &
#		fi
	done;
elif [ $1 == "topology" ]; then
	for i in $HOSTS; do
#		if [ $i == "ferrari" ]; then
#			ssh root@$i "export DISPLAY=:0.0; xterm -geometry 80x24+0+350 -e '/root/Desktop/gustavo/workgen/topology/topology.sh'" &
#		else
		ssh root@$i "export DISPLAY=:0.0; xterm -e '/root/Desktop/gustavo/workgen/topology/topology.sh'" &
#		fi
	done;
elif [ $1 == "whiteboard" ]; then
	for i in $HOSTS; do
#		if [ $i == "ferrari" ]; then
#			ssh root@$i "export DISPLAY=:0.0; xterm -geometry 80x24+0+350 -e '/root/Desktop/gustavo/workgen/whiteboard/whiteboard.sh'" &
#		else
		ssh root@$i "export DISPLAY=:0.0; xterm -e '/root/Desktop/gustavo/workgen/whiteboard/whiteboard.sh'" &
#		fi
	done;
fi
