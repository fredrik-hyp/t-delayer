#!/bin/sh

if [ `whoami` != "root" ]; then
	echo "Execute as root"
	exit
fi

interface=1
pathnumber=0
if [ $# -eq 1 ]; then
	interface="$1";
elif [ $# -eq 2 ]; then
	interface="$1";
	pathnumber="$2";
fi

SELECT=`su - postgres -c "psql delayerdb -t --field-separator ' ' --no-align --pset pager=off -c \"SELECT * FROM delayercoms WHERE streaminterface_id='$interface';\""`

if [ ! -z "$SELECT" ]; then
	echo "Already running command, run delete to clear the database or investigate why its not removed by the delayer"
	exit
fi

echo "2) Delayern läser in action column från delayercoms tabell, action = \"jump\""
echo "2.1) Delayern läser ut tid att hoppa till, t"
echo "2.1.3_b) annars pausa när man kommer till playto-tid"

su - postgres -c "psql delayerdb -c \"INSERT INTO delayercoms (streaminterface_id, pathnumber, action, jumptopos, startplayat, playto) VALUES ('$interface', '$pathnumber', 'jump', now()-(60 || 'second')::interval, NULL, now()+(20 || 'second')::interval);\""

PID=`pgrep -f "delayer st$interface"`
if [ "$PID" != "" ]; then
	echo "kill -s 10 "$PID
	kill -s 10 $PID
fi
