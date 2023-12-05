#!/bin/sh

if [ `whoami` != "root" ]; then
	echo "Execute as root"
	exit
fi

interface=1
if [ $# -eq 1 ]; then
        interface="$1";
else
	echo "usage: sh pause_5xPath_play_with_60s_gap.sh <interface>"
	exit 1
fi

sh pause.sh $interface 0
sleep 10
sh pause.sh $interface 1
sleep 10
sh pause.sh $interface 2
sleep 10
sh pause.sh $interface 3
sleep 10
sh pause.sh $interface 4
sleep 100

sh play.sh $interface 0
sleep 30
sh play.sh $interface 1
sleep 30
sh play.sh $interface 2
sleep 30
sh play.sh $interface 3
sleep 30
sh play.sh $interface 4




