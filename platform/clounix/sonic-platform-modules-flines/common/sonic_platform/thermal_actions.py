import os
import time
import syslog
import sys

from sonic_platform_base.sonic_thermal_control.thermal_action_base import ThermalPolicyActionBase
from sonic_platform_base.sonic_thermal_control.thermal_json_object import thermal_json_object
from sonic_py_common import logger
from sonic_platform.eeprom import Eeprom
from .chassis import Chassis
# from .thermal import Thermal
# from .thermal_infos import *
# from .fan import Fan
# from sonic_platform.fault import Fault
from .helper import APIHelper

SYSLOG_IDENTIFIER = 'thermalctld'
helper_logger = logger.Logger(SYSLOG_IDENTIFIER)

PLATFORM_CAUSE_DIR = "/host/reboot-cause/platform"

THERMAL_OVERLOAD_POSITION_FILE = "/usr/share/sonic/platform/api_files/reboot-cause/platform/thermal_overload_position"

LAST_TEMP = '/tmp/last_temp'


@thermal_json_object('thermal_control.control')
class ControlThermalAlgoAction(ThermalPolicyActionBase):
    """
    Action to control the thermal control algorithm
    """
    # JSON field definition
    JSON_FIELD_STATUS = 'status'
    speed = 60

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
        if not self.status:
            return
        fans = thermal_info_dict['fan_info'].fans.values()
        for fan in fans:
            fan.set_speed(60)


@thermal_json_object("fan.all.set_speed")
class SetFanSpeedAction(ThermalPolicyActionBase):
    JSON_FIELD_SPEED = "speed"

    def __init__(self):
        self.speed = None

    def load_from_json(self, json_obj):
        if self.JSON_FIELD_SPEED in json_obj:
            speed = float(json_obj[self.JSON_FIELD_SPEED])
            if speed < 0 or speed > 100:
                raise ValueError(
                    'SetFanSpeedAction invalid speed value {} in JSON policy file, valid value should be [0, 100]'.format(speed))
            self.speed = speed
        else:
            raise ValueError("SetFanSpeedAction missing field in json file")

    def execute(self, thermal_info_dict):
        # ztime = time.strftime('%F %T', time.localtime())
        # print('--------{0} policy: fan.all.set_speed'.format(ztime))
        fans = thermal_info_dict['fan_info'].fans.values()
        for fan in fans:
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
        helper_logger.log_error(
            "Error: thermal overload !!!!!!!!!!!!!!!!!!Please reboot Now!!")
        helper_logger.log_error("Error: thermal overload !!!!!!!!!!!!!!!!!!")
        helper_logger.log_error("recorded the fault cause begin...")
        print("Error: thermal overload !!!!!!!!!!!!!!!!!!Please reboot Now!!")
        # wait for all record actions done
        time.sleep(35)
        wait_ms = 30
        while wait_ms > 0:
            if os.path.isfile(THERMAL_OVERLOAD_POSITION_FILE):
                thermal_overload_pos = self.__api_helper.read_one_line_file(
                    THERMAL_OVERLOAD_POSITION_FILE)
            if "critical threshold" in thermal_overload_pos:
                break
            time.sleep(1)
            helper_logger.log_error("wait ############for recorded")
            wait_ms = wait_ms - 1
        helper_logger.log_error("recorded the fault cause...done")
        cmd = 'bash /usr/share/sonic/platform/thermal_overload_control.sh {}'.format(
            thermal_overload_pos)
        APIHelper().run_command(cmd)


