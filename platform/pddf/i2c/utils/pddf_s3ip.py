#!/usr/bin/env python


"""
Usage: %(scriptName)s [options] command object

options:
    -h | --help     : this help message
    -d | --debug    : run with debug mode
    -f | --force    : ignore error during createation or clean
command:
    create     : create S3IP sysfs nodes
    clean      : clean S3IP sysfs nodes
"""

import logging
import getopt
import os
import shutil
import subprocess
import sys
from sonic_py_common import device_info
from sonic_platform_pddf_base import pddfapi


PROJECT_NAME = 'PDDF-S3IP'
verbose = False
DEBUG = False
args = []
FORCE = 0

# Instantiate the class pddf_api
try:
    pddf_api = pddfapi.PddfApi()
except Exception as e:
    print("%s" % str(e))
    sys.exit()



if DEBUG == True:
    print(sys.argv[0])
    print('ARGV: ', sys.argv[1:])

def main():
    global DEBUG
    global args
    global FORCE

    if len(sys.argv)<2:
        show_help()

    options, args = getopt.getopt(sys.argv[1:], 'hdf', ['help',
                                                       'debug',
                                                       'force',
                                                          ])
    if DEBUG == True:
        print(options)
        print(args)
        print(len(sys.argv))

    # Check for the JSON field 'enable_s3ip'
    if 'enable_s3ip' in pddf_api.data['PLATFORM'].keys():
        if pddf_api.data['PLATFORM']['enable_s3ip'] != 'yes':
            print("S3IP SysFS creation is not enabled for this platform. Exiting ..")
            sys.exit()
    else:
        print("S3IP SysFS is not supported on this platform. Exiting ..")
        sys.exit()


    for opt, arg in options:
        if opt in ('-h', '--help'):
            show_help()
        elif opt in ('-d', '--debug'):
            DEBUG = True
            logging.basicConfig(level=logging.INFO)
        elif opt in ('-f', '--force'):
            FORCE = 1
        else:
            logging.info('no option')
    for arg in args:
        if arg == 'create':
            do_create()
        elif arg == 'clean':
           do_clean()
        else:
            show_help()
    return 0


def show_help():
    print(__doc__ % {'scriptName' : sys.argv[0].split("/")[-1]})
    sys.exit(0)

def my_log(txt):
    if DEBUG == True:
        print("[PDDF-S3IP]"+txt)
    return

def log_os_system(cmd, show):
    logging.info('Run :'+cmd)
    status, output = subprocess.getstatusoutput(cmd)
    my_log (cmd +"with result:" + str(status))
    my_log ("      output:"+output)
    if status:
        logging.info('Failed :'+cmd)
        if show:
            print('Failed :'+cmd)
    return  status, output

def create_s3ip_temp_sysfs():
    print("Creating temperature sensors sysfs ..")

    log_os_system('sudo mkdir -p -m 777 /sys_switch/temp_sensor', 1)
    num_temps = pddf_api.data['PLATFORM']['num_temps'] if 'num_temps' in pddf_api.data['PLATFORM'] else 0
    cmd = 'sudo echo "{}" > /sys_switch/temp_sensor/number'.format(num_temps)
    log_os_system(cmd, 1)

    for t in range(1, num_temps+1):
        dev_name = 'TEMP{}'.format(t)
        dev = pddf_api.data[dev_name]
        cmd = 'sudo mkdir -p -m 777 /sys_switch/temp_sensor/temp{}'.format(t)
        log_os_system(cmd, 1)

        # alias i.e. display name
        try:
            if dev['dev_attr']['display_name']:
                cmd = 'sudo echo "{}" > /sys_switch/temp_sensor/temp{}/alias'.format(dev['dev_attr']['display_name'], t)
        except Exception as err:
            cmd = 'sudo echo "{}" > /sys_switch/temp_sensor/temp{}/alias'.format('NA', t)

        log_os_system(cmd, 1)

        # type i.e. dev_type
        try:
            if 'virt_parent' in dev['dev_info'].keys():
                virt_p_name = dev['dev_info']['virt_parent']
                i2c_dev = pddf_api.data[virt_p_name]
            else:
                i2c_dev = dev

            if 'i2c' in i2c_dev.keys() and 'topo_info' in i2c_dev['i2c'].keys() and \
                    'dev_type' in i2c_dev['i2c']['topo_info'].keys():
                cmd = 'sudo echo "{}" > /sys_switch/temp_sensor/temp{}/type'.format(i2c_dev['i2c']['topo_info']['dev_type'], t)
            else:
                # BMC based TEMP sensors or TEMP sensors with hwmon path e.g. pch or cpu temps
                cmd = 'sudo echo "{}" > /sys_switch/temp_sensor/temp{}/type'.format('NA', t)
        except Exception as err:
            cmd = 'sudo echo "{}" > /sys_switch/temp_sensor/temp{}/type'.format('NA', t)

        log_os_system(cmd, 1)

        # max i.e. Alarm high threshold in mill degree celsius
        try:
            max_val = 'NA'
            bmc_attr = pddf_api.check_bmc_based_attr(dev_name, 'temp1_high_crit_threshold')
            if bmc_attr is not None and bmc_attr!={}:
                output = pddf_api.bmc_get_cmd(bmc_attr)
                if output.replace('.','',1).isdigit():
                    min_val = float(output['status'])*1000
                cmd = 'sudo echo "{}" > /sys_switch/temp_sensor/temp{}/max'.format(max_val, t)
            else:
                # I2C based attribute
                output = pddf_api.get_attr_name_output(dev_name, 'temp1_high_crit_threshold')
                if output:
                    cmd = 'sudo echo "{}" > /sys_switch/temp_sensor/temp{}/max'.format(output['status'], t)
                else:
                    node = pddf_api.get_path(dev_name, 'temp1_high_crit_threshold')
                    if node:
                        cmd = 'sudo ln -s {} /sys_switch/temp_sensor/temp{}/max'.format(node, t)
                    else:
                        cmd = 'sudo echo "{}" > /sys_switch/temp_sensor/temp{}/max'.format(max_val, t)
        except Exception as err:
            cmd = 'sudo echo "{}" > /sys_switch/temp_sensor/temp{}/max'.format(max_val, t)

        log_os_system(cmd, 1)

        # min i.e Alarm recovery threhsold in milli degree celsius
        try:
            min_val = 'NA'
            bmc_attr = pddf_api.check_bmc_based_attr(dev_name, 'temp1_high_threshold')
            if bmc_attr is not None and bmc_attr!={}:
                output = pddf_api.bmc_get_cmd(bmc_attr)
                if output.replace('.','',1).isdigit():
                    min_val = float(output['status'])*1000
                cmd = 'sudo echo "{}" > /sys_switch/temp_sensor/temp{}/min'.format(min_val, t)
            else:
                # I2C based attribute
                output = pddf_api.get_attr_name_output(dev_name, 'temp1_high_threshold')
                if output:
                    cmd = 'sudo echo "{}" > /sys_switch/temp_sensor/temp{}/min'.format(output['status'], t)
                else:
                    node = pddf_api.get_path(dev_name, 'temp1_high_threshold')
                    if node:
                        cmd = 'sudo ln -s {} /sys_switch/temp_sensor/temp{}/min'.format(node, t)
                    else:
                        cmd = 'sudo echo "{}" > /sys_switch/temp_sensor/temp{}/min'.format(min_val, t)
        except Exception as err:
            cmd = 'sudo echo "{}" > /sys_switch/temp_sensor/temp{}/min'.format(min_val, t)

        log_os_system(cmd, 1)

        # value i.e. current temperature in milli degree celsius
        try:
            val = 'NA'
            bmc_attr = pddf_api.check_bmc_based_attr(dev_name, 'temp1_input')
            if bmc_attr is not None and bmc_attr!={}:
                output = pddf_api.bmc_get_cmd(bmc_attr)
                if output.replace('.','',1).isdigit():
                    val = float(output['status'])*1000
                cmd = 'sudo echo "{}" > /sys_switch/temp_sensor/temp{}/value'.format(val, t)
            else:
                # I2C based attribute
                node = pddf_api.get_path(dev_name, 'temp1_input')
                if node:
                    cmd = 'sudo ln -s {} /sys_switch/temp_sensor/temp{}/value'.format(node, t)
                else:
                    cmd = 'sudo echo "{}" > /sys_switch/temp_sensor/temp{}/value'.format(val, t)
        except Exception as err:
            cmd = 'sudo echo "{}" > /sys_switch/temp_sensor/temp{}/value'.format(val, t)

        log_os_system(cmd, 1)

    print("Completed temperature sensors sysfs creation")


