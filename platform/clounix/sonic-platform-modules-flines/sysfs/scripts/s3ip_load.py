#!/usr/bin/env python3
# -*- coding: UTF-8 -*-

import json
import os
import sys
import shutil

CONFIG_FILE = '/etc/s3ip/s3ip_sysfs_conf.json'
TARGET_ROOT = '/sys_switch'
SOURCE_ROOT = '/sys/s3ip'


def create_links():
    if not os.path.exists(CONFIG_FILE):
        return False
    
    try:
        with open(CONFIG_FILE, 'r', encoding='utf-8') as f:
            config = json.load(f)
    except Exception:
        return False
    
    os.makedirs(TARGET_ROOT, mode=0o755, exist_ok=True)
    
    paths = config.get('s3ip_syfs_paths', [])
    for item in paths:
        src = item.get('value')
        dst = item.get('path')
        
        if not src or not dst:
            continue
        
        if not os.path.exists(src):
            continue
        
        try:
            if os.path.isdir(src):
                if dst == TARGET_ROOT:
                    for sub_item in os.listdir(src):
                        sub_src = os.path.join(src, sub_item)
                        sub_dst = os.path.join(TARGET_ROOT, sub_item)
                        if os.path.isfile(sub_src) or os.path.islink(sub_src):
                            if os.path.islink(sub_dst) or os.path.exists(sub_dst):
                                os.remove(sub_dst)
                            os.symlink(sub_src, sub_dst)
                else:
                    if os.path.islink(dst) or os.path.exists(dst):
                        os.remove(dst)
                    os.symlink(src, dst)
            else:
                dst_dir = os.path.dirname(dst)
                os.makedirs(dst_dir, mode=0o755, exist_ok=True)
                if os.path.islink(dst) or os.path.exists(dst):
                    os.remove(dst)
                os.symlink(src, dst)
        except Exception:
            pass
    
    return True


def remove_links():
    if os.path.exists(TARGET_ROOT):
        try:
            shutil.rmtree(TARGET_ROOT)
        except Exception:
            pass
    return True


def main():
    if len(sys.argv) < 2:
        print("Usage: s3ip_load.py [create|remove]")
        sys.exit(0)

    if os.geteuid() != 0:
        print("Error: This command requires root privileges.")
        sys.exit(1)

    cmd = sys.argv[1].lower()

    if cmd in ['create', 'start']:
        success = create_links()
        sys.exit(0 if success else 1)
        
    elif cmd in ['remove', 'stop']:
        success = remove_links()
        sys.exit(0 if success else 1)
    else:
        print(f"Unknown argument: {cmd}")
        sys.exit(1)


if __name__ == '__main__':
    main()

