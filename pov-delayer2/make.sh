#!/bin/sh

#########################
#     VERSION SETUP     #
#########################
# Available versions for pov-manager
# master)
# tags/v0.7.3-test2
# tags/v0.7.4-test1
# tags/v0.7.5-beta1
# tags/v0.7.5-beta2
# tags/v0.8.0
# tags/v1.0.0-rc1
# tags/v1.0.0-rc2
# tags/v2.0.0-rc1

CC=cc
CFLAGS=" -DVERSION=\"3.1.4_dev\""
POV_MANAGER_VERSION="master"

if [ "${POV_MANAGER_VERSION}" = "master" ]; then
	POV_COMMON_VERSION="master"
	POV_DATABASE_VERSION="master"
elif [ ${POV_MANAGER_VERSION} = "tags/3.1.3" ]; then
	POV_COMMON_VERSION="2.1.4"
	POV_DATABASE_VERSION="20.1"
elif [ ${POV_MANAGER_VERSION} = "tags/3.1.2" ]; then
	POV_COMMON_VERSION="2.1.4"
	POV_DATABASE_VERSION="20.1"
elif [ ${POV_MANAGER_VERSION} = "tags/3.1.1" ]; then
	POV_COMMON_VERSION="2.1.2"
	POV_DATABASE_VERSION="20.1"
elif [ ${POV_MANAGER_VERSION} = "tags/3.1.0" ]; then
	POV_COMMON_VERSION="2.1.1"
	POV_DATABASE_VERSION="20.1"
elif [ ${POV_MANAGER_VERSION} = "tags/3.0.0-rc3" ]; then
	POV_COMMON_VERSION="2.0.1"
	POV_DATABASE_VERSION="18.2"
elif [ ${POV_MANAGER_VERSION} = "tags/3.0.0-rc2" ]; then
	POV_COMMON_VERSION="2.0.1"
	POV_DATABASE_VERSION="18.2"
elif [ ${POV_MANAGER_VERSION} = "tags/3.0.0-rc1" ]; then
	POV_COMMON_VERSION="2.0.1"
	POV_DATABASE_VERSION="18.1"
elif [ ${POV_MANAGER_VERSION} = "tags/2.0.6" ]; then
	POV_COMMON_VERSION="2.0.0"
	POV_DATABASE_VERSION="14.1"
elif [ ${POV_MANAGER_VERSION} = "tags/2.0.5" ]; then
	POV_COMMON_VERSION="2.0.0"
	POV_DATABASE_VERSION="14.1"
elif [ ${POV_MANAGER_VERSION} = "tags/2.0.4" ]; then
	POV_COMMON_VERSION="2.0.0"
	POV_DATABASE_VERSION="14.1"
elif [ ${POV_MANAGER_VERSION} = "tags/2.0.3" ]; then
	POV_COMMON_VERSION="2.0.0"
	POV_DATABASE_VERSION="14.1"
elif [ ${POV_MANAGER_VERSION} = "tags/2.0.2" ]; then
	POV_COMMON_VERSION="2.0.0"
	POV_DATABASE_VERSION="14.1"
elif [ ${POV_MANAGER_VERSION} = "tags/v2.0.1" ]; then
	POV_COMMON_VERSION="2.0.0"
	POV_DATABASE_VERSION="14.0"
elif [ ${POV_MANAGER_VERSION} = "tags/v2.0.1-rc1" ]; then
	POV_COMMON_VERSION="2.0.0"
	POV_DATABASE_VERSION="13.0"
elif [ ${POV_MANAGER_VERSION} = "tags/v2.0.0" ]; then
	POV_COMMON_VERSION="2.0.0"
	POV_DATABASE_VERSION="13.0"
elif [ ${POV_MANAGER_VERSION} = "tags/v2.0.0-rc1" ]; then
	POV_COMMON_VERSION="2.0.0-rc1"
	POV_DATABASE_VERSION="13.0"
elif [ ${POV_MANAGER_VERSION} = "tags/v1.0.1-rc1" ]; then
	POV_COMMON_VERSION="1.0.1-rc1"
	POV_DATABASE_VERSION="10.3"
elif [ ${POV_MANAGER_VERSION} = "tags/v1.0.0" ]; then
	POV_COMMON_VERSION="0.12"
	POV_DATABASE_VERSION="10.3"
