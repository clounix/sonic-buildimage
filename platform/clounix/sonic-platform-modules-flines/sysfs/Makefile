PWD		= $(shell pwd)
SYSFS_OUT_PUT := $(PWD)/build
sysfs_out_put_dir := $(SYSFS_OUT_PUT)/S3IP_sysfs/
export sysfs_out_put_dir
prefix ?= usr/local
SYSFS_DRIVER_DIR = $(PWD)/s3ip_sysfs_frame
SWITCH_DRIVER_DIR = $(PWD)/bsp_driver

KBUILD_EXTRA_SYMBOLS += $(SYSFS_DRIVER_DIR)/Module.symvers
KBUILD_EXTRA_SYMBOLS += $(SWITCH_DRIVER_DIR)/Module.symvers
export KBUILD_EXTRA_SYMBOLS

all :
	$(MAKE) -C $(SYSFS_DRIVER_DIR)
	$(MAKE) -C $(SWITCH_DRIVER_DIR)

clean : uninstall
	-rm  -rf $(SYSFS_OUT_PUT)
	$(MAKE) -C $(SYSFS_DRIVER_DIR) clean
	$(MAKE) -C $(SWITCH_DRIVER_DIR) clean



install: 
	install -d $(DESTDIR)/$(KERNEL_SRC)/clounix
	install -D $(sysfs_out_put_dir)/*.ko \
		$(DESTDIR)/$(KERNEL_SRC)/clounix
	install -D scripts/s3ip_load.py \
		$(DESTDIR)/$(prefix)/bin/s3ip_load.py
	install -D scripts/s3ip_sysfs_conf.json \
		$(DESTDIR)/etc/s3ip/s3ip_sysfs_conf.json
	install -D scripts/s3ip_sysfs_tool.sh \
		$(DESTDIR)/$(prefix)/bin/s3ip_sysfs_tool.sh


uninstall:
	-rm -f $(DESTDIR)$(prefix)/bin/s3ip_load.py
	-rm -f $(DESTDIR)/$(KERNEL_SRC)/clounix
	-rm -f $(DESTDIR)/etc/s3ip
	-rm -f $(DESTDIR)/$(prefix)/bin/s3ip_sysfs_tool.sh
	-rm -f $(DESTDIR)/lib/systemd/system/s3ip-sysfs.service

