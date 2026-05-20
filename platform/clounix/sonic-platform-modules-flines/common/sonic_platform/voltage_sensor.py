#!/usr/bin/env python

try:
    from sonic_platform_pddf_base.pddf_voltage_sensor import PddfVoltageSensor
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")


class VoltageSensor(PddfVoltageSensor):
    """PDDF Generic VoltageSensor class"""

    def __init__(self, index, pddf_data=None, pddf_plugin_data=None):
        PddfVoltageSensor.__init__(self, index, pddf_data, pddf_plugin_data)

    def get_value(self):
        output = self.pddf_obj.get_attr_name_output(self.sensor_obj_name, "volt1_input")
        if not output:
            return None

        if output['status'].isalpha():
            attr_value = None
        else:
            attr_value = float(output['status'])
        if self.sensor_index == 2:
            attr_value /= 1000

        return attr_value