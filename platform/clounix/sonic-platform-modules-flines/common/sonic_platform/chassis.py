#!/usr/bin/env python

#############################################################################
# PDDF
# Module contains an implementation of SONiC Chassis API
#
#############################################################################

try:
    import os
    import sys
    import subprocess
    import time
    import datetime
    import syslog
    from sonic_platform_pddf_base.pddf_chassis import PddfChassis
    from sonic_platform.watchdog import Watchdog
    from sonic_platform.thermal import Thermal
    from .helper import APIHelper
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

SFP_STATUS_INSERTED = '1'
SFP_STATUS_REMOVED = '0'
REBOOT_HISTORY_DIR = "/var/log/reboot-cause"
REBOOT_HISTORY_FILE = "/var/log/reboot-cause/history"
REBOOT_CAUSE_FILE = "/host/reboot-cause/reboot-cause.txt"

def get_platform_cpu_num():
    cpu_sensor_label = []
    match = []
    try:        
        from re import findall
        from subprocess import getstatusoutput
        
        # Try Intel path first
        cmd = "ls /sys/devices/platform/*coretemp*/hwmon/hwmon*/temp*_input 2>/dev/null"
        _, ret = getstatusoutput(cmd)
        match = findall(r".*temp[0-9]*_input", ret)
        
        # If Intel not found, try Phytium path
        if not match:
            cmd = "ls /sys/devices/platform/PHYT0008:00/PHYT000D:00/hwmon/hwmon*/temp*_input 2>/dev/null"
            _, ret = getstatusoutput(cmd)
            match = findall(r".*temp[0-9]*_input", ret)
        
        for node in match:
            node = node.replace('coretemp', 'core')
            index = node.find('temp')
            node = node[index:]
            node = node.replace('temp', '')
            node = node.replace('_input', '')
            cpu_sensor_label.append(int(node))
    except:
        pass
    cpu_sensor_label.sort()
    return len(match), cpu_sensor_label

