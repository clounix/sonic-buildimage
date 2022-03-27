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

HOST = ''
SOCKET_LIST = []
SOCKET_MAX_CLIENT = 10
QUEUES = []
THREADS = []
FUNCS = {}

class GlobalThread(common.threading.Thread):
    def __init__(self,threadname, q):
        common.threading.Thread.__init__(self,name = threadname)
        self.queue = q

    def run(self):
        while common.RUN:
            message = self.queue.get()
            self.onMessage(message)

    def onMessage(self, message):

        """
        Commands:
             uninstall	: Uninstall platform drivers
        """
        if len(message.command) < 1:
            result = self.onMessage.__doc__
        else:
            if message.command[0] == 'uninstall':
                common.RUN = False
                #doUninstall()
                os.system("/usr/local/bin/s3ip_sysfs_tool.sh stop")
                result = 'Success'
            else:
                result = self.onMessage.__doc__
        if (message.callback is not None):
            message.callback(result)

class messageObject(object):
    def __init__(self, command, callback):
        super(messageObject, self).__init__()
        self.command = command
        self.callback = callback

def callback(sock, result):
	sock.sendall(result.encode(encoding='utf-8'))

def messageHandler():
    server_socket = common.socket.socket(common.socket.AF_INET, common.socket.SOCK_STREAM)
    server_socket.setsockopt(common.socket.SOL_SOCKET, common.socket.SO_REUSEADDR, 1)
    server_socket.bind((HOST, common.SOCKET_PORT))
    server_socket.listen(SOCKET_MAX_CLIENT)
    SOCKET_LIST.append(server_socket)

    while(common.RUN):
        ready_to_read,ready_to_write,in_error = common.select.select(SOCKET_LIST,[],[],0)
        for sock in ready_to_read:
            if sock == server_socket:
                sockfd, addr = server_socket.accept()
                SOCKET_LIST.append(sockfd)
            else:
                try:
                    data = sock.recv(common.SOCKET_RECV_BUFFER).decode(encoding='utf-8')
                    if data:
                        cb = common.partial(callback, sock)
                        cmdlist = data.split()

                        if cmdlist[0] not in common.CMD_TYPE:
                            callback(sock, 'Fail')
                            continue

                        msg = messageObject(cmdlist[1:], cb)
                        FUNCS[cmdlist[0]].put(msg)
                        continue
                    else:
                        if sock in SOCKET_LIST:
                            SOCKET_LIST.remove(sock)
                except:
                    raise
                continue
        common.time.sleep(0.2)

    server_socket.close()

KERNEL_MODULE = [
    'clounix_sysfs_main',
    'clounix_fpga',
    'clounix_fpga_cpld',
    'clounix_fpga_i2c',
    'clounix_fpga_fan',
    'clounix_fpga_sfp',
    'clounix_fpga_led',
    'i2c_dev',
    'i2c-mux',
    'i2c-mux-pca9548',
    'lm75',
    'adm1166',
    'pmbus_core',
    'mp2882',
    'adm1275',
    'gwcr-psu',
    'tps546b24a',
    'at24'
]

def checkDriver():
    for i in range(0, len(KERNEL_MODULE)):
        status, output = common.doBash("lsmod | grep " + KERNEL_MODULE[i])
        if status:
            status, output = common.doBash("modprobe " + KERNEL_MODULE[i])
    return

