# -*- coding:utf-8
from function import *

class ChipTypeUtil(object):
    def __init__(self):
        self.s3ip_path = "/sys_switch"
    #CL8000-48C8D	8T CRB	16‘h1021	Lightning
	#CL12800-32D	12.8T CRB	16‘h1022	Lightning
	#EV-256-F48D8E	NB 25.6T EVB	16‘h1023	NB
	#EV-256-BRB	NB 25.6T BRB	16‘h2023	NB
	#EV-256-SLT	NB 25.6T SLT	16‘h3023	NB
	#EV-128-F48D8E	NB 12.8T EVB	16‘h1024	NB
	#EV-128-SLT	NB 12.8T SLT	16‘h2024	NB
    def get_board_type(self):
        
        board_list = {
                "0x1021":"CL8000-48C8D",
                "0x1022":"CL12800-32D",
                "0x1023":"EV-256-F48D8E",
                "0x2023":"EV-256-BRB",
                "0x3023":"EV-256-SLT",
                "0x1024":"EV-128-F48D8E",
                "0x2024":"EV-128-SLT",
                "0x3024":"EV-20-48Y8C",
                "0x4024":"EV-40-24C8D"
        }
        cmd = "cat /sys/kernel/pddf/devices/sysstatus/sysstatus_data/fpga_board_version"
        status, output = run_command(cmd)
        # output = "0x2024"
        board_code = output.strip()
        for key, value in board_list.items():
            if key == board_code:
                return value
        return "N/A"
    
    def get_cpld_type(self):
        cpld_type = {}
        cmd = "cat "+ self.s3ip_path + "/cpld/cpld1/type"
        status, output = run_command(cmd)
        cpld1_running_version = output.strip()
        cpld_type["CPLD1"] = cpld1_running_version

        cmd = "cat "+ self.s3ip_path + "/cpld/cpld2/type"
        status, output = run_command(cmd)
        cpld2_running_version = output.strip()
        cpld_type["CPLD2"] = cpld2_running_version

        return cpld_type

    def get_fpga_type(self):

        cmd = "cat "+ self.s3ip_path + "/fpga/fpga1/type"
        status, output = run_command(cmd)
        fpga_running_type = output.strip()
        return fpga_running_type