def create_s3ip_volt_sysfs():
    print("Creating voltage sensors sysfs ..")

    log_os_system('sudo mkdir -p -m 777 /sys_switch/vol_sensor', 1)
    num_voltage_sensors = pddf_api.data['PLATFORM']['num_voltage_sensors'] if 'num_voltage_sensors' in pddf_api.data['PLATFORM'] else 0
    cmd = 'sudo echo "{}" > /sys_switch/vol_sensor/number'.format(num_voltage_sensors)
    log_os_system(cmd, 1)

    for volt_idx in range(1, num_voltage_sensors + 1):
        cmd = 'sudo mkdir -p -m 777 /sys_switch/vol_sensor/vol{}'.format(volt_idx)
        log_os_system(cmd, 1)

        dev_name = 'VOLTAGE{}'.format(volt_idx)
        dev = pddf_api.data[dev_name]

        # alias
        try:
            alias = 'N/A'
            if dev['dev_attr']['display_name']:
                alias = dev['dev_attr']['display_name']

            cmd = 'sudo echo "{}" > /sys_switch/vol_sensor/vol{}/alias'.format(alias, volt_idx)
        except Exception as err:
            cmd = 'sudo echo "{}" > /sys_switch/vol_sensor/vol{}/alias'.format('NA', volt_idx)

        log_os_system(cmd, 1)

        # type
        try:
            if 'virt_parent' in dev['dev_info']:
                virt_p_name = dev['dev_info']['virt_parent']
                i2c_dev = pddf_api.data[virt_p_name]
            else:
                raise Exception

            dev_type = 'N/A'
            if 'i2c' in i2c_dev.keys() and 'topo_info' in i2c_dev['i2c'].keys() and \
                    'dev_type' in i2c_dev['i2c']['topo_info'].keys():
                dev_type = i2c_dev['i2c']['topo_info']['dev_type']
            cmd = 'sudo echo "{}" > /sys_switch/vol_sensor/vol{}/type'.format(dev_type, volt_idx)
        except Exception as err:
            cmd = 'sudo echo "{}" > /sys_switch/vol_sensor/vol{}/type'.format('NA', volt_idx)

        log_os_system(cmd, 1)

        # max
        try:
            max_val = 'NA'
            bmc_attr = pddf_api.check_bmc_based_attr(dev_name, 'volt1_crit_high_threshold')
            if bmc_attr is not None and bmc_attr != {}:
                output = pddf_api.bmc_get_cmd(bmc_attr)
                if output.replace('.','',1).isdigit():
                    max_val = float(output['status'])*1000
                cmd = 'sudo echo "{}" > /sys_switch/vol_sensor/vol{}/max'.format(max_val, volt_idx)
            else:
                # I2C based attribute
                output = pddf_api.get_attr_name_output(dev_name, 'volt1_crit_high_threshold')
                if output:
                    cmd = 'sudo echo "{}" > /sys_switch/vol_sensor/vol{}/max'.format(output['status'], volt_idx)
                else:
                    node = pddf_api.get_path(dev_name, 'volt1_crit_high_threshold')
                    if node:
                        cmd = 'sudo ln -s {} /sys_switch/vol_sensor/vol{}/max'.format(node, volt_idx)
                    else:
                        cmd = 'sudo echo "{}" > /sys_switch/vol_sensor/vol{}/max'.format(max_val, volt_idx)
        except Exception as err:
            cmd = 'sudo echo "{}" > /sys_switch/vol_sensor/vol{}/max'.format('N/A', volt_idx)

        log_os_system(cmd, 1)

        # min
        try:
            min_val = 'NA'
            bmc_attr = pddf_api.check_bmc_based_attr(dev_name, 'volt1_high_threshold')
            if bmc_attr is not None and bmc_attr != {}:
                output = pddf_api.bmc_get_cmd(bmc_attr)
                if output.replace('.','',1).isdigit():
                    min_val = float(output['status'])*1000
                cmd = 'sudo echo "{}" > /sys_switch/vol_sensor/vol{}/min'.format(min_val, volt_idx)
            else:
                # I2C based attribute
                output = pddf_api.get_attr_name_output(dev_name, 'volt1_high_threshold')
                if output:
                    cmd = 'sudo echo "{}" > /sys_switch/vol_sensor/vol{}/min'.format(output['status'], volt_idx)
                else:
                    node = pddf_api.get_path(dev_name, 'volt1_high_threshold')
                    if node:
                        cmd = 'sudo ln -s {} /sys_switch/vol_sensor/vol{}/min'.format(node, volt_idx)
                    else:
                        cmd = 'sudo echo "{}" > /sys_switch/vol_sensor/vol{}/min'.format(min_val, volt_idx)
        except Exception as err:
            cmd = 'sudo echo "{}" > /sys_switch/vol_sensor/vol{}/min'.format('N/A', volt_idx)

        log_os_system(cmd, 1)

        # value
        try:
            bmc_attr = pddf_api.check_bmc_based_attr(dev_name, 'volt1_input')
            if bmc_attr is not None and bmc_attr != {}:
                output = pddf_api.bmc_get_cmd(bmc_attr)
                val = 'NA'
                if output.replace('.','',1).isdigit():
                    val = float(output['status'])*1000
                cmd = 'sudo echo "{}" > /sys_switch/vol_sensor/vol{}/value'.format(val, volt_idx)
            else:
                # I2C based attribute
                node = pddf_api.get_path(dev_name, 'volt1_input')
                if node:
                    cmd = 'sudo ln -s {} /sys_switch/vol_sensor/vol{}/value'.format(node, volt_idx)
                else:
                    cmd = 'sudo echo "{}" > /sys_switch/vol_sensor/vol{}/value'.format('N/A', volt_idx)
        except Exception as err:
            cmd = 'sudo echo "{}" > /sys_switch/vol_sensor/vol{}/value'.format('N/A', volt_idx)

        log_os_system(cmd, 1)

        # range
        try:
            range_val = 'N/A'
            if dev['dev_attr']['range']:
                range_val = dev['dev_attr']['range']

            cmd = 'sudo echo "{}" > /sys_switch/vol_sensor/vol{}/range'.format(range_val, volt_idx)
        except Exception as err:
            cmd = 'sudo echo "{}" > /sys_switch/vol_sensor/vol{}/range'.format('N/A', volt_idx)

        log_os_system(cmd, 1)

        # nominal_value
        try:
            nominal_val = 'N/A'
            if dev['dev_attr']['nominal']:
                nominal_val = dev['dev_attr']['nominal']

            cmd = 'sudo echo "{}" > /sys_switch/vol_sensor/vol{}/nominal_value'.format(nominal_val, volt_idx)
        except Exception as err:
            cmd = 'sudo echo "{}" > /sys_switch/vol_sensor/vol{}/nominal_value'.format('N/A', volt_idx)

        log_os_system(cmd, 1)

    print("Completed VOLTAGE sysfs creation")

def create_s3ip_curr_sysfs():
    print("Creating current sensors sysfs ..")

    log_os_system('sudo mkdir -p -m 777 /sys_switch/curr_sensor', 1)
    num_current_sensors = pddf_api.data['PLATFORM']['num_current_sensors'] if 'num_current_sensors' in pddf_api.data['PLATFORM'] else 0
    cmd = 'sudo echo "{}" > /sys_switch/curr_sensor/number'.format(num_current_sensors)
    log_os_system(cmd, 1)

    for curr_idx in range(1, num_current_sensors + 1):
        cmd = 'sudo mkdir -p -m 777 /sys_switch/curr_sensor/curr{}'.format(curr_idx)
        log_os_system(cmd, 1)

        dev_name = 'CURRENT{}'.format(curr_idx)
        dev = pddf_api.data[dev_name]

        # alias
        try:
            alias = 'N/A'
            if dev['dev_attr']['display_name']:
                alias = dev['dev_attr']['display_name']

            cmd = 'sudo echo "{}" > /sys_switch/curr_sensor/curr{}/alias'.format(alias, curr_idx)
        except Exception as err:
            cmd = 'sudo echo "{}" > /sys_switch/curr_sensor/curr{}/alias'.format('NA', curr_idx)

        log_os_system(cmd, 1)

        # type
        try:
            if 'virt_parent' in dev['dev_info']:
                virt_p_name = dev['dev_info']['virt_parent']
                i2c_dev = pddf_api.data[virt_p_name]
            else:
                raise Exception

            dev_type = 'N/A'
            if 'i2c' in i2c_dev.keys() and 'topo_info' in i2c_dev['i2c'].keys() and \
                    'dev_type' in i2c_dev['i2c']['topo_info'].keys():
                dev_type = i2c_dev['i2c']['topo_info']['dev_type']
            cmd = 'sudo echo "{}" > /sys_switch/curr_sensor/curr{}/type'.format(dev_type, curr_idx)
        except Exception as err:
            cmd = 'sudo echo "{}" > /sys_switch/curr_sensor/curr{}/type'.format('NA', curr_idx)

        log_os_system(cmd, 1)

        # max
        try:
            max_val = 'NA'
            bmc_attr = pddf_api.check_bmc_based_attr(dev_name, 'current1_crit_high_threshold')
            if bmc_attr is not None and bmc_attr != {}:
                output = pddf_api.bmc_get_cmd(bmc_attr)
                if output.replace('.','',1).isdigit():
                    max_val = float(output['status'])*1000
                cmd = 'sudo echo "{}" > /sys_switch/curr_sensor/curr{}/max'.format(max_val, curr_idx)
            else:
                # I2C based attribute
                output = pddf_api.get_attr_name_output(dev_name, 'current1_crit_high_threshold')
                if output:
                    cmd = 'sudo echo "{}" > /sys_switch/curr_sensor/curr{}/max'.format(output['status'], curr_idx)
                else:
                    node = pddf_api.get_path(dev_name, 'current1_crit_high_threshold')
                    if node:
                        cmd = 'sudo ln -s {} /sys_switch/curr_sensor/curr{}/max'.format(node, curr_idx)
                    else:
                        cmd = 'sudo echo "{}" > /sys_switch/curr_sensor/curr{}/max'.format(max_val, curr_idx)
        except Exception as err:
            cmd = 'sudo echo "{}" > /sys_switch/curr_sensor/curr{}/max'.format('N/A', curr_idx)

        log_os_system(cmd, 1)

        # min
        try:
            min_val = 'NA'
            bmc_attr = pddf_api.check_bmc_based_attr(dev_name, 'current1_high_threshold')
            if bmc_attr is not None and bmc_attr != {}:
                output = pddf_api.bmc_get_cmd(bmc_attr)
                if output.replace('.','',1).isdigit():
                    min_val = float(output['status'])*1000
                cmd = 'sudo echo "{}" > /sys_switch/curr_sensor/curr{}/min'.format(min_val, curr_idx)
            else:
                # I2C based attribute
                output = pddf_api.get_attr_name_output(dev_name, 'current1_high_threshold')
                if output:
                    cmd = 'sudo echo "{}" > /sys_switch/curr_sensor/curr{}/min'.format(output['status'], curr_idx)
                else:
                    node = pddf_api.get_path(dev_name, 'current1_high_threshold')
                    if node:
                        cmd = 'sudo ln -s {} /sys_switch/curr_sensor/curr{}/min'.format(node, curr_idx)
                    else:
                        cmd = 'sudo echo "{}" > /sys_switch/curr_sensor/curr{}/min'.format(min_val, curr_idx)
        except Exception as err:
            cmd = 'sudo echo "{}" > /sys_switch/curr_sensor/curr{}/min'.format('N/A', curr_idx)

        log_os_system(cmd, 1)

        # value
        try:
            bmc_attr = pddf_api.check_bmc_based_attr(dev_name, 'current1_input')
            if bmc_attr is not None and bmc_attr != {}:
                output = pddf_api.bmc_get_cmd(bmc_attr)
                val = 'NA'
                if output.replace('.','',1).isdigit():
                    val = float(output['status'])*1000
                cmd = 'sudo echo "{}" > /sys_switch/curr_sensor/curr{}/value'.format(val, curr_idx)
            else:
                # I2C based attribute
                node = pddf_api.get_path(dev_name, 'current1_input')
                if node:
                    cmd = 'sudo ln -s {} /sys_switch/curr_sensor/curr{}/value'.format(node, curr_idx)
                else:
                    cmd = 'sudo echo "{}" > /sys_switch/curr_sensor/curr{}/value'.format('N/A', curr_idx)
        except Exception as err:
            cmd = 'sudo echo "{}" > /sys_switch/curr_sensor/curr{}/value'.format('N/A', curr_idx)

        log_os_system(cmd, 1)

    print("Completed CURRENT sysfs creation")

def create_s3ip_syseeprom_sysfs():
    print("Creating the SysEEPROM sysfs ..")
    syseeprom_node = pddf_api.get_path("EEPROM1", "eeprom")
    cmd = 'sudo ln -s {} /sys_switch/syseeprom'.format(syseeprom_node)
    log_os_system(cmd, 1)
    print("Completed SysEEPROM sysfs creation")

