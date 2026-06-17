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

@thermal_json_object("fan.led.set")
class SetFanLedAction(ThermalPolicyActionBase):
    STATUS_LED_COLOR_GREEN = "green"
    STATUS_LED_COLOR_AMBER = "yellow"
    STATUS_LED_COLOR_RED = "red"

    def execute(self, thermal_info_dict):
        fan_info = thermal_info_dict['fan_info']
        absence = len(fan_info.get_absence_fans())
        fault = len(fan_info.get_fault_fans())
        try:
            from sonic_platform.sysled import SYSLED
            fanled = SYSLED()
            fanled_color = fanled.get_fan_led_status()
            if absence >= 4 or fault >= 4:
                if fanled_color != self.STATUS_LED_COLOR_RED:
                    fanled.set_fan_led_status(self.STATUS_LED_COLOR_RED)
            if (absence < 4 and absence >= 1) or (fault < 4 and fault >= 1):
                if fanled_color != self.STATUS_LED_COLOR_AMBER:
                    fanled.set_fan_led_status(self.STATUS_LED_COLOR_AMBER)
            if absence == 0 and fault == 0:
                if fanled_color != self.STATUS_LED_COLOR_GREEN:
                    fanled.set_fan_led_status(self.STATUS_LED_COLOR_GREEN)
        except Exception as e:
            print(e)


