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

su - postgres -c "psql delayerdb -c \"DELETE FROM delayercoms WHERE streaminterface_id='$interface';\""
