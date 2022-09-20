#!/usr/bin/python
# -*- coding: UTF-8 -*-
import json
import os
import sys

if __name__ == '__main__':
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
                   command = "mkdir -p -m 777 " + path
                else:
                   command = "rm -rf" + path
                os.system(command)
                command = "echo " + op + "\"" + s3ip_sysfs_path['value'] + "\"" + " " + s3ip_sysfs_path['path'] + " > " + "/sys/switch/clounix_cmd"
                os.system(command)
            elif s3ip_sysfs_path['type'] == "path" :
                command = "echo " + op + s3ip_sysfs_path['value'] + " " + s3ip_sysfs_path['path'] + " > " + "/sys/switch/clounix_cmd"
                os.system(command)
            elif s3ip_sysfs_path['type'] == "elem" :
                #create for all of the elem
                for i in os.listdir(s3ip_sysfs_path['value']):
                    source_item = s3ip_sysfs_path['value'] + '/{}'.format(i)
                    target_item = s3ip_sysfs_path['path'] + '/{}'.format(i)
                    if op == "del ":
                        command = "echo " + op + target_item + " > " + "/sys/switch/clounix_cmd"
                    else:
                        command = "echo " + op+ source_item + " " + target_item + " > " + "/sys/switch/clounix_cmd"
                    os.system(command)
            else:
                print('error type:' + s3ip_sysfs_path['type'])
