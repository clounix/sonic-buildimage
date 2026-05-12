import os
import sys
import time

from sonic_platform_base.sonic_thermal_control.thermal_action_base import ThermalPolicyActionBase
from sonic_platform_base.sonic_thermal_control.thermal_json_object import thermal_json_object
from sonic_py_common import logger
from .thermal_infos import ChassisInfo
#from sonic_platform.fault import Fault
from .helper import APIHelper

SYSLOG_IDENTIFIER = 'thermalctld'
helper_logger = logger.Logger(SYSLOG_IDENTIFIER)

PLATFORM_CAUSE_DIR = "/host/reboot-cause/platform"

THERMAL_OVERLOAD_POSITION_FILE = "/usr/share/sonic/platform/api_files/reboot-cause/platform/thermal_overload_position"
@thermal_json_object('thermal_control.control')
class ControlThermalAlgoAction(ThermalPolicyActionBase):
    """
    Action to control the thermal control algorithm
    """
    # JSON field definition
    JSON_FIELD_STATUS = 'status'

    def __init__(self):
        self.status = True

    def load_from_json(self, json_obj):
        """
        Construct ControlThermalAlgoAction via JSON. JSON example:
            {
                "type": "thermal_control.control"
                "status": "true"
            }
        :param json_obj: A JSON object representing a ControlThermalAlgoAction action.
        :return:
        """
        if ControlThermalAlgoAction.JSON_FIELD_STATUS in json_obj:
            status_str = json_obj[ControlThermalAlgoAction.JSON_FIELD_STATUS].lower(
            )
            if status_str == 'true':
                self.status = True
            elif status_str == 'false':
                self.status = False
            else:
                raise ValueError('Invalid {} field value, please specify true of false'.
                                 format(ControlThermalAlgoAction.JSON_FIELD_STATUS))
        else:
            raise ValueError('ControlThermalAlgoAction '
                             'missing mandatory field {} in JSON policy file'.
                             format(ControlThermalAlgoAction.JSON_FIELD_STATUS))

    def execute(self, thermal_info_dict):
        """
        Disable thermal control algorithm
        :param thermal_info_dict: A dictionary stores all thermal information.
        :return:
        """
        if ChassisInfo.INFO_NAME in thermal_info_dict:
            chassis_info_obj = thermal_info_dict[ChassisInfo.INFO_NAME]
            chassis = chassis_info_obj.get_chassis()
            thermal_manager = chassis.get_thermal_manager()
            if self.status:
                thermal_manager.start_thermal_control_algorithm()
            else:
                thermal_manager.stop_thermal_control_algorithm()

@thermal_json_object("fan.all.set_speed")
class SetFanSpeedAction(ThermalPolicyActionBase):
    JSON_FIELD_SPEED = "speed"

    def __init__(self):
      self.speed = None

    def load_from_json(self, json_obj):
      if self.JSON_FIELD_SPEED in json_obj:
         speed = float(json_obj[self.JSON_FIELD_SPEED])
         if speed < 0 or speed > 100:
            raise ValueError('SetFanSpeedAction invalid speed value {} in JSON policy file, valid value should be [0, 100]'.format(speed))
         self.speed = speed
      else:
         raise ValueError("SetFanSpeedAction missing field in json file")
    
    def execute(self, thermal_info_dict):
      for fan in thermal_info_dict['fan_info'].fans.values():
         fan.set_speed(self.speed)


