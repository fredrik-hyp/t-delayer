#!/bin/sh

if [ `whoami` != "root" ]; then
	echo "Execute as root"
	exit
fi

interface=1
if [ $# -eq 1 ]
then
	interface="$1";
fi


su - postgres -c "psql delayerdb -x --pset pager=off -c \"SELECT * FROM streaminterfaceconfigs WHERE streaminterface_id='$interface';\""
su - postgres -c "psql delayerdb -x --pset pager=off -c \"SELECT * FROM streaminterfacesources WHERE streaminterfaceconfig_id='$interface';\""
su - postgres -c "psql delayerdb -x --pset pager=off -c \"SELECT * FROM streaminterfacesinks WHERE streaminterfaceconfig_id='$interface';\""
su - postgres -c "psql delayerdb -x --pset pager=off -c \"SELECT * FROM streaminterfacebackupsinks WHERE streaminterfacesink_id='$interface';\""
