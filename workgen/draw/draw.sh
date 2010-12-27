#!/bin/sh

JAVA_HOME="/usr/lib/java"
PATH="/root/Desktop/gustavo"
TEST="workgen/draw"
SCREENS="screenshots"
JGROUPS="JGroups-2.10.0.Alpha4.src"
WORKLOAD="workload-draw.sh"

export DISPLAY=:0.0
export CLASSPATH="${PATH}/${JGROUPS}/classes:${PATH}/${JGROUPS}/conf:${PATH}/${JGROUPS}/lib"

ETH=`/sbin/ifconfig | /usr/bin/grep -m 1 eth | /usr/bin/cut -d' ' -f1`

HOSTNAME=`/bin/hostname`
if [ $HOSTNAME == "ferrari" ]; then
	ETH="eth1"
fi

IP="`/sbin/ifconfig ${ETH} | /usr/bin/awk '/dr:/{gsub(/.*:/,"",$2);print$2}'`"

EXPERIMENTO="draw`/usr/bin/date +%d%m%H%M`"

/bin/mkdir -p ${PATH}/${SCREENS}/${EXPERIMENTO}

#DRAW="/usr/lib/java/bin/java org.jgroups.demos.Draw -state -bind_addr ${IP} -use_unicasts"

COMANDO="/usr/lib/java/bin/java org.jgroups.demos.Draw -bind_addr ${IP}"

/bin/dmesg -c

if [ `/bin/hostname` == "fusca" ]; then
	/bin/sleep 1
elif [ `/bin/hostname` == "dkv" ]; then
	/bin/sleep 4
elif [ `/bin/hostname` == "passat" ]; then
	/bin/sleep 7
elif [ `/bin/hostname` == "porsche" ]; then
	/bin/sleep 10
fi

cd ${PATH}/${JGROUPS}/src
${COMANDO} &

if [ `/bin/hostname` == "fusca" ]; then
	/bin/sleep 14
elif [ `/bin/hostname` == "dkv" ]; then
	/bin/sleep 10
elif [ `/bin/hostname` == "passat" ]; then
	/bin/sleep 7
elif [ `/bin/hostname` == "porsche" ]; then
	/bin/sleep 4
fi

cd ${PATH}/${SCREENS}/${EXPERIMENTO}
/usr/bin/scrot screen-draw-`/usr/bin/date +%d%m%H%M%S`.png

if [ ${1} = "com_falha" ]; then
	/sbin/insmod ${PATH}/pie/pie_core.ko

	if [ `/bin/hostname` != "fusca" ]; then
		/bin/sleep 1
	else
		cd ${PATH}/pie/parser
		./dispatcher draw.tkl &
	fi
	cd ${PATH}/${TEST}
	export DISPLAY=:0.0; time ./${WORKLOAD} $HOSTNAME &
else
	cd ${PATH}/${TEST}
	export DISPLAY=:0.0; time ./${WORKLOAD} $HOSTNAME &
fi

for i in `/bin/seq 1 12`; do
	cd ${PATH}/${SCREENS}/${EXPERIMENTO}
	/usr/bin/scrot -d 5 screen-draw-`/usr/bin/date +%d%m%H%M%S`.png
done

/bin/sleep 5

/bin/killall -q java

cd ${PATH}/${SCREENS}/${EXPERIMENTO}
/usr/bin/scrot screen-draw-`/usr/bin/date +%d%m%H%M%S`.png

/bin/cp ${PATH}/pie/parser/draw.tkl ${PATH}/${SCREENS}/${EXPERIMENTO}/

/sbin/rmmod pie_core
/bin/tac /var/log/messages | /bin/grep activating_timer | /bin/head -n8 > ${PATH}/${SCREENS}/${EXPERIMENTO}/debug
/bin/dmesg -c

echo Execucao encerrada com sucesso!
