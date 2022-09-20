#!/usr/bin/env python3
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

import common, device
import os
KERNEL_MODULE = [
    'i2c_dev',
    'i2c-mux',
    'pmbus_core',
    'i2c-mux-pca9548',
    'at24',
    's3ip_sysfs',
    'clounix_sysfs_main',
    'clounix_fpga',
    'clounix_fpga_i2c',
    'clounix_platform',
    'syseeprom_dev_driver',
    'fan_dev_driver',
    'cpld_dev_driver',
    'sysled_dev_driver',
    'psu_dev_driver',
    'transceiver_dev_driver',
    'temp_sensor_dev_driver',
    'vol_sensor_dev_driver',
    'fpga_dev_driver',
    'watchdog_dev_driver',
    'curr_sensor_dev_driver',
    'adm1275',
    'gwcr-psu',
    'mp2882',
    'tps546b24a',
    'tmp75',
    'adm1166',
    'cputemp'
]

def checkDriver():
    for i in range(0, len(KERNEL_MODULE)):
        status, output = common.doBash("lsmod | grep " + KERNEL_MODULE[i])
        if status:
            status, output = common.doBash("modprobe " + KERNEL_MODULE[i])
    return

i2c_topology_dict=[
    {'bus': "i2c-0",   'driver': "clx_pca9548",   'address': "0x70"},
    {'bus': "i2c-6",   'driver': "clx_hwmon_fan", 'address': "0x60"},
    {'bus': "i2c-5",   'driver': "tmp75c",        'address': "0x4a"},
    {'bus': "i2c-5",   'driver': "tmp75c",        'address': "0x49"},
    {'bus': "i2c-5",   'driver': "tmp75c",        'address': "0x48"},
    {'bus': "i2c-6",   'driver': "tmp75c",        'address': "0x48"},
    {'bus': "i2c-6",   'driver': "tmp75c",        'address': "0x49"},
    {'bus': "i2c-13",  'driver': "24c02",         'address': "0x50"},
    {'bus': "i2c-5",   'driver': "adm1278",       'address': "0x10"},
    {'bus': "i2c-3",   'driver': "mp2882",        'address': "0x20"},
    {'bus': "i2c-3",   'driver': "mp2882",        'address': "0x21"},
    {'bus': "i2c-2",   'driver': "adm1166",       'address': "0x34"},
    {'bus': "i2c-2",   'driver': "adm1166",       'address': "0x36"},
    {'bus': "i2c-1",   'driver': "24c02",         'address': "0x50"},
    {'bus': "i2c-1",   'driver': "24c02",         'address': "0x52"},
    {'bus': "i2c-6",   'driver': "24c256",        'address': "0x51"},
    {'bus': "i2c-3",   'driver': "tps546b24a",    'address': "0x29"},
    {'bus': "i2c-1",   'driver': "gw1200d",       'address': "0x58"},
    {'bus': "i2c-1",   'driver': "gw1200d",       'address': "0x5a"}
]

def doInstall():
    status, output = common.doBash("depmod -a")
    status, output = common.doBash("rmmod coretemp")
    status, output = common.doBash("rmmod lm75")
    checkDriver()

    os.system("/usr/local/bin/s3ip_load.py create")

    for o in i2c_topology_dict:
        status, output = common.doBash("echo " + o['driver'] + " " + o['address'] + " > " + common.I2C_PREFIX + o['bus'] + "/new_device")

    return

def do_platformApiInit():
    print("Platform API Init....")
    status, output = common.doBash("/usr/local/bin/platform_api_mgnt.sh init")
    return

def do_platformApiInstall():
    print("Platform API Install....")
    status, output = common.doBash("/usr/local/bin/platform_api_mgnt.sh install")
    return

# Platform uninitialize
def doUninstall():
    for o in i2c_topology_dict:
        status, output = common.doBash("echo " + o['address'] + " > " + common.I2C_PREFIX + o['bus'] + "/delete_device")
    for i in range(len(KERNEL_MODULE)-1, -1, -1):
        status, output = common.doBash("modprobe -rq " + KERNEL_MODULE[i])
    return

def main():
    args = common.sys.argv[1:]

    if len(args[0:]) < 1:
        common.sys.exit(0)

    if args[0] == 'install':
        common.RUN = True
        doInstall()
        do_platformApiInit()
        do_platformApiInstall()

    if args[0] == 'uninstall':
        common.RUN = False
        doUninstall()
        os.system("/usr/local/bin/s3ip_load.py stop")

    common.sys.exit(0)

if __name__ == "__main__":
    main()
