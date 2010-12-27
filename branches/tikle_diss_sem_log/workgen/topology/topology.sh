#!/bin/sh

JAVA_HOME="/usr/lib/java"
PATH="/root/Desktop/gustavo"
TEST="workgen/topology"
SCREENS="screenshots"
JGROUPS="JGroups-2.10.0.Alpha4.src"

export DISPLAY=:0.0
export CLASSPATH="${PATH}/${JGROUPS}/classes:${PATH}/${JGROUPS}/conf:${PATH}/${JGROUPS}/lib"

EXPERIMENTO="topology`/usr/bin/date +%d%m%H%M`"

/bin/mkdir -p ${PATH}/${SCREENS}/${EXPERIMENTO}

COMANDO="/usr/lib/java/bin/java org.jgroups.demos.Topology"

cd ${PATH}/${JGROUPS}/src
${COMANDO} &

/bin/sleep 5

cd ${PATH}/${SCREENS}/${EXPERIMENTO}
/usr/bin/scrot screen-topology-`/usr/bin/date +%d%m%H%M%S`.png

/sbin/insmod ${PATH}/pie/pie_core.ko

if [ `/bin/hostname` == "fusca" ]; then
	/bin/sleep 1
	cd ${PATH}/pie/parser
	./dispatcher topology.tkl &
fi

if [ `/bin/hostname` != "fusca" ]; then
	/bin/sleep 1
fi

for i in `/bin/seq 1 12`; do
	cd ${PATH}/${SCREENS}/${EXPERIMENTO}
	/usr/bin/scrot -d 5 screen-topology-`/usr/bin/date +%d%m%H%M%S`.png
done

/bin/sleep 10

cd ${PATH}/${SCREENS}/${EXPERIMENTO}
/usr/bin/scrot screen-topology-`/usr/bin/date +%d%m%H%M%S`.png

/bin/cp ${PATH}/pie/parser/topology.tkl ${PATH}/${SCREENS}/${EXPERIMENTO}/

/bin/killall -q java
/sbin/rmmod pie_core
/bin/tail /var/log/messages > ${PATH}/${SCREENS}/${EXPERIMENTO}/num_pacotes
/bin/dmesg -c

echo Execucao encerrada com sucesso!
