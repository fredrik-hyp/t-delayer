#!/bin/sh

if [ `whoami` != "root" ]; then
	echo "Execute as root"
	exit
fi

interface=1
no_path=2
if [ $# -eq 1 ]
then
        interface="$1";
elif [ $# -eq 2 ]
then
        interface="$1";
        no_path="$2";
else
        echo "Usage: ./show_delayerstats.sh <streaminterface_id#> <no path#>"
        echo "Use default streaminterface_id = 1"
        echo "----------------------------------"
        echo ""
fi

echo "===== delayerstats =====\n"
su - postgres -c "psql delayerdb --pset pager=off -c \"SELECT * FROM delayerstats WHERE streaminterface_id='$interface';\""
echo "===== streamstats =====\n"
su - postgres -c "psql delayerdb --pset pager=off -c \"SELECT * FROM streamstats WHERE streaminterface_id='$interface' order by id desc limit $no_path;\""
echo "===== streaminterfacelivestatistics =====\n"
su - postgres -c "psql delayerdb --pset pager=off -c \"SELECT * FROM streaminterfacelivestatistics WHERE streaminterface_id='$interface' order by id desc limit $no_path;\""
