#!/bin/sh
#############################
#####			#####
# Environment configuration #
#####			#####
#############################

tempfile=/tmp/host_ip

###
# send the ssh key pair to defined hosts
###
send_keypair()
{
	for hosts in $1; do
		ssh-copy-id -i ~/.ssh/id_rsa.pub `id -n -u`@${hosts} > /dev/null
		retval=$?
		if [ $retval -ne 0 ]; then
			quit $retval "Something went wrong. Check ssh key pairs locally or in ${hosts}"
		fi
	done

	return $retval
}

###
# manually hosts configuration
###
hosts_manually()
{
	dialog						\
		--title 'Host IP'			\
		--ok-label 'Save & Add Other'		\
		--cancel-label 'Done'			\
		--inputbox 'Give a Host IP Address:' 0 0 2> $tempfile

	retval=$?

	if [ $retval -eq 0 ]; then
		host_ip=(${host_ip[*]} `cat $tempfile`)
		hosts_manually
	else
		return 0
	fi
}

###
# automatically hosts configuration
###
hosts_automatically()
{
	interfaces=`ifconfig -a | sed 's/[ \t].*//;/^\(lo\|\)$/d'`
	for iface in $interfaces; do
		network=`ifconfig $iface | sed -rn "s/.*r:([^ ]+) .*/\1/p" | awk -F"." '{ print $1"."$2"."$3".*" }'`
		if [ "${network}" != "" ]; then
			list=(${list[*]} $iface $network)
		fi
	done

	choose_iface=$(dialog --stdout \
        	        --menu 'Which one you would like to use?' 0 0 0 \
			${list[*]} )

	network=`ifconfig $choose_iface | sed -rn "s/.*r:([^ ]+) .*/\1/p" | awk -F"." '{ print $1"."$2"."$3 }'`
	dialog --title 'Wait' --infobox 'Scanning network...' 0 0; for octet in `seq 153 153`; do
		ping -c1 -W1 $network.$octet > /dev/null
		retval=$?
		if [ $retval -eq 0 ]; then
			open="`nmap -sT -p 22 -oG - $network.$octet | grep open`"
			if [ -n "${open}" ]; then
				host_ip="${host_ip} \"${network}.${octet}\" \"\" ON"
			fi
		fi
	done

	command="dialog --separate-output --checklist \"Confirm the host list\" 0 0 0 ${host_ip}"
	sh -c "${command}" 2> $tempfile
	retval=$?
	host_ip="`cat $tempfile`"

	rm -rf $tempfile

	return $retval
}

###
# host definition
###
config_host()
{
	dialog											\
		--title 'Hosts definition'							\
		--yesno 'Configure hosts manually?\nYes - Manually\nNo  - Automatically'	\
		0 0

	retval=$?
	return $retval
}

###
# Generate ssh key pairs
###
ssh_keygen()
{
	echo -ne "###############################\n"
	echo -ne "# Generating ssh key pair\n#\n"

	ssh-keygen -t rsa -N "" -f ~/.ssh/id_rsa

	if [ $? -eq 0 ]; then
		return $?
	else
		quit 1 "Failed to generate ssh key pair. Review its existence in `~`/.ssh/ directory"
	fi
}

###
# Handle the questions made during
# the environment configuration
###
confirm()
{
	dialog 				\
		--title 'Question'	\
		--yesno "$1"		\
		0 0

	retval=$?
	return $retval
}

###
# sequential environment configuration
###
proceed()
{
	confirm "This host is the one that will be the experiment coordinator?"
	if [ $retval -eq 0 ]; then
		continue;
	else
		quit 0 "Please, perform this setup in a proper host coordinador"
	fi

	confirm "This host have the ssh-key pairs?"
	if [ $retval -eq 0 ]; then
		continue;
	else
		ssh_keygen
	fi

	config_host
	if [ $retval -eq 0 ]; then
		hosts_manually
		rm -rf $tempfile
	else
		hosts_automatically
	fi

	if [ $retval -eq 0 ]; then
		echo -ne "#####################################################\n"
		echo -ne "# Configuring hosts...\n"
		echo -ne "# \t${host_ip}\n#\n"
		echo -ne "# A root password of each host will be prompted.\n"
		send_keypair "${host_ip}"
	else
		quit 1 "Something went wrong while configuring hosts"
	fi

	quit 0 "Configuration finished"
}

###
# Control the quit cases
###
quit()
{
	echo -ne "#####################################################\n"
	echo -ne "# quitting:" $2".\n"
	echo -ne "#####################################################\n"
	exit $1
}

###
# Begin the environment configuration
###
init()
{
	echo -ne "#####################################################\n"
	echo -ne "# Welcome, warrior\n#\n"
	echo -ne "# This setup allows to prepare the whole environment\n"
	echo -ne "# and make the process of your management easier.\n#\n"
	echo -ne "# We'll need some informations about other hosts,\n"
	echo -ne "# like IP, and some interactions, like passwords.\n#\n"
	echo -ne "# Would you like to configure de environment? (yes/no)\n"
	echo -ne "# "
	read next

	if [ $next = "yes" ]; then
		dialog --title 'Wait' --infobox 'Starting configuration in 3 seconds...' 4 35; sleep 0
		proceed
	else
		quit 0 "User chooses to quit"
	fi
}

##
# Starting point
#
if [ `id -u` -ne 0 ]; then
	quit 1 "You must be root to proceed"
else
	init
fi
