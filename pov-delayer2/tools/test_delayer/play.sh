#!/bin/sh

if [ `whoami` != "root" ]; then
	echo "Execute as root"
	exit
fi

echo "3) Om action är satt till pause eller play så ska uppspelningen"
echo "pausas eller fortsätta, inget hopp ska göras i detta fall utan spela"
echo "från currpos"

interface=1
pathnumber=0
if [ $# -eq 1 ]; then
        interface="$1";
elif [ $# -eq 2 ]; then
        interface="$1";
        pathnumber="$2";
fi

su - postgres -c "psql delayerdb -c \"INSERT INTO delayercoms (streaminterface_id, pathnumber, action, jumptopos, startplayat, playto) VALUES ('$interface', '$pathnumber', 'play', NULL, NULL, NULL);\""

PID=`pgrep -f "delayer st$interface"`
if [ "$PID" != "" ]; then
	echo "kill -s 10 "$PID
	kill -s 10 $PID
fi
