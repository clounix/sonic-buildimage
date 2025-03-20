#!/bin/bash

set -e

ZVTYSH=/var/vtysh || true
ZAFU=/bin/afulnx_64
ZMOD=$ZVTYSH/amifldrv_mod || true
ZID=`id -u` || true
ZCODE="\x1b[32m" || true
ZBYPASS_INSTALL=""
ZBIOS=/bin/bios.sh

delay_exit(){
    ZCODE="\x1b[31m"
    if [[ $2 == 0 ]]
	then
		ZCODE="\x1b[32m"
	else
		ZCODE="\x1b[31m"
	fi
	echo -e "$ZCODE$1\x1b[0m"
	sleep 5 
	exit $2
}

check_environment(){
	[ ! -f $ZMOD ] && delay_exit "bios driver is broken." 1
	[ -z "$1" ] && delay_exit "bin file path is required." 2
	[ ! -f $1 ] && delay_exit "path is error." 3
	[[ $ZID != 0 ]] && delay_exit "root priviledge is required." 4
	return 0
}

upgrade(){
	cp -f $ZMOD amifldrv_mod
	ls -alh | grep ami
	insmod amifldrv_mod
	lsmod | grep ami
	afulnx_64 $1 /p /b /x /n /r
	delay_exit "online BIOS upgrade finished." 0
	return 0
}

prepare(){
	[[ $ZID != 0 ]] && delay_exit "root priviledge is required." 4
	[ -f $ZAFU ] && ZBYPASS_INSTALL="BYPASS" && return 0
	[ ! -f afulnx_64 -o ! -f amifldrv_mod ] && delay_exit "current directory is broken." 5
	[ ! -d $ZVTYSH ] && delay_exit "vtysh directory is broken." 6
	return 0
}

install(){
	cp -f afulnx_64 $ZAFU
	chmod +x $ZAFU
	file $ZAFU
	cp -f amifldrv_mod $ZMOD
	file $ZMOD
#	cp -f bios.sh $ZBIOS
#	chmod +x $ZBIOS
	file $ZBIOS
	return 0
}

prepare
[ -z "$ZBYPASS_INSTALL" ] && install
check_environment $*
upgrade $*
