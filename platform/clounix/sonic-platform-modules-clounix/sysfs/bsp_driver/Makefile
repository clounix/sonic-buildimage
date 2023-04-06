PWD		= $(shell pwd)

INTERFACE_DIR:= if
CLX_DRIVER_DIR:= driver
PLAT_DRV := $(CLX_DRIVER_DIR)/platform
CPLD_DRV := $(CLX_DRIVER_DIR)/cpld
FPGA_DRV := $(CLX_DRIVER_DIR)/fpga
E2P_DRV := $(CLX_DRIVER_DIR)/eeprom
FAN_DRV := $(CLX_DRIVER_DIR)/fan
CURR_DRV := $(CLX_DRIVER_DIR)/curr_sensor
VOL_DRV := $(CLX_DRIVER_DIR)/vol_sensor
TEMP_DRV := $(CLX_DRIVER_DIR)/temp_sensor
LED_DRV := $(CLX_DRIVER_DIR)/led
PSU_DRV := $(CLX_DRIVER_DIR)/psu
WDT_DRV := $(CLX_DRIVER_DIR)/watchdog
XCVR_DRV := $(CLX_DRIVER_DIR)/xcvr
MISC_DRV := $(CLX_DRIVER_DIR)/misc

MAKEFILE_FILE_PATH = $(abspath $(lastword $(MAKEFILE_LIST)))
DEV_SYSFS_HEADER_DIR = $(abspath $(MAKEFILE_FILE_PATH)/../../s3ip_sysfs_frame/include)
SWITCH_DVR_HEADER_DIR = $(abspath $(MAKEFILE_FILE_PATH)/../../bsp_driver/include)
EXTRA_CFLAGS:=  -I$(M)/include \
				-I$(M)/$(INTERFACE_DIR) \
				-I$(M)/$(PLAT_DRV) \
				-I$(M)/$(CPLD_DRV) \
				-I$(M)/$(FPGA_DRV) \
				-I$(M)/$(E2P_DRV) \
				-I$(M)/$(FAN_DRV) \
				-I$(M)/$(CURR_DRV) \
				-I$(M)/$(VOL_DRV) \
				-I$(M)/$(TEMP_DRV) \
				-I$(M)/$(LED_DRV) \
				-I$(M)/$(PSU_DRV) \
				-I$(M)/$(WDT_DRV) \
				-I$(M)/$(XCVR_DRV) \
				-I$(M)/$(MISC_DRV)
EXTRA_CFLAGS+= -I$(DEV_SYSFS_HEADER_DIR)
EXTRA_CFLAGS+= -I$(SWITCH_DVR_HEADER_DIR)
EXTRA_CFLAGS+= -Wall

obj-m := clounix_sysfs_main.o
clounix_sysfs_main-objs := $(PLAT_DRV)/drv_sysfs_clx.o

obj-m += clounix_platform.o
clounix_platform-objs := clx_platform_driver.o \
		$(INTERFACE_DIR)/clx_driver.o \
		$(PLAT_DRV)/drv_platform_common.o

obj-m += fpga_dev_driver.o
fpga_dev_driver-objs := fpga_device_driver.o \
			$(INTERFACE_DIR)/fpga_interface.o \
			$(FPGA_DRV)/drv_fpga_clx.o \
			$(FPGA_DRV)/drv_fpga_xlinx.o \
			$(FPGA_DRV)/drv_fpga_anlogic.o

obj-m += i2c_master_dev_driver.o
i2c_master_dev_driver-objs := i2c_master_device_driver.o \
			$(INTERFACE_DIR)/i2c_master_interface.o \
			$(PLAT_DRV)/drv_i2c_xilinx.o \
			$(PLAT_DRV)/drv_i2c_anlogic.o

obj-m += syseeprom_dev_driver.o
syseeprom_dev_driver-objs := syseeprom_device_driver.o \
			$(INTERFACE_DIR)/syseeprom_interface.o \
			$(E2P_DRV)/drv_syseeprom.o

obj-m += fan_dev_driver.o
fan_dev_driver-objs := fan_device_driver.o \
			$(INTERFACE_DIR)/fan_interface.o \
			$(FAN_DRV)/drv_fan_clx.o

obj-m += cpld_dev_driver.o
cpld_dev_driver-objs := cpld_device_driver.o \
			$(INTERFACE_DIR)/cpld_interface.o \
			$(CPLD_DRV)/drv_cpld_lattice.o

obj-m += sysled_dev_driver.o
sysled_dev_driver-objs := sysled_device_driver.o \
			$(INTERFACE_DIR)/sysled_interface.o \
			$(LED_DRV)/drv_sysled_fpga.o

#obj-m += slot_device_driver.o

obj-m += psu_dev_driver.o
psu_dev_driver-objs := psu_device_driver.o \
			$(INTERFACE_DIR)/psu_interface.o \
			$(PSU_DRV)/drv_psu_clx.o

obj-m += transceiver_dev_driver.o
transceiver_dev_driver-objs := transceiver_device_driver.o \
			$(INTERFACE_DIR)/xcvr_interface.o \
			$(XCVR_DRV)/drv_xcvr_fpga.o

obj-m += temp_sensor_dev_driver.o
temp_sensor_dev_driver-objs := temp_sensor_device_driver.o \
			$(INTERFACE_DIR)/temp_interface.o \
			$(TEMP_DRV)/drv_temp_sensor_clx.o

obj-m += vol_sensor_dev_driver.o
vol_sensor_dev_driver-objs := vol_sensor_device_driver.o \
			$(INTERFACE_DIR)/voltage_interface.o \
			$(VOL_DRV)/drv_vol_sensor_clx.o

obj-m += curr_sensor_dev_driver.o
curr_sensor_dev_driver-objs := curr_sensor_device_driver.o \
			$(INTERFACE_DIR)/current_interface.o \
			$(CURR_DRV)/drv_curr_sensor_clx.o

obj-m += watchdog_dev_driver.o
watchdog_dev_driver-objs := watchdog_device_driver.o \
			$(INTERFACE_DIR)/watchdog_interface.o \
			$(WDT_DRV)/drv_wdt_clx.o

obj-m += lpc_syscpld_dev_driver.o
lpc_syscpld_dev_driver-objs := lpc_syscpld_device_driver.o \
			$(INTERFACE_DIR)/lpc_syscpld_interface.o \
			$(MISC_DRV)/drv_lpc_cpld.o
obj-m += i2c-mux-pca9548.o
obj-m += adm1275.o
obj-m += mp2882.o
obj-m += clx_psu_dev.o
    clx_psu_dev-objs := clx_psu.o clx_psu_cfg.o
obj-m += tps546b24a.o
obj-m += tmp75.o
obj-m += adm1166.o
obj-m += cputemp_dev_driver.o
cputemp_dev_driver-objs := cputemp_common.o cputemp.o via-cputemp.o

DRV_DIRS := $(shell find $(CLX_DRIVER_DIR) -maxdepth 3 -type d)
DRV_OBJS = $(foreach dir,$(DRV_DIRS),$(wildcard $(dir)/*.o))
DRV_OBJS += $(foreach dir,$(INTERFACE_DIR),$(wildcard $(dir)/*.o))

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
	rm -rf $(DRV_OBJS)
