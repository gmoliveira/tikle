#!/bin/sh

#################################################
# Esquema de diretorios:			#
#						#
# Raiz:						#
#	/root/gustavo				#
#						#
# JGroups:					#
#	/root/gustavo/JGroups-2.10.0.Alpha3.src	#
#						#
# Workload:					#
#	/root/gustavo/workgen/			#
#						#
# Screenshots:					#
#	/root/gustavo/screenshots		#
#################################################

PATH="/root/gustavo"
TEST="workgen"
SCREENS="screenshots"
JGROUPS="JGroups-2.10.0.Alpha4.src"
WORKLOAD="workload-cluster.sh" 		   # script que gera o workload para a aplicacao DRAW

DURATION=120				   # duracao do experimento
LOG="dump.log" 				   # dump do wireshark

EXPERIMENTO="cluster`/bin/date +%d%m%H%M`"
ETH=`/sbin/ifconfig | /usr/bin/grep -m 1 eth | /usr/bin/cut -d' ' -f1`
IP="`/sbin/ifconfig ${ETH} | /usr/bin/awk '/dr:/{gsub(/.*:/,"",$2);print$2}'`"
CLUSTER="/usr/lib/java/bin/java -Djgroups.bind_addr=${IP} -jar ${PATH}/${JGROUPS}/src/org/jgroups/demos/demo.jar"
WIRESHARK_ARGS="-Q --display=:0.0 -i ${ETH} -k -S -l -p -N n -t ad -a duration:${DURATION} -w ${PATH}/${SCREENS}/${EXPERIMENTO}/${LOG}"

/bin/mkdir -p ${PATH}/${SCREENS}/${EXPERIMENTO}

/usr/bin/wireshark ${WIRESHARK_ARGS} &

/bin/sleep 10
time /usr/bin/xterm -geometry 100x30+0+0 -e "${CLUSTER}" &

/bin/sleep 3
cd ${PATH}/${SCREENS}/${EXPERIMENTO}
/usr/bin/import -window root screencluster-`/bin/date +%d%m%H%M%S`.png

if [ "$1" == "com_falha"]; then
	/bin/dmesg -c
	/sbin/insmod ${PATH}/pie/pie_core.ko
	cd ${PATH}/pie/parser
	./dispatcher $2
	cd ${PATH}/${TEST}
	./${WORKLOAD} $3 &
else
	cd ${PATH}/${TEST}
	./${WORKLOAD} $1 &
fi

#/bin/sleep 5
cd ${PATH}/${SCREENS}/${EXPERIMENTO}
/usr/bin/import -window root screencluster-`/bin/date +%d%m%H%M%S`.png

for i in `/bin/seq 1 5`;
do
	/bin/sleep 20
	cd ${PATH}/${SCREENS}/${EXPERIMENTO}
	/usr/bin/import -window root screencluster-`/bin/date +%d%m%H%M%S`.png
done

cd ${PATH}/${TEST}
./${WORKLOAD} q &
#/bin/killall java

/bin/sleep 1
cd ${PATH}/${SCREENS}/${EXPERIMENTO}
/usr/bin/import -window root screencluster-`/bin/date +%d%m%H%M%S`.png

echo Execucao encerrada com sucesso!
