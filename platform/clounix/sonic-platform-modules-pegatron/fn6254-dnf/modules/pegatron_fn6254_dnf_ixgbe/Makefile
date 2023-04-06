################################################################################
#
# Intel(R) 10GbE PCI Express Linux Network Driver
# Copyright(c) 1999 - 2017 Intel Corporation.
#
# This program is free software; you can redistribute it and/or modify it
# under the terms and conditions of the GNU General Public License,
# version 2, as published by the Free Software Foundation.
#
# This program is distributed in the hope it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
# more details.
#
# The full GNU General Public License is included in this distribution in
# the file called "COPYING".
#
# Contact Information:
# Linux NICS <linux.nics@intel.com>
# e1000-devel Mailing List <e1000-devel@lists.sourceforge.net>
# Intel Corporation, 5200 N.E. Elam Young Parkway, Hillsboro, OR 97124-6497
#
################################################################################

obj-m += pegatron_fn6254_dnf_ixgbe.o

define pegatron_fn6254_dnf_ixgbe-y
	ixgbe_main.o
	ixgbe_api.o
	ixgbe_common.o
	ixgbe_dcb.o
	ixgbe_dcb_82598.o
	ixgbe_dcb_82599.o
	ixgbe_ethtool.o
	ixgbe_lib.o
	ixgbe_mbx.o
	ixgbe_sriov.o
	ixgbe_param.o
	ixgbe_phy.o
	ixgbe_procfs.o
	ixgbe_82598.o
	ixgbe_82599.o
	ixgbe_x540.o
	ixgbe_x550.o
endef
pegatron_fn6254_dnf_ixgbe-y := $(strip ${pegatron_fn6254_dnf_ixgbe-y})

pegatron_fn6254_dnf_ixgbe-y += ixgbe_dcb_nl.o

pegatron_fn6254_dnf_ixgbe-y += ixgbe_debugfs.o

pegatron_fn6254_dnf_ixgbe-y += ixgbe_fcoe.o

pegatron_fn6254_dnf_ixgbe-y += ixgbe_ptp.o

pegatron_fn6254_dnf_ixgbe-y += ixgbe_sysfs.o

pegatron_fn6254_dnf_ixgbe-y += kcompat.o

