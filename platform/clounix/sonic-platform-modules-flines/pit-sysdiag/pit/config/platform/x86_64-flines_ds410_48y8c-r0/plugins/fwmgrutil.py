# -*- coding:utf-8
from pit_util_common import run_command
from test_case import TestCaseCommon
from errcode import E
import re
import json
import os


class FwMgrUtil(object):
    def __init__(self):
        self.clx_update_tool = "/clx_fpga -f "
        self.cpld_update_tool = "/cpld_update_app "

    # 固件版本查看接口
    def get_fw_version(self, fw_type_list, fw_extr=None):
        """
            @fw_type_list firmware type list, should be list of the strings:'bios', 'uboot', 'bmc', 'cpld', .etc.
            @fw_extra OPTIONAL, extra information string, for fw_type 'BIOS'/'Uboot'/'BMC', value should be one of 'master'/'slave'
        	Retrieves all firmwares' version on the device
        	@return DICT, key=firmware name, value=DICT,include "version" and "description"
        """
        result = {}
        status, ret = run_command("show platform firmware status")
        for item in fw_type_list:
            for str in ret.splitlines():
                if item in str:
                    str_list = str.split()
                    str_len = len(str_list)
                    for i in range(0, str_len):
                        if item == str_list[i]:
                            result[item] = {}
                            result[item]['version'] = str_list[i + 1]
                            desc = ''
                            for j in range(i + 2, str_len):
                                desc += str_list[j] + ' '
                            result[item]['description'] = desc
                            break
        print("Result:", result)
        return result
        
    def get_bios_version(self):
        bios_version = {}
        cmd = "dmidecode -t bios |grep Version| awk '{print $2}'"
        status, output = run_command(cmd)
        bios_running_version = output.strip()
        return bios_running_version
    
    def get_cpld_version(self):
        cpld_version = {}
        cmd = "cat /sys/s3ip/cpld/cpld1/hw_version"
        status, output = run_command(cmd)
        cpld1_running_version = output.strip()
        cpld_version["CPLD1"] = cpld1_running_version

        cmd = "cat /sys/s3ip/cpld/cpld2/hw_version"
        status, output = run_command(cmd)
        cpld2_running_version = output.strip()
        cpld_version["CPLD2"] = cpld2_running_version

        cmd = "i2cget -f -y 6 0x60 0x00"
        status, output = run_command(cmd)
        fan_cpld_running_version = output.strip()
        cpld_version["FAN_CPLD"] = fan_cpld_running_version

        return cpld_version

    def get_sdk_version(self):
        sdk_version = {}
        cmd = "clx_diag sai show version | grep 'SAI head version'|awk '{print $4}'"
        status, output = run_command(cmd)
        sdk_running_version = output.strip()
        return sdk_running_version
    
    def get_fpga_version(self):
        fpga_version = {}
        cmd = "cat /sys/s3ip/fpga/fpga1/hw_version"
        status, output = run_command(cmd)
        fpga_running_version = output.strip()
        return fpga_running_version

    # 获取下次启动Flash以及Flash主备切换
    # BIOS
    def get_bios_current_boot_flash(self):
        return "master"/"slave"

    def get_bios_next_boot_flash(self):
        return "master"/"slave"

    def set_bios_next_boot_flash(self, flash):
        return True/False

    # Uboot
    def get_uboot_current_boot_flash(self):
        return "master"/"slave"

    def get_uboot_next_boot_flash(self):
        return "master"/"slave"

    def set_uboot_next_boot_flash(self, flash):
        return True/False

    # BMC
    def get_bmc_current_boot_flash(self):
        return "master"/"slave"

    def get_bmc_next_boot_flash(self):
        return "master"/"slave"

    def set_bmc_next_boot_flash(self, flash):
        return True/False
    
    def fpga_upgrade(self, fw_path, platform_path):
        ret = False
        #update
        cmd = platform_path + self.clx_update_tool + fw_path + " -b 3"
        try:
            status, retstr = run_command(cmd)
            if status != 0:
                return ret
            for line in retstr.splitlines():
                if ("Done") in line:
                    print(line)
                    ret = True
                    break
        except Exception as e:
            print(str(e))

        return ret

    def upgrade_fan_cpld(self):
        ret = False
        cmd = "systemctl stop pmon"
        status, retstr = run_command(cmd)
        if status != 0:
		print("upgrade fan cpld, stop pmon error!")
        	return ret
        cmd = "i2cset -f -y 0 0x26 0x7 0xFC"
        status, retstr = run_command(cmd)
        if status != 0:
		print("upgrade fan cpld, set port 1 output error!")
        	return ret
        cmd = "i2cset -f -y 0 0x26 0x3 0xFE"
        status, retstr = run_command(cmd)
        if status != 0:
		print("upgrade fan cpld, set port 1 gpio high error!")
        	return ret

	ret = True
        return ret

    def upgrade_main_cpld(self):
        ret = False
        cmd = "systemctl stop pmon"
        status, retstr = run_command(cmd)
        if status != 0:
		print("upgrade man cpld, stop pmon error!")
        	return ret
        cmd = "i2cset -f -y 0 0x26 0x7 0xFC"
        status, retstr = run_command(cmd)
        if status != 0:
		print("upgrade man cpld, set port 1 output error!")
        	return ret
        cmd = "i2cset -f -y 0 0x26 0x3 0xFD"
        status, retstr = run_command(cmd)
        if status != 0:
		print("upgrade man cpld, set gpio error!")
        	return ret
	ret = True
        return ret

    def cpld_recover(self):
        ret = False
        cmd = "i2cset -f -y 0 0x26 0x3 0xFF"
        status, retstr = run_command(cmd)
        if status != 0:
		print("cpld reover error!")
        	return ret
        cmd = "systemctl start pmon"
        status, retstr = run_command(cmd)
        if status != 0:
		print("cpld reover, start pmon error!")
        	return ret
	ret = True
        return ret

    def cpld1_upgrade(self, fw_path, platform_path):
        ret = False
	ret = self.upgrade_main_cpld()
	if ret == False:
		return ret
        cmd = platform_path + self.cpld_update_tool + fw_path
	print("cpld upgrade:",cmd)
        try:
            status, retstr = run_command(cmd)
            if status != 0:
                return ret
            for line in retstr.splitlines():
                if ("Success") in line:
                    print(line)
                    ret = True
                    break
        except Exception as e:
            print(str(e))

        return ret
    
    def cpld2_upgrade(self, fw_path, platform_path):
        ret = False
	ret = self.upgrade_fan_cpld()
	if ret == False:
		return ret
        cmd = platform_path + self.cpld_update_tool + fw_path
        try:
            status, retstr = run_command(cmd)
            if status != 0:
                return ret
            for line in retstr.splitlines():
                if ("Success") in line:
                    print(line)
                    ret = True
                    break
        except Exception as e:
            print(str(e))

        return ret


    # 固件更新
    def firmware_upgrade(self, fw_type, fw_path, fw_extr=None, platform_path=None):
        """
        	@fw_type: firmware type, should be one of the strings:'bios', 'uboot', 'bmc', 'cpld', .etc.
        	@fw_path: target firmware file
        	@fw_extra OPTIONAL, extra information string, for fw_type 'BIOS'/'Uboot'/'BMC', value should be one of 'master'/'slave'/'both'
        	@return: Boolean
        """
        ret = False
        print("=====firmware upgrade=====",fw_type, fw_path, platform_path)
        if fw_type == "FPGA":
            ret = self.fpga_upgrade(fw_path, platform_path)
        elif fw_type == "CPLD-1":
            ret = self.cpld1_upgrade(fw_path, platform_path)
	    self.cpld_recover()
        elif fw_type == "CPLD-2":
            ret = self.cpld2_upgrade(fw_path, platform_path)
	    self.cpld_recover()
        elif fw_type == "FAN-CPLD":
            ret = self.cpld2_upgrade(fw_path, platform_path)
	    self.cpld_recover()
        else:
            pass

        return ret
        # cmd = "config platform firmware install chassis component %s fw %s -y" % (fw_type, fw_path)
        # try:
        #     retstr = run_command(cmd, True)
        #     for line in retstr.splitlines():
        #         if ("%s upgrade success" % fw_type) in line:
        #             print(line)
        #             ret = True
        #             break
        # except Exception as e:
        #     print(str(e))
        # return ret

    def firmware_refresh(self, item, fw_path, fw_extra=None):
        print("======firmware_refresh=====",item, fw_path, fw_extra)
        if item == "CPLD-1" or item == "CPLD-2" or item == "FAN-CPLD":
            return True
        if item == "FPGA":
            os.system("sleep 10; busybox devmem 0xf8000604  32 0x00000001 && busybox devmem 0xf8000600  32 0x00000001 &")
            return True

    # 获取FRU列表
    def get_frus(self):
        """
        	Retrieves all FRU names on the device
        	@return: LIST
        """
        #return FRU_NAME_LIST  # ['sys', 'smb', 'pdb']
        pass

    # 获取FRU内容
    def read_fru(self, fru_name):
        """
        	Read the FRU content.
        	@fru_name: string, the name of FRU
        	@return: TUPLE,(status, output), status=True if FRU acceed success,
        			output=string of the FRU content.
        """
        #return statue, content
        pass
	
    # 烧录FRU
    def program_fru(self, fru_name, fru_bin_file):
        """
        	program the FRU
        	@fru_name: the name of FRU
        	@fru_bin_file: the file of FRU raw binary.
        	@return: True if programed success.
        """
        return False
