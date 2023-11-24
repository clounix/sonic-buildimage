#!/usr/bin/env python

########################################################################
# DELLEMC S6000
#
# Module contains an implementation of SONiC Platform Base API and
# provides the Components' (e.g., BIOS, CPLD, FPGA, etc.) available in
# the platform
#
########################################################################

try:
    import os
    import subprocess
    from sonic_platform_base.component_base import ComponentBase
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

BIOS_QUERY_VERSION_COMMAND = "dmidecode -s bios-version"


class Component(ComponentBase):
    """DellEMC Platform-specific Component class"""

    CPLD_DIR = "/sys/bus/i2c/devices/{}/cpld_sw_version"

    CHASSIS_COMPONENTS = [
        ["CPLD-A", "Main board side CPLD A"],
        ["CPLD-B", "Main board side CPLD B"],
        ["CPLD-C", "Main board side CPLD C"],
        ["CPLD-D", "NPU board side CPLD"],
        ["MCU", "Main board side MCU"],
        ["BIOS", ("Performs initialization of hardware components during "
                  "booting")],
    ]

    def __init__(self, component_index):
        self.index = component_index
        self.name = self.CHASSIS_COMPONENTS[self.index][0]
        self.description = self.CHASSIS_COMPONENTS[self.index][1]

    def _get_cpld_register(self, reg_name):
        rv = 'ERR'
        mb_reg_file = self.CPLD_DIR.format(reg_name)

        if (not os.path.isfile(mb_reg_file)):
            return rv

        try:
            with open(mb_reg_file, 'r') as fd:
                rv = fd.read()
        except Exception as error:
            rv = 'ERR'

        rv = rv.rstrip('\r\n')
        rv = rv.lstrip(" ")
        return rv

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

    def _get_command_result(self, cmdline):
        try:
            proc = subprocess.Popen(cmdline.split(), stdout=subprocess.PIPE,
                                    stderr=subprocess.STDOUT)
            stdout = proc.communicate()[0]
            proc.wait()
            result = stdout.rstrip('\n')
        except OSError:
            result = None

        return result

    def _get_cpld_version(self, cpld_number):
        cpld_version_reg = ['6-0074', '7-0075', '8-0076', '1-0018']

        cpld_version = self._get_cpld_register(cpld_version_reg[cpld_number])

        if cpld_version != 'ERR':
            return str(int(cpld_version, 16))
        else:
            return 'NA'

    def get_name(self):
        """
        Retrieves the name of the component

        Returns:
            A string containing the name of the component
        """
        return self.name

    def get_description(self):
        """
        Retrieves the description of the component

        Returns:
            A string containing the description of the component
        """
        return self.description

    def get_firmware_version(self):
        """
        Retrieves the firmware version of the component

        Returns:
            A string containing the firmware version of the component
        """
        if(self.index <= 3):
            return self._get_cpld_version(self.index)
        elif(self.index == 4):
            mcu_version = self.__get_attr_value("/sys/bus/i2c/devices/4-0070/mb_fw_version")
            if mcu_version != 'ERR':
                return mcu_version
            else:
                return 'NA'
        else:
            bios_ver = self._get_command_result(BIOS_QUERY_VERSION_COMMAND)

            if not bios_ver:
                return 'NA'
            else:
                return bios_ver
    def install_firmware(self, image_path):
        """
        Installs firmware to the component

        Args:
            image_path: A string, path to firmware image

        Returns:
            A boolean, True if install was successful, False if not
        """
        if(self.index <= 3):
            print('CPLD frimware upgrade did not implement')
            return False
        elif(self.index == 4):
            print('MCU frimware upgrade did not implement')
            return False
        else:
            cmd = "/usr/local/bin/install_firmware.sh bios "+ image_path

        os.system(cmd)
        return True
