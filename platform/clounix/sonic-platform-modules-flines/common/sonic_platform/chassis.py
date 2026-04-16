#!/usr/bin/env python
#
# Name: chassis.py, version: 1.0
#
# Description: Module contains the definitions of SONiC platform APIs
#

try:
    import os
    import sys
    import time
    import syslog
    import datetime
    from sonic_platform_base.chassis_base import ChassisBase
    from sonic_platform.eeprom import Eeprom
    from sonic_platform.fan import Fan
    from sonic_platform.fan_drawer import FanDrawer
    from sonic_platform.psu import Psu
    from sonic_platform.sfp import Sfp
    from sonic_platform.thermal import Thermal
    from sonic_platform.component import Component
    from sonic_platform.watchdog import Watchdog
    from sonic_platform.sysled import SYSLED
    from sonic_platform.cpld import CPLD
    from sonic_platform.fpga import FPGA
    from .helper import APIHelper
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

SFP_STATUS_INSERTED = '1'
SFP_STATUS_REMOVED = '0'

REBOOT_CAUSE_FILE = "/host/reboot-cause/reboot-cause.txt"
THERMAL_OVERLOAD_POSITION_FILE = "/host/reboot-cause/platform/thermal_overload_position"
ADDITIONAL_FAULT_CAUSE_FILE = "/host/reboot-cause/platform/additional_fault_cause"
REBOOT_EEPROM_PATH = "/sys/s3ip/fpga/reboot_cause"
REBOOT_HISTORY_DIR = "/var/log/reboot-cause"
REBOOT_HISTORY_FILE = "/var/log/reboot-cause/history"
SYS_POWER_STATUS_HISTORY_PATH = "/sys/s3ip/fpga/fpga1/power_status_history"
SYS_POWER_STATUS_CTRL_PATH = "/sys/s3ip/fpga/fpga1/power_status_ctrl"


class Chassis(ChassisBase):

    REBOOT_CAUSE_CPU_COLD_RESET = "CPU Cold Reset"
    REBOOT_CAUSE_CPU_WARM_RESET = "CPU Warm Reset"
    REBOOT_CAUSE_BIOS_RESET = "BIOS Reset"
    REBOOT_CAUSE_PSU_SHUTDOWN = "PSU Shutdown"
    REBOOT_CAUSE_BMC_SHUTDOWN = "BMC Shutdown"

    def __init__(self):
        self.__api_helper = APIHelper()
        chassis_conf = self.__api_helper.get_attr_conf("chassis")
        self.__conf = chassis_conf

        self.__num_of_psus = len(chassis_conf['psus'])
        self.__num_of_sfps = len(chassis_conf['sfps'])
        self.__num_of_thermals = len(chassis_conf['thermals'])
        self.__num_of_components = len(chassis_conf['components'])
        self.__num_of_fans = len(chassis_conf['fans'])
        self.__num_of_fan_drawers = len(chassis_conf['fan_drawers'])
        self.__attr_led_path_prefix = '/sys/s3ip/sysled/'
        
        self._cpld_list = []
        self._fpga_list = []
        
        ChassisBase.__init__(self)

        try:
            self._eeprom = Eeprom()
        except Exception as e:
            syslog.syslog(syslog.LOG_WARNING, f"Chassis: EEPROM initialization failed: {e}")
            self._eeprom = None

        fan_conf = self.__conf['fans']

        for x in range(0, self.__num_of_fans):
            fan_conf[x].update({'container': 'chassis'})
            fan_conf[x].update({'container_index': 0})
            fan = Fan(x, fan_conf)
            self._fan_list.append(fan)

        for drawer_index in range(0, self.__num_of_fan_drawers):
            fan_drawer = FanDrawer(
                drawer_index, fandrawer_conf=chassis_conf['fan_drawers'])
            self._fan_drawer_list.append(fan_drawer)

        for index in range(0, self.__num_of_psus):
            psu = Psu(index, psu_conf=chassis_conf['psus'])
            self._psu_list.append(psu)

        for index in range(0, self.__num_of_sfps):
            sfp = Sfp(index, sfp_conf=chassis_conf['sfps'])
            self._sfp_list.append(sfp)

        thermal_conf = self.__conf['thermals']
        for x in range(0, self.__num_of_thermals):
            thermal_conf[x].update({'container': 'chassis'})
            thermal_conf[x].update({'container_index': 0})
            thermal = Thermal(x, thermal_conf)
            self._thermal_list.append(thermal)

        for index in range(0, self.__num_of_components):
            component = Component(
                index, component_conf=chassis_conf['components'])
            self._component_list.append(component)

        self._watchdog = Watchdog(watchdog_conf=chassis_conf['watchdog'])
        self._sysled = SYSLED()
        self.__initialize_cpld()
        self.__initialize_fpga()