i2c_topology_dict=[
    {'bus': "i2c-0",   'driver': "clx_pca9548",       'address': "0x70"},
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
SOFT_LINK_NODE_TYPE = ['sensor', 'transceiver', 'fan', 'cpld', 'syseeprom', 'sysled', 'watchdog', 'psu']
TRANSCIEVER_PREFIX  = '/sys/switch/transceiver/'
PSU_PREFIX          = '/sys/switch/psu/'
FAN_PREFIX          = '/sys/switch/fan/'
CPLD_PREFIX         = '/sys/switch/cpld/'
SYSEEPROM_PREFIX    = '/sys/switch/syseeprom/'
SYSLED_PREFIX       = '/sys/switch/sysled/'
WATCHDOG_PREFIX     = '/sys/switch/watchdog/'

SYSEEPROM_TOP_NODE_DICT = [
    {'src_node':common.I2C_PREFIX+'13-0050/eeprom',      'dest_node' : SYSEEPROM_PREFIX+'eeprom'},
]
WATCHDOG_TOP_NODE_DICT = [
    #not supported{'src_node':'',       'dest_node':WATCHDOG_PREFIX+'loglevel'},
    #not supported{'src_node':'',       'dest_node':WATCHDOG_PREFIX+'debug'},
    #not supported{'src_node':'/sys/bus/platform/devices/iTCO_wdt.1.auto/watchdog/watchdog0/identity',      'dest_node':WATCHDOG_PREFIX+'identity'},
    #not supported{'src_node':'/sys/bus/platform/devices/iTCO_wdt.1.auto/watchdog/watchdog0/state',      'dest_node':WATCHDOG_PREFIX+'state'},
    #not supported{'src_node':'/sys/bus/platform/devices/iTCO_wdt.1.auto/watchdog/watchdog0/timeleft',      'dest_node':WATCHDOG_PREFIX+'timeleft'},
    #not supported{'src_node':'/sys/bus/platform/devices/iTCO_wdt.1.auto/watchdog/watchdog0/timeout',      'dest_node':WATCHDOG_PREFIX+'timeout'},
    #not supported{'src_node':'/sys/bus/platform/devices/iTCO_wdt.1.auto/watchdog/watchdog0/reset',      'dest_node':WATCHDOG_PREFIX+'reset'},
    #not supported{'src_node':'/sys/bus/platform/devices/iTCO_wdt.1.auto/watchdog/watchdog0/enable',      'dest_node':WATCHDOG_PREFIX+'enable'},
]
SYSLED_TOP_NODE_DICT = [
    #not supported{'src_node':'/sys/module/pegatron_fn8032_bnf_cpld/parameters/loglevel',         'dest_node':SYSLED_PREFIX+'loglevel'},
    #not supported{'src_node':common.I2C_PREFIX+'6-0074/debug_sysled',        'dest_node':SYSLED_PREFIX+'debug'},
   #not supported {'src_node':common.I2C_PREFIX+'6-0074/sys_led',      'dest_node':SYSLED_PREFIX+'sys_led_status'},
    #not supported{'src_node':common.I2C_PREFIX+'6-0074/',      'dest_node':SYSLED_PREFIX+'bmc_led_status'},
   #not supported {'src_node':common.I2C_PREFIX+'6-0074/fan_led',      'dest_node':SYSLED_PREFIX+'fan_led_status'},
   #not supported {'src_node':common.I2C_PREFIX+'6-0074/pwr_led',      'dest_node':SYSLED_PREFIX+'psu_led_status'},
   #not supported {'src_node':common.I2C_PREFIX+'6-0074/loc_led',      'dest_node':SYSLED_PREFIX+'id_led_status'},
]

TRANSCIEVER_TOP_NODE_DICT = [
    {'src_node':'/sys/module/clounix_fpga_cpld/parameters/loglevel', 'dest_node' : TRANSCIEVER_PREFIX+'loglevel'},
    {'src_node':common.FPGA_PREFIX+'/debug_sfp',            'dest_node' : TRANSCIEVER_PREFIX+'debug'},
    {'src_node':common.FPGA_PREFIX+'/sfp_port_num',         'dest_node' : TRANSCIEVER_PREFIX+'num'},
    {'src_node':common.FPGA_PREFIX+'/sfp_all_present',      'dest_node' : TRANSCIEVER_PREFIX+'present'},
    {'src_node':common.FPGA_PREFIX+'/sfp_all_power_on',     'dest_node' : TRANSCIEVER_PREFIX+'power_on'},
]
TRANSCIEVER_PORT_NODE_DICT = [
    {'src_node' : common.FPGA_PREFIX+'/sfp{}_eeprom',      'dest_node' : TRANSCIEVER_PREFIX+'eth{}/eeprom'},
    {'src_node' : common.FPGA_PREFIX+'/sfp{}_present',     'dest_node' : TRANSCIEVER_PREFIX+'eth{}/present'},
    {'src_node' : common.FPGA_PREFIX+'/sfp{}_reset',       'dest_node' : TRANSCIEVER_PREFIX+'eth{}/reset'},
    {'src_node' : common.FPGA_PREFIX+'/sfp{}_lowpower',    'dest_node' : TRANSCIEVER_PREFIX+'eth{}/lpmode'},
    {'src_node' : common.FPGA_PREFIX+'/sfp{}_irq_status',  'dest_node' : TRANSCIEVER_PREFIX+'eth{}/interrupt'},
    {'src_node' : common.FPGA_PREFIX+'/sfp{}_power_on',    'dest_node' : TRANSCIEVER_PREFIX+'eth{}/power_on'},
]
TRANSCIEVER_QDDPORT_NODE_DICT = [
    {'src_node' : common.FPGA_PREFIX+'/sfp{}_power_fault', 'dest_node' : TRANSCIEVER_PREFIX+'eth{}/power_fault'},

]
FAN_TOP_NODE_DICT = [
    {'src_node' : '/sys/module/clounix_fpga_fan/parameters/loglevel',      'dest_node' : FAN_PREFIX+'loglevel'},
    {'src_node' : common.I2C_PREFIX+'6-0060/hwmon/hwmon1/fan_motor_num',   'dest_node' : FAN_PREFIX+'num_motors'},
    {'src_node' : common.I2C_PREFIX+'6-0060/hwmon/hwmon1/fan_num',         'dest_node' : FAN_PREFIX+'num'},
    {'src_node' : common.I2C_PREFIX+'6-0060/hwmon/hwmon1/fan_vendor',      'dest_node' : FAN_PREFIX+'fan_vendor'},
    {'src_node' : common.I2C_PREFIX+'6-0060/hwmon/hwmon1/fan_cpld_ver',    'dest_node' : FAN_PREFIX+'cpld_version'},
    {'src_node' : common.I2C_PREFIX+'6-0060/hwmon/hwmon1/fan_boardId',     'dest_node' : FAN_PREFIX+'boardId'},
    {'src_node' : common.I2C_PREFIX+'6-0060/hwmon/hwmon1/fan_status_alert','dest_node' : FAN_PREFIX+'status_alert'},
    {'src_node' : common.I2C_PREFIX+'6-0060/hwmon/hwmon1/fan_usb_fault',   'dest_node' : FAN_PREFIX+'usb_fault'},
    {'src_node' : common.I2C_PREFIX+'6-0060/hwmon/hwmon1/fan_eeprom_write_enable',   'dest_node' : FAN_PREFIX+'fan_eeprom_write_enable'},
]
FAN_ATTR_NODE_DICT = [
    {'src_node' : common.I2C_PREFIX+'6-0060/hwmon/hwmon1/fan{}_present',    'dest_node' : FAN_PREFIX+'fan{}/status'},
    {'src_node' : common.I2C_PREFIX+'6-0060/hwmon/hwmon1/fan{}_speed',      'dest_node' : FAN_PREFIX+'fan{}/speed'},
    {'src_node' : common.I2C_PREFIX+'6-0060/hwmon/hwmon1/fan{}_inner_rpm',      'dest_node' : FAN_PREFIX+'fan{}/inner_rpm'},
    {'src_node' : common.I2C_PREFIX+'6-0060/hwmon/hwmon1/fan{}_outer_rpm',      'dest_node' : FAN_PREFIX+'fan{}/outer_rpm'},
    {'src_node' : common.I2C_PREFIX+'6-0060/hwmon/hwmon1/fan{}_led_red',    'dest_node' : FAN_PREFIX+'fan{}/led_red'},
    {'src_node' : common.I2C_PREFIX+'6-0060/hwmon/hwmon1/fan{}_led_green',  'dest_node' : FAN_PREFIX+'fan{}/led_green'},
    {'src_node' : common.I2C_PREFIX+'6-0060/hwmon/hwmon1/fan_pwm',          'dest_node' : FAN_PREFIX+'fan{}/ratio'},
    {'src_node' : common.I2C_PREFIX+'6-0060/hwmon/hwmon1/fan{}_airflow_dir','dest_node' : FAN_PREFIX+'fan{}/direction'}
]
FAN_MOTOR_NODE_DICT = [

]

PSU_TOP_NODE_DICT = [
    #not supported{'src_node':'/sys/module/pegatron_hwmon_mcu/parameters/loglevel',         'dest_node':PSU_PREFIX+'loglevel'},
    #not supported{'src_node':'/sys/module/pegatron_hwmon_mcu/parameters/debug',            'dest_node':PSU_PREFIX+'debug'},
]

PSU_STAT_NODE_DICT = [
   {'src_node' : common.I2C_PREFIX+'{}/psu_{}_prst',      'dest_node' : PSU_PREFIX+'psu{}/status'},
]

PSU_ATTR_NODE_DICT_1 = [
   {'src_node' : common.I2C_PREFIX+'{}/led_status',      'dest_node' : PSU_PREFIX+'psu{}/led_status'},
   {'src_node' : common.I2C_PREFIX+'{}/mfr_name',      'dest_node' : PSU_PREFIX+'psu{}/model_name'},
   {'src_node' : common.I2C_PREFIX+'{}/mfr_serial',        'dest_node' : PSU_PREFIX+'psu{}/serial_number'},
   {'src_node' : common.I2C_PREFIX+'{}/mfr_id',      'dest_node' : PSU_PREFIX+'psu{}/vendor'},
   {'src_node' : common.I2C_PREFIX+'{}/mfr_date',      'dest_node' : PSU_PREFIX+'psu{}/date'},
   {'src_node' : common.I2C_PREFIX+'{}/mfr_revision',      'dest_node' : PSU_PREFIX+'psu{}/hardware_version'},
   #not supported{'src_node' : common.I2C_PREFIX+'{}/psu_alarm',          'dest_node' : PSU_PREFIX+'psu{}/alarm'},
   #not supported{'src_node' : common.I2C_PREFIX+'{}/curr_in_max',          'dest_node' : PSU_PREFIX+'psu{}/alarm_threshold_curr'},
   #not supported{'src_node' : common.I2C_PREFIX+'{}/vol_in_max',          'dest_node' : PSU_PREFIX+'psu{}/alarm_threshold_vol'},
   #not supported{'src_node' : common.I2C_PREFIX+'{}/manuafacture_part_num',          'dest_node' : PSU_PREFIX+'psu{}/part_number'},
   #not supported{'src_node' : common.I2C_PREFIX+'{}/power_out_max',          'dest_node' : PSU_PREFIX+'psu{}/max_output_power'},
   #not supported{'src_node' : common.I2C_PREFIX+'{}/psu_{}_status',          'dest_node' : PSU_PREFIX+'psu{}/status'},
   #not supported{'src_node' : common.I2C_PREFIX+'{}/pwr_led',          'dest_node' : PSU_PREFIX+'psu{}/led_status'},
   #not supported{'src_node' : common.I2C_PREFIX+'{}/psu_type',          'dest_node' : PSU_PREFIX+'psu{}/type'},
   #not supported{'src_node' : common.I2C_PREFIX+'{}/sensor_num',          'dest_node' : PSU_PREFIX+'psu{}/num_temp_sensors'},
   #not supported{'src_node' : common.I2C_PREFIX+'{}/fan1_speed',          'dest_node' : PSU_PREFIX+'psu{}/fan_speed'},
   #not supported{'src_node' : common.I2C_PREFIX+'{}/vol_out_max',          'dest_node' : PSU_PREFIX+'psu{}/max_output_vol'},
   #not supported{'src_node' : common.I2C_PREFIX+'{}/vol_out_min',          'dest_node' : PSU_PREFIX+'psu{}/min_output_vol'}
]
PSU_ATTR_NODE_DICT_2 = [
   #not supported{'src_node' : common.I2C_PREFIX+'{}/psu_alarm',          'dest_node' : PSU_PREFIX+'psu{}/alarm'},
   #not supported{'src_node' : common.I2C_PREFIX+'{}/curr_in_max',          'dest_node' : PSU_PREFIX+'psu{}/alarm_threshold_curr'},
   #not supported{'src_node' : common.I2C_PREFIX+'{}/vol_in_max',          'dest_node' : PSU_PREFIX+'psu{}/alarm_threshold_vol'},
   #not supported{'src_node' : common.I2C_PREFIX+'{}/manuafacture_part_num',          'dest_node' : PSU_PREFIX+'psu{}/part_number'},
   #not supported{'src_node' : common.I2C_PREFIX+'{}/power_out_max',          'dest_node' : PSU_PREFIX+'psu{}/max_output_power'},
   {'src_node' : common.I2C_PREFIX+'{}/curr1_input',          'dest_node' : PSU_PREFIX+'psu{}/in_curr'},
   {'src_node' : common.I2C_PREFIX+'{}/in1_input',          'dest_node' : PSU_PREFIX+'psu{}/in_vol'},
   {'src_node' : common.I2C_PREFIX+'{}/power1_input',          'dest_node' : PSU_PREFIX+'psu{}/in_power'},
   {'src_node' : common.I2C_PREFIX+'{}/curr2_input',          'dest_node' : PSU_PREFIX+'psu{}/out_curr'},
   {'src_node' : common.I2C_PREFIX+'{}/in2_input',          'dest_node' : PSU_PREFIX+'psu{}/out_vol'},
   {'src_node' : common.I2C_PREFIX+'{}/power2_input',          'dest_node' : PSU_PREFIX+'psu{}/out_power'},
   #not supported{'src_node' : common.I2C_PREFIX+'{}/psu_{}_status',          'dest_node' : PSU_PREFIX+'psu{}/status'},
   #not supported{'src_node' : common.I2C_PREFIX+'{}/pwr_led',          'dest_node' : PSU_PREFIX+'psu{}/led_status'},
   #not supported{'src_node' : common.I2C_PREFIX+'{}/psu_type',          'dest_node' : PSU_PREFIX+'psu{}/type'},
   #not supported{'src_node' : common.I2C_PREFIX+'{}/sensor_num',          'dest_node' : PSU_PREFIX+'psu{}/num_temp_sensors'},
   #not supported{'src_node' : common.I2C_PREFIX+'{}/fan1_speed',          'dest_node' : PSU_PREFIX+'psu{}/fan_speed'},
   #not supported{'src_node' : common.I2C_PREFIX+'{}/vol_out_max',          'dest_node' : PSU_PREFIX+'psu{}/max_output_vol'},
   #not supported{'src_node' : common.I2C_PREFIX+'{}/vol_out_min',          'dest_node' : PSU_PREFIX+'psu{}/min_output_vol'}
]
PSU_SENSOR_NODE_DICT = [
    {'src_node' : common.I2C_PREFIX+'{}/temp{}_label',      'dest_node' : PSU_PREFIX+'psu{}/temp{}/temp_alias'},
    {'src_node' : common.I2C_PREFIX+'{}/temp{}_type',      'dest_node' : FAN_PREFIX+'psu{}/temp{}/temp_type'},
    {'src_node' : common.I2C_PREFIX+'{}/temp{}_max',      'dest_node' : PSU_PREFIX+'psu{}/temp{}/temp_max'},
    {'src_node' : common.I2C_PREFIX+'{}/temp{}_crit',      'dest_node' : PSU_PREFIX+'psu{}/temp{}/temp_max_hyst'},
    {'src_node' : common.I2C_PREFIX+'{}/temp{}_input',      'dest_node' : PSU_PREFIX+'psu{}/temp{}/temp_input'},
]

CPLD_TOP_NODE_DICT = [
    {'src_node':'/sys/module/clounix_fpga_cpld/parameters/loglevel', 'dest_node' : CPLD_PREFIX+'loglevel'},
    {'src_node':common.FPGA_PREFIX+'/debug_cpld',             'dest_node' : CPLD_PREFIX+'debug'},
    {'src_node':common.FPGA_PREFIX+'/cpld_num',               'dest_node' : CPLD_PREFIX+'num'},
    {'src_node' : common.FPGA_PREFIX+'/cpld_type',            'dest_node' : CPLD_PREFIX+'type'}
]
CPLD_DETAIL_NODE_DICT = [
    {'src_node' : common.FPGA_PREFIX+'/cpld{}_reset',      'dest_node' : CPLD_PREFIX+'cpld{}/cpld_reset'},
    {'src_node' : common.FPGA_PREFIX+'/cpld{}_enable',     'dest_node' : CPLD_PREFIX+'cpld{}/cpld_enable'},
    {'src_node' : common.FPGA_PREFIX+'/cpld{}_ver',     'dest_node' : CPLD_PREFIX+'cpld{}/cpld_ver'}
]

def linkDeviceNode(node_type):
    if node_type == 'sensor':
        num = str(device.THERMAL_SENSOR_NUM)
        cmd = device.SYSSENSOR_PREFIX + "num_temp_sensors " + num
        common.doBash("echo add " + cmd + " > /sys/switch/clounix_cmd")
        for sensor_index in range(device.THERMAL_SENSOR_NUM):
            thermal_info = device.THERMAL_COMBINATION[sensor_index]
            for work_index in range(device.NEED_WORK):
                src_path = thermal_info[device.PATH_INDEX] + thermal_info[work_index]
                dst_path = device.SENSOR_PATH.format(sensor_index)
                dst_path = dst_path + device.THERMAL_NODE_COMBINATION[work_index]
                if work_index < 3 :
                    common.doBash("echo " + src_path + " " + dst_path + " > /sys/switch/clounix_cmd")
                else :
                    common.doBash("echo " + "add " + dst_path + " " + thermal_info[work_index] + " > /sys/switch/clounix_cmd")

    elif node_type == 'transceiver':
        #transceiver top level
        for node in TRANSCIEVER_TOP_NODE_DICT:
            #check whether the soft link exists
            if not os.path.exists(node['dest_node']) :
                status, output = common.doBash("echo "+node['src_node']+' '+node['dest_node']+" > /sys/switch/clounix_cmd")
       #transceiver  port node
        for port_num in range(device.SFP_MIN_NUM + 1, device.TOTAL_PORT_NUM + 1):
            for node in TRANSCIEVER_PORT_NODE_DICT:
                #check whether the link exists
                if not os.path.exists(node['dest_node'].format(port_num)) :
                    status, output = common.doBash("echo "+node['src_node'].format(port_num)+' '+node['dest_node'].format(port_num)+" > /sys/switch/clounix_cmd")

        #transceiver QDD port node
        for port_num in range(device.QSFP_MIN_NUM + 1, device.TOTAL_PORT_NUM + 1):
            for node in TRANSCIEVER_QDDPORT_NODE_DICT:
                #check whether the link exists
                if not os.path.exists(node['dest_node'].format(port_num)) :
                    status, output = common.doBash("echo "+node['src_node'].format(port_num)+' '+node['dest_node'].format(port_num)+" > /sys/switch/clounix_cmd")
    elif node_type == 'fan':
        for node in FAN_TOP_NODE_DICT:
            #check whether the soft link exists
            if not os.path.exists(node['dest_node']) :
                status, output = common.doBash("echo "+node['src_node']+' '+node['dest_node']+" > /sys/switch/clounix_cmd")

        for fan_num in range(1, device.FAN_NUM + 1):
            for node in FAN_ATTR_NODE_DICT:
                if not os.path.exists(node['dest_node'].format(fan_num)) :
                    status, output = common.doBash("echo "+node['src_node'].format(fan_num)+' '+node['dest_node'].format(fan_num)+" > /sys/switch/clounix_cmd")
                    #fan motor level
                    for motor_num in range(0, device.MOTOR_NUM_PER_FAN):
                        for motor_node in FAN_MOTOR_NODE_DICT:
                            if not os.path.exists(motor_node['dest_node'].format(fan_num,motor_num)) :
                                status, output = common.doBash("echo "+motor_node['src_node'].format(fan_num)+' '+motor_node['dest_node'].format(fan_num,motor_num)+" > /sys/switch/clounix_cmd")
    elif node_type == 'cpld':
        for node in CPLD_TOP_NODE_DICT:
            #check whether the soft link exists
            if not os.path.exists(node['dest_node']) :
                status, output = common.doBash("echo "+node['src_node']+' '+node['dest_node']+" > /sys/switch/clounix_cmd")

        for cpld_num in range(1, device.CPLD_NUM + 1):
            for node in CPLD_DETAIL_NODE_DICT:
                #check whether the link exists
                if not os.path.exists(node['dest_node'].format(cpld_num)) :
                    status, output = common.doBash("echo "+node['src_node'].format(cpld_num)+' '+node['dest_node'].format(cpld_num)+" > /sys/switch/clounix_cmd")
    elif node_type == 'psu':
        for node in PSU_TOP_NODE_DICT:
            #check whether the soft link exists
            if not os.path.exists(node['dest_node']) :
                status, output = common.doBash("echo "+node['src_node']+' '+node['dest_node']+" > /sys/switch/clounix_cmd")

        for psu_num in range(1, device.PSU_NUM + 1):
            if psu_num == 1:
                bus1 = '1-0058/hwmon/hwmon12'
                bus2 = '1-0058/hwmon/hwmon13'
            elif psu_num == 2:
                if os.path.exists(common.I2C_PREFIX + "1-0058/hwmon/hwmon12"):
                    bus1 = '1-005a/hwmon/hwmon14'
                    bus2 = '1-005a/hwmon/hwmon15'
                else:
                    bus1 = '1-0058/hwmon/hwmon12'
                    bus2 = '1-0058/hwmon/hwmon13'
            
            for node in PSU_ATTR_NODE_DICT_1:
                if not os.path.exists(node['dest_node'].format(psu_num)) :
                    status, output = common.doBash("echo "+node['src_node'].format(bus1)+' '+node['dest_node'].format(psu_num)+" > /sys/switch/clounix_cmd")
            for node in PSU_ATTR_NODE_DICT_2:
                if not os.path.exists(node['dest_node'].format(psu_num)) :
                    status, output = common.doBash("echo "+node['src_node'].format(bus2)+' '+node['dest_node'].format(psu_num)+" > /sys/switch/clounix_cmd")

            for node in PSU_STAT_NODE_DICT:
                if not os.path.exists(node['dest_node'].format(psu_num)) :
                    status, output = common.doBash("echo "+node['src_node'].format(bus1, psu_num)  + ' ' + node['dest_node'].format(psu_num) + " > /sys/switch/clounix_cmd")

            for temp_sensor_num in range(0, device.TEMP_SENSOR_NUM_PER_PSU):
                for temp_sensor_node in PSU_SENSOR_NODE_DICT:
                    if not os.path.exists(temp_sensor_node['dest_node'].format(psu_num,temp_sensor_num)) :
                        status, output = common.doBash("echo "+temp_sensor_node['src_node'].format(bus2,temp_sensor_num+1)+' '+temp_sensor_node['dest_node'].format(psu_num,temp_sensor_num)+" > /sys/switch/clounix_cmd")

    elif node_type == 'syseeprom':
        for node in SYSEEPROM_TOP_NODE_DICT:
            #check whether the soft link exists
            if not os.path.exists(node['dest_node']) :
                status, output = common.doBash("echo "+node['src_node']+' '+node['dest_node']+" > /sys/switch/clounix_cmd")

    elif node_type == 'watchdog':
        for node in WATCHDOG_TOP_NODE_DICT:
            #check whether the soft link exists
            if not os.path.exists(node['dest_node']) :
                status, output = common.doBash("echo "+node['src_node']+' '+node['dest_node']+" > /sys/switch/clounix_cmd")

    elif node_type == 'sysled':
        for node in SYSLED_TOP_NODE_DICT:
            #check whether the soft link exists
            if not os.path.exists(node['dest_node']) :
                status, output = common.doBash("echo "+node['src_node']+' '+node['dest_node']+" > /sys/switch/clounix_cmd")

    common.doBash("echo 1 > " + common.FPGA_PREFIX + "front_sys_stat_led")
    common.doBash("echo 1 > " + common.FPGA_PREFIX + "front_location_led")
    common.doBash("echo 1 > " + common.FPGA_PREFIX + "back_location_led")


def deleteLinkDeviceNode(node_type):
    if node_type == 'sensor':
        cmd = device.SYSSENSOR_PREFIX + "num_temp_sensors"
        common.doBash("echo del " + cmd + " > /sys/swtich/clounix_cmd")
        for sensor_index in range(device.THERMAL_SENSOR_NUM):
            for work_index in range(device.NEED_WORK):
                dst_path = device.SENSOR_PATH.format(sensor_index)
                dst_path = dst_path + device.THERMAL_NODE_COMBINATION[work_index]
                common.doBash("del " + dst_path + " > /sys/switch/clounix_cmd")

    elif node_type == 'transceiver':
        #transceiver
        for node in TRANSCIEVER_TOP_NODE_DICT:
            #check whether the link exists
            if os.path.exists(node['dest_node']) :
                status, output = common.doBash("echo del "+node['dest_node']+" > /sys/switch/clounix_cmd")
        for port_num in range(device.QSFP_MIN_NUM + 1, device.TOTAL_PORT_NUM + 1):
            for node in TRANSCIEVER_QDDPORT_NODE_DICT:
                #check whether the link exists
                if os.path.exists(node['dest_node'].format(port_num)) :
                    #del /sys/switch/transceiver/eth*/*
                    status, output = common.doBash("echo del "+node['dest_node'].format(port_num)+" > /sys/switch/clounix_cmd")

        for port_num in range(device.SFP_MIN_NUM + 1, device.TOTAL_PORT_NUM + 1):
            for node in TRANSCIEVER_PORT_NODE_DICT:
                #check whether the link exists
                if os.path.exists(node['dest_node'].format(port_num)) :
                    #del /sys/switch/transceiver/eth*/*
                    status, output = common.doBash("echo del "+node['dest_node'].format(port_num)+" > /sys/switch/clounix_cmd")
            #del /sys/switch/transceiver/eth*
            status, output = common.doBash("echo del /sys/switch/transceiver/eth{}".format(port_num)+" > /sys/switch/clounix_cmd")
    elif node_type == 'fan':
        #fan
        for node in FAN_TOP_NODE_DICT:
            #check whether the link exists
            if os.path.exists(node['dest_node']) :
                status, output = common.doBash("echo del "+node['dest_node']+" > /sys/switch/clounix_cmd")

        for fan_num in range(1, device.FAN_NUM + 1):
            for node in FAN_ATTR_NODE_DICT:
                for motor_num in range(0, device.MOTOR_NUM_PER_FAN):
                    for motor_node in FAN_MOTOR_NODE_DICT:
                        #del /sys/switch/fan/fan*/motor*/*
                        if os.path.exists(motor_node['dest_node'].format(fan_num,motor_num)) :
                            status, output = common.doBash("echo del "+motor_node['dest_node'].format(fan_num,motor_num)+" > /sys/switch/clounix_cmd")
                #del /sys/switch/fan/fan*/motor*
                if os.path.exists("/sys/switch/fan/fan{}/motor{}".format(fan_num,motor_num)) :
                    status, output = common.doBash("echo del /sys/switch/fan/fan{}/motor{}".format(fan_num,motor_num)+" > /sys/switch/clounix_cmd")
                #check whether the link exists
                if os.path.exists(node['dest_node'].format(fan_num)) :
                    #del /sys/switch/fan/fan*/*
                    status, output = common.doBash("echo del "+node['dest_node'].format(fan_num)+" > /sys/switch/clounix_cmd")
            #del /sys/switch/fan/fan*
            status, output = common.doBash("echo del /sys/switch/fan/fan{}".format(fan_num)+" > /sys/switch/clounix_cmd")
    elif node_type == 'cpld':
        #transceiver
        for node in CPLD_TOP_NODE_DICT:
            #check whether the link exists
            if os.path.exists(node['dest_node']) :
                status, output = common.doBash("echo del "+node['dest_node']+" > /sys/switch/clounix_cmd")

        for cpld_num in range(1, device.CPLD_NUM + 1):
            for node in CPLD_DETAIL_NODE_DICT:
                #check whether the link exists
                if os.path.exists(node['dest_node'].format(cpld_num)) :
                    #del /sys/switch/cpld/cpld*/*
                    status, output = common.doBash("echo del "+node['dest_node'].format(cpld_num)+" > /sys/switch/clounix_cmd")
            #del /sys/switch/cpld/cpld*
            status, output = common.doBash("echo del /sys/switch/cpld/cpld{}".format(cpld_num)+" > /sys/switch/clounix_cmd")
    elif node_type == 'psu':
        #psu
        for node in PSU_TOP_NODE_DICT:
            #check whether the link exists
            if os.path.exists(node['dest_node']) :
                status, output = common.doBash("echo del "+node['dest_node']+" > /sys/switch/clounix_cmd")

        for psu_num in range(1, device.PSU_NUM + 1):
            for node in PSU_ATTR_NODE_DICT:
                for temp_sensor_num in range(0, device.MOTOR_NUM_PER_FAN):
                    for motor_node in PSU_SENSOR_NODE_DICT:
                        #del /sys/switch/psu/psu*/temp*/*
                        if os.path.exists(motor_node['dest_node'].format(psu_num,temp_sensor_num)) :
                            status, output = common.doBash("echo del "+motor_node['dest_node'].format(psu_num,temp_sensor_num)+" > /sys/switch/clounix_cmd")
                #del /sys/switch/psu/psu*/temp*
                if os.path.exists("/sys/switch/psu/psu{}/temp{}".format(psu_num,temp_sensor_num)) :
                    status, output = common.doBash("echo del /sys/switch/psu/psu{}/temp{}".format(psu_num,temp_sensor_num)+" > /sys/switch/clounix_cmd")
                #check whether the link exists
                if os.path.exists(node['dest_node'].format(psu_num)) :
                    #del /sys/switch/psu/psu*/*
                    status, output = common.doBash("echo del "+node['dest_node'].format(psu_num)+" > /sys/switch/clounix_cmd")
            #del /sys/switch/psu/psu*
            status, output = common.doBash("echo del /sys/switch/psu/psu{}".format(psu_num)+" > /sys/switch/clounix_cmd")

    elif node_type == 'syseeprom':
        for node in SYSEEPROM_TOP_NODE_DICT:
            #check whether the link exists
            if os.path.exists(node['dest_node']) :
                status, output = common.doBash("echo del "+node['dest_node']+" > /sys/switch/clounix_cmd")

    elif node_type == 'watchdog':
        for node in WATCHDOG_TOP_NODE_DICT:
            #check whether the link exists
            if os.path.exists(node['dest_node']) :
                status, output = common.doBash("echo del "+node['dest_node']+" > /sys/switch/clounix_cmd")

    elif node_type == 'sysled':
        for node in SYSLED_TOP_NODE_DICT:
            #check whether the link exists
            if os.path.exists(node['dest_node']) :
                status, output = common.doBash("echo del "+node['dest_node']+" > /sys/switch/clounix_cmd")

def blink_sys_led():
    if os.path.exists("/proc/vmcore"):
        common.doBash("echo 0 > " + common.FPGA_PREFIX + "front_sys_stat_led")

def doInstall():
    status, output = common.doBash("depmod -a")
    status, output = common.doBash("rmmod hid_cp2112")
    checkDriver()
    blink_sys_led()

    for o in i2c_topology_dict:
        status, output = common.doBash("echo " + o['driver'] + " " + o['address'] + " > " + common.I2C_PREFIX + o['bus'] + "/new_device")

    for node_type in SOFT_LINK_NODE_TYPE:
        linkDeviceNode(node_type)
    return

def setupThreads():
    global THREADS, QUEUES

    # Queues
    # Global
    queueGlobal = common.Queue.Queue()
    QUEUES.append(queueGlobal)
    FUNCS['global'] = queueGlobal

    # Device
    queueDevice = common.Queue.Queue()
    QUEUES.append(queueDevice)
    FUNCS['device'] = queueDevice

    # Threads
    # Global
    threadGlobal = GlobalThread('Global Handler', queueGlobal)
    THREADS.append(threadGlobal)

    # Device
    threadDevice = device.DeviceThread('Device Handler', queueDevice)
    THREADS.append(threadDevice)

    # Check platform status
    threadPlatformStatus = device.PlatformStatusThread('Platform Status Handler', 0.3)
    THREADS.append(threadPlatformStatus)

def functionInit():
    setupThreads()
    for thread in THREADS:
        thread.start()
    return

def deviceInit():
    msg = messageObject(['init'], None)
    FUNCS['device'].put(msg)
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
    for node_type in SOFT_LINK_NODE_TYPE:
        deleteLinkDeviceNode(node_type)
    for o in i2c_topology_dict:
        status, output = common.doBash("echo " + o['address'] + " > " + common.I2C_PREFIX + o['bus'] + "/delete_device")
    for i in range(len(KERNEL_MODULE)-1, -1, -1):
        status, output = common.doBash("modprobe -rq " + KERNEL_MODULE[i])
    return

def main(): #start 20200219
    args = common.sys.argv[1:]

    if len(args[0:]) < 1:
        common.sys.exit(0)

    if args[0] == 'install':
        common.RUN = True
        #doInstall()
        os.system("/usr/local/bin/s3ip_sysfs_tool.sh start")
        do_platformApiInit()
        do_platformApiInstall()
        functionInit()
        deviceInit()
        messageHandler()

    common.sys.exit(0)

if __name__ == "__main__":
    main()