def create_s3ip_fan_sysfs():
    print("Creating the System Fan sysfs ..")
    log_os_system('sudo mkdir -p -m 777 /sys_switch/fan', 1)
    num_fantrays = pddf_api.data['PLATFORM']['num_fantrays'] if 'num_fantrays' in pddf_api.data['PLATFORM'] else 0
    num_fans_pertray = pddf_api.data['PLATFORM']['num_fans_pertray'] if 'num_fans_pertray' in pddf_api.data['PLATFORM'] else 1
    cmd = 'sudo echo "{}" > /sys_switch/fan/number'.format(num_fantrays)
    log_os_system(cmd, 1)

    #fan_eepromwp
    fan_eepromwp = "NA"
    attr = "fan_eepromwp"
    node = pddf_api.get_path('FAN-CTRL', attr)
    if node:
        cmd = 'sudo ln -s {} /sys_switch/fan/eepromwp'.format(node)
    else:
        cmd = 'sudo echo "{}" > /sys_switch/fan/eepromwp'.format(fan_eepromwp)
    log_os_system(cmd, 1)

    for f in range(1, num_fantrays+1):
        cmd = 'sudo mkdir -p -m 777 /sys_switch/fan/fan{}'.format(f)
        log_os_system(cmd, 1)

        # model_name of the fan
        model = 'NA'
        cmd = 'sudo echo "{}" > /sys_switch/fan/fan{}/model_name'.format(model, f)
        log_os_system(cmd, 1)

        # serial_num of the fan
        try:
            val = 'NA'
            attr = 'fan{}_sn'.format(f)
            bmc_attr = pddf_api.check_bmc_based_attr('FAN-CTRL', attr)
            if bmc_attr is not None and bmc_attr!={}:
                output = pddf_api.bmc_get_cmd(bmc_attr)
                val = output.rstrip()
                cmd = 'sudo echo "{}" > /sys_switch/fan/fan{}/serial_number'.format(val, f)
            else:
                # I2C based attribute
                node = pddf_api.get_path('FAN-CTRL', attr)
                if node:
                    cmd = 'sudo ln -s {} /sys_switch/fan/fan{}/serial_number'.format(node, f)
                else:
                    cmd = 'sudo echo "{}" > /sys_switch/fan/fan{}/serial_number'.format(val, f)
        except Exception as err:
            cmd = 'sudo echo "{}" > /sys_switch/fan/fan{}/serial_number'.format(val, f)

        log_os_system(cmd, 1)

        # part_number of the fan
        try:
            val = 'NA'
            attr = 'fan{}_pn'.format(f)
            bmc_attr = pddf_api.check_bmc_based_attr('FAN-CTRL', attr)
            if bmc_attr is not None and bmc_attr!={}:
                output = pddf_api.bmc_get_cmd(bmc_attr)
                val = output.rstrip()
                cmd = 'sudo echo "{}" > /sys_switch/fan/fan{}/part_number'.format(val, f)
            else:
                # I2C based attribute
                node = pddf_api.get_path('FAN-CTRL', attr)
                if node:
                    cmd = 'sudo ln -s {} /sys_switch/fan/fan{}/part_number'.format(node, f)
                else:
                    cmd = 'sudo echo "{}" > /sys_switch/fan/fan{}/part_number'.format(val, f)
        except Exception as err:
            cmd = 'sudo echo "{}" > /sys_switch/fan/fan{}/part_number'.format(val, f)

        log_os_system(cmd, 1)

        # hardware_vesion
        try:
            val = 'NA'
            attr = 'fan_hw_version'
            bmc_attr = pddf_api.check_bmc_based_attr('FAN-CTRL', attr)
            if bmc_attr is not None and bmc_attr!={}:
                output = pddf_api.bmc_get_cmd(bmc_attr)
                val = output.rstrip()
                cmd = 'sudo echo "{}" > /sys_switch/fan/fan{}/hardware_version'.format(val, f)
            else:
                # I2C based attribute
                node = pddf_api.get_path('FAN-CTRL', attr)
                if node:
                    cmd = 'sudo ln -s {} /sys_switch/fan/fan{}/hardware_version'.format(node, f)
                else:
                    cmd = 'sudo echo "{}" > /sys_switch/fan/fan{}/hardware_version'.format(val, f)
        except Exception as err:
            cmd = 'sudo echo "{}" > /sys_switch/fan/fan{}/hardware_version'.format(val, f)

        log_os_system(cmd, 1)

        mot_num = num_fans_pertray
        cmd = 'sudo echo "{}" > /sys_switch/fan/fan{}/motor_number'.format(mot_num, f)
        log_os_system(cmd, 1)

        # direction
        try:
            val = 'NA'
            attr = 'fan{}_direction'.format((f-1)*mot_num + 1)
            bmc_attr = pddf_api.check_bmc_based_attr('FAN-CTRL', attr)
            if bmc_attr is not None and bmc_attr!={}:
                output = pddf_api.bmc_get_cmd(bmc_attr)
                val = output.rstrip()
                cmd = 'sudo echo "{}" > /sys_switch/fan/fan{}/direction'.format(val, f)
            else:
                # I2C based attribute
                node = pddf_api.get_path('FAN-CTRL', attr)
                if node:
                    cmd = 'sudo ln -s {} /sys_switch/fan/fan{}/direction'.format(node, f)
                else:
                    cmd = 'sudo echo "{}" > /sys_switch/fan/fan{}/direction'.format(val, f)
        except Exception as err:
            cmd = 'sudo echo "{}" > /sys_switch/fan/fan{}/direction'.format(val, f)

        log_os_system(cmd, 1)

        # ratio i.e. duty cycle
        try:
            dc = 'NA'
            # attr = 'fan_duty_cycle'
            attr = 'fan{}_pwm'.format((f-1)*mot_num + 1)
            bmc_attr = pddf_api.check_bmc_based_attr('FAN-CTRL', attr)
            if bmc_attr is not None and bmc_attr!={}:
                output = pddf_api.bmc_get_cmd(bmc_attr)
                dc = output.rstrip()
                cmd = 'sudo echo "{}" > /sys_switch/fan/fan{}/ratio'.format(dc, f)
            else:
                # I2C based attribute
                node = pddf_api.get_path('FAN-CTRL', attr)
                if node:
                    cmd = 'sudo ln -s {} /sys_switch/fan/fan{}/ratio'.format(node, f)
                else:
                    cmd = 'sudo echo "{}" > /sys_switch/fan/fan{}/ratio'.format(dc, f)
        except Exception as err:
            cmd = 'sudo echo "{}" > /sys_switch/fan/fan{}/ratio'.format(dc, f)

        log_os_system(cmd, 1)

        # status
        try:
            status = 'NA'
            attr1 = 'fan{}_present'.format((f-1)*mot_num+1)
            attr2 = 'fan{}_input'.format((f-1)*mot_num+1)
            bmc_attr1 = pddf_api.check_bmc_based_attr('FAN-CTRL', attr1)
            bmc_attr2 = pddf_api.check_bmc_based_attr('FAN-CTRL', attr2)

            if not bmc_attr1 and not bmc_attr2:
                # Both present and speed are I2C based, so status attribute would be created by default
                node = pddf_api.get_path('FAN-CTRL', attr1)
                if node:
                    new_node = node.replace('present','status')
                    cmd = 'sudo ln -s {} /sys_switch/fan/fan{}/status'.format(new_node, f)
            else:
                cmd = 'sudo echo "{}" > /sys_switch/fan/fan{}/status'.format(status, f)
        except Exception as err:
            cmd = 'sudo echo "{}" > /sys_switch/fan/fan{}/status'.format(status, f)

        log_os_system(cmd, 1)

        # led_status
        result, output = pddf_api.get_system_led_color("FANTRAY{}_LED".format(f))
        if result:
            node = "/sys/kernel/{}/fantray{}_led".format(pddf_api.get_led_cur_state_path(), f)
            cmd = 'sudo ln -s {} /sys_switch/fan/fan{}/led_status'.format(node, f)
        else:
            led_status = 'NA'
            cmd = 'sudo echo "{}" > /sys_switch/fan/fan{}/led_status'.format(led_status, f)
        log_os_system(cmd, 1)

        # motor related sysfs
        for m in range(1, mot_num+1):
            cmd = 'sudo mkdir -p -m 777 /sys_switch/fan/fan{}/motor{}'.format(f,m)
            log_os_system(cmd, 1)

            # motor speed
            try:
                speed = 'NA'
                attr = 'fan{}_input'.format((f-1)*mot_num + m)
                bmc_attr = pddf_api.check_bmc_based_attr('FAN-CTRL', attr)
                if bmc_attr is not None and bmc_attr!={}:
                    output = pddf_api.bmc_get_cmd(bmc_attr)
                    output = output.rstrip()
                    if output.replace('.','',1).isdigit():
                        speed = int(float(output))
                    cmd = 'sudo echo "{}" > /sys_switch/fan/fan{}/motor{}/speed'.format(speed, f, m)
                else:
                    # I2C based attribute
                    node = pddf_api.get_path('FAN-CTRL', attr)
                    if node:
                        cmd = 'sudo ln -s {} /sys_switch/fan/fan{}/motor{}/speed'.format(node, f, m)
                    else:
                        cmd = 'sudo echo "{}" > /sys_switch/fan/fan{}/motor{}/speed'.format(speed, f, m)
            except Exception as err:
                cmd = 'sudo echo "{}" > /sys_switch/fan/fan{}/motor{}/speed'.format(speed, f, m)

            log_os_system(cmd, 1)

            attr = 'fan{}'.format((f-1)*mot_num + m)
            # motor speed max
            try:
               speed_max=pddf_api.data['FAN-CTRL']['speed_max'][attr]
            except Exception:
                speed_max = "N/A"
            cmd = 'sudo echo "{}" > /sys_switch/fan/fan{}/motor{}/speed_max'.format(speed_max, f, m)

            log_os_system(cmd, 1)

            # motor speed min
            try:
               speed_min=pddf_api.data['FAN-CTRL']['speed_min'][attr]
            except Exception:
                speed_min = "N/A"
            cmd = 'sudo echo "{}" > /sys_switch/fan/fan{}/motor{}/speed_min'.format(speed_min, f, m)

            log_os_system(cmd, 1)

            # motor speed tolerance
            try:
                speed_tolerance = 'NA'
                attr = 'fan{}_speed_tolerance'.format((f-1)*mot_num + m)
                bmc_attr = pddf_api.check_bmc_based_attr('FAN-CTRL', attr)
                if bmc_attr is not None and bmc_attr!={}:
                    output = pddf_api.bmc_get_cmd(bmc_attr)
                    output = output.rstrip()
                    if output.replace('.','',1).isdigit():
                        speed_tolerance = int(float(output))
                    cmd = 'sudo echo "{}" > /sys_switch/fan/fan{}/motor{}/speed_tolerance'.format(speed_tolerance, f, m)
                else:
                    # I2C based attribute
                    node = pddf_api.get_path('FAN-CTRL', attr)
                    if node:
                        cmd = 'sudo ln -s {} /sys_switch/fan/fan{}/motor{}/speed_tolerance'.format(node, f, m)
                    else:
                        cmd = 'sudo echo "{}" > /sys_switch/fan/fan{}/motor{}/speed_tolerance'.format(speed_tolerance, f, m)
            except Exception as err:
                cmd = 'sudo echo "{}" > /sys_switch/fan/fan{}/motor{}/speed_tolerance'.format(speed_tolerance, f, m)

            log_os_system(cmd, 1)

            # motor speed target
            try:
                speed_target = 'NA'
                attr = 'fan{}_speed_target'.format((f-1)*mot_num + m)
                bmc_attr = pddf_api.check_bmc_based_attr('FAN-CTRL', attr)
                if bmc_attr is not None and bmc_attr!={}:
                    output = pddf_api.bmc_get_cmd(bmc_attr)
                    output = output.rstrip()
                    if output.replace('.','',1).isdigit():
                        speed_target = int(float(output))
                    cmd = 'sudo echo "{}" > /sys_switch/fan/fan{}/motor{}/speed_target'.format(speed, f, m)
                else:
                    # I2C based attribute
                    node = pddf_api.get_path('FAN-CTRL', attr)
                    if node:
                        cmd = 'sudo ln -s {} /sys_switch/fan/fan{}/motor{}/speed_target'.format(node, f, m)
                    else:
                        cmd = 'sudo echo "{}" > /sys_switch/fan/fan{}/motor{}/speed_target'.format(speed_target, f, m)
            except Exception as err:
                cmd = 'sudo echo "{}" > /sys_switch/fan/fan{}/motor{}/speed_target'.format(speed_target, f, m)

            log_os_system(cmd, 1)

    print("Completed System Fan sysfs creation")

