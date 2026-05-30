from sonic_platform_base.sonic_thermal_control.thermal_manager_base import ThermalManagerBase
from .thermal_actions import *
from .thermal_conditions import *
from .thermal_infos import *
from .helper import APIHelper

class ThermalManager(ThermalManagerBase):
    @classmethod
    def start_thermal_control_algorithm(cls):
        """
        Start vendor specific thermal control algorithm. The default behavior of this function is a no-op.
        :return:
        """
        return True

    @classmethod
    def stop_thermal_control_algorithm(cls):
        """
        Stop thermal control algorithm
        Returns:
            bool: True if set success, False if fail.
        """
        return True

    @classmethod
    def deinitialize(cls):
        """
        Destroy thermal manager, including any vendor specific cleanup. The default behavior of this function
        is a no-op.
        :return:
        """
        return True
