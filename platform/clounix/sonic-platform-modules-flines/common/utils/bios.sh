#!/bin/bash

set -e

export ZAFU=/usr/local/bin/afulnx_64
export ZMOD=/usr/lib/modules/6.1.0-29-2-amd64/extra/amifldrv.ko
export ZID=`id -u`
export ZCODE="\x1b[32m"
export ZBYPASS_INSTALL=""
export ZSH=/usr/local/bin/bios.sh

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
	[[ $ZID != 0 ]] && delay_exit "root priviledge is required." 4
	[ ! -f $ZAFU ] && delay_exit "missing flash application." 5
	[[ ! "`file --mime-type $ZAFU`" =~ "application/x-executable" ]] && delay_exit "invalid flash application." 6
	[ ! -f $ZMOD ] && delay_exit "missing flash driver." 1
	[[ ! "`file --mime-type $ZMOD`" =~ "application/x-object" ]] && delay_exit "invalid flash application." 6
	[ -z "$1" ] && delay_exit "missing file path parameter." 2
	[ ! -f $1 ] && delay_exit "invalid rom file path." 3
	file $ZAFU
	file $ZMOD
	return 0
}

upgrade(){
	insmod $ZMOD
	lsmod | grep ami
	$ZAFU $1 /p /b /x /n /r
	rmmod amifldrv
	delay_exit "online BIOS upgrade finished." 0
	return 0
}

check_environment $*
upgrade $*