def create_s3ip_psu_sysfs():
    print("Creating the System PSU sysfs ..")
    log_os_system('sudo mkdir -p -m 777 /sys_switch/psu', 1)
    num_psus = pddf_api.data['PLATFORM']['num_psus'] if 'num_psus' in pddf_api.data['PLATFORM'] else 0
    cmd = 'sudo echo "{}" > /sys_switch/psu/number'.format(num_psus)
    log_os_system(cmd, 1)

    for p in range(1, num_psus+1):
        cmd = 'sudo mkdir -p -m 777 /sys_switch/psu/psu{}'.format(p)
        log_os_system(cmd, 1)

        # alarm
        try:
            alarm = 'NA'
            dev = 'PSU{}'.format(p)
            attr = 'psu_alarm'
            bmc_attr = pddf_api.check_bmc_based_attr(dev, attr)
            if bmc_attr is not None and bmc_attr!={}:
                output = pddf_api.bmc_get_cmd(bmc_attr)
                output = output.rstrip()
                alarm = ''.join(c for c in output if 0 < ord(c) < 127)
                cmd = 'sudo echo "{}" > /sys_switch/psu/psu{}/alarm'.format(alarm, p)
            else:
                # I2C based attribute
                node = pddf_api.get_path(dev, attr)
                if node:
                    cmd = 'sudo ln -s {} /sys_switch/psu/psu{}/alarm'.format(node, p)
                else:
                    cmd = 'sudo echo "{}" > /sys_switch/psu/psu{}/alarm'.format(alarm, p)
        except Exception as err:
            cmd = 'sudo echo "{}" > /sys_switch/psu/psu{}/alarm'.format(alarm, p)

        log_os_system(cmd, 1)

        # vendor
        try:
            vendor = 'NA'
            dev = 'PSU{}'.format(p)
            attr = 'psu_mfr_id'
            bmc_attr = pddf_api.check_bmc_based_attr(dev, attr)
            if bmc_attr is not None and bmc_attr!={}:
                output = pddf_api.bmc_get_cmd(bmc_attr)
                output = output.rstrip()
                vendor = ''.join(c for c in output if 0 < ord(c) < 127)
                cmd = 'sudo echo "{}" > /sys_switch/psu/psu{}/vendor'.format(vendor, p)
            else:
                # I2C based attribute
                node = pddf_api.get_path(dev, attr)
                if node:
                    cmd = 'sudo ln -s {} /sys_switch/psu/psu{}/vendor'.format(node, p)
                else:
                    cmd = 'sudo echo "{}" > /sys_switch/psu/psu{}/vendor'.format(vendor, p)
        except Exception as err:
            cmd = 'sudo echo "{}" > /sys_switch/psu/psu{}/vendor'.format(vendor, p)

        log_os_system(cmd, 1)

        # model name
        try:
            model = 'NA'
            dev = 'PSU{}'.format(p)
            attr = 'psu_model_name'
            bmc_attr = pddf_api.check_bmc_based_attr(dev, attr)
            if bmc_attr is not None and bmc_attr!={}:
                output = pddf_api.bmc_get_cmd(bmc_attr)
                output = output.rstrip()
                model = ''.join(c for c in output if 0 < ord(c) < 127)
                cmd = 'sudo echo "{}" > /sys_switch/psu/psu{}/model_name'.format(model, p)
            else:
                # I2C based attribute
                node = pddf_api.get_path(dev, attr)
                if node:
                    cmd = 'sudo ln -s {} /sys_switch/psu/psu{}/model_name'.format(node, p)
                else:
                    cmd = 'sudo echo "{}" > /sys_switch/psu/psu{}/model_name'.format(model, p)
        except Exception as err:
            cmd = 'sudo echo "{}" > /sys_switch/psu/psu{}/model_name'.format(model, p)

        log_os_system(cmd, 1)

        # serial_number
        try:
            serial = 'NA'
            dev = 'PSU{}'.format(p)
            attr = 'psu_serial_num'
            bmc_attr = pddf_api.check_bmc_based_attr(dev, attr)
            if bmc_attr is not None and bmc_attr!={}:
                output = pddf_api.bmc_get_cmd(bmc_attr)
                output = output.rstrip()
                serial = ''.join(c for c in output if 0 < ord(c) < 127)
                cmd = 'sudo echo "{}" > /sys_switch/psu/psu{}/serial_number'.format(serial, p)
            else:
                # I2C based attribute
                node = pddf_api.get_path(dev, attr)
                if node:
                    cmd = 'sudo ln -s {} /sys_switch/psu/psu{}/serial_number'.format(node, p)
                else:
                    cmd = 'sudo echo "{}" > /sys_switch/psu/psu{}/serial_number'.format(serial, p)
        except Exception as err:
            cmd = 'sudo echo "{}" > /sys_switch/psu/psu{}/serial_number'.format(serial, p)

        log_os_system(cmd, 1)

        # part_number is the same as model name(refer to `fruidutil get psu`)
        try:
            part = 'NA'
            dev = 'PSU{}'.format(p)
            attr = 'psu_model_name'
            bmc_attr = pddf_api.check_bmc_based_attr(dev, attr)
            if bmc_attr is not None and bmc_attr!={}:
                output = pddf_api.bmc_get_cmd(bmc_attr)
                output = output.rstrip()
                part = ''.join(c for c in output if 0 < ord(c) < 127)
                cmd = 'sudo echo "{}" > /sys_switch/psu/psu{}/part_number'.format(part, p)
            else:
                # I2C based attribute
                node = pddf_api.get_path(dev, attr)
                if node:
                    cmd = 'sudo ln -s {} /sys_switch/psu/psu{}/part_number'.format(node, p)
                else:
                    cmd = 'sudo echo "{}" > /sys_switch/psu/psu{}/part_number'.format(part, p)
        except Exception as err:
            cmd = 'sudo echo "{}" > /sys_switch/psu/psu{}/part_number'.format(part, p)

        log_os_system(cmd, 1)

        # type (AC or DC. Fixing it to AC for now)
        psutype = 1 # for AC
        cmd = 'sudo echo "{}" > /sys_switch/psu/psu{}/type'.format(psutype, p)
        log_os_system(cmd, 1)

        # in_curr (input current in milli amps)
        try:
            in_curr = 'NA'
            dev = 'PSU{}'.format(p)
            attr = 'psu_i_in'
            bmc_attr = pddf_api.check_bmc_based_attr(dev, attr)
            if bmc_attr is not None and bmc_attr!={}:
                output = pddf_api.bmc_get_cmd(bmc_attr)
                output = output.rstrip()
                if output.replace('.', '', 1).isdigit():
                    in_curr = float(output)
                cmd = 'sudo echo "{}" > /sys_switch/psu/psu{}/in_curr'.format(in_curr, p)
            else:
                # I2C based attribute
                node = pddf_api.get_path(dev, attr)
                if node:
                    cmd = 'sudo ln -s {} /sys_switch/psu/psu{}/in_curr'.format(node, p)
                else:
                    cmd = 'sudo echo "{}" > /sys_switch/psu/psu{}/in_curr'.format(in_curr, p)
        except Exception as err:
            cmd = 'sudo echo "{}" > /sys_switch/psu/psu{}/in_curr'.format(in_curr, p)

        log_os_system(cmd, 1)

        # in_vol (input current in milli volts)
        try:
            in_vol = 'NA'
            dev = 'PSU{}'.format(p)
            attr = 'psu_v_in'
            bmc_attr = pddf_api.check_bmc_based_attr(dev, attr)
            if bmc_attr is not None and bmc_attr!={}:
                output = pddf_api.bmc_get_cmd(bmc_attr)
                output = output.rstrip()
                if output.replace('.', '', 1).isdigit():
                    in_vol = float(output)
                cmd = 'sudo echo "{}" > /sys_switch/psu/psu{}/in_vol'.format(in_vol, p)
            else:
                # I2C based attribute
                node = pddf_api.get_path(dev, attr)
                if node:
                    cmd = 'sudo ln -s {} /sys_switch/psu/psu{}/in_vol'.format(node, p)
                else:
                    cmd = 'sudo echo "{}" > /sys_switch/psu/psu{}/in_vol'.format(in_vol, p)
        except Exception as err:
            cmd = 'sudo echo "{}" > /sys_switch/psu/psu{}/in_vol'.format(in_vol, p)

        log_os_system(cmd, 1)

        # in_power (input power in micro watts)
        try:
            in_power = 'NA'
            dev = 'PSU{}'.format(p)
            attr = 'psu_p_in'
            bmc_attr = pddf_api.check_bmc_based_attr(dev, attr)
            if bmc_attr is not None and bmc_attr!={}:
                output = pddf_api.bmc_get_cmd(bmc_attr)
                output = output.rstrip()
                if output.replace('.', '', 1).isdigit():
                    in_power = float(output)
                cmd = 'sudo echo "{}" > /sys_switch/psu/psu{}/in_power'.format(in_power, p)
            else:
                # I2C based attribute
                node = pddf_api.get_path(dev, attr)
                if node:
                    cmd = 'sudo ln -s {} /sys_switch/psu/psu{}/in_power'.format(node, p)
                else:
                    cmd = 'sudo echo "{}" > /sys_switch/psu/psu{}/in_power'.format(in_power, p)
        except Exception as err:
            cmd = 'sudo echo "{}" > /sys_switch/psu/psu{}/in_power'.format(in_power, p)

        log_os_system(cmd, 1)

        # out_power (output power in micro watts)
        try:
            out_power = 'NA'
            dev = 'PSU{}'.format(p)
            attr = 'psu_p_out'
            bmc_attr = pddf_api.check_bmc_based_attr(dev, attr)
            if bmc_attr is not None and bmc_attr!={}:
                output = pddf_api.bmc_get_cmd(bmc_attr)
                output = output.rstrip()
                if output.replace('.', '', 1).isdigit():
                    out_power = float(output)
                cmd = 'sudo echo "{}" > /sys_switch/psu/psu{}/out_power'.format(out_power, p)
            else:
                # I2C based attribute
                node = pddf_api.get_path(dev, attr)
                if node:
                    cmd = 'sudo ln -s {} /sys_switch/psu/psu{}/out_power'.format(node, p)
                else:
                    cmd = 'sudo echo "{}" > /sys_switch/psu/psu{}/out_power'.format(out_power, p)
        except Exception as err:
            cmd = 'sudo echo "{}" > /sys_switch/psu/psu{}/out_power'.format(out_power, p)

        log_os_system(cmd, 1)

        # out_max_power (input current in micro watts)
        try:
            out_max_power = 'NA'
            dev = 'PSU{}'.format(p)
            attr = 'psu_p_out_max'
            bmc_attr = pddf_api.check_bmc_based_attr(dev, attr)
            if bmc_attr is not None and bmc_attr!={}:
                output = pddf_api.bmc_get_cmd(bmc_attr)
                output = output.rstrip()
                if output.replace('.', '', 1).isdigit():
                    out_max_power = float(output)
                cmd = 'sudo echo "{}" > /sys_switch/psu/psu{}/out_max_power'.format(out_max_power, p)
            else:
                # I2C based attribute
                node = pddf_api.get_path(dev, attr)
                if node:
                    cmd = 'sudo ln -s {} /sys_switch/psu/psu{}/out_max_power'.format(node, p)
                else:
                    cmd = 'sudo echo "{}" > /sys_switch/psu/psu{}/out_max_power'.format(out_max_power, p)
        except Exception as err:
            cmd = 'sudo echo "{}" > /sys_switch/psu/psu{}/out_max_power'.format(out_max_power, p)

        log_os_system(cmd, 1)

        # out_curr (input current in milli amps)
        try:
            out_curr = 'NA'
            dev = 'PSU{}'.format(p)
            attr = 'psu_i_out'
            bmc_attr = pddf_api.check_bmc_based_attr(dev, attr)
            if bmc_attr is not None and bmc_attr!={}:
                output = pddf_api.bmc_get_cmd(bmc_attr)
                output = output.rstrip()
                if output.replace('.', '', 1).isdigit():
                    out_curr = float(output)
                cmd = 'sudo echo "{}" > /sys_switch/psu/psu{}/out_curr'.format(out_curr, p)
            else:
                # I2C based attribute
                node = pddf_api.get_path(dev, attr)
                if node:
                    cmd = 'sudo ln -s {} /sys_switch/psu/psu{}/out_curr'.format(node, p)
                else:
                    cmd = 'sudo echo "{}" > /sys_switch/psu/psu{}/out_curr'.format(out_curr, p)
        except Exception as err:
            cmd = 'sudo echo "{}" > /sys_switch/psu/psu{}/out_curr'.format(out_curr, p)

        log_os_system(cmd, 1)

        # out_curr_max
        try:
            out_vol = 'NA'
            dev = 'PSU{}'.format(p)
            attr = 'psu_i_out_max'
            bmc_attr = pddf_api.check_bmc_based_attr(dev, attr)
            if bmc_attr is not None and bmc_attr!={}:
                output = pddf_api.bmc_get_cmd(bmc_attr)
                output = output.rstrip()
                if output.replace('.', '', 1).isdigit():
                    out_vol = float(output)
                cmd = 'sudo echo "{}" > /sys_switch/psu/psu{}/alarm_threshold_curr'.format(out_vol, p)
            else:
                # I2C based attribute
                node = pddf_api.get_path(dev, attr)
                if node:
                    cmd = 'sudo ln -s {} /sys_switch/psu/psu{}/alarm_threshold_curr'.format(node, p)
                else:
                    cmd = 'sudo echo "{}" > /sys_switch/psu/psu{}/alarm_threshold_curr'.format(out_vol, p)
        except Exception as err:
            cmd = 'sudo echo "{}" > /sys_switch/psu/psu{}/alarm_threshold_curr'.format(out_vol, p)

        log_os_system(cmd, 1)

        # out_vol (input current in milli volts)
        try:
            out_vol = 'NA'
            dev = 'PSU{}'.format(p)
            attr = 'psu_v_out'
            bmc_attr = pddf_api.check_bmc_based_attr(dev, attr)
            if bmc_attr is not None and bmc_attr!={}:
                output = pddf_api.bmc_get_cmd(bmc_attr)
                output = output.rstrip()
                if output.replace('.', '', 1).isdigit():
                    out_vol = float(output)
                cmd = 'sudo echo "{}" > /sys_switch/psu/psu{}/out_vol'.format(out_vol, p)
            else:
                # I2C based attribute
                node = pddf_api.get_path(dev, attr)
                if node:
                    cmd = 'sudo ln -s {} /sys_switch/psu/psu{}/out_vol'.format(node, p)
                else:
                    cmd = 'sudo echo "{}" > /sys_switch/psu/psu{}/out_vol'.format(out_vol, p)
        except Exception as err:
            cmd = 'sudo echo "{}" > /sys_switch/psu/psu{}/out_vol'.format(out_vol, p)

        log_os_system(cmd, 1)

        # out_vol_max
        try:
            out_vol = 'NA'
            dev = 'PSU{}'.format(p)
            attr = 'psu_v_out_max'
            bmc_attr = pddf_api.check_bmc_based_attr(dev, attr)
            if bmc_attr is not None and bmc_attr!={}:
                output = pddf_api.bmc_get_cmd(bmc_attr)
                output = output.rstrip()
                if output.replace('.', '', 1).isdigit():
                    out_vol = float(output)
                cmd = 'sudo echo "{}" > /sys_switch/psu/psu{}/alarm_threshold_vol'.format(out_vol, p)
            else:
                # I2C based attribute
                node = pddf_api.get_path(dev, attr)
                if node:
                    cmd = 'sudo ln -s {} /sys_switch/psu/psu{}/alarm_threshold_vol'.format(node, p)
                else:
                    cmd = 'sudo echo "{}" > /sys_switch/psu/psu{}/alarm_threshold_vol'.format(out_vol, p)
        except Exception as err:
            cmd = 'sudo echo "{}" > /sys_switch/psu/psu{}/alarm_threshold_vol'.format(out_vol, p)

        log_os_system(cmd, 1)

        # number of temps
        ntemps = 3 # fixing it for now
        temp_alias = ["PSU{} Ambient".format(p), "PSU{} SR Hotspot".format(p), "PSU{} PFC Hotspot".format(p)]
        dev_name = 'PSU{}-PMBUS'.format(p)
        dev_obj = pddf_api.data[dev_name]

        for t in range(1, ntemps + 1):
            cmd = 'sudo mkdir -p -m 777 /sys_switch/psu/psu{}/temp{}'.format(p, t)
            log_os_system(cmd, 1)

            # Add PSU temperature sensors attributes

            # alias i.e. display name
            cmd = 'sudo echo "{}" > /sys_switch/psu/psu{}/temp{}/alias'.format(temp_alias[t - 1], p, t)
            log_os_system(cmd, 1)

            # type i.e. dev_type
            try:
                if 'i2c' in dev_obj.keys() and 'topo_info' in dev_obj['i2c'].keys() and 'dev_type' in dev_obj['i2c']['topo_info'].keys():
                    cmd = 'sudo echo "{}" > /sys_switch/psu/psu{}/temp{}/type'.format(dev_obj['i2c']['topo_info']['dev_type'], p, t)
                else:
                    # BMC based TEMP sensors or TEMP sensors with hwmon path e.g. pch or cpu temps
                    cmd = 'sudo echo "{}" > /sys_switch/psu/psu{}/temp{}/type'.format('NA', p, t)
            except Exception as err:
                cmd = 'sudo echo "{}" > /sys_switch/psu/psu{}/temp{}/type'.format('NA', p, t)

            log_os_system(cmd, 1)

            # temp_input
            try:
                out_vol = 'NA'
                dev = 'PSU{}'.format(p)
                attr = 'psu_temp{}_input'.format(t)
                bmc_attr = pddf_api.check_bmc_based_attr(dev, attr)
                if bmc_attr is not None and bmc_attr!={}:
                    output = pddf_api.bmc_get_cmd(bmc_attr)
                    output = output.rstrip()
                    if output.replace('.', '', 1).isdigit():
                        out_vol = float(output)
                    cmd = 'sudo echo "{}" > /sys_switch/psu/psu{}/temp{}/value'.format(out_vol, p, t)
                else:
                    # I2C based attribute
                    node = pddf_api.get_path(dev, attr)
                    if node:
                        cmd = 'sudo ln -s {} /sys_switch/psu/psu{}/temp{}/value'.format(node, p, t)
                    else:
                        cmd = 'sudo echo "{}" > /sys_switch/psu/psu{}/temp{}/value'.format(out_vol, p, t)
            except Exception as err:
                cmd = 'sudo echo "{}" > /sys_switch/psu/psu{}/temp{}/value'.format(out_vol, p, t)

            log_os_system(cmd, 1)

            # temp_min
            try:
                temp_min = 'NA'
                dev = 'PSU{}'.format(p)
                attr = 'psu_temp{}_high_threshold'.format(t)
                bmc_attr = pddf_api.check_bmc_based_attr(dev, attr)
                if bmc_attr is not None and bmc_attr!={}:
                    output = pddf_api.bmc_get_cmd(bmc_attr)
                    output = output.rstrip()
                    if output.replace('.', '', 1).isdigit():
                        temp_min = float(output)
                    cmd = 'sudo echo "{}" > /sys_switch/psu/psu{}/temp{}/min'.format(temp_min, p, t)
                else:
                    # I2C based attribute
                    try:
                        output = pddf_api.get_attr_name_output(dev, attr)
                        if output:
                            cmd = 'sudo echo "{}" > /sys_switch/psu/psu{}/temp{}/min'.format(output['status'], p, t)
                    except Exception:
                        node = pddf_api.get_path(dev, attr)
                        if node:
                            cmd = 'sudo ln -s {} /sys_switch/psu/psu{}/temp{}/min'.format(node, p, t)
                        else:
                            cmd = 'sudo echo "{}" > /sys_switch/psu/psu{}/temp{}/min'.format(temp_min, p, t)
            except Exception as err:
                cmd = 'sudo echo "{}" > /sys_switch/psu/psu{}/temp{}/min'.format(temp_min, p, t)

            log_os_system(cmd, 1)

            # temp_max
            try:
                temp_max = 'NA'
                dev = 'PSU{}'.format(p)
                attr = 'psu_temp{}_high_crit_threshold'.format(t)
                bmc_attr = pddf_api.check_bmc_based_attr(dev, attr)
                if bmc_attr is not None and bmc_attr!={}:
                    output = pddf_api.bmc_get_cmd(bmc_attr)
                    output = output.rstrip()
                    if output.replace('.', '', 1).isdigit():
                        temp_max = float(output)
                    cmd = 'sudo echo "{}" > /sys_switch/psu/psu{}/temp{}/max'.format(temp_max, p, t)
                else:
                    # I2C based attribute
                    try:
                        output = pddf_api.get_attr_name_output(dev, attr)
                        if output:
                            cmd = 'sudo echo "{}" > /sys_switch/psu/psu{}/temp{}/max'.format(output['status'], p, t)
                    except Exception:
                        node = pddf_api.get_path(dev, attr)
                        if node:
                            cmd = 'sudo ln -s {} /sys_switch/psu/psu{}/temp{}/max'.format(node, p, t)
                        else:
                            cmd = 'sudo echo "{}" > /sys_switch/psu/psu{}/temp{}/max'.format(temp_max, p, t)
            except Exception as err:
                cmd = 'sudo echo "{}" > /sys_switch/psu/psu{}/temp{}/max'.format(temp_max, p, t)

            log_os_system(cmd, 1)

        # number of power sensors
        patten = 'PSU{}'.format(p)
        try:
            power_num = pddf_api.data[patten]['dev_attr']['num_psu_power']
        except Exception as err:
            power_num = 1
        cmd = 'sudo echo "{}" > /sys_switch/psu/psu{}/num_power_sensors '.format(power_num, p)
        log_os_system(cmd, 1)

        for power in range(1, power_num+1):
            cmd = 'sudo mkdir -p -m 777 /sys_switch/psu/psu{}/power_sensor{}'.format(p, power)
            log_os_system(cmd, 1)

            # Add PSU power sensors attributes here

        # num_temp_sensors
        patten = 'PSU{}'.format(p)
        try:
            temp_num = pddf_api.data[patten]['dev_attr']['num_psu_temp']
        except Exception as err:
            temp_num = 0
        cmd = 'sudo echo "{}" > /sys_switch/psu/psu{}/num_temp_sensors'.format(temp_num, p)
        log_os_system(cmd, 1)

        # present
        try:
            pres = 'NA'
            dev = 'PSU{}'.format(p)
            attr = 'psu_present'
            bmc_attr = pddf_api.check_bmc_based_attr(dev, attr)
            if bmc_attr is not None and bmc_attr!={}:
                output = pddf_api.bmc_get_cmd(bmc_attr)
                output = output.rstrip()
                pres = output
                cmd = 'sudo echo "{}" > /sys_switch/psu/psu{}/present'.format(pres, p)
            else:
                # I2C based attribute
                node = pddf_api.get_path(dev, attr)
                if node:
                    cmd = 'sudo ln -s {} /sys_switch/psu/psu{}/present'.format(node, p)
                else:
                    cmd = 'sudo echo "{}" > /sys_switch/psu/psu{}/present'.format(pres, p)
        except Exception as err:
            cmd = 'sudo echo "{}" > /sys_switch/psu/psu{}/present'.format(pres, p)

        log_os_system(cmd, 1)

        # out_status
        try:
            status = 'NA'
            dev = 'PSU{}'.format(p)
            attr = 'psu_power_good'
            bmc_attr = pddf_api.check_bmc_based_attr(dev, attr)
            if bmc_attr is not None and bmc_attr!={}:
                output = pddf_api.bmc_get_cmd(bmc_attr)
                output = output.rstrip()
                status = output
                cmd = 'sudo echo "{}" > /sys_switch/psu/psu{}/out_status'.format(status, p)
            else:
                # I2C based attribute
                node = pddf_api.get_path(dev, attr)
                if node:
                    cmd = 'sudo ln -s {} /sys_switch/psu/psu{}/out_status'.format(node, p)
                else:
                    cmd = 'sudo echo "{}" > /sys_switch/psu/psu{}/out_status'.format(status, p)
        except Exception as err:
            cmd = 'sudo echo "{}" > /sys_switch/psu/psu{}/out_status'.format(status, p)

        log_os_system(cmd, 1)

        # in_status
        try:
            status = 'NA'
            dev = 'PSU{}'.format(p)
            attr = 'psu_acok'
            bmc_attr = pddf_api.check_bmc_based_attr(dev, attr)
            if bmc_attr is not None and bmc_attr!={}:
                output = pddf_api.bmc_get_cmd(bmc_attr)
                output = output.rstrip()
                status = output
                cmd = 'sudo echo "{}" > /sys_switch/psu/psu{}/in_status'.format(status, p)
            else:
                # I2C based attribute
                node = pddf_api.get_path(dev, attr)
                if node:
                    cmd = 'sudo ln -s {} /sys_switch/psu/psu{}/in_status'.format(node, p)
                else:
                    cmd = 'sudo echo "{}" > /sys_switch/psu/psu{}/in_status'.format(status, p)
        except Exception as err:
            cmd = 'sudo echo "{}" > /sys_switch/psu/psu{}/in_status'.format(status, p)

        log_os_system(cmd, 1)

        # fan_speed
        try:
            fan_speed = 'NA'
            dev = 'PSU{}'.format(p)
            attr = 'psu_fan1_speed_rpm'
            bmc_attr = pddf_api.check_bmc_based_attr(dev, attr)
            if bmc_attr is not None and bmc_attr!={}:
                output = pddf_api.bmc_get_cmd(bmc_attr)
                output = output.rstrip()
                if output.replace('.', '', 1).isdigit():
                    fan_speed = int(float(output))
                cmd = 'sudo echo "{}" > /sys_switch/psu/psu{}/fan_speed'.format(fan_speed, p)
            else:
                # I2C based attribute
                node = pddf_api.get_path(dev, attr)
                if node:
                    cmd = 'sudo ln -s {} /sys_switch/psu/psu{}/fan_speed'.format(node, p)
                else:
                    cmd = 'sudo echo "{}" > /sys_switch/psu/psu{}/fan_speed'.format(fan_speed, p)
        except Exception as err:
            cmd = 'sudo echo "{}" > /sys_switch/psu/psu{}/fan_speed'.format(fan_speed, p)

        log_os_system(cmd, 1)

        # fan_ratio
        try:
            fan_ratio = 'NA'
            dev = 'PSU{}'.format(p)
            attr = 'psu_fan1_ratio'
            bmc_attr = pddf_api.check_bmc_based_attr(dev, attr)
            if bmc_attr is not None and bmc_attr!={}:
                output = pddf_api.bmc_get_cmd(bmc_attr)
                output = output.rstrip()
                if output.replace('.', '', 1).isdigit():
                    fan_ratio = int(float(output))
                cmd = 'sudo echo "{}" > /sys_switch/psu/psu{}/fan_ratio'.format(fan_ratio, p)
            else:
                # I2C based attribute
                node = pddf_api.get_path(dev, attr)
                if node:
                    cmd = 'sudo ln -s {} /sys_switch/psu/psu{}/fan_ratio'.format(node, p)
                else:
                    cmd = 'sudo echo "{}" > /sys_switch/psu/psu{}/fan_ratio'.format(fan_ratio, p)
        except Exception as err:
            cmd = 'sudo echo "{}" > /sys_switch/psu/psu{}/fan_ratio'.format(fan_ratio, p)

        log_os_system(cmd, 1)

        # led_status
        result, output = pddf_api.get_system_led_color("PSU{}_LED".format(p))
        if result:
            node = "/sys/kernel/{}/psu{}_led".format(pddf_api.get_led_cur_state_path(), p)
            cmd = 'sudo ln -s {} /sys_switch/psu/psu{}/led_status'.format(node, p)
        else:
            led_status = 'NA'
            cmd = 'sudo echo "{}" > /sys_switch/psu/psu{}/led_status'.format(led_status, p)
        log_os_system(cmd, 1)

    # fruidutil rely on /sys_switch/psu/ number and present
    for p in range(1, num_psus+1):
        # hardware_version
        try:
            hardware_version = 'NA'
            status, output = log_os_system('fruidutil get psu {}'.format(p), 1)
            if status == 0 :
                pattern = r'productVersion\s*:\s*(\S+)'
                matches = re.findall(pattern, output)
                if len(matches) >= 1 :
                    hardware_version = matches[0]
            cmd = 'sudo echo "{}" > /sys_switch/psu/psu{}/hardware_version'.format(hardware_version, p)
        except Exception as err:
            cmd = 'sudo echo "{}" > /sys_switch/psu/psu{}/hardware_version'.format(hardware_version, p)

        log_os_system(cmd, 1)

    print("Completed PSU sysfs creation")

