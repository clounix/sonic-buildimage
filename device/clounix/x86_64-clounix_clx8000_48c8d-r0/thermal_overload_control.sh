#!/bin/bash
#2022.12.23 by songqh
# 
#
#

PATH=/sbin:/bin:/usr/sbin:/usr/bin:/usr/local/bin

DEVICE=$(lspci | grep "Xilinx Corporation Device 7021" | awk  -F ' '  '{print $1}')

if [ "$DEVICE" == "" ];then
     echo "Not found fpga device"
     exit 1
fi

SETREG_FILE=/sys/bus/pci/devices/0000:$DEVICE/power_cycle

prog="$0"
command="$1"
echo $1
echo $DEVICE
if [[ $EUID -ne 0 ]]; then
    echo "This script must be run as root"
    exit 1
fi

usage() {
    echo "Usage: thermal_overload_control.sh [option] <command>"
    echo
    echo "Options:"
    echo "  -h, --help          : to print this message."
    echo
    echo "Commands:"
    echo
    echo "  cpu:  To enabling CPU thermal overload handler"
    echo
    echo "  asic : To enabling ASIC thermal overload handler"
    echo
}
power_cycle() {
    echo 0x1 > ${SETREG_FILE}
}

cpu_overload() {
	logger "Enable CPU thermal overload control"
    power_cycle
}

asic_overload() {
    logger "Enable ASIC thermal overload control"
    power_cycle
}

if [ $# -lt 1 ]; then
    usage
    exit -1
fi

case "$command" in
-h | --help)
    usage
    ;;
cpu)
	cpu_overload
	;;
asic)
	asic_overload
	;;
*)
	usage
	exit -1
	;;
esac

exit $?
