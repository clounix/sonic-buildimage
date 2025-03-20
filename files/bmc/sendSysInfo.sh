#!/bin/bash

while true
do
	#set bmc time
	ipmitool -I open sel time set now
	sleep 3
	#get cpu temperature
	CPU_Temp=`cat /sys/class/thermal/thermal_zone0/temp`
	CPU_T=$(( CPU_Temp / 1000 ))
	#send CPU temperature to BMC every 3s
	ipmitool -I open raw 0x2e 0x11 $CPU_T > /dev/null 
	sleep 3
done