##############################################
# Device methods
##############################################
    def _get_attr_val(self, attr_file, default='N/A'):
        attr_path = attr_file
        attr_value = default

        try:
            with open(attr_path, 'r') as fd:
                attr_value = fd.read().strip()
                valtype = type(default)
                if valtype == float:
                    attr_value = float(attr_value)
                elif valtype == int:
                    attr_value = int(attr_value)
        except Exception as e:
            syslog.syslog(syslog.LOG_WARNING, f"Error reading {attr_path}: {e}")
            attr_value = 'N/A'

        return attr_value

    def get_thermal_manager(self):
        from .thermal_manager import ThermalManager
        return ThermalManager

    def get_name(self):
        return self.__conf['name']

    def get_presence(self):
        return True

    def get_model(self):
        if self._eeprom is None:
            return "N/A"
        try:
            return self._eeprom.part_number_str()
        except Exception:
            return "N/A"

    def get_serial(self):
        if self._eeprom is None:
            return "N/A"
        try:
            return self._eeprom.serial_number_str()
        except Exception:
            return "N/A"

    def get_status(self):
        return True

##############################################
# Chassis methods
##############################################
    def get_position_in_parent(self):
        return -1

    def is_replaceable(self):
        return False

    def set_status_led(self, color):
        ret_val = False
        if color == "green":
            led_value = 1
        elif color == "red":
            led_value = 3
        else:
            return False
        ret_val = self.__api_helper.write_txt_file(
            self.__attr_led_path_prefix + 'sys_led_status', str(led_value))
        return ret_val

    def get_status_led(self):
        color = "off"
        attr_rv = self.__api_helper.read_one_line_file(
            self.__attr_led_path_prefix + 'sys_led_status')
        try:
            val = int(attr_rv, 16)
            if val == 0x0 or val == 0x1 or val == 0x04:
                color = "green"
            else:
                color = "red"
        except (ValueError, TypeError):
            color = "off"
        return color

    def get_eeprom(self):
        return self._eeprom

    def get_base_mac(self):
        if self._eeprom is None:
            return "N/A"
        try:
            return self._eeprom.base_mac_address()
        except Exception:
            return "N/A"

    def get_serial_number(self):
        if self._eeprom is None:
            return "N/A"
        try:
            return self._eeprom.serial_number_str()
        except Exception:
            return "N/A"

    def get_system_eeprom_info(self):
        if self._eeprom is None:
            return {}
        try:
            return self._eeprom.system_eeprom_info()
        except Exception:
            return {}

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
            
            if os.path.exists(REBOOT_HISTORY_FILE):
                with open(REBOOT_HISTORY_FILE, 'r') as f:
                    existing_content = f.read()
                lines = existing_content.strip().split('\n')
                if len(lines) > 1 and lines[0].startswith("Name"):
                    data_lines = lines[1:]
                    data_lines.insert(0, record.strip())
                    content = header + '\n'.join(data_lines[:10]) + '\n'
                else:
                    content = header + record
            else:
                content = header + record
            
            with open(REBOOT_HISTORY_FILE, 'w') as f:
                f.write(content)
                
        except Exception as e:
            syslog.syslog(syslog.LOG_ERR, f"Failed to write reboot history: {e}")

    def get_reboot_cause(self):
        syslog.syslog(syslog.LOG_DEBUG, "get_reboot_cause: Starting reboot cause detection")
        
        reboot_cause = (self.REBOOT_CAUSE_NON_HARDWARE, "Unknown")

        sw_reboot_cause = self.__api_helper.read_one_line_file(
            REBOOT_CAUSE_FILE) or "Unknown"
        syslog.syslog(syslog.LOG_DEBUG, f"get_reboot_cause: Software cause = {sw_reboot_cause}")
        
        if sw_reboot_cause != "Unknown" and sw_reboot_cause.strip():
            reboot_cause = (self.REBOOT_CAUSE_NON_HARDWARE, sw_reboot_cause.strip())
            self._write_reboot_history(sw_reboot_cause.strip())
            return reboot_cause

        if os.path.isfile(THERMAL_OVERLOAD_POSITION_FILE):
            thermal_overload_pos = self.__api_helper.read_one_line_file(
                THERMAL_OVERLOAD_POSITION_FILE)
            if "CPU" in thermal_overload_pos:
                reboot_cause = (self.REBOOT_CAUSE_THERMAL_OVERLOAD_CPU,
                                'Thermal Overload: CPU')
            elif "ASIC" in thermal_overload_pos:
                reboot_cause = (
                    self.REBOOT_CAUSE_THERMAL_OVERLOAD_ASIC, 'Thermal Overload: ASIC')
            else:
                reboot_cause = (
                    self.REBOOT_CAUSE_THERMAL_OVERLOAD_OTHER, thermal_overload_pos)
            os.remove(THERMAL_OVERLOAD_POSITION_FILE)
            self._write_reboot_history(reboot_cause[1])
            return reboot_cause

        try:
            with open(REBOOT_EEPROM_PATH, 'rb+') as binfile:
                binfile.seek(0)
                hw_reboot_cause = binfile.read(1).hex().zfill(2)

                if (hw_reboot_cause != 'ff'):
                    reboot_cause = {
                        '00': (self.REBOOT_CAUSE_NON_HARDWARE, 'Non-Hardware'),
                        '01': (self.REBOOT_CAUSE_POWER_LOSS, 'Power Loss'),
                        '02': (self.REBOOT_CAUSE_THERMAL_OVERLOAD_CPU, 'Thermal Overload: CPU'),
                        '03': (self.REBOOT_CAUSE_THERMAL_OVERLOAD_ASIC, 'Thermal Overload: ASIC'),
                        '04': (self.REBOOT_CAUSE_THERMAL_OVERLOAD_OTHER, 'Thermal Overload: Other'),
                        '05': (self.REBOOT_CAUSE_INSUFFICIENT_FAN_SPEED, 'Insufficient Fan Speed'),
                        '06': (self.REBOOT_CAUSE_WATCHDOG, 'Watchdog'),
                        '07': (self.REBOOT_CAUSE_HARDWARE_OTHER, 'Hardware - Other'),
                        '08': (self.REBOOT_CAUSE_CPU_COLD_RESET, 'CPU Cold Reset'),
                        '09': (self.REBOOT_CAUSE_CPU_WARM_RESET, 'CPU Warm Reset'),
                        '10': (self.REBOOT_CAUSE_BIOS_RESET, 'BIOS Reset'),
                        '11': (self.REBOOT_CAUSE_PSU_SHUTDOWN, 'PSU Shutdown'),
                        '12': (self.REBOOT_CAUSE_BMC_SHUTDOWN, 'BMC Shutdown')
                    }.get(hw_reboot_cause, (self.REBOOT_CAUSE_HARDWARE_OTHER, 'Hardware - Other'))

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

        if reboot_cause[1] == "Unknown" and os.path.isfile(SYS_POWER_STATUS_HISTORY_PATH):
            try:
                self.__api_helper.write_txt_file(SYS_POWER_STATUS_CTRL_PATH, "1")
                time.sleep(0.5)
                power_status_history = self.__api_helper.read_one_line_file(SYS_POWER_STATUS_HISTORY_PATH).strip()
                
                if power_status_history and power_status_history.lower() != '0xffff':
                    reboot_cause = (
                        self.REBOOT_CAUSE_POWER_LOSS,
                        f'Power Loss - POWER STATUS HISTORY: {power_status_history}'
                    )
                self._write_reboot_history(reboot_cause[1])
                time.sleep(0.5)
            except Exception as e:
                syslog.syslog(syslog.LOG_ERR, f"FPGA power status read failed: {e}")
            finally:
                try:
                    self.__api_helper.write_txt_file(SYS_POWER_STATUS_CTRL_PATH, "0")
                except Exception:
                    pass
            return reboot_cause

        syslog.syslog(syslog.LOG_WARNING, "get_reboot_cause: All detection methods failed, returning Unknown")
        self._write_reboot_history("Unknown")
        return reboot_cause

    @property
    def _get_presence_bitmap(self):
        bits = []
        for x in self._sfp_list:
            bits.append(str(int(x.get_presence())))
        rev = "".join(bits[::-1])
        return int(rev, 2)

    data = {'present': 0}

    def get_transceiver_change_event(self, timeout=0):
        port_dict = {}

        if timeout == 0:
            cd_ms = sys.maxsize
        else:
            cd_ms = timeout

        # poll per second
        while cd_ms > 0:
            reg_value = self._get_presence_bitmap
            changed_ports = self.data['present'] ^ reg_value
            if changed_ports != 0:
                break
            time.sleep(1)
            cd_ms = cd_ms - 1000

        if changed_ports != 0:
            for port in range(0, self.__num_of_sfps):
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
        res_dict['sfp'].clear()
        status, res_dict['sfp'] = self.get_transceiver_change_event(timeout)
        return status, res_dict

    def __initialize_cpld(self):
        cpld_num_str = self._get_attr_val('/sys/s3ip/cpld/number', 'N/A')
        if cpld_num_str == 'N/A' or not str(cpld_num_str).isdigit():
            syslog.syslog(syslog.LOG_WARNING, "CPLD node not available, skipping")
            return
        cpld_num = int(cpld_num_str)
        if cpld_num <= 0:
            return

        for index in range(0, cpld_num):
            cpld = CPLD(index + 1)
            self._cpld_list.append(cpld)

    def __initialize_fpga(self):
        fpga_num_str = self._get_attr_val('/sys/s3ip/fpga/number', 'N/A')
        if fpga_num_str == 'N/A' or not str(fpga_num_str).isdigit():
            syslog.syslog(syslog.LOG_WARNING, "FPGA node not available, skipping")
            return
        fpga_num = int(fpga_num_str)
        if fpga_num <= 0:
            return

        for index in range(0, fpga_num):
            fpga = FPGA(index + 1)
            self._fpga_list.append(fpga)

    ##############################################
    # CPLD methods
    ##############################################
    def get_num_cplds(self):
        return len(self._cpld_list)

    def get_all_cplds(self):
        return self._cpld_list

    def get_cpld(self, index):
        cpld = None
        try:
            cpld = self._cpld_list[index]
        except IndexError:
            raise RuntimeError("CPLD Index Error!!!")
        return cpld

    ##############################################
    # FPGA methods
    ##############################################
    def get_num_fpgas(self):
        return len(self._fpga_list)

    def get_all_fpgas(self):
        return self._fpga_list

    def get_fpga(self, index):
        fpga = None
        try:
            fpga = self._fpga_list[index]
        except IndexError:
            raise RuntimeError("FPGA Index Error!!!")
        return fpga

    def get_sysled(self):
        return self._sysled