class Chassis(PddfChassis):
    """
    PDDF Platform-specific Chassis class
    """
    REBOOT_CAUSE_CPU_COLD_RESET = "CPU Cold Reset"
    REBOOT_CAUSE_CPU_WARM_RESET = "CPU Warm Reset"
    REBOOT_CAUSE_BIOS_RESET = "BIOS Reset"
    REBOOT_CAUSE_PSU_SHUTDOWN = "PSU Shutdown"
    REBOOT_CAUSE_BMC_SHUTDOWN = "BMC Shutdown"
    def __init__(self, pddf_data=None, pddf_plugin_data=None):
        PddfChassis.__init__(self, pddf_data, pddf_plugin_data)

        self.__api_helper = APIHelper()
        self.chassis_conf = self.__api_helper.get_attr_conf("chassis")
        self.__initialize_components()

        # CORE THERMALs
        cpu_num, self.cpu_sensor_label_list = get_platform_cpu_num()
        for i in range(cpu_num):
            thermal = Thermal(0, self.pddf_obj, self.plugin_data)
            thermal.thermal_index = self.cpu_sensor_label_list[i]
            if i == 0:
                thermal.thermal_obj_name = "CPU_Package"
            else:
                thermal.thermal_obj_name = "CPU_Core_{}".format(i-1)
            thermal.thermal_obj = {}
            thermal.is_core_thermal = True
            self._thermal_list.append(thermal)
        
        # fpga pvt thermal
        num = self.platform_inventory['num_fpga_pvt_temp'] if 'num_fpga_pvt_temp' in self.platform_inventory else 0
        pvt_chip_type = 'NA'
        if num != 0:
            pvt_chip_type = self.platform_inventory['fpga_pvt_temp_type']

        for i in range(num):
            thermal = Thermal(0, self.pddf_obj, self.plugin_data)
            thermal.thermal_index = i + 1
            thermal.thermal_obj_name = "FPGA_PVT_TEMP_{}".format(i + 1)
            thermal.thermal_obj = None
            thermal.is_fpga_pvt_thermal = True
            thermal.pvt_chip_type = pvt_chip_type
            self._thermal_list.append(thermal)
        
    # Provide the functions/variables below for which implementation is to be overwritten
    def get_watchdog(self):
        """
        Retreives hardware watchdog device on this chassis
        Returns:
            An object derived from WatchdogBase representing the hardware
            watchdog device
        """
        if self._watchdog is None:
            self._watchdog = Watchdog()

        return self._watchdog
    def _generate_reboot_record_name(self):
        current_time = datetime.datetime.now(datetime.timezone.utc)
        return current_time.strftime("%Y_%m_%d_%H_%M_%S")

    def _write_reboot_history(self, cause, user="N/A", comment="N/A"):
        try:
            if not os.path.exists(REBOOT_HISTORY_DIR):
                os.makedirs(REBOOT_HISTORY_DIR, exist_ok=True)
            
            record_name = self._generate_reboot_record_name()
            current_time = datetime.datetime.now(datetime.timezone.utc)
            timestamp = current_time.strftime("%Y-%m-%d %H:%M:%S UTC")
            
            header = "Name,Cause,Time,User,Comment\n"
            record = f"{record_name},{cause},{timestamp},{user},{comment}\n"
            
            existing_lines = []
            if os.path.exists(REBOOT_HISTORY_FILE):
                with open(REBOOT_HISTORY_FILE, 'r') as f:
                    existing_content = f.read()
                lines = existing_content.strip().split('\n')
                if len(lines) > 1 and lines[0].startswith("Name"):
                     existing_lines = lines[1:]

            new_data_lines = [record.strip()] + existing_lines[:9]
            content = header + '\n'.join(new_data_lines) + '\n'

            with open(REBOOT_HISTORY_FILE, 'w') as f:
                f.write(content)
                f.flush()
                os.fsync(f.fileno())

            syslog.syslog(syslog.LOG_DEBUG,
                        f"Reboot cause written and fsynced to {REBOOT_HISTORY_FILE}")

        except Exception as e:
            syslog.syslog(syslog.LOG_ERR, f"Failed to write reboot history: {e}")

    def find_software_reboot_cause_from_reboot_cause_file(self):
        software_reboot_cause = "Unknown"
        if os.path.isfile(REBOOT_CAUSE_FILE):
            with open(REBOOT_CAUSE_FILE) as cause_file:
                software_reboot_cause = cause_file.readline().rstrip('\n')

        return software_reboot_cause

    def _get_onie_machine(self):
        try:
            with open('/host/machine.conf', 'r') as f:
                for line in f:
                    if line.startswith('onie_machine='):
                        return line.split('=', 1)[1].strip()
        except Exception:
            pass
        return ''

    def get_reboot_cause(self):
    
        THERMAL_OVERLOAD_POSITION_FILE = "/host/reboot-cause/platform/thermal_overload_position"
        ADDITIONAL_FAULT_CAUSE_FILE = "/host/reboot-cause/platform/additional_fault_cause"

        reboot_cause = (self.REBOOT_CAUSE_NON_HARDWARE, "Unknown")

        platform = self._get_onie_machine()
        is_lpc_platform = 'ds410g_48y8c' in platform

        cpu_rst_val = None
        try:
            if is_lpc_platform:
                addr_file = '/sys/kernel/lpc_cpld/addr'
                data_file = '/sys/kernel/lpc_cpld/data'

                with open(addr_file, 'w') as f:
                    time.sleep(0.1)
                    f.write('0x11')
                with open(data_file, 'r') as f:
                    time.sleep(0.1)
                    raw = f.read().strip()
                if raw:
                    cpu_rst_val = raw
                with open(data_file, 'w') as f:
                    time.sleep(0.1)
                    f.write('0xff')
                with open(addr_file, 'w') as f:
                    time.sleep(0.1)
                    f.write('0x00')
            else:
                ret, val = subprocess.getstatusoutput("i2cget -y -a 1 0x7f 0x7")
                if ret == 0 and val:
                    cpu_rst_val = val.strip()
                time.sleep(1)
                subprocess.getstatusoutput("i2cset -y -a 1 0x7f 0x7 0xff")
        except Exception:
            pass

        sw_cause = self.find_software_reboot_cause_from_reboot_cause_file()

        if os.path.isfile(THERMAL_OVERLOAD_POSITION_FILE):
            thermal_overload_pos = self.__api_helper.read_one_line_file(
                THERMAL_OVERLOAD_POSITION_FILE) or "Unknown"
            if thermal_overload_pos != "Unknown":
                str = thermal_overload_pos
                if str.find('CPU_') >= 0:
                    reboot_cause = (
                        self.REBOOT_CAUSE_THERMAL_OVERLOAD_CPU, 'Thermal Overload: CPU')
                elif str.find('ASIC') >= 0:
                    reboot_cause = (
                        self.REBOOT_CAUSE_THERMAL_OVERLOAD_ASIC, 'Thermal Overload: ASIC')
                else:
                    reboot_cause = (
                        self.REBOOT_CAUSE_THERMAL_OVERLOAD_OTHER, thermal_overload_pos)

                os.remove(THERMAL_OVERLOAD_POSITION_FILE)
                self._write_reboot_history(reboot_cause[1])
                return reboot_cause

        try:
            bus = int(self.pddf_obj.data['RC_EEPROM']['i2c']['topo_info']['parent_bus'], 0)
            dev_addr = int(self.pddf_obj.data['RC_EEPROM']['i2c']['topo_info']['dev_addr'], 0)
            REBOOT_EEPROM_PATH = '/sys/bus/i2c/devices/{}-{:04x}/eeprom'.format(bus, dev_addr)
            with open(REBOOT_EEPROM_PATH, 'rb+') as binfile:
                binfile.seek(0)
                raw_byte = binfile.read(1)
                hw_reboot_cause = raw_byte.hex().zfill(2)
                print(f"hw_reboot_cause: {hw_reboot_cause}")
                if (hw_reboot_cause != 'ff'):
                    reboot_cause = {
                        '00': (self.REBOOT_CAUSE_NON_HARDWARE, 'Non-Hardware'),
                        '01': (self.REBOOT_CAUSE_POWER_LOSS, 'Power Loss'),
                        '02': (self.REBOOT_CAUSE_THERMAL_OVERLOAD_CPU, 'Thermal Overload: CPU'),
                        '03': (self.REBOOT_CAUSE_THERMAL_OVERLOAD_ASIC, 'FPGA PVT Overload: ASIC'),
                        '04': (self.REBOOT_CAUSE_THERMAL_OVERLOAD_OTHER, 'Thermal Overload: Other'),
                        '05': (self.REBOOT_CAUSE_INSUFFICIENT_FAN_SPEED, 'Insufficient Fan Speed'),
                        '06': (self.REBOOT_CAUSE_WATCHDOG, 'Watchdog'),
                        '07': (self.REBOOT_CAUSE_HARDWARE_OTHER, 'Hardware - Other'),
                        '08': (self.REBOOT_CAUSE_CPU_COLD_RESET, 'CPU Cold Reset'),
                        '09': (self.REBOOT_CAUSE_CPU_WARM_RESET, 'CPU Warm Reset'),
                        '10': (self.REBOOT_CAUSE_BIOS_RESET, 'BIOS Reset'),
                        '11': (self.REBOOT_CAUSE_PSU_SHUTDOWN, 'PSU Shutdown'),
                        '12': (self.REBOOT_CAUSE_BMC_SHUTDOWN, 'BMC Shutdown')
                    }.get(hw_reboot_cause, (self.REBOOT_CAUSE_HARDWARE_OTHER, f'Hardware - Other (0x{hw_reboot_cause})'))

                    try:
                        binfile.seek(0)
                        binfile.write(bytes([0xff]))
                        binfile.flush()
                    except Exception as e:
                        syslog.syslog(syslog.LOG_WARNING, f"Failed to clear reboot_cause: {e}")

                    self._write_reboot_history(reboot_cause[1])
                    return reboot_cause
        except Exception as e:
            syslog.syslog(syslog.LOG_ERR, f"Failed to read hardware reboot cause: {e}")

        SYS_POWER_STATUS_HISTORY_PATH = os.popen('find /sys -name power_history_record 2>/dev/null').read().strip()
        SYS_POWER_STATUS_CTRL_PATH = os.popen('find /sys -name ctrl_history_record 2>/dev/null').read().strip()

        if len(SYS_POWER_STATUS_HISTORY_PATH) == 0 or len(SYS_POWER_STATUS_CTRL_PATH) == 0:
            print("no power history record node find, pls check driver")
        elif reboot_cause[1] == "Unknown" and os.path.isfile(SYS_POWER_STATUS_HISTORY_PATH):
            try:
                self.__api_helper.write_txt_file(SYS_POWER_STATUS_CTRL_PATH, "1")
                time.sleep(0.5)
                power_status_history = self.__api_helper.read_one_line_file(SYS_POWER_STATUS_HISTORY_PATH).strip()

                if power_status_history and power_status_history.lower() != '0xffff':
                    raw_val = int(power_status_history, 16)

                    POWER_RAILS = {
                        15: 'PSU Shutdown',
                        14: 'P1V0_STBY',
                        13: 'P1V8_STB',
                        12: 'P3V3_STB',
                        11: 'P1V0_MGT_STBY',
                        10: 'P1V2_MGT_STBY',
                        9: 'P5V',
                        8: 'CPU Cold Reset',
                        7: 'MAC_P1V8_VDDIO',
                        6: 'MAC_P0V8_VDDK',
                        5: 'MAC_P1V25_AVDD',
                        4: 'MAC_P1V8_AVDDH',
                        3: 'P1V8_CLK',
                        2: 'P3V3_CLK',
                        1: 'P3V3_SFP1',
                        0: 'P3V3_SFP2'
                    }

                    failed_rails = [name for bit, name in POWER_RAILS.items()
                                    if not (raw_val >> bit) & 1]

                    cmd_bits = []
                    if (raw_val >> 17) & 1:
                        cmd_bits.append('POWER CYCLE')
                    if (raw_val >> 18) & 1:
                        cmd_bits.append('POWER DELAY CYCLE')

                    detail_parts = [f'REG = 0x{raw_val:08X}']
                    if failed_rails:
                        detail_parts.append('FAIL = ' + ','.join(failed_rails))
                    if cmd_bits:
                        detail_parts.append('CMD = ' + ','.join(cmd_bits))

                    reboot_cause = (
                        self.REBOOT_CAUSE_POWER_LOSS,
                        'Power Loss: ' + ' | '.join(detail_parts)
                    )
                    self._write_reboot_history(reboot_cause[1])
                    return reboot_cause
            except Exception as e:
                syslog.syslog(syslog.LOG_ERR, f"FPGA power status read failed: {e}")
            finally:
                try:
                    time.sleep(0.5)
                    self.__api_helper.write_txt_file(SYS_POWER_STATUS_CTRL_PATH, "0")
                except Exception:
                    pass

        if cpu_rst_val is not None:
            if cpu_rst_val == '0xfe':
                reboot_cause = (self.REBOOT_CAUSE_CPU_COLD_RESET, 'CPU Cold Reset')
            elif cpu_rst_val == '0xff' and sw_cause == "Unknown": 
                reboot_cause = (self.REBOOT_CAUSE_CPU_WARM_RESET, 'CPU Warm Reset')
            if reboot_cause[1] != "Unknown":
                self._write_reboot_history(reboot_cause[1])
                return reboot_cause

        return reboot_cause
     
    def get_thermal_manager(self):
        return None
		
    def __initialize_components(self):
        from sonic_platform.component import Component

        for index in range(len(self.chassis_conf['components'])):
            component = Component(index,self.chassis_conf['components'])
            self._component_list.append(component) 
			
    @property
    def _get_presence_bitmap(self):

        bits = []

        for x in self._sfp_list:
          bits.append(str(int(x.get_presence())))

        rev = "".join(bits[::-1]) if len(bits) > 0 else "0"
        return int(rev,2)
    
    data = {'present':0}
    def get_transceiver_change_event(self, timeout=0):
        port_dict = {}

        if timeout == 0:
            cd_ms = sys.maxsize
        else:
            cd_ms = timeout

        #poll per second
        while cd_ms > 0:
            reg_value = self._get_presence_bitmap
            changed_ports = self.data['present'] ^ reg_value
            if changed_ports != 0:
                break
            time.sleep(1)
            cd_ms = cd_ms - 1000

        if changed_ports != 0:
            for port in range(0, len(self._sfp_list)):
                # Mask off the bit corresponding to our port
                mask = (1 << (port - 0))
                if changed_ports & mask:
                    if (reg_value & mask) == 0:
                        port_dict[port] = SFP_STATUS_REMOVED
                    else:
                        port_dict[port] = SFP_STATUS_INSERTED

            # Update cache
            self.data['present'] = reg_value
            return True, port_dict
        else:
            return True, {}
        return False, {}

    def get_change_event(self, timeout=0):
        res_dict = {
            'component': {},
            'fan': {},
            'module': {},
            'psu': {},
            'sfp': {},
            'thermal': {},
        }
        ''' get transceiver change event '''
        res_dict['sfp'].clear()
        status, res_dict['sfp'] = self.get_transceiver_change_event(timeout)
        return status, res_dict

    def initizalize_system_led(self):
        return True
    
    def get_status_led(self):
        return self.get_system_led('SYS_LED')
    
    def set_status_led(self, color):
        return self.set_system_led('SYS_LED',color)