@thermal_json_object('thermal_control.normalization')
class NormalizationAction(ThermalPolicyActionBase):
    speed = 60
    cpu_up_threshold = [48.0, 53.0, 60.0]
    cpu_down_threshold = [46.0, 50.0, 56.0]
    u48_up_threshold = [51.0, 55.0, 59.0]
    u48_down_threshold = [49.0, 53.0, 57.0]
    u49_up_threshold = [34.0, 41.0, 50.0]
    u49_down_threshold = [32.0, 41.0, 49.0]
    fan_speed_ratio = [40, 60, 80, 100]

    def __init__(self):
        self.__api_helper = APIHelper()

    def get_thermals(self, thermal_info_dict):
        chassis = thermal_info_dict['chassis_info'].get_chassis()
        ithermal = chassis.get_num_thermals()
        thermals = {}
        for i in range(ithermal):
            thermal = chassis.get_thermal(i)
            zname = thermal.get_name()
            if 'Package' in zname:
                thermals['cpu'] = thermal
            if '0x48' in zname:
                thermals['0x48'] = thermal
            if '0x49' in zname:
                thermals['0x49'] = thermal
        # print('----[get_thermals] useful thermals count: {}'.format(len(thermals)))
        return thermals

    def get_rules(self, nows) -> True:
        lasts = []
        with open(LAST_TEMP, 'r+') as f:
            lasts = f.readlines()
            f.close()
        # print('----[get_rules] last temps: {}'.format(lasts))
        if len(lasts) < 3:
            return True
        temps = {}
        for i in range(3):
            temps[i] = float(lasts[i])
        # print('----[get_rules] {} < {}'.format(temps,nows))
        if temps[0] < nows[0] or temps[1] < nows[1] or temps[2] < nows[2]:
            return True
        return False

    def has_warning(self, thermal_info_dict: Chassis) -> True:
        chassis = thermal_info_dict['chassis_info'].get_chassis()
        ithermal = chassis.get_num_thermals()
        for i in range(ithermal):
            thermal = chassis.get_thermal(i)
            if thermal.get_temperature() > thermal.get_high_threshold():
                return True
        return False

    def print_temperature(self, thermal_info_dict):
        # print('thermal count: {0}'.format(ithermal))
        chassis = thermal_info_dict['chassis_info'].get_chassis()
        ithermal = chassis.get_num_thermals()
        zformat = '{:<20}{:<15}{:<15}{:<15}'
        print(zformat.format('name', 'temperature', 'high', 'critical'))
        for i in range(ithermal):
            thermal = chassis.get_thermal(i)
            zname = thermal.get_name()
            print(
                zformat.format(
                    zname,
                    thermal.get_temperature(),
                    thermal.get_high_threshold(),
                    thermal.get_high_critical_threshold()
                )
            )

    def get_ratio(self):
        default_ratio = 0
        file_path = '/sys/s3ip/fan/fan1/ratio'

        try:
            attr_rv = self.__api_helper.read_one_line_file(file_path)
            if attr_rv is not None:
                ratio = int(attr_rv, 10)
                if ratio >= 0:
                    return ratio
        except (ValueError, TypeError) as e:
            print(f"Error reading ratio: {e}")
        except Exception as e:
            print(f"Unexpected error: {e}")
        return default_ratio

    def step_speed(self, thermals):
        nows = [
            thermals['cpu'].get_temperature(),
            thermals['0x48'].get_temperature(),
            thermals['0x49'].get_temperature()
        ]

        current_ratio = self.get_ratio()
        if current_ratio <= 0:
            self.speed = 60
            print(
                f"Invalid current_ratio: {current_ratio}. Setting speed to 60.")
            return nows

        # print(f"current_ratio: {current_ratio}.")
        try:
            index = self.fan_speed_ratio.index(current_ratio)
        except ValueError:
            self.speed = 60
            print(
                f"Current ratio {current_ratio} not found in fan_speed_ratio. Setting speed to 60.")
            return nows
        # print(f"current_index: {index}.")
        if index < 1:
            # print("Boundary case: Lowest speed.")
            # print(f"nows[0]: {nows[0]}.")
            # print(f"nows[1]: {nows[1]}.")
            # print(f"nows[2]: {nows[2]}.")
            if ((nows[0] > self.cpu_up_threshold[0] or
                nows[1] > self.u48_up_threshold[0] or
                    nows[2] > self.u49_up_threshold[0])):
                self.speed = self.fan_speed_ratio[index + 1]
                # print(f"current_speed: {self.speed}.")
                return nows
        elif index > 2:
            # print("Boundary case: Highest speed.")
            # print(f"nows[0]: {nows[0]}.")
            # print(f"nows[1]: {nows[1]}.")
            # print(f"nows[2]: {nows[2]}.")
            if ((nows[0] < self.cpu_down_threshold[2] and
                nows[1] < self.u48_down_threshold[2] and
                    nows[2] < self.u49_down_threshold[2])):
                self.speed = self.fan_speed_ratio[index - 1]
                # print(f"current_speed: {self.speed}.")
                return nows
        else:
            # print("Middle state.")
            # print(f"nows[0]: {nows[0]}.")
            # print(f"nows[1]: {nows[1]}.")
            # print(f"nows[2]: {nows[2]}.")
            if ((nows[0] > self.cpu_up_threshold[index] or
                nows[1] > self.u48_up_threshold[index] or
                    nows[2] > self.u49_up_threshold[index])):
                self.speed = self.fan_speed_ratio[index + 1]
                # print(f"current_speed: {self.speed}.")
                return nows
            elif ((nows[0] < self.cpu_down_threshold[index - 1] and
                  nows[1] < self.u48_down_threshold[index - 1] and
                  nows[2] < self.u49_down_threshold[index - 1])):
                self.speed = self.fan_speed_ratio[index - 1]
                # print(f"current_speed: {self.speed}.")
                return nows
            else:
                self.speed = current_ratio
                # print(f"current_speed: {self.speed}.")

        return nows

    def save_temps(self, temps):
        strs = []
        for i in range(3):
            strs.append(str(temps[i]))
        with open(LAST_TEMP, 'w+') as f:
            f.writelines(strs)
            f.flush()
            f.close()

    def update_speed(self, thermal_info_dict):
        temps = []
        if self.has_warning(thermal_info_dict):
            # print('----[update_speed] has warning')
            self.speed = 100
            temps = [self.cpu_up_threshold[2],
                     self.u48_up_threshold[2], self.u49_up_threshold[2]]
        else:
            thermals = self.get_thermals(thermal_info_dict)
            # print('----[update_speed] thermals count: {}'.format(len(thermals)))
            temps = self.step_speed(thermals)
        # print('----[update_speed] save temps {}'.format(temps))
        self.save_temps(temps)

    def execute(self, thermal_info_dict):
        # ztime = time.strftime('%F %T', time.localtime())
        # print('--------{0} policy: thermal_control.control'.format(ztime))
        # dict['fan_info'] is FanInfo type
        chassis = thermal_info_dict['chassis_info'].get_chassis()
        # print('----[execute] drawer count: {0}'.format(chassis.get_num_fan_drawers()))
        fan_info = thermal_info_dict['fan_info']
        fan_info.collect(chassis)
        fans = fan_info.fans.values()
        faults = fan_info.get_absence_fans()
        # print('----[execute] total count: {0}, faults count: {1}'.format(len(fans),len(faults)))
        if len(faults) > 0:
            for fan in fans:
                fan.set_speed(100)
            return
        # self.print_temperature(thermal_info_dict)
        self.update_speed(thermal_info_dict)
        for fan in fans:
            fan.set_speed(self.speed)