def create_s3ip_xcvr_sysfs():
    print("Creating the xcvr sysfs ..")
    log_os_system('sudo mkdir -p -m 777 /sys_switch/transceiver', 1)
    num_ports = pddf_api.data['PLATFORM']['num_ports'] if 'num_ports' in pddf_api.data['PLATFORM'] else 0
    cmd = 'sudo echo "{}" > /sys_switch/transceiver/number'.format(num_ports)
    log_os_system(cmd, 1)

    # power_on
    power_on = 1
    cmd = 'sudo echo "{}" > /sys_switch/transceiver/power_on'.format(power_on)
    log_os_system(cmd, 1)

    # port related attributes
    for p in range(1, num_ports+1):
        cmd = 'sudo mkdir -p -m 777 /sys_switch/transceiver/eth{}'.format(p)
        log_os_system(cmd, 1)

        # tx_fault
        try:
            fault = 'NA'
            dev = 'PORT{}'.format(p)
            attr = 'xcvr_txfault'
            bmc_attr = pddf_api.check_bmc_based_attr(dev, attr)
            if bmc_attr is not None and bmc_attr!={}:
                output = pddf_api.bmc_get_cmd(bmc_attr)
                output = output.rstrip()
                fault = int(output)
                cmd = 'sudo echo "{}" > /sys_switch/transceiver/eth{}/tx_fault'.format(fault, p)
            else:
                # I2C based attribute
                node = pddf_api.get_path(dev, attr)
                if node:
                    cmd = 'sudo ln -s {} /sys_switch/transceiver/eth{}/tx_fault'.format(node, p)
                else:
                    cmd = 'sudo echo "{}" > /sys_switch/transceiver/eth{}/tx_fault'.format(fault, p)
        except Exception as err:
            cmd = 'sudo echo "{}" > /sys_switch/transceiver/eth{}/tx_fault'.format(fault, p)

        log_os_system(cmd, 1)

        # tx_disable
        try:
            tx_disable = 'NA'
            dev = 'PORT{}'.format(p)
            attr = 'xcvr_txdisable'.format()
            bmc_attr = pddf_api.check_bmc_based_attr(dev, attr)
            if bmc_attr is not None and bmc_attr!={}:
                output = pddf_api.bmc_get_cmd(bmc_attr)
                output = output.rstrip()
                tx_disable = int(output)
                cmd = 'sudo echo "{}" > /sys_switch/transceiver/eth{}/tx_disable'.format(tx_disable, p)
            else:
                # I2C based attribute
                node = pddf_api.get_path(dev, attr)
                if node:
                    cmd = 'sudo ln -s {} /sys_switch/transceiver/eth{}/tx_disable'.format(node, p)
                else:
                    cmd = 'sudo echo "{}" > /sys_switch/transceiver/eth{}/tx_disable'.format(tx_disable, p)
        except Exception as err:
            cmd = 'sudo echo "{}" > /sys_switch/transceiver/eth{}/tx_disable'.format(tx_disable, p)

        log_os_system(cmd, 1)

        # presence
        try:
            present = 'NA'
            dev = 'PORT{}'.format(p)
            attr = 'xcvr_present'.format()
            bmc_attr = pddf_api.check_bmc_based_attr(dev, attr)
            if bmc_attr is not None and bmc_attr!={}:
                output = pddf_api.bmc_get_cmd(bmc_attr)
                output = output.rstrip()
                present = int(output)
                cmd = 'sudo echo "{}" > /sys_switch/transceiver/eth{}/present'.format(present, p)
            else:
                # I2C based attribute
                node = pddf_api.get_path(dev, attr)
                if node:
                    cmd = 'sudo ln -s {} /sys_switch/transceiver/eth{}/present'.format(node, p)
                else:
                    cmd = 'sudo echo "{}" > /sys_switch/transceiver/eth{}/present'.format(present, p)
        except Exception as err:
            cmd = 'sudo echo "{}" > /sys_switch/transceiver/eth{}/present'.format(present, p)

        log_os_system(cmd, 1)

        # rx_los
        try:
            rx_los = 'NA'
            dev = 'PORT{}'.format(p)
            attr = 'xcvr_rxlos'.format()
            bmc_attr = pddf_api.check_bmc_based_attr(dev, attr)
            if bmc_attr is not None and bmc_attr!={}:
                output = pddf_api.bmc_get_cmd(bmc_attr)
                output = output.rstrip()
                rx_los = int(output)
                cmd = 'sudo echo "{}" > /sys_switch/transceiver/eth{}/rx_los'.format(rx_los, p)
            else:
                # I2C based attribute
                node = pddf_api.get_path(dev, attr)
                if node:
                    cmd = 'sudo ln -s {} /sys_switch/transceiver/eth{}/rx_los'.format(node, p)
                else:
                    cmd = 'sudo echo "{}" > /sys_switch/transceiver/eth{}/rx_los'.format(rx_los, p)
        except Exception as err:
            cmd = 'sudo echo "{}" > /sys_switch/transceiver/eth{}/rx_los'.format(rx_los, p)

        log_os_system(cmd, 1)

        # reset
        try:
            reset = 'NA'
            dev = 'PORT{}'.format(p)
            attr = 'xcvr_reset'.format()
            bmc_attr = pddf_api.check_bmc_based_attr(dev, attr)
            if bmc_attr is not None and bmc_attr!={}:
                output = pddf_api.bmc_get_cmd(bmc_attr)
                output = output.rstrip()
                reset = int(output)
                cmd = 'sudo echo "{}" > /sys_switch/transceiver/eth{}/reset'.format(reset, p)
            else:
                # I2C based attribute
                node = pddf_api.get_path(dev, attr)
                if node:
                    cmd = 'sudo ln -s {} /sys_switch/transceiver/eth{}/reset'.format(node, p)
                else:
                    cmd = 'sudo echo "{}" > /sys_switch/transceiver/eth{}/reset'.format(reset, p)
        except Exception as err:
            cmd = 'sudo echo "{}" > /sys_switch/transceiver/eth{}/reset'.format(reset, p)

        log_os_system(cmd, 1)

        # low_power_mode
        try:
            low_power_mode = 'NA'
            dev = 'PORT{}'.format(p)
            attr = 'xcvr_lpmode'.format()
            bmc_attr = pddf_api.check_bmc_based_attr(dev, attr)
            if bmc_attr is not None and bmc_attr!={}:
                output = pddf_api.bmc_get_cmd(bmc_attr)
                output = output.rstrip()
                low_power_mode = int(output)
                cmd = 'sudo echo "{}" > /sys_switch/transceiver/eth{}/low_power_mode'.format(low_power_mode, p)
            else:
                # I2C based attribute
                node = pddf_api.get_path(dev, attr)
                if node:
                    cmd = 'sudo ln -s {} /sys_switch/transceiver/eth{}/low_power_mode'.format(node, p)
                else:
                    cmd = 'sudo echo "{}" > /sys_switch/transceiver/eth{}/low_power_mode'.format(low_power_mode, p)
        except Exception as err:
            cmd = 'sudo echo "{}" > /sys_switch/transceiver/eth{}/low_power_mode'.format(low_power_mode, p)

        log_os_system(cmd, 1)

        # power_on
        try:
            power_on = 'NA'
            dev = 'PORT{}'.format(p)
            attr = 'xcvr_power_en'.format()
            bmc_attr = pddf_api.check_bmc_based_attr(dev, attr)
            if bmc_attr is not None and bmc_attr!={}:
                output = pddf_api.bmc_get_cmd(bmc_attr)
                output = output.rstrip()
                power_on = int(output)
                cmd = 'sudo echo "{}" > /sys_switch/transceiver/eth{}/power_on'.format(power_on, p)
            else:
                # I2C based attribute
                node = pddf_api.get_path(dev, attr)
                if node:
                    cmd = 'sudo ln -s {} /sys_switch/transceiver/eth{}/power_on'.format(node, p)
                else:
                    cmd = ''
        except Exception as err:
            cmd = ''

        log_os_system(cmd, 1)

        if os.path.exists('/sys_switch/transceiver/eth{}/power_on'.format(p)):
            # echo 1 > power_on
            power_on = 1 # PDDF doesnt have a power_on attribute hence fixing its value for now
            cmd = 'sudo echo "{}" > /sys_switch/transceiver/eth{}/power_on'.format(power_on, p)
            log_os_system(cmd, 1)

        # overwrite_en
        try:
            overwrite_en = 'NA'
            dev = 'PORT{}'.format(p)
            attr = 'xcvr_overwrite_en'.format()
            bmc_attr = pddf_api.check_bmc_based_attr(dev, attr)
            if bmc_attr is not None and bmc_attr!={}:
                output = pddf_api.bmc_get_cmd(bmc_attr)
                output = output.rstrip()
                overwrite_en = int(output)
                cmd = 'sudo echo "{}" > /sys_switch/transceiver/eth{}/overwrite_en'.format(overwrite_en, p)
            else:
                # I2C based attribute
                node = pddf_api.get_path(dev, attr)
                if node:
                    cmd = 'sudo ln -s {} /sys_switch/transceiver/eth{}/overwrite_en'.format(node, p)
                else:
                    cmd = ''
        except Exception as err:
            cmd = ''

        log_os_system(cmd, 1)

        # power_fault
        try:
            power_fault = 'NA'
            dev = 'PORT{}'.format(p)
            attr = 'xcvr_power_fault'.format()
            bmc_attr = pddf_api.check_bmc_based_attr(dev, attr)
            if bmc_attr is not None and bmc_attr!={}:
                output = pddf_api.bmc_get_cmd(bmc_attr)
                output = output.rstrip()
                power_fault = int(output)
                cmd = 'sudo echo "{}" > /sys_switch/transceiver/eth{}/power_fault'.format(power_fault, p)
            else:
                # I2C based attribute
                node = pddf_api.get_path(dev, attr)
                if node:
                    cmd = 'sudo ln -s {} /sys_switch/transceiver/eth{}/power_fault'.format(node, p)
                else:
                    cmd = ''
        except Exception as err:
            cmd = ''

        log_os_system(cmd, 1)

        # interrupt
        try:
            interrupt = 'NA'
            dev = 'PORT{}'.format(p)
            attr = 'xcvr_intr_status'.format()
            bmc_attr = pddf_api.check_bmc_based_attr(dev, attr)
            if bmc_attr is not None and bmc_attr!={}:
                output = pddf_api.bmc_get_cmd(bmc_attr)
                output = output.rstrip()
                interrupt = int(output)
                cmd = 'sudo echo "{}" > /sys_switch/transceiver/eth{}/interrupt'.format(interrupt, p)
            else:
                # I2C based attribute
                node = pddf_api.get_path(dev, attr)
                if node:
                    cmd = 'sudo ln -s {} /sys_switch/transceiver/eth{}/interrupt'.format(node, p)
                else:
                    cmd = 'sudo echo "{}" > /sys_switch/transceiver/eth{}/interrupt'.format(interrupt, p)
        except Exception as err:
            cmd = 'sudo echo "{}" > /sys_switch/transceiver/eth{}/interrupt'.format(interrupt, p)

        log_os_system(cmd, 1)

        # eeprom
        try:
            eeprom = 'NA'
            dev = 'PORT{}'.format(p)
            attr = 'eeprom'.format()
            # I2C based attribute
            node = pddf_api.get_path(dev, attr)
            if node:
                cmd = 'sudo ln -s {} /sys_switch/transceiver/eth{}/eeprom'.format(node, p)
            else:
                cmd = 'sudo echo "{}" > /sys_switch/transceiver/eth{}/eeprom'.format(eeprom, p)
        except Exception as err:
            cmd = 'sudo echo "{}" > /sys_switch/transceiver/eth{}/eeprom'.format(eeprom, p)

        log_os_system(cmd, 1)

    print("Completed transceiver sysfs creation")

