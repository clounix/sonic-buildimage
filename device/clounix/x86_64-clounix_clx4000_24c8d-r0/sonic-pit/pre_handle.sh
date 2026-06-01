#!/bin/bash
check_dut_env() {
	echo "==============================================================="
	echo "            check DUT environment                 "
	echo "show version"
	show version
	sleep 0.1

	echo "show platform firmware status"
	show platform firmware status
	sleep 0.1

	echo "show system-health summary"
	show system-health summary
	sleep 0.1

	echo "show platform psustatus"
	show platform psustatus
	sleep 0.1

	echo "show platform fan"
	show platform fan
	sleep 0.1

	echo "show platform temperature"
	show platform temperature

	echo "================================================================="
}

if [ $# -eq 0 ]; then
	check_dut_env
fi

