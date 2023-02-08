#!/usr/bin/env python

#
# Name: psu.py, version: 1.0
#
# Description: Module contains the definitions of SONiC platform APIs 
#

try:
    import os
    from sonic_platform_base.psu_base import PsuBase
    from sonic_platform.fan import Fan
    from sonic_platform.thermal import Thermal
    from .helper import APIHelper
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

class Psu(PsuBase):

    def __init__(self, index,psu_conf):
        self.__num_of_fans = len(psu_conf[index]['fans'])
        self.__num_of_thermals = len(psu_conf[index]['thermals'])
        self.__index = index
        self.__conf = psu_conf
        self.__api_helper = APIHelper()
        self.__attr_path_prefix = '/sys/switch/psu/psu{}/'.format(self.__index + 1)
        self.__attr_led_path_prefix = '/sys/switch/sysled/'

        self._fan_list = []
        self._thermal_list = []
        PsuBase.__init__(self)
        # Initialize PSU FAN
        fan_conf=self.__conf[self.__index]['fans']
        for x in range(0, self.__num_of_fans):
            fan_conf[x].update({'container':'psu'})
            fan_conf[x].update({'container_index': self.__index})
            fan = Fan(x, fan_conf)
            self._fan_list.append(fan)

        # Initialize PSU THERMAL
        thermal_conf=self.__conf[self.__index]['thermals']
        for x in range(0, self.__num_of_thermals):
            thermal_conf[x].update({'container':'psu'})
            thermal_conf[x].update({'container_index': self.__index})
            thermal = Thermal(x,thermal_conf)
            self._thermal_list.append(thermal)

##############################################
# Device methods
##############################################

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

        attr_rv = self.__api_helper.read_one_line_file(self.__attr_path_prefix + 'status')
        if (attr_rv != None):
            attr_rv = int(attr_rv, 16)
            if ((attr_rv & 0x3) != 0):
                presence = True
        return presence

    def get_model(self):
        """
        Retrieves the model number (or part number) of the device

        Returns:
            string: Model/part number of device
        """
        model = 'N/A'

        attr_rv = self.__api_helper.read_one_line_file(self.__attr_path_prefix + 'model_name')
        if (attr_rv != None):
            model = attr_rv

        return model

    def get_serial(self):
        """
        Retrieves the serial number of the device

        Returns:
            string: Serial number of device
        """
        serial = 'N/A'

        attr_rv = self.__api_helper.read_one_line_file(self.__attr_path_prefix + 'serial_number')
        if (attr_rv != None):
            serial = attr_rv

        return serial

    def get_status(self):
        """
        Retrieves the operational status of the device

        Returns:
            A boolean value, True if device is operating properly, False if not
        """
        status = False
        
        attr_rv = self.__api_helper.read_one_line_file(self.__attr_path_prefix + 'status')
        if (attr_rv != None):
            attr_rv = int(attr_rv, 16)
            if((attr_rv & 0x1) == 1):
                status = True
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
        if (attr_rv != None):
            voltage_out = float(attr_rv)

        return voltage_out/1000

    def get_current(self):
        """
        Retrieves present electric current supplied by PSU

        Returns:
            A float number, the electric current in amperes, e.g 15.4
        """
        current_out = 0.0

        attr_rv = self.__api_helper.read_one_line_file(self.__attr_path_prefix + 'out_curr')
        if (attr_rv != None):
            current_out = float(attr_rv)

        return current_out/1000

    def get_power(self):
        """
        Retrieves current energy supplied by PSU

        Returns:
            A float number, the power in watts, e.g. 302.6
        """
        power_out = 0.0

        attr_rv = self.__api_helper.read_one_line_file(self.__attr_path_prefix + 'out_power')
        if (attr_rv != None):
            power_out = float(attr_rv)

        return power_out/1000000

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
        ret_val = False
        led_value = 0
        if color == self.STATUS_LED_COLOR_GREEN:
            led_value = 1
        elif color == self.STATUS_LED_COLOR_RED:
            led_value = 2
        else:
            return False
        ret_val = self.__api_helper.write_txt_file(self.__attr_led_path_prefix + 'psu_led_status',led_value) 
        return ret_val

    ## SYSLED status
    STATUS_LED_COLOR_OFF = "off"
    STATUS_LED_COLOR_GREEN = "green"
    STATUS_LED_COLOR_AMBER = "amber"
    STATUS_LED_COLOR_RED = "red"
    STATUS_LED_COLOR_GREEN_FLASHING = "green_blink"
    STATUS_LED_COLOR_AMBER_FLASHING = "amber_blink"
    STATUS_LED_COLOR_RED_FLASHING = "red_blink"
    STATUS_LED_COLOR_BLUE = "blue"
    STATUS_LED_COLOR_BLUE_FLASHING = "blue_blink"

    led_dict_str = {
       "0" : STATUS_LED_COLOR_OFF,
       "1" : STATUS_LED_COLOR_GREEN,
       "2" : STATUS_LED_COLOR_AMBER,
       "3" : STATUS_LED_COLOR_RED,
       "4" : STATUS_LED_COLOR_RED_FLASHING,
       "5" : STATUS_LED_COLOR_AMBER_FLASHING,
       "6" : STATUS_LED_COLOR_GREEN_FLASHING,
       "7" : STATUS_LED_COLOR_BLUE,
       "8" : STATUS_LED_COLOR_BLUE_FLASHING
    }

    def get_status_led(self):
        """
        Gets the state of the PSU status LED

        Returns:
            A string, one of the predefined STATUS_LED_COLOR_* strings above
        """
        status = self.__api_helper.read_one_line_file(self.__attr_path_prefix + 'led_status')
        if status not in self.led_dict_str:
            return False
        return self.led_dict_str[status]



    def get_voltage_high_threshold(self):
        """
        Retrieves the high threshold PSU voltage output

        Returns:
            A float number, the high threshold output voltage in volts,
            e.g. 12.1
        """
        voltage_high_threshold = 0.0

        attr_rv = self.__api_helper.read_one_line_file(self.__attr_path_prefix + 'max_output_vol')
        if (attr_rv != None):
            voltage_high_threshold = float(attr_rv)
        else:
            voltage_high_threshold = (12000*1.05)

        return voltage_high_threshold/1000

    def get_voltage_low_threshold(self):
        """
        Retrieves the low threshold PSU voltage output

        Returns:
            A float number, the low threshold output voltage in volts,
            e.g. 12.1
        """
        voltage_low_threshold = 0.0

        attr_rv = self.__api_helper.read_one_line_file(self.__attr_path_prefix + 'min_output_vol')
        if (attr_rv != None):
            voltage_low_threshold = float(attr_rv)
        else:
            voltage_low_threshold = (12000*0.95)
        return voltage_low_threshold/1000

    def get_maximum_supplied_power(self):
        """
        Retrieves the maximum supplied power by PSU

        Returns:
            A float number, the maximum power output in Watts.
            e.g. 1200.1
        """
        max_power_out = 0.0

        attr_rv = self.__api_helper.read_one_line_file(self.__attr_path_prefix + 'max_output_power')
        if (attr_rv != None):
            max_power_out = float(attr_rv)

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
        else:
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
        else:
            return 0.0


        