def create_s3ip_sysled_sysfs():
    print("Creating the SysLED sysfs ..")
    log_os_system('sudo mkdir -p -m 777 /sys_switch/sysled', 1)
    # Fixing the LED enum state for now
    # SYS-LED
    result, output = pddf_api.get_system_led_color("SYS_LED")
    if result:
        node = "/sys/kernel/{}/sys_led".format(pddf_api.get_led_cur_state_path())
        cmd = 'sudo ln -s {} /sys_switch/sysled/sys_led_status'.format(node)
    else:
        sysled = 'NA'
        cmd = 'sudo echo "{}" > /sys_switch/sysled/sys_led_status'.format(sysled)
    log_os_system(cmd, 1)

    # BMC-LED
    result, output = pddf_api.get_system_led_color("BMC_LED")
    if result:
        node = "/sys/kernel/{}/bmc_led".format(pddf_api.get_led_cur_state_path())
        cmd = 'sudo ln -s {} /sys_switch/sysled/bmc_led_status'.format(node)
    else:
        bmcled = 'NA'
        cmd = 'sudo echo "{}" > /sys_switch/sysled/bmc_led_status'.format(bmcled)
    log_os_system(cmd, 1)

    # FAN-LED
    result, output = pddf_api.get_system_led_color("FAN_LED")
    if result:
        node = "/sys/kernel/{}/fan_led".format(pddf_api.get_led_cur_state_path())
        cmd = 'sudo ln -s {} /sys_switch/sysled/fan_led_status'.format(node)
    else:
        fanled = 'NA'
        cmd = 'sudo echo "{}" > /sys_switch/sysled/fan_led_status'.format(fanled)
    log_os_system(cmd, 1)

    # SYS_PSU-LED
    result, output = pddf_api.get_system_led_color("SYS_PSU_LED")
    if result:
        node = "/sys/kernel/{}/sys_psu_led".format(pddf_api.get_led_cur_state_path())
        cmd = 'sudo ln -s {} /sys_switch/sysled/psu_led_status'.format(node)
    else:
        psuled = 'NA'
        cmd = 'sudo echo "{}" > /sys_switch/sysled/psu_led_status'.format(psuled)
    log_os_system(cmd, 1)

    # LOC-LED
    result, output = pddf_api.get_system_led_color("LOC_LED")
    if result:
        node = "/sys/kernel/{}/loc_led".format(pddf_api.get_led_cur_state_path())
        cmd = 'sudo ln -s {} /sys_switch/sysled/id_led_status'.format(node)
    else:
        locled = 'NA'
        cmd = 'sudo echo "{}" > /sys_switch/sysled/id_led_status'.format(locled)
    log_os_system(cmd, 1)

    # led mode(only for SYS-LED)
    if 'SYSSTATUS' in pddf_api.data and 'attr_list' in pddf_api.data['SYSSTATUS']:
        attr_list = pddf_api.data['SYSSTATUS']['attr_list']
        for attr in attr_list:
            if attr['attr_name'].endswith('sys_led_mode'):
                node = '/sys/kernel/pddf/devices/sysstatus/sysstatus_data/{}'.format(attr['attr_name'])
                if os.path.exists(node):
                    cmd = 'sudo ln -s {} /sys_switch/sysled/mode'.format(node)
                    log_os_system(cmd, 1)

    print("Completed the SysLED sysfs creation")

