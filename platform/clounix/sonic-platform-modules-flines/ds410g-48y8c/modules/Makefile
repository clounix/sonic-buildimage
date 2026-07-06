obj-m:= pddf_custom_psu.o
obj-m += pddf_custom_optoe.o
obj-m += pddf_custom_wdt.o
obj-m += pddf_custom_fan.o
obj-m += clounix_fpga_register_init.o
obj-m += clounix_fpga_sysfs.o
obj-m += pddf_custom_debug.o
obj-m += pddf_custom_reboot_eeprom.o
obj-m += pddf_custom_led.o
obj-m += pddf_custom_sysstatus.o
obj-m += pddf_custom_xcvr.o
obj-m += pddf_custom_xilinx_device_algo.o
obj-m += pddf_custom_lpc_module.o
obj-m += amifldrv.o
amifldrv-objs += amiwrap.o amishipped.o
CFLAGS_pddf_custom_debug.o = -I$(M)/../../../../pddf/i2c/modules/include
CFLAGS_pddf_custom_psu.o := -I$(M)/../../../../pddf/i2c/modules/include
CFLAGS_pddf_custom_fan.o := -I$(M)/../../../../pddf/i2c/modules/include
CFLAGS_pddf_custom_led.o := -I$(M)/../../../../pddf/i2c/modules/include
CFLAGS_pddf_custom_wdt.o := -I$(M)/../../../../pddf/i2c/modules/include
CFLAGS_pddf_custom_sysstatus.o := -I$(M)/../../../../pddf/i2c/modules/include
CFLAGS_pddf_custom_xcvr.o := -I$(M)/../../../../pddf/i2c/modules/include
CFLAGS_pddf_custom_xilinx_device_algo.o := -I$(M)/../../../../pddf/i2c/modules/include
KBUILD_EXTRA_SYMBOLS := $(M)/../../../../pddf/i2c/Module.symvers.PDDF

$(obj)/amishipped.o: $(obj)/amifldrv.o_shipped
	@echo "cmd_amishipped.o := cp $(M)/amishipped.o $(M)/amifldrv.o" > $(M)/.amishipped.o.cmd
	@echo "source_amishipped.o := $(M)/amishipped.o" >> $(M)/.amishipped.o.cmd
	@cp $< $@

clean-files += amishipped.o