@thermal_json_object('switch.power_cycling')
class SwitchPolicyAction(ThermalPolicyActionBase):
    """
    Base class for thermal action. Once all thermal conditions in a thermal policy are matched,
    all predefined thermal action will be executed.
    """

    def execute(self, thermal_info_dict):
        """
        Take action when thermal condition matches. For example, power cycle the switch.
        :param thermal_info_dict: A dictionary stores all thermal information.
        :return:
        """
        self.__api_helper = APIHelper()
        helper_logger.log_error("Error: thermal overload !!!!!!!!!!!!!!!!!!Please reboot Now!!")
        helper_logger.log_error("Error: thermal overload !!!!!!!!!!!!!!!!!!")
        helper_logger.log_error("recorded the fault cause begin...")
        print("Error: thermal overload !!!!!!!!!!!!!!!!!!Please reboot Now!!") 
         #wait for all record actions done
        thermal_overload_pos = 'cpu'
        wait_ms =  30
        while wait_ms > 0:
            try:
                fd = open(THERMAL_OVERLOAD_POSITION_FILE, 'rb',buffering=0)
                thermal_overload_pos =  fd.readline()
                fd.close()
                if "critical threshold" in str(thermal_overload_pos):
                    break
            except Exception as e:
                print(e)    
            time.sleep(1/1000)
            helper_logger.log_error("wait ############for recorded")
            wait_ms = wait_ms - 1
        helper_logger.log_error("recorded the fault cause...done")
        os.system("sync")
        cmd = 'bash /usr/share/sonic/platform/thermal_overload_control.sh {}'.format(thermal_overload_pos)
        APIHelper().run_command(cmd)


