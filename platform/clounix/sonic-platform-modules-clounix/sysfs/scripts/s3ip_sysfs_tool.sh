#! /bin/bash
INSTALL_PATH=/usr/lib/modules/$(uname -r)/clounix
s3ip_start(){
        sudo modprobe i2c_dev
        sudo modprobe i2c_mux
        sudo modprobe pmbus_core
        sudo modprobe i2c_mux-pca9548
        sudo modprobe at24
        sudo insmod $INSTALL_PATH/s3ip_sysfs.ko
        sudo insmod $INSTALL_PATH/clounix_sysfs_main.ko
        sudo insmod $INSTALL_PATH/clounix_fpga.ko
        sudo insmod $INSTALL_PATH/clounix_fpga_i2c.ko
        sudo insmod $INSTALL_PATH/clounix_platform.ko
        sudo insmod $INSTALL_PATH/syseeprom_dev_driver.ko
        sudo insmod $INSTALL_PATH/fan_dev_driver.ko
        sudo insmod $INSTALL_PATH/cpld_dev_driver.ko
        sudo insmod $INSTALL_PATH/sysled_dev_driver.ko
        sudo insmod $INSTALL_PATH/psu_dev_driver.ko
        sudo insmod $INSTALL_PATH/transceiver_dev_driver.ko
        sudo insmod $INSTALL_PATH/temp_sensor_dev_driver.ko
        sudo insmod $INSTALL_PATH/vol_sensor_dev_driver.ko
        sudo insmod $INSTALL_PATH/fpga_dev_driver.ko
        sudo insmod $INSTALL_PATH/watchdog_dev_driver.ko
        sudo insmod $INSTALL_PATH/curr_sensor_dev_driver.ko
        #sudo rm -rf /sys_switch
        sudo insmod $INSTALL_PATH/adm1275.ko
        sudo insmod $INSTALL_PATH/gwcr-psu.ko
        sudo insmod $INSTALL_PATH/mp2882.ko
        sudo insmod $INSTALL_PATH/tps546b24a.ko
        sudo insmod $INSTALL_PATH/tmp75.ko
        sudo insmod $INSTALL_PATH/adm1166.ko
        sudo rmmod coretemp
        sudo insmod $INSTALL_PATH/cputemp.ko
        sudo /usr/local/bin/s3ip_load.py create
        echo "s3ip service start"
}
s3ip_stop(){
        sudo /usr/local/bin/s3ip_load.py remove
        sudo rmmod cputemp
        sudo rmmod adm1166
        sudo rmmod at24
        sudo rmmod adm1275
        sudo rmmod gwcr-psu
        sudo rmmod mp2882
        sudo rmmod tps546b24a
        sudo rmmod tmp75
        sudo rmmod curr_sensor_dev_driver
        sudo rmmod watchdog_dev_driver
        sudo rmmod fpga_dev_driver
        sudo rmmod vol_sensor_dev_driver
        sudo rmmod temp_sensor_dev_driver
        sudo rmmod transceiver_dev_driver
        sudo rmmod psu_dev_driver
        sudo rmmod sysled_dev_driver
        sudo rmmod cpld_dev_driver
        sudo rmmod fan_dev_driver
        sudo rmmod syseeprom_dev_driver
        sudo rmmod clounix_platform
        sudo rmmod clounix_fpga_i2c
        sudo rmmod clounix_fpga
        sudo rmmod clounix_sysfs_main
        sudo rmmod s3ip_sysfs
        sudo rmmod i2c_mux-pca9548
        sudo rmmod i2c_mux
        sudo rmmod i2c_dev                
 
        #sudo rm -rf /sys_switch
	echo "s3ip service stop"

}

case "$1" in
    start)
	s3ip_start
        ;;
    stop)
	s3ip_stop
	;;
    status)
        sudo tree -l /sys_switch
	;;
    restart)
	s3ip_stop
	s3ip_start
	;;	
    *)
        echo "Usage: $0 {start|stop|status|restart}"
	exit 1
esac
exit 

