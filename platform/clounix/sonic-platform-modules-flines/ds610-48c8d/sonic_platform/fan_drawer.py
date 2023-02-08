#!/usr/bin/env python

#############################################################################
# Clounix
#
# Module contains an implementation of SONiC Platform Base API and
# provides the fan status which are available in the platform
#
#############################################################################

try:
    from sonic_platform_base.fan_drawer_base import FanDrawerBase
    from sonic_platform.fan import Fan
    from .helper import APIHelper
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")


class FanDrawer(FanDrawerBase):

    def __init__(self, index,fandrawer_conf):
        self.__api_helper = APIHelper()
        self.__conf = fandrawer_conf
        self.__num_of_fans = len(self.__conf[index]['fans'])
        self.__index = index
        self._fan_list = []
        self.__attr_led_path_prefix = '/sys/switch/sysled/'
        FanDrawerBase.__init__(self)

        # Initialize FAN_DRAWER FAN
        fan_conf=self.__conf[self.__index]['fans']
        for x in range(0, self.__num_of_fans):
            fan_conf[x].update({'container':'fan_drawer'})
            fan_conf[x].update({'container_index': self.__index})
            fan = Fan(x, fan_conf)
            self._fan_list.append(fan)

    def set_status_led(self, color):
        """
        Sets the state of the fan drawer status LED
        Args:
            color: A string representing the color with which to set the
                   fan drawer status LED
        Returns:
            bool: True if status LED state is set successfully, False if not
        """
        ret_val = False
        led_value = 0
        if color == "green":
            led_value = 1
        elif color == "red":
            led_value = 2
        elif color == "yellow":
            led_value = 3
        elif color == "off":
            led_value = 0
        else:
            return False
        ret_val = self.__api_helper.write_txt_file(self.__attr_led_path_prefix + 'fan_led_status',led_value)

        return ret_val

    def get_status_led(self):
        """
        Gets the state of the fan drawer LED
        Returns:
            A string, one of the predefined STATUS_LED_COLOR_* strings above
        """
        color = "off"
        attr_rv = self.__api_helper.read_one_line_file(self.__attr_led_path_prefix + 'fan_led_status')
        if (int(attr_rv, 16) == 0x1):
            color = "green"
        elif(int(attr_rv, 16) == 0x2):
            color = "red"
        elif(int(attr_rv, 16) == 0x3):
            color = "yellow"
        else:
            color = "off"

        return color

    def get_name(self):
        """
        Retrieves the name of the device
            Returns:
            string: The name of the device
        """
        return self.__conf[self.__index]['name']

    def get_presence(self):
        """
        Retrieves the presence of the FAN
        Returns:
            bool: True if FAN is present, False if not
        """
        return self._fan_list[0].get_presence()

    def get_model(self):
        """
        Retrieves the model number (or part number) of the device
        Returns:
            string: Model/part number of device
        """
        return self._fan_list[0].get_model()

    def get_serial(self):
        """
        Retrieves the serial number of the device
        Returns:
            string: Serial number of device
        """
        return self._fan_list[0].get_serial()

    def get_status(self):
        """
        Retrieves the operational status of the device
        Returns:
            A boolean value, True if device is operating properly, False if not
        """
        return self._fan_list[0].get_status()

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
