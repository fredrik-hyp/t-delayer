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
su - postgres -c "psql delayerdb --pset pager=off -c \"SELECT * FROM delayercoms WHERE streaminterface_id='$interface';\""