elif [ ${POV_MANAGER_VERSION} = "tags/v1.0.0-rc4" ]; then
	POV_COMMON_VERSION="0.12"
	POV_DATABASE_VERSION="0.8"
elif [ ${POV_MANAGER_VERSION} = "tags/v1.0.0-rc3" ]; then
	POV_COMMON_VERSION="0.12"
	POV_DATABASE_VERSION="0.8"
elif [ ${POV_MANAGER_VERSION} = "tags/v1.0.0-rc2" ]; then
	POV_COMMON_VERSION="0.10"
	POV_DATABASE_VERSION="0.8"
elif [ ${POV_MANAGER_VERSION} = "tags/v1.0.0-rc1" ]; then
	POV_COMMON_VERSION="0.9"
	POV_DATABASE_VERSION="0.8"
elif [ ${POV_MANAGER_VERSION} = "tags/v0.8.0" ]; then
	POV_COMMON_VERSION="tags/0.8"
	POV_DATABASE_VERSION="tags/7.0"
elif [ ${POV_MANAGER_VERSION} = "tags/v0.7.5-beta2" ]; then
	POV_COMMON_VERSION="tags/0.7"
	POV_DATABASE_VERSION="tags/6.0"
elif [ ${POV_MANAGER_VERSION} = "tags/v0.7.5-beta1" ]; then
	POV_COMMON_VERSION="tags/0.6"
	POV_DATABASE_VERSION="tags/5.1"
elif [ ${POV_MANAGER_VERSION} = "tags/v0.7.4-test1" ]; then
	POV_COMMON_VERSION="tags/0.5"
	POV_DATABASE_VERSION="tags/4.1"
elif [ ${POV_MANAGER_VERSION} = "tags/v0.7.3-test2" ]; then
	POV_COMMON_VERSION="tags/0.5"
	POV_DATABASE_VERSION="tags/4.1"
else
	echo "No valid pov-manager version $POV_MANAGER_VERSION"
	exit
fi


##########################
#  CHECKOUT DEPENDENCIES #
##########################
if [ 0 -eq 1 ]; then

if [ ! -d "../pov-common/" ]; then
	echo "Cloning pov-common"
	cd ..
	git clone git@192.168.15.221:poverix/pov-common.git
	cd -
	echo  "";
	if [ ! -d "../pov-common/" ]; then
		echo "Cloning of pov-common FAILED!"
		exit;
	fi
fi

if [ ! -d "../pov-database/src/c/" ]; then
	echo "Cloning pov-database"
	cd ..
	git clone git@192.168.15.221:poverix/pov-database.git
	cd -
	echo "";
	if [ ! -d "../pov-database/" ]; then
		echo "Cloning of pov-database FAILED!"
		exit;
	fi
fi

git checkout ${POV_MANAGER_VERISON} 
if [ ${POV_MANAGER_VERSION} = "master" ]; then
	git pull origin master
fi

cd ../pov-common
git checkout ${POV_COMMON_VERSION}
cd -
if [ ${POV_COMMON_VERSION} = "master" ]; then
	cd ../pov-common
	git pull origin master
	cd -
fi

cd ../pov-database
git checkout ${POV_DATABASE_VERSION}
cd -
if [ ${POV_DATABASE_VERSION} = "master" ]; then
	cd ../pov-database
	git pull origin master
	cd -
fi

fi

#########################
# ORIGINAL BUILDSECTION #
#########################

DELAYERLIBFLAGS=" -L/usr/lib/ -lrt -lpthread -L/usr/local/lib/ -lpq "

DBSRC="../pov-database/src/c/dbcommon.c ../pov-database/src/c/dbstreaminterfaces.c ../pov-database/src/c/dbsysteminfo.c "
INCLUDE=" -I ./ -I ../pov-database/src/c/ -I ../pov-common/ -I/usr/local/include/ -I/usr/include/postgresql/"
DELAYERSRC=" ../pov-common/common.c ../pov-common/common-db.c ../pov-common/common-time.c "
DELAYERSRC=$DELAYERSRC" ../pov-common/buffer.c ../pov-common/thread_traps.c ../pov-common/thread_statistic.c ../pov-common/thread_pokepovmanager.c "
DELAYERSRC=$DELAYERSRC" ../pov-common/prot_rudp.c ../pov-common/stat_func.c "
DELAYERSRC=$DELAYERSRC" $DBSRC"
CFLAGS=$CFLAGS" -std=gnu99 -Wall "
INSTALL=0
INSTALL_PACKAGE=0
TESTTOOL=0

