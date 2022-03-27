#!/usr/bin/python
# -*- coding: UTF-8 -*- 
import json
import os
import sys

I2C_PREFIX="/sys/bus/i2c/devices/"

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
    {'bus': "i2c-3",  'driver': "mp2882",         'address': "0x20"},
    {'bus': "i2c-3",  'driver': "mp2882",         'address': "0x21"},
    {'bus': "i2c-2",  'driver': "adm1166",        'address': "0x34"},
    {'bus': "i2c-2",  'driver': "adm1166",        'address': "0x36"},
    {'bus': "i2c-1",  'driver': "24c02",          'address': "0x50"},
    {'bus': "i2c-1",  'driver': "24c02",          'address': "0x52"},
    {'bus': "i2c-6",  'driver': "24c256",         'address': "0x51"},
    {'bus': "i2c-3",  'driver': "tps546b24a",     'address': "0x29"},
    {'bus': "i2c-1",  'driver': "gw1200d",        'address': "0x58"},
    {'bus': "i2c-1",  'driver': "gw1200d",        'address': "0x5a"},
]

if __name__ == '__main__':
    #os.system("sudo rm -rf /sys_switch;sudo mkdir -p -m 777 /sys_switch")
    
#    with open('/etc/s3ip/s3ip_sysfs_conf.json', 'r') as jsonfile:

    args = sys.argv[1:]
    if len(args[0:]) < 1:
        print (main.__doc__)
        sys.exit(0)
    #default operation
    op=""
    extra_cmd = "new_device"
    if args[0] == 'remove':
        extra_cmd = "delete_device"
        op="del "
    with open('/etc/s3ip/s3ip_sysfs_conf.json', 'r') as jsonfile:
        json_string = json.load(jsonfile)
        for s3ip_sysfs_path in json_string['s3ip_syfs_paths']:
            #print('path:' + s3ip_sysfs_path['path'])
            #print('type:' + s3ip_sysfs_path['type'])
            #print('value:' + s3ip_sysfs_path['value'])
            
            if s3ip_sysfs_path['type'] == "string" :
                (path, file) = os.path.split(s3ip_sysfs_path['path'])
                if op == "": 
                   #创建文件
                   command = "sudo mkdir -p -m 777 " + path
                else:
                   command = "sudo rm -rf" + path
                #print(command)
                os.system(command)
                #temp solution: creake link
                #command = "sudo echo " +  "\"" + s3ip_sysfs_path['value'] + "\"" + " > " + s3ip_sysfs_path['path']
                command = "sudo echo " + op + "\"" + s3ip_sysfs_path['value'] + "\"" + " " + s3ip_sysfs_path['path'] + " > " + "/sys/switch/clounix_cmd"
                #print(command)
                os.system(command)
            elif s3ip_sysfs_path['type'] == "path" :
                #temp solution: creake link
                #command = "sudo ln -s " + s3ip_sysfs_path['value'] + " " + s3ip_sysfs_path['path']
                command = "sudo echo " + op + s3ip_sysfs_path['value'] + " " + s3ip_sysfs_path['path'] + " > " + "/sys/switch/clounix_cmd"
                #print(command)
                os.system(command)
            elif s3ip_sysfs_path['type'] == "elem" :
                #create for all of the elem 
                for i in os.listdir(s3ip_sysfs_path['value']):
                    source_item = s3ip_sysfs_path['value'] + '/{}'.format(i)
                    target_item = s3ip_sysfs_path['path'] + '/{}'.format(i)
                    command = "sudo echo " + op+ source_item + " " + target_item + " > " + "/sys/switch/clounix_cmd"
                    if op == "del": 
                        command = "sudo echo " + op + target_item + " > " + "/sys/switch/clounix_cmd"
                    #print(command)
                    os.system(command)
            else:
                print('error type:' + s3ip_sysfs_path['type'])

        #new device
    for o in i2c_topology_dict:
        if args[0] == 'remove':
            os.system("echo " + o['address'] + " > " + I2C_PREFIX + o['bus'] + "/" + extra_cmd)
        else:
            os.system("echo " + o['driver'] + " " + o['address'] + " > " + I2C_PREFIX + o['bus'] + "/" + extra_cmd)
            
