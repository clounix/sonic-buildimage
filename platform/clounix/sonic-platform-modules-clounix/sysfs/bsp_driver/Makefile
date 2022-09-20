PWD		= $(shell pwd)

INTERFACE_DIR:= if
CLX_DRIVER_DIR:= driver

MAKEFILE_FILE_PATH = $(abspath $(lastword $(MAKEFILE_LIST)))
DEV_SYSFS_HEADER_DIR = $(abspath $(MAKEFILE_FILE_PATH)/../../s3ip_sysfs_frame/include)
SWITCH_DVR_HEADER_DIR = $(abspath $(MAKEFILE_FILE_PATH)/../../bsp_driver/include)
EXTRA_CFLAGS:= -I$(M)/include -I$(M)/$(INTERFACE_DIR) $(M)/$(CLX_DRIVER_DIR)
EXTRA_CFLAGS+= -I$(DEV_SYSFS_HEADER_DIR)
EXTRA_CFLAGS+= -I$(SWITCH_DVR_HEADER_DIR)
EXTRA_CFLAGS+= -Wall

obj-m := clounix_sysfs_main.o
clounix_sysfs_main-objs := $(CLX_DRIVER_DIR)/clounix_sysfs_main.o

obj-m += clounix_fpga_i2c.o
clounix_fpga_i2c-objs := $(CLX_DRIVER_DIR)/clounix_fpga_i2c.o

obj-m += clounix_fpga.o
clounix_fpga-objs := $(CLX_DRIVER_DIR)/clounix_fpga_clx8000.o

obj-m += clounix_platform.o
clounix_platform-objs := clx_platform_driver.o \
		$(CLX_DRIVER_DIR)/clx_driver_common.o \
		$(INTERFACE_DIR)/clx_driver.o

obj-m += syseeprom_dev_driver.o
syseeprom_dev_driver-objs := syseeprom_device_driver.o \
			$(INTERFACE_DIR)/syseeprom_interface.o \
			$(CLX_DRIVER_DIR)/syseeprom_driver_clx8000.o

obj-m += fan_dev_driver.o
fan_dev_driver-objs := fan_device_driver.o \
			$(INTERFACE_DIR)/fan_interface.o \
			$(CLX_DRIVER_DIR)/fan_driver_clx8000.o

obj-m += cpld_dev_driver.o
cpld_dev_driver-objs := cpld_device_driver.o \
			$(INTERFACE_DIR)/cpld_interface.o \
			$(CLX_DRIVER_DIR)/cpld_driver_clx8000.o

obj-m += sysled_dev_driver.o
sysled_dev_driver-objs := sysled_device_driver.o \
			$(INTERFACE_DIR)/sysled_interface.o \
			$(CLX_DRIVER_DIR)/sysled_driver_clx8000.o

obj-m += slot_device_driver.o

obj-m += psu_dev_driver.o
psu_dev_driver-objs := psu_device_driver.o \
			$(INTERFACE_DIR)/psu_interface.o \
			$(CLX_DRIVER_DIR)/psu_driver_clx8000.o

obj-m += transceiver_dev_driver.o
transceiver_dev_driver-objs := transceiver_device_driver.o \
			$(INTERFACE_DIR)/xcvr_interface.o \
			$(CLX_DRIVER_DIR)/xcvr_driver_clx8000.o

obj-m += temp_sensor_dev_driver.o
temp_sensor_dev_driver-objs := temp_sensor_device_driver.o \
			$(INTERFACE_DIR)/temp_interface.o \
			$(CLX_DRIVER_DIR)/temp_driver_clx8000.o

obj-m += vol_sensor_dev_driver.o
vol_sensor_dev_driver-objs := vol_sensor_device_driver.o \
			$(INTERFACE_DIR)/voltage_interface.o \
			$(CLX_DRIVER_DIR)/voltage_driver_clx8000.o

obj-m += curr_sensor_dev_driver.o
curr_sensor_dev_driver-objs := curr_sensor_device_driver.o \
			$(INTERFACE_DIR)/current_interface.o \
			$(CLX_DRIVER_DIR)/current_driver_clx8000.o

obj-m += fpga_dev_driver.o
fpga_dev_driver-objs := fpga_device_driver.o \
			$(INTERFACE_DIR)/fpga_interface.o \
			$(CLX_DRIVER_DIR)/fpga_driver_clx8000.o

obj-m += watchdog_dev_driver.o
watchdog_dev_driver-objs := watchdog_device_driver.o \
			$(INTERFACE_DIR)/watchdog_interface.o \
			$(CLX_DRIVER_DIR)/watchdog_driver_clx8000.o

obj-m += adm1275.o
obj-m += mp2882.o
obj-m += gwcr-psu.o
obj-m += tps546b24a.o
obj-m += tmp75.o
obj-m += adm1166.o
obj-m += cputemp.o
obj-m += s3ip_netlink.o

all:
	$(MAKE) -C $(KERNEL_SRC)/build M=$(PWD) modules
	$(CC) -o s3ip_proxy s3ip_proxy.c -I./include
	@if [ ! -d $(sysfs_out_put_dir) ]; then mkdir -p $(sysfs_out_put_dir) ;fi
	cp -p  $(PWD)/*.ko $(sysfs_out_put_dir)
	
clean:
	rm -f $(PWD)/*.o $(PWD)/*.ko $(PWD)/*.mod.c $(PWD)/.*.cmd $(PWD)/*.mod 
	rm -f $(PWD)/Module.markers $(PWD)/Module.symvers $(PWD)/modules.order 
	rm -rf $(PWD)/.tmp_versions
	rm -rf $(PWD)/.cache.mk
