#!/bin/sh

if [ `whoami` != "root" ]; then
	echo "Execute as root"
	exit
fi

interface=1
time_to_jump=10
pathnumber=0
if [ $# -eq 2 ]; then
        interface="$1";
	time_to_jump="$2";
elif [ $# -eq 3 ]; then
	interface="$1";
	pathnumber="$2";
	time_to_jump="$3";
fi

SELECT=`su - postgres -c "psql delayerdb -t --field-separator ' ' --no-align --pset pager=off -c \"SELECT * FROM delayercoms WHERE streaminterface_id='$interface';\""`

if [ ! -z "$SELECT" ]; then
	echo "Already running command, run delete to clear the database or investigate why its not removed by the delayer"
	exit
fi

su - postgres -c "psql delayerdb -c \"INSERT INTO delayercoms (streaminterface_id, pathnumber, action, jumptopos, startplayat, playto) VALUES ('$interface', '$pathnumber', 'jump', now()+($time_to_jump || 'year')::interval, NULL, NULL);\""

PID=`pgrep -f "delayer st$interface"`
if [ "$PID" != "" ]; then
	echo "kill -s 10 "$PID
	kill -s 10 $PID
fi
