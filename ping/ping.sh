#!/bin/sh

PATH="/root/Desktop/com_log"
SCREENS="screenshots"
WORKLOAD="cli"

export DISPLAY=:0.0

HOSTNAME=`/bin/hostname`

EXPERIMENTO="ping`/usr/bin/date +%d%m%H%M`"

/bin/mkdir -p ${PATH}/${SCREENS}/${EXPERIMENTO}

COMANDO="/usr/bin/xterm -e /root/Desktop/com_log/ping/srv ${1}"
COMANDO2="/root/Desktop/com_log/ping/cli ${1} ${HOSTNAME}"

/bin/dmesg -c
/sbin/rmmod pie_core
${COMANDO} &

cd ${PATH}/${SCREENS}/${EXPERIMENTO}
/usr/bin/scrot screen-draw-`/usr/bin/date +%d%m%H%M%S`.png

/sbin/insmod ${PATH}/pie/pie_core.ko
cd ${PATH}/ping
export DISPLAY=:0.0; $COMANDO2 &

for i in `/bin/seq 1 12`; do
	cd ${PATH}/${SCREENS}/${EXPERIMENTO}
	/usr/bin/scrot -d 5 screen-ping-`/usr/bin/date +%d%m%H%M%S`.png
done

/bin/sleep 5

/bin/killall -q srv
/bin/killall -q cli

cd ${PATH}/${SCREENS}/${EXPERIMENTO}
/usr/bin/scrot screen-ping-`/usr/bin/date +%d%m%H%M%S`.png

/sbin/rmmod pie_core
/bin/dmesg -c

echo Execucao encerrada com sucesso!