@thermal_json_object('thermal_control.normalization')
class NormalizationAction(ThermalPolicyActionBase):
    speed = 50 
    cpu_up_threshold = [48.0, 49.0, 51.0, 55.0]
    cpu_down_threshold = [46.0, 47.0, 49.0, 53.0]
    u48_up_threshold = [45.0, 47.0, 49.0, 53.0]
    u48_down_threshold = [43.0, 45.0, 47.0, 51.0]
    u49_up_threshold = [39.0, 43.0, 46.0, 50.0]
    u49_down_threshold = [37.0, 41.0, 44.0, 48.0]
    u4a_up_threshold = [37.0, 41.0, 45.0, 49.0]
    u4a_down_threshold = [35.0, 39.0, 43.0, 47.0]
    u4b_up_threshold = [38.0, 41.0, 45.0, 49.0]
    u4b_down_threshold = [36.0, 39.0, 43.0, 47.0]
    fan_speed_ratio = [30, 50, 70, 90, 100]
    LAST_TEMP = '/tmp/last_temp'

    def __init__(self):
        self.__api_helper = APIHelper()

    def get_thermals(self, thermal_info_dict):
        chassis = thermal_info_dict['chassis_info'].get_chassis()
        thermals = {}
        for i in range(chassis.get_num_thermals()):
            thermal = chassis.get_thermal(i)
            zname = thermal.get_name()
            if 'Package' in zname:
                thermals['CPU'] = thermal
            elif '0x48' in zname:
                thermals['0x48'] = thermal
            elif '0x49' in zname:
                thermals['0x49'] = thermal
            elif '0x4a' in zname:
                thermals['0x4a'] = thermal
            elif '0x4b' in zname:
                thermals['0x4b'] = thermal
        return thermals

    def get_rules(self, nows: list) -> bool:
        if not os.path.exists(self.LAST_TEMP):
            return True
        with open(self.LAST_TEMP, 'r') as f:
            lasts = f.readlines()
        if len(lasts) < 5:
            return True
        try:
            last_temps = [float(l.strip()) for l in lasts[:5]]
            return any(l < n for l, n in zip(last_temps, nows))
        except ValueError:
            return True

    def has_warning(self, thermal_info_dict: dict) -> bool:
        chassis = thermal_info_dict['chassis_info'].get_chassis()
        for i in range(chassis.get_num_thermals()):
            thermal = chassis.get_thermal(i)
            if thermal.get_temperature() > thermal.get_high_threshold():
                return True
        return False

    def get_ratio(self):
        default_ratio = 0
        file_path = '/sys/bus/i2c/devices/i2c-104/104-0060/fan1_pwm'
        try:
            attr_rv = self.__api_helper.read_one_line_file(file_path)
            if attr_rv is not None:
                ratio = int(attr_rv, 10)
                if ratio >= 0:
                    return ratio
        except (ValueError, TypeError) as e:
            helper_logger.log_warning(f"Error reading ratio file: {e}")
        except Exception as e:
            helper_logger.log_warning(f"Unexpected error reading ratio: {e}")
        return default_ratio

    def step_speed(self, thermals):
        nows = [
            thermals['CPU'].get_temperature(),
            thermals['0x48'].get_temperature(),
            thermals['0x49'].get_temperature(),
            thermals['0x4a'].get_temperature(),
            thermals['0x4b'].get_temperature()
        ]

        current_ratio = self.get_ratio()
        print('current_ratio:', current_ratio)
        if current_ratio <= 0:
            self.speed = 50
            helper_logger.log_warning(f"Invalid current_ratio: {current_ratio}. Setting speed to 50.")
            return nows

        try:
            index = self.fan_speed_ratio.index(current_ratio)
        except ValueError:
            self.speed = 50
            helper_logger.log_warning(f"Current ratio {current_ratio} not found in fan_speed_ratio. Setting speed to 50.")
            return nows

        if index < 1:
            if (nows[0] > self.cpu_up_threshold[0] or
                nows[1] > self.u48_up_threshold[0] or
                nows[2] > self.u49_up_threshold[0] or
                nows[3] > self.u4a_up_threshold[0] or
                nows[4] > self.u4b_up_threshold[0]):
                self.speed = self.fan_speed_ratio[index + 1]
                return nows
        
        elif index > 3:
            if (nows[0] < self.cpu_down_threshold[3] and
                nows[1] < self.u48_down_threshold[3] and
                nows[2] < self.u49_down_threshold[3] and
                nows[3] < self.u4a_down_threshold[3] and
                nows[4] < self.u4b_down_threshold[3]):
                self.speed = self.fan_speed_ratio[index - 1]
                return nows
        
        else:
            if (nows[0] > self.cpu_up_threshold[index] or
                nows[1] > self.u48_up_threshold[index] or
                nows[2] > self.u49_up_threshold[index] or
                nows[3] > self.u4a_up_threshold[index] or
                nows[4] > self.u4b_up_threshold[index]):
                self.speed = self.fan_speed_ratio[index + 1]

            elif (nows[0] < self.cpu_down_threshold[index - 1] and
                  nows[1] < self.u48_down_threshold[index - 1] and
                  nows[2] < self.u49_down_threshold[index - 1] and
                  nows[3] < self.u4a_down_threshold[index - 1] and
                  nows[4] < self.u4b_down_threshold[index - 1]):
                self.speed = self.fan_speed_ratio[index - 1]
            else:
                self.speed = current_ratio

        return nows


    def save_temps(self, temps: list):
        try:
            with open(self.LAST_TEMP, 'w') as f:
                f.write('\n'.join(f'{t:.2f}' for t in temps) + '\n')
        except IOError as e:
            helper_logger.log_warning(f"Failed to save temps to {LAST_TEMP}: {e}")

    def update_speed(self, thermal_info_dict):
        temps = []
        if self.has_warning(thermal_info_dict):
            self.speed = 100
            temps = [self.cpu_up_threshold[3],
                     self.u48_up_threshold[3], 
                     self.u49_up_threshold[3],
                     self.u4a_up_threshold[3],
                     self.u4b_up_threshold[3]]
        else:
            thermals = self.get_thermals(thermal_info_dict)
            temps = self.step_speed(thermals)
        print('updata_speed')
        #self.save_temps(temps)

    def execute(self, thermal_info_dict):
        print('thermal_control.normalization')
        #chassis = thermal_info_dict['chassis_info'].get_chassis()
        fan_info = thermal_info_dict['fan_info']
        #fan_info.collect(chassis)
        fans = fan_info.fans.values()
        #faults = fan_info.get_absence_fans()

        #if len(faults) > 0:
        #    helper_logger.log_error("Fan fault detected, setting all fans to 100%.")
        #    for fan in fans:
        #        fan.set_speed(100)
        #    return

        self.update_speed(thermal_info_dict)
        for fan in fans:
            print('fan name:', fan.get_name(),'speed:', self.speed)
            fan.set_speed(self.speed)