@thermal_json_object("insufficient.fan.speed")
class InsufficientFanSpeedAction(ThermalPolicyActionBase):
    JSON_FILED_FAN_WINDOW = 'window'

    def __init__(self):
      self.last_time = 0
      self.curr_time = 0
      self.count = 0
      self.window = 6

    def load_from_json(self, json_obj):
        if self.JSON_FILED_FAN_WINDOW in json_obj:
            window = float(json_obj[self.JSON_FILED_FAN_FAULT_NUM])
            if window < 0:
                raise ValueError('InsufficientFanSpeedAction invalid window value {} in JSON policy file, valid value should be > 0'.format(window))
            self.window = window

    def get_uptime(self):
        """
        Utility to get the system up time.
        :return: System up time in seconds.
        """
        with open('/proc/uptime', 'r') as f:
            uptime_seconds = float(f.readline().split()[0])
        return uptime_seconds

    def execute(self, thermal_info_dict):
        self.curr_time = self.get_uptime()
        if self.last_time - self.curr_time >= 300:
            self.count = 0
        self.last_time = self.curr_time

        self.count += 1
        if self.count >= self.window:
            self.count = 0
            helper_logger.log_error("insufficient fan speed, will reboot now!!")
            print('insufficient fan speed, will reboot now!!')
            if ChassisInfo.INFO_NAME in thermal_info_dict:
                chassis_info_obj = thermal_info_dict[ChassisInfo.INFO_NAME]
                chassis = chassis_info_obj.get_chassis()
                try:
                    REBOOT_EEPROM_PATH = chassis.pddf_obj.get_path("RC_EEPROM", "eeprom")
                    if os.path.isfile(REBOOT_EEPROM_PATH):
                        with open(REBOOT_EEPROM_PATH, 'rb+') as binfile:
                            try:
                                binfile.seek(0)
                                binfile.write(bytes([0x5]))
                                binfile.flush()
                            except Exception as e:
                                print(f"Failed to set reboot eeprom: {e}")
                        os.system("sync")
                        time.sleep(5)
                        power_cycle_path = chassis.pddf_obj.get_fpga_pci_sysfs_attr('power_cycle')
                        cmd = 'echo 0x4 > {}'.format(power_cycle_path)
                        APIHelper().run_command(cmd)
                    else:
                        print('check reboot eeprom path!!!')
                except Exception as e:
                        print('insufficient fan speed set reboot eeprom fail!!', e)


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
    LAST_TEMP = '/tmp/last_temp'

    def __init__(self):
        self.speed = 50
        self.cpu_up_threshold = [57.0, 59.0, 61.0, 64.0]
        self.cpu_down_threshold = [55.0, 57.0, 59.0, 62.0]
        self.u48_up_threshold = [47.0, 49.0, 51.0, 54.0]
        self.u48_down_threshold = [45.0, 47.0, 49.0, 52.0]
        self.u49_up_threshold = [41.0, 44.0, 47.0, 51.0]
        self.u49_down_threshold = [39.0, 42.0, 45.0, 49.0]
        self.u4a_up_threshold = [37.0, 41.0, 45.0, 49.0]
        self.u4a_down_threshold = [35.0, 39.0, 43.0, 47.0]
        self.u4b_up_threshold = [38.0, 41.0, 45.0, 49.0]
        self.u4b_down_threshold = [36.0, 39.0, 43.0, 47.0]
        self.pvt_up_threshold = [68.0, 70.0, 72.0, 74.0]
        self.pvt_down_threshold = [66.0, 68.0, 70.0, 72.0]
        self.fan_speed_ratio = [30, 50, 70, 90, 100]

    def load_from_json(self, json_obj):
        self.speed = json_obj.get('speed', self.speed)
        self.cpu_up_threshold = json_obj.get('cpu_up_threshold', self.cpu_up_threshold)
        self.cpu_down_threshold = json_obj.get('cpu_down_threshold', self.cpu_down_threshold)
        self.u48_up_threshold = json_obj.get('u48_up_threshold', self.u48_up_threshold)
        self.u48_down_threshold = json_obj.get('u48_down_threshold', self.u48_down_threshold)
        self.u49_up_threshold = json_obj.get('u49_up_threshold', self.u49_up_threshold)
        self.u49_down_threshold = json_obj.get('u49_down_threshold', self.u49_down_threshold)
        self.u4a_up_threshold = json_obj.get('u4a_up_threshold', self.u4a_up_threshold)
        self.u4a_down_threshold = json_obj.get('u4a_down_threshold', self.u4a_down_threshold)
        self.u4b_up_threshold = json_obj.get('u4b_up_threshold', self.u4b_up_threshold)
        self.u4b_down_threshold = json_obj.get('u4b_down_threshold', self.u4b_down_threshold)
        self.pvt_up_threshold = json_obj.get('pvt_up_threshold', self.pvt_up_threshold)
        self.pvt_down_threshold = json_obj.get('pvt_down_threshold', self.pvt_down_threshold)
        raw_ratio = json_obj.get('fan_speed_ratio', self.fan_speed_ratio)
        if isinstance(raw_ratio, list):
            self.fan_speed_ratio = raw_ratio

    def get_ratio(self, chassis):
        default_ratio = 0
        try:
            if chassis is None:
                import sonic_platform.platform
                chassis = sonic_platform.platform.Platform().get_chassis()
            attr = "fan1_pwm"
            output = chassis.pddf_obj.get_attr_name_output("FAN-CTRL", attr)
            if not output:
                return default_ratio
            output['status'] = output['status'].rstrip()
            if output['status'].isalpha():
                return default_ratio
            else:
                fpwm = int(float(output['status']))
            pwm_to_dc = eval(chassis.plugin_data['FAN']['pwm_to_duty_cycle'])
            speed_percentage = int(round(pwm_to_dc(fpwm)))
            return speed_percentage
        except (ValueError, TypeError) as e:
            helper_logger.log_warning(f"Error reading ratio file: {e}")
        except Exception as e:
            helper_logger.log_warning(f"Unexpected error reading ratio: {e}")
        return default_ratio

    def step_speed(self, thermal_info_dict):
        current_temps = {}
        temp_list = []
        warning = []
        required_sensors = ['CPU', '0x48', '0x49', '0x4a', '0x4b', 'FPGA_PVT']
        chassis = thermal_info_dict['chassis_info'].get_chassis()

        max_thresholds = [self.cpu_up_threshold[3],
                self.u48_up_threshold[3], 
                self.u49_up_threshold[3],
                self.u4a_up_threshold[3],
                self.u4b_up_threshold[3],
                self.pvt_up_threshold[3]]

        for i in range(chassis.get_num_thermals()):
            try:
                thermal = chassis.get_thermal(i)
                if thermal is None:
                    continue
                sensor_name = thermal.get_name()

                temp = thermal.get_temperature()
                high_threshold = thermal.get_high_threshold()

                if temp is None or high_threshold is None:
                    helper_logger.log_warning(f"Thermal sensor {sensor_name} returned None value. temp={temp}, high_threshold={high_threshold}")
                    continue

                if temp > high_threshold:
                    helper_logger.log_warning(f"Thermal warning: {sensor_name} temperature {temp} exceeds high threshold {high_threshold}")
                    warning.append(sensor_name)

                if 'Package' in sensor_name:
                    current_temps['CPU'] = temp
                elif '0x48' in sensor_name:
                    current_temps['0x48'] = temp
                elif '0x49' in sensor_name:
                    current_temps['0x49'] = temp
                elif '0x4a' in sensor_name:
                    current_temps['0x4a'] = temp
                elif '0x4b' in sensor_name:
                    current_temps['0x4b'] = temp
                elif 'FPGA_PVT' in sensor_name:
                    current_temps['FPGA_PVT'] = temp
            except Exception as e:
                helper_logger.log_error(f"Error checking thermal sensor {sensor_name}: {e}")
                continue

        #Is an alarm triggered?
        if len(warning) > 0:
            helper_logger.log_warning(f"Thermal warning")
            self.speed = 100
            temp_list = max_thresholds
            return temp_list

        missing_sensors = [s for s in required_sensors if s not in current_temps]
        if missing_sensors:
            helper_logger.log_error(f"Missing required sensors: {missing_sensors}, setting fan speed to 100%")
            self.speed = 100
            return max_thresholds

        for name in required_sensors:
            temp = current_temps[name]
            temp_list.append(temp)

        current_ratio = self.get_ratio(chassis)
        if current_ratio <= 0:
            self.speed = 50
            helper_logger.log_warning(f"Invalid current_ratio: {current_ratio}. Setting speed to 50.")
            return temp_list

        try:
            index = self.fan_speed_ratio.index(current_ratio)
        except ValueError:
            self.speed = 50
            helper_logger.log_warning(f"Current ratio {current_ratio} not found in fan_speed_ratio. Setting speed to 50.")
            return temp_list

        if index < 1:
            if (temp_list[0] > self.cpu_up_threshold[0] or
                temp_list[1] > self.u48_up_threshold[0] or
                temp_list[2] > self.u49_up_threshold[0] or
                temp_list[3] > self.u4a_up_threshold[0] or
                temp_list[4] > self.u4b_up_threshold[0] or
                temp_list[5] > self.pvt_up_threshold[0]):
                self.speed = self.fan_speed_ratio[index + 1]
                return temp_list
        
        elif index > 3:
            if (temp_list[0] < self.cpu_down_threshold[3] and
                temp_list[1] < self.u48_down_threshold[3] and
                temp_list[2] < self.u49_down_threshold[3] and
                temp_list[3] < self.u4a_down_threshold[3] and
                temp_list[4] < self.u4b_down_threshold[3] and
                temp_list[5] < self.pvt_down_threshold[3]):
                self.speed = self.fan_speed_ratio[index - 1]
                return temp_list
        
        else:
            if (temp_list[0] > self.cpu_up_threshold[index] or
                temp_list[1] > self.u48_up_threshold[index] or
                temp_list[2] > self.u49_up_threshold[index] or
                temp_list[3] > self.u4a_up_threshold[index] or
                temp_list[4] > self.u4b_up_threshold[index] or
                temp_list[5] > self.pvt_up_threshold[index]):
                self.speed = self.fan_speed_ratio[index + 1]

            elif (temp_list[0] < self.cpu_down_threshold[index - 1] and
                  temp_list[1] < self.u48_down_threshold[index - 1] and
                  temp_list[2] < self.u49_down_threshold[index - 1] and
                  temp_list[3] < self.u4a_down_threshold[index - 1] and
                  temp_list[4] < self.u4b_down_threshold[index - 1] and
                  temp_list[5] < self.pvt_down_threshold[index - 1]):
                self.speed = self.fan_speed_ratio[index - 1]
            else:
                self.speed = current_ratio

        return temp_list

    def save_temps(self, temps: list):
        try:
            with open(self.LAST_TEMP, 'w') as f:
                f.write('\n'.join(f'{t:.2f}' for t in temps) + '\n')
        except IOError as e:
            helper_logger.log_warning(f"Failed to save temps to {self.LAST_TEMP}: {e}")

    def update_speed(self, thermal_info_dict):
        temps = []
        temps = self.step_speed(thermal_info_dict)
        self.save_temps(temps)

    def execute(self, thermal_info_dict):
        try:
            self.update_speed(thermal_info_dict)
            fan_info = thermal_info_dict['fan_info']
            fans = fan_info.fans.values()
            for fan in fans:
                fan.set_speed(self.speed)
        except Exception as e:
            helper_logger.log_critical(f"Critical error in thermal normalization action: {e}. Setting all fans to 100% for safety.")
            try:
                fan_info = thermal_info_dict.get('fan_info')
                if fan_info:
                    fans = fan_info.fans.values()
            except:
                pass
            for fan in fans:
                fan.set_speed(100)