def create_s3ip_fpga_sysfs():
    print("Creating the FPGA sysfs ..")
    log_os_system('sudo mkdir -p -m 777 /sys_switch/fpga', 1)
    fpga_dev = [k for k in pddf_api.data.keys() if 'FPGAPCIE' in k]
    num_fpgas = len(fpga_dev)
    cmd = 'sudo echo "{}" > /sys_switch/fpga/number'.format(num_fpgas)
    log_os_system(cmd, 1)

    n = 0
    for f in fpga_dev:
        n = n + 1
        dev = pddf_api.data[f]
        alias = dev['dev_info']['device_name']
        cmd = 'sudo mkdir -p -m 777 /sys_switch/fpga/fpga{}'.format(n)
        log_os_system(cmd, 1)

        cmd = 'sudo echo "{}" > /sys_switch/fpga/fpga{}/alias'.format(alias, n)
        log_os_system(cmd, 1)

        # type
        dev_type = dev['dev_info']['dev_type']
        cmd = 'sudo echo "{}" > /sys_switch/fpga/fpga{}/type'.format(dev_type, n)
        log_os_system(cmd, 1)

        # firmware_version
        try:
            fw_ver = 'NA'
            dev = 'SYSSTATUS'
            attr = 'fpga_firmware_version'
            bmc_attr = pddf_api.check_bmc_based_attr(dev, attr)
            if bmc_attr is not None and bmc_attr!={}:
                output = pddf_api.bmc_get_cmd(bmc_attr)
                output = output.rstrip()
                fw_ver = output
                cmd = 'sudo echo "{}" > /sys_switch/fpga/fpga{}/firmware_version'.format(fw_ver, n)
            else:
                node = pddf_api.get_path(dev, attr)
                if node:
                    cmd = 'sudo ln -s {} /sys_switch/fpga/fpga{}/firmware_version'.format(node, n)
                else:
                    cmd = 'sudo echo "{}" > /sys_switch/fpga/fpga{}/firmware_version'.format(fw_ver, n)
        except Exception as err:
            cmd = 'sudo echo "{}" > /sys_switch/fpga/fpga{}/firmware_version'.format(fw_ver, n)

        log_os_system(cmd, 1)

        # board version
        try:
            board_ver = 'NA'
            dev = 'SYSSTATUS'
            attr = 'fpga_board_version'
            bmc_attr = pddf_api.check_bmc_based_attr(dev, attr)
            if bmc_attr is not None and bmc_attr!={}:
                output = pddf_api.bmc_get_cmd(bmc_attr)
                output = output.rstrip()
                board_ver = output
                cmd = 'sudo echo "{}" > /sys_switch/fpga/fpga{}/board_version'.format(board_ver, n)
            else:
                node = pddf_api.get_path(dev, attr)
                if node:
                    cmd = 'sudo ln -s {} /sys_switch/fpga/fpga{}/board_version'.format(node, n)
                else:
                    cmd = 'sudo echo "{}" > /sys_switch/fpga/fpga{}/board_version'.format(board_ver, n)
        except Exception as err:
            cmd = 'sudo echo "{}" > /sys_switch/fpga/fpga{}/board_version'.format(board_ver, n)
        log_os_system(cmd, 1)

        # reg_test
        # TODO
    print("Completed FPGA sysfs creation")

