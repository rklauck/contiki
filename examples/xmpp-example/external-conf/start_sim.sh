#!/bin/bash
#
# Automated Contiki Simulation Setup
#

###  Variables  ###

SCRIPT=`readlink -f $0`
BASE_DIR=`dirname $SCRIPT`
CONTIKI_HOME="../../contiki-2.6/"

USE_PROSODY=1


###  Check Parameter  ###

## optional ##
if [ $# -ge 2 ]
then
	PROJECT=$1
	if [ ! -d ${PROJECT} ]
	then
		echo " Project ${PROJECT} does not exist"
		exit 1
	fi

	CSC_FILE=$2
	if [ ! -f ${PROJECT}/csc/${CSC_FILE} ]
	then
		echo " CSC-File ${PROJECT}/csc/${CSC_FILE} does not exist"
		exit 1
	fi

	IMAGE=$3
	if [ ! ${IMAGE} ]	# no parameter given, use default
	then
		IMAGE=${PROJECT}
	fi
else
	echo " Usage: $0 project_name csc_file [image_file] "
	exit 1
fi

echo ""
echo " Automated Contiki Simulation Setup "
echo ""


###  Step 0 - Build xmpp-TSP for z1  ###

if [ ! -f ${PROJECT}/${IMAGE}.z1 ]
then
	echo " Build image ${IMAGE} for z1.. "
	make -C ${PROJECT}/ TARGET=z1 ${IMAGE}

	if [ $? -eq 1 ]
	then
		echo " Error - Could not build image ${IMAGE} for z1"
		exit 1
	fi
else
	echo " Found ${IMAGE}.z1"
fi

echo ""



###  Step 1 - start 6tunnel  ###

echo " Start 6tunnel : 6tunnel -46 5223 localhost 5222.. "

pid=`ps -C 6tunnel | grep "6tunnel" | awk '{ print $1 }'`

if [ $pid ]
then
	echo " 6tunnel is already running. Restarting.. "
	kill $pid	
fi

6tunnel -46 5223 localhost 5222

if [ $? -eq 1 ]
then
	echo " Error - Could not run 6tunnel -46 5223 localhost 5222"
	exit 1
fi

echo ""



###  Step 2 - cooja  ###

## check cooja listening port
pid=`lsof | grep 60001 | awk '{ print $2 }'`

if [ $pid ]
then
	echo " Some program is listening on port 60001. Stopping it.. "
	kill $pid

	if [ $? -eq 1 ]
	then
		echo " Error - Could not stop process listening on port 60001."
		exit 1
	fi
fi

echo " Start cooja: make -C ${PROJECT}/ TARGET=cooja csc/${CSC_FILE}.. "

## start cooja
make -C ${PROJECT}/ TARGET=cooja csc/${CSC_FILE} > ${BASE_DIR}/cooja_output.log &

if [ $? -eq 1 ]
then
	echo " Error - Could not start cooja."
	exit 1
fi

echo ""


###  Make sure cooja is up and running  ###
echo " Wait until cooja has started. Then press Return to continue.. "
read


###  Step 3 - Restart Prosody

echo " Restart Prosody : sudo prosodyctl restart"

sudo prosodyctl restart

if [ $? -eq 1 ]
then
	echo " Error - Could not restart prosody"
	exit 1
fi

echo ""


###  Step 4 - tunslip6  ###

echo " Start tunslip6 : make -C ${PROJECT}/ TARGET=z1 connect-router-cooja.. "

make -C ${PROJECT}/ TARGET=z1 connect-router-cooja

if [ $? -eq 1 ]
then
	echo " Error - Could not start tunslip6"
	exit 1
fi

echo ""


###  Step 5 - Clean up  ###

echo " Restarting Prosody"
sudo prosodyctl restart


echo ""
exit 0
