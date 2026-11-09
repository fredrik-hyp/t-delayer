#!/bin/sh

if [ `whoami` != "root" ]; then
	echo "Execute as root"
	exit
fi

interface=0
pathnumber=0
if [ $# -ne 2 ]; then
	echo "Usage: ./jump_05s_from_start.sh <streaminterface_id #> <pathnumber #>";
	exit
fi

interface="$1";
pathnumber="$2";

# -- Check presence of command --

SELECT=`su - postgres -c "psql delayerdb -t --field-separator ' ' --no-align --pset pager=off -c \"SELECT * FROM delayercoms WHERE streaminterface_id='$interface';\""`

if [ ! -z "$SELECT" ]; then
	echo "Already running command, run delete to clear the database or investigate why its not removed by the delayer"
	exit
fi

echo "2) The delayer reads the action column from the delayercoms table, action = \"jump\""
echo "2.1) The delayer reads the time to jump to, t"
echo "2.1.1) If startplayat is NULL, pause; play is sent as described in step 3 below"

# -- Command --

su - postgres -c "psql delayerdb -c \"INSERT INTO delayercoms (streaminterface_id, pathnumber, action, jumptopos, startplayat, playto) VALUES ('$interface', '$pathnumber', 'jump', (select lastpos-(5 || 'second')::interval from delayerstats where streaminterface_id='$interface' AND pathnumber='$pathnumber'), NULL, NULL);\""

PID=`pgrep -f "delayer st$interface"`
if [ "$PID" != "" ]; then
	echo "kill -s 10 "$PID
	kill -s 10 $PID
fi