def create_s3ip_cpld_sysfs():
    print("Creating the CPLD sysfs ..")
    log_os_system('sudo mkdir -p -m 777 /sys_switch/cpld', 1)
    cpld_dev = [k for k in pddf_api.data.keys() if 'CPLD' in k]
    num_cplds = len(cpld_dev)
    cmd = 'sudo echo "{}" > /sys_switch/cpld/number'.format(num_cplds)
    log_os_system(cmd, 1)

    n = 0
    for c in cpld_dev:
        n = n + 1
        dev = pddf_api.data[c]
        alias = dev['dev_info']['device_name']
        cmd = 'sudo mkdir -p -m 777 /sys_switch/cpld/cpld{}'.format(n)
        log_os_system(cmd, 1)

        # alias
        cmd = 'sudo echo "{}" > /sys_switch/cpld/cpld{}/alias'.format(alias, n)
        log_os_system(cmd, 1)

        # type
        cpld_type  = dev['dev_info']['dev_type']
        cmd = 'sudo echo {} > /sys_switch/cpld/cpld{}/type'.format(cpld_type, n)
        log_os_system(cmd, 1)

        # firmware_version
        try:
            fw_ver = 'NA'
            dev = 'SYSSTATUS'
            attr = 'cpld{}_version'.format(n)
            bmc_attr = pddf_api.check_bmc_based_attr(dev, attr)
            if bmc_attr is not None and bmc_attr!={}:
                output = pddf_api.bmc_get_cmd(bmc_attr)
                output = output.rstrip()
                fw_ver = output
                cmd = 'sudo echo "{}" > /sys_switch/cpld/cpld{}/firmware_version'.format(fw_ver, n)
            else:
                node = pddf_api.get_path(dev, attr)
                if node:
                    cmd = 'sudo ln -s {} /sys_switch/cpld/cpld{}/firmware_version'.format(node, n)
                else:
                    cmd = 'sudo echo "{}" > /sys_switch/cpld/cpld{}/firmware_version'.format(fw_ver, n)
        except Exception as err:
            cmd = 'sudo echo "{}" > /sys_switch/cpld/cpld{}/firmware_version'.format(fw_ver, n)

        log_os_system(cmd, 1)

        # board_version
        board_ver = 'NA'
        cmd = 'sudo echo {} > /sys_switch/cpld/cpld{}/board_version'.format(board_ver, n)
        log_os_system(cmd, 1)

        # reg test
    # parse cpld/fpga reg from SYSSTATUS
    if 'SYSSTATUS' in pddf_api.data and 'attr_list' in pddf_api.data['SYSSTATUS']:
        attr_list = pddf_api.data['SYSSTATUS']['attr_list']
        for attr in attr_list:
            if 'r_' in attr['attr_name'][:2]:
                node = '/sys/kernel/pddf/devices/sysstatus/sysstatus_data/{}'.format(attr['attr_name'])
                if os.path.exists(node):
                    cmd = 'sudo ln -s {} /sys_switch/{}/{}/{}'.format(node,attr['attr_name'][2:6],attr['attr_name'][2:7],attr['attr_name'])
                    log_os_system(cmd, 1)

    print("Completed CPLD sysfs creation")

def create_s3ip_watchdog_sysfs():
    print("Creating the watchdog sysfs ..")
    # watchdog sysfs are not supported in PDDF
    watchdog_path = '/sys/watchdog/'
    if os.path.exists(watchdog_path):
        cmd = 'sudo ln -s "{}"  /sys_switch/'.format(watchdog_path)
        log_os_system(cmd, 1)
    else:
        log_os_system('sudo mkdir -p -m 777 /sys_switch/watchdog', 1)
        identify  = 'NA'
        cmd = 'sudo echo {} > /sys_switch/watchdog/identify'.format(identify)
        log_os_system(cmd, 1)

        enable = 'NA'
        cmd = 'sudo echo {} > /sys_switch/watchdog/enable'.format(enable)
        log_os_system(cmd, 1)

        reset = 'NA'
        cmd = 'sudo echo {} > /sys_switch/watchdog/reset'.format(reset)
        log_os_system(cmd, 1)

        timeout = 'NA'
        cmd = 'sudo echo {} > /sys_switch/watchdog/timeout'.format(timeout)
        log_os_system(cmd, 1)

        timeleft = 'NA'
        cmd = 'sudo echo {} > /sys_switch/watchdog/timeleft'.format(timeleft)
        log_os_system(cmd, 1)

    print("Completed watchdog sysfs creation")

def create_s3ip_slot_sysfs():
    print("Creating the slot sysfs ..")
    # slot sysfs are not supported by PDDF


    print("Completed slot sysfs creation")



def create_s3ip_power_sysfs():
    print("Creating the power sysfs ..")
    # power sysfs are not supported by PDDF

    print("Completed power sysfs creation")




def do_create():
    print("Checking system....")

    status, output = log_os_system('systemctl is-active pddf-platform-init.service', 1)
    if status:
        print("Error: Status of PDDF platform service can't be fetched. Exiting ..")
        return
    if output != 'active':
        print("Error: PDDF platform service is not active. Exiting ..")
        return

    # Create the /sys_switch folders
    log_os_system('sudo rm -rf /sys_switch; sudo mkdir -p -m 777 /sys_switch', 1)


    # Start sysfs creations
    create_s3ip_temp_sysfs()
    create_s3ip_volt_sysfs()
    create_s3ip_curr_sysfs()
    create_s3ip_syseeprom_sysfs()
    create_s3ip_fan_sysfs()
    create_s3ip_psu_sysfs()
    create_s3ip_xcvr_sysfs()
    create_s3ip_sysled_sysfs()
    create_s3ip_fpga_sysfs()
    create_s3ip_cpld_sysfs()
    create_s3ip_watchdog_sysfs()
    create_s3ip_slot_sysfs()
    create_s3ip_power_sysfs()

    return

def do_clean():
    print("Checking system....")

    if not os.path.exists('/sys_switch/'):
        print(PROJECT_NAME.upper() +" sysfs are not present")
        return

    print("Remove all the s3ip sysfs ..")
    status, output = log_os_system('sudo rm -rf /sys_switch/', 1)
    if status:
        print("Error: Unable to remove the s3ip sysfs generated on the system")
    return



if __name__ == "__main__":
    main()
