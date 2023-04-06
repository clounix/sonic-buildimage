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

import json
import common, device
import os
from sonic_py_common import device_info

PLATFORM_CFG_MODULE = "clounix_platform"


def checkDriver(kernel_module):
    status, output = common.doBash("lsmode | grep " + PLATFORM_CFG_MODULE)
    if status:
        common.doBash("modprobe " + PLATFORM_CFG_MODULE + " clx_platform=\""
                + str(device_info.get_platform()) + "\"")

    for mod in kernel_module:
        status, output = common.doBash("lsmod | grep " + mod)
        if status:
            status, output = common.doBash("modprobe " + mod)
    return

def platform_load_json():

    platform_name = str(device_info.get_platform())
    config_name = "sonic_platform"
    if ('arm64' not in platform_name) and ('x86_64' not in platform_name):
        status, output = common.doBash("uname -m")
        config_name = config_name + "-" + output

    prod_file = "/usr/share/sonic/device/" + platform_name + "/" + config_name + ".json"

    try:
        with open(prod_file, 'r') as jsonfile:
            json_string = json.load(jsonfile)
    except IOError as e:
        return NULL, NULL

    return json_string['i2c_topology_dict'], json_string['kernel_module_list']

def doInstall():
    status, output = common.doBash("depmod -a")
    status, output = common.doBash("rmmod coretemp")
    status, output = common.doBash("rmmod lm75")
    status, output = common.doBash("rmmod via_cputemp")
    i2c_topology_dict, kernel_module = platform_load_json()
    checkDriver(kernel_module)

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
    i2c_topology_dict, kernel_module = platform_load_json()
    for o in i2c_topology_dict:
        status, output = common.doBash("echo " + o['address'] + " > " + common.I2C_PREFIX + o['bus'] + "/delete_device")
    for mod in kernel_module:
        status, output = common.doBash("modprobe -rq " + mod)
    common.doBash("rmmod " + PLATFORM_CFG_MODULE)
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
        device.deviceInit()

    if args[0] == 'uninstall':
        common.RUN = False
        doUninstall()
        os.system("/usr/local/bin/s3ip_load.py stop")

    common.sys.exit(0)

if __name__ == "__main__":
    main()