echo "usage: ./make.sh [noarg | -d | -dX] [-g|-T] [-m] [-p]"
echo "  [noarg] release + install"
echo "---------------------------------------------------"
echo "  debug builds are not supposed to run in daemonized mode"
echo "  -d debug build"
echo "  -db debug build buffer"
echo "  -ds debug build SQL"
echo "  -dH debug heartbeat"
echo "  -dT debug TRYLOCK"
echo "  -dA all debug flags above"
echo "  -g use developer (OHDATA) GPG keys for reading licensefile"
echo "  -T build test tools"
echo ""
echo ""

if [ $# -eq 0 ]; then
	echo "Compile and install - no debug printouts"
	INSTALL=1
elif [ "$1" = "-db" ]; then
	echo "Debug compile buffer.c"
	INSTALL=0
	CFLAGS=$CFLAGS" -g3 -D NODAEMON_DEBUG_BUFFER"
elif [ "$1" = "-ds" ]; then
	echo "Debug compile SQL"
	INSTALL=0
	CFLAGS=$CFLAGS" -g3 -D NODAEMON_DEBUG_DB"
elif [ "$1" = "-d" ]; then
	echo "Debug compile"
	INSTALL=0
	CFLAGS=$CFLAGS" -g3 -D NODAEMON_DEBUG"
elif [ "$1" = "-dH" ]; then
	echo "Debug compile Heartbeat timeout"
	INSTALL=0
	CFLAGS=$CFLAGS" -g3 -D NODAEMON_DEBUG_HEARTBEAT"
elif [ "$1" = "-dT" ]; then
	echo "Debug compile TRYLOCK"
	INSTALL=0
	CFLAGS=$CFLAGS" -g3 -D TRYLOCK"
elif [ "$1" = "-dA" ]; then
	echo "Debug compile all debug flags"
	INSTALL=0
	CFLAGS=$CFLAGS" -g3 -D NODAEMON_DEBUG -D NODAEMON_DEBUG_BUFFER -D NODAEMON_DEBUG_DB -D NODAEMON_DEBUG_HEARTBEAT -D TRYLOCK"
elif [ "$1" = "-g" ]; then
	echo "Developer GPG keys"
	INSTALL=1
	CFLAGS=$CFLAGS" -D USE_DEVELOPER_GPG_KEYS"
elif [ "$1" = "-T" ]; then
	echo "Building testtools"
	TESTTOOL=1
elif [ "$1" = "-p" ]; then
	echo "Create package"
	INSTALL=0
	INSTALL_PACKAGE=1
else

	echo "Unkown command"
	exit;
fi

if [ $# -eq 2 ]; then
	if [ "$2" = "-g" ]; then
		echo "Developer GPG keys"
		CFLAGS="$CFLAGS -D USE_DEVELOPER_GPG_KEYS"
	elif [ "$2" = "-T" ]; then
		echo "Building testtools"
		TESTTOOL=1
	else
		echo "Unkown command"
		exit;
	fi
fi


echo "Compile"

echo "============================delayer============================="
echo "$CC $INCLUDE -D DELAYERD $CFLAGS delayer_main.c delayer_other.c $DELAYERSRC  $DELAYERLIBFLAGS -o delayer"
echo ""
$CC $INCLUDE -D DELAYERD $CFLAGS delayer_main.c delayer_other.c ../pov-database/src/c/dbdelayers.c $DELAYERSRC  $DELAYERLIBFLAGS -o delayer


if [ $TESTTOOL = 1 ]; then
	echo ""
	echo "=============== BUILDING TESTTOOLS =================="
	echo ""
	cd tools/show_statistic/
	make
	cd ../read_chunk_file/
	./make.sh
fi

echo ""


if [ $INSTALL = 1 ]; then
	echo "Installing"
	install -m 555 delayer /usr/sbin/
fi
if [ $INSTALL_PACKAGE = 1 ]; then
	echo "Installing into pov-delayer2/tools/create_package/delayer-4.0.0"
	install -m 555 delayer tools/create_package/delayer-4.0.0/usr/local/bin/
fi

echo "done"
