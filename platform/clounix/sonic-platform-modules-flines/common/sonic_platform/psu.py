#!/usr/bin/env python

#
# Name: psu.py, version: 1.0
#
# Description: Module contains the definitions of SONiC platform APIs 
#

import copy

try:
    from sonic_platform_base.psu_base import PsuBase
    from sonic_platform.fan import Fan
    from sonic_platform.thermal import Thermal
    from .helper import APIHelper
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")


class Psu(PsuBase):
    STATUS_LED_COLOR_OFF = "off"
    STATUS_LED_COLOR_GREEN = "green"
    STATUS_LED_COLOR_AMBER = "amber"
    STATUS_LED_COLOR_RED = "red"
    STATUS_LED_COLOR_BLUE = "blue"
    STATUS_LED_COLOR_GREEN_FLASHING = "green_blink"
    STATUS_LED_COLOR_AMBER_FLASHING = "amber_blink"
    STATUS_LED_COLOR_RED_FLASHING = "red_blink"
    STATUS_LED_COLOR_BLUE_FLASHING = "blue_blink"

    led_dict_str = {
        "0": STATUS_LED_COLOR_OFF,
        "1": STATUS_LED_COLOR_GREEN,
        "2": STATUS_LED_COLOR_AMBER,
        "3": STATUS_LED_COLOR_RED,
        "4": STATUS_LED_COLOR_BLUE,
        "5": STATUS_LED_COLOR_GREEN_FLASHING,
        "6": STATUS_LED_COLOR_AMBER_FLASHING,
        "7": STATUS_LED_COLOR_RED_FLASHING,
        "8": STATUS_LED_COLOR_BLUE_FLASHING
    }

    def __init__(self, index, psu_conf):
        self.__index = index
        self.__conf = psu_conf
        self.__api_helper = APIHelper()
        self.__attr_path_prefix = f'/sys/s3ip/psu/psu{self.__index + 1}/'
        self.__attr_led_path_prefix = '/sys/s3ip/sysled/'

        self._fan_list = []
        self._thermal_list = []
        PsuBase.__init__(self)

        # Initialize PSU FAN
        fan_conf_list = copy.deepcopy(self.__conf[self.__index]['fans'])
        for x in range(len(fan_conf_list)):
            fan_conf_list[x].update({'container': 'psu', 'container_index': self.__index})
            self._fan_list.append(Fan(x, fan_conf_list))

        # Initialize PSU THERMAL
        thermal_conf_list = copy.deepcopy(self.__conf[self.__index]['thermals'])
        for x in range(len(thermal_conf_list)):
            thermal_conf_list[x].update({'container': 'psu', 'container_index': self.__index})
            self._thermal_list.append(Thermal(x, thermal_conf_list))

    ##############################################
    # Device methods
    ##############################################

    def get_num_fans(self):
        """
        Retrieves the number of fan modules available on this PSU

        Returns:
            An integer, the number of fan modules available on this PSU
        """
        return len(self._fan_list)

    def get_name(self):
        """
        Retrieves the name of the device

        Returns:
            string: The name of the device
        """
        return self.__conf[self.__index]['name']

    def get_presence(self):
        """
        Retrieves the presence of the device

        Returns:
            bool: True if device is present, False if not
        """
        presence = False
        attr_rv = self.__api_helper.read_one_line_file(self.__attr_path_prefix + 'present')
        if attr_rv is not None:
            try:
                if (int(attr_rv.strip(), 16) & 0x3) != 0:
                    presence = True
            except ValueError:
                pass
        return presence

    def get_model(self):
        """
        Retrieves the model number (or part number) of the device

        Returns:
            string: Model/part number of device
        """
        model = 'N/A'
        attr_rv = self.__api_helper.read_one_line_file(self.__attr_path_prefix + 'model_name')
        if attr_rv is not None:
            model = attr_rv.strip()
        return model

    def get_serial(self):
        """
        Retrieves the serial number of the device

        Returns:
            string: Serial number of device
        """
        serial = 'N/A'
        attr_rv = self.__api_helper.read_one_line_file(self.__attr_path_prefix + 'serial_number')
        if attr_rv is not None:
            serial = attr_rv.strip()
        return serial

    def get_status(self):
        """
        Retrieves the operational status of the device

        Returns:
            A boolean value, True if device is operating properly, False if not
        """
        status = False
        attr_rv = self.__api_helper.read_one_line_file(self.__attr_path_prefix + 'out_status')
        if attr_rv is not None:
            try:
                if (int(attr_rv.strip(), 16) & 0x1) == 1:
                    status = True
            except ValueError:
                pass
        return status

    def get_position_in_parent(self):
        """
        Retrieves 1-based relative physical position in parent device.
        Returns:
            integer: The 1-based relative physical position in parent
            device or -1 if cannot determine the position
        """
        return self.__index + 1

    def is_replaceable(self):
        """
        Indicate whether Fan is replaceable.
        Returns:
            bool: True if it is replaceable.
        """
        return True

    ##############################################
    # PSU methods
    ##############################################

    def get_voltage(self):
        """
        Retrieves current PSU voltage output

        Returns:
            A float number, the output voltage in volts,
            e.g. 12.1
        """
        voltage_out = 0.0
        attr_rv = self.__api_helper.read_one_line_file(self.__attr_path_prefix + 'out_vol')
        if attr_rv is not None:
            try:
                voltage_out = float(attr_rv.strip()) / 1000
            except ValueError:
                pass
        return voltage_out

    def get_current(self):
        """
        Retrieves present electric current supplied by PSU

        Returns:
            A float number, the electric current in amperes, e.g 15.4
        """
        current_out = 0.0
        attr_rv = self.__api_helper.read_one_line_file(self.__attr_path_prefix + 'out_curr')
        if attr_rv is not None:
            try:
                current_out = float(attr_rv.strip()) / 1000
            except ValueError:
                pass
        return current_out

    def get_power(self):
        """
        Retrieves current energy supplied by PSU

        Returns:
            A float number, the power in watts, e.g. 302.6
        """
        power_out = 0.0
        attr_rv = self.__api_helper.read_one_line_file(self.__attr_path_prefix + 'out_power')
        if attr_rv is not None:
            try:
                power_out = float(attr_rv.strip()) / 1000000
            except ValueError:
                pass
        return power_out

    def get_powergood_status(self):
        """
        Retrieves the powergood status of PSU

        Returns:
            A boolean, True if PSU has stablized its output voltages and passed all
            its internal self-tests, False if not.
        """
        return self.get_status()

    def set_status_led(self, color):
        """
        Sets the state of the PSU status LED

        Args:
            color: A string representing the color with which to set the
                   PSU status LED

        Returns:
            bool: True if status LED state is set successfully, False if not
        """
        led_value = 0
        if color == self.STATUS_LED_COLOR_GREEN:
            led_value = 1
        elif color == self.STATUS_LED_COLOR_RED:
            led_value = 3
        else:
            return False

        led_file = f'psu{self.__index + 1}_led_status'
        ret_val = self.__api_helper.write_txt_file(
            self.__attr_led_path_prefix + led_file,
            str(led_value)
        )
        return ret_val

    def get_status_led(self):
        """
        Gets the state of the PSU status LED

        Returns:
            A string, one of the predefined STATUS_LED_COLOR_* strings above
        """
        status = self.__api_helper.read_one_line_file(self.__attr_path_prefix + 'led_status')
        if status is None:
            return self.STATUS_LED_COLOR_OFF
        status = status.strip()
        return self.led_dict_str.get(status, self.STATUS_LED_COLOR_OFF)

    def get_voltage_high_threshold(self):
        """
        Retrieves the high threshold PSU voltage output

        Returns:
            A float number, the high threshold output voltage in volts,
            e.g. 12.1
        """
        voltage_high_threshold = 12000 * 1.05
        attr_rv = self.__api_helper.read_one_line_file(self.__attr_path_prefix + 'out_max_vol')
        if attr_rv is not None:
            try:
                voltage_high_threshold = float(attr_rv.strip())
            except ValueError:
                pass
        return voltage_high_threshold / 1000

    def get_voltage_low_threshold(self):
        """
        Retrieves the low threshold PSU voltage output

        Returns:
            A float number, the low threshold output voltage in volts,
            e.g. 12.1
        """
        voltage_low_threshold = 12000 * 0.95
        attr_rv = self.__api_helper.read_one_line_file(self.__attr_path_prefix + 'out_min_vol')
        if attr_rv is not None:
            try:
                voltage_low_threshold = float(attr_rv.strip())
            except ValueError:
                pass
        return voltage_low_threshold / 1000

    def get_maximum_supplied_power(self):
        """
        Retrieves the maximum supplied power by PSU

        Returns:
            A float number, the maximum power output in Watts.
            e.g. 1200.1
        """
        max_power_out = 0.0
        attr_rv = self.__api_helper.read_one_line_file(self.__attr_path_prefix + 'out_max_power')
        if attr_rv is not None:
            try:
                max_power_out = float(attr_rv.strip()) / 1000000
            except ValueError:
                pass
        return max_power_out

    def get_temperature_high_threshold(self):
        """
        Retrieves the high threshold temperature of PSU

        Returns:
            A float number, the high threshold temperature of PSU in
            Celsius up to nearest thousandth of one degree Celsius,
            e.g. 30.125
        """
        if self.get_presence():
            return self.get_thermal(0).get_high_threshold()
        return 0.0

    def get_temperature(self):
        """
        Retrieves current temperature reading from PSU

        Returns:
            A float number of current temperature in Celsius up to
            nearest thousandth of one degree Celsius, e.g. 30.125
        """
        if self.get_presence():
            return self.get_thermal(0).get_temperature()
        return 0.0
