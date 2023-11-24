#!/usr/bin/env python
#
# Name: thermal.py, version: 1.0
#
# Description: Module contains the definitions of SONiC platform APIs 
#

try:
    import os
    from sonic_platform_base.thermal_base import ThermalBase
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

class Thermal(ThermalBase):

    def __init__(self, index):
        self.__index = index
        #thermal name list
        self.__thermal_name_list = [ "LM75BD / Ambient MAC side",
                                     "LM75BD / Ambient MAC",
                                     "LM75BD / Ambient FAN",
                                     "LM75BD / Ambient NPU"]
        self.themerl_attr=['lm75_48_temp', 'lm75_49_temp', 'lm75_4a_temp', 'temp1_input']
        self.themerl_critical_attr='smartFan_max_temp'
        self.themerl_high_attr='smartFan_mid_temp'
        self.themerl_low_attr='smartFan_min_temp'
        self.thermal_name=self.__thermal_name_list[self.__index]        
        if (index <=2):
            self.__temperature_attr = "/sys/bus/i2c/devices/4-0070/{}".format(self.themerl_attr[self.__index])
            self.__temperature_critical_attr = "/sys/bus/i2c/devices/4-0070/{}".format(self.themerl_critical_attr)
            self.__temperature_high_attr = "/sys/bus/i2c/devices/4-0070/{}".format(self.themerl_high_attr)
            self.__temperature_low_attr = "/sys/bus/i2c/devices/4-0070/{}".format(self.themerl_low_attr)
        else:
            self.__temperature_attr = "/sys/bus/i2c/devices/1-004a/hwmon/hwmon1/{}".format(self.themerl_attr[self.__index])
            self.__temperature_critical_attr = None
            self.__temperature_high_attr = None
            self.__temperature_low_attr = None

    def __get_attr_value(self, attr_path):

        retval = 'ERR'
        if (not os.path.isfile(attr_path)):
            return retval

        try:
            with open(attr_path, 'r') as fd:
                retval = fd.read()
        except Exception as error:
            logging.error("Unable to open ", attr_path, " file !")

        retval = retval.rstrip(' \t\n\r')
        return retval

##############################################
# Device methods
##############################################

    def get_name(self):
        """
        Retrieves the name of the device

        Returns:
            string: The name of the device
        """
        return self.thermal_name or "Unknown"

    def get_presence(self):
        """
        Retrieves the presence of the device

        Returns:
            bool: True if device is present, False if not
        """
        return True

    def get_model(self):
        """
        Retrieves the model number (or part number) of the device

        Returns:
            string: Model/part number of device
        """
        return "N/A"

    def get_serial(self):
        """
        Retrieves the serial number of the device

        Returns:
            string: Serial number of device
        """
        return "N/A"

    def get_status(self):
        """
        Retrieves the operational status of the device

        Returns:
            A boolean value, True if device is operating properly, False if not
        """
        return self.get_presence()

##############################################
# THERMAL methods
##############################################

    def get_temperature(self):
        """
        Retrieves current temperature reading from thermal

        Returns:
            A float number of current temperature in Celsius up to nearest thousandth
            of one degree Celsius, e.g. 30.125 
        """
        temperature = 0.0
        attr_path = self.__temperature_attr

        temperature = self.__get_attr_value(attr_path)
        return temperature

    def get_high_critical_threshold(self):
        """
        Retrieves the high critical threshold temperature of thermal

        Returns:
            A float number, the high critical threshold temperature of thermal in Celsius
            up to nearest thousandth of one degree Celsius, e.g. 30.125
        """
        temperature_hard_code = 80.0
        '''
        temperature = 0.0
        if self.__temperature_critical_attr:
            attr_path = self.__temperature_critical_attr    
            temperature = self.__get_attr_value(attr_path)
            return temperature
        else:
            raise NotImplementedError
        '''
        return "{:.1f}".format(temperature_hard_code)

    def get_high_threshold(self):
        """
        Retrieves the high threshold temperature of thermal

        Returns:
            A float number, the high threshold temperature of thermal in Celsius
            up to nearest thousandth of one degree Celsius, e.g. 30.125
        """
        temperature_hard_code = 75.0
        '''
        temperature = 0.0
        if self.__temperature_high_attr:
            attr_path = self.__temperature_high_attr    
            temperature = self.__get_attr_value(attr_path)
            return temperature
        else:
            raise NotImplementedError
        '''
        return "{:.1f}".format(temperature_hard_code)

    def get_low_threshold(self):
        """
        Retrieves the low threshold temperature of thermal

        Returns:
            A float number, the low threshold temperature of thermal in Celsius
            up to nearest thousandth of one degree Celsius, e.g. 30.125
        """
        raise NotImplementedError

    def set_high_threshold(self, temperature):
        """
        Sets the high threshold temperature of thermal

        Args : 
            temperature: A float number up to nearest thousandth of one degree Celsius, 
            e.g. 30.125

        Returns:
            A boolean, True if threshold is set successfully, False if not
        """
        raise NotImplementedError

    def set_low_threshold(self, temperature):
        """
        Sets the low threshold temperature of thermal

        Args : 
            temperature: A float number up to nearest thousandth of one degree Celsius,
            e.g. 30.125

        Returns:
            A boolean, True if threshold is set successfully, False if not
        """
        raise NotImplementedError

