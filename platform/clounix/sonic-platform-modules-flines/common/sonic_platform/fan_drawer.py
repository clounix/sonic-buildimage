#!/usr/bin/env python


try:
    from sonic_platform_pddf_base.pddf_fan_drawer import PddfFanDrawer
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")


class FanDrawer(PddfFanDrawer):
    """PDDF Platform-Specific Fan-Drawer class"""

    def __init__(self, tray_idx, pddf_data=None, pddf_plugin_data=None):
        # idx is 0-based
        PddfFanDrawer.__init__(self, tray_idx, pddf_data, pddf_plugin_data)

    # Provide the functions/variables below for which implementation is to be overwritten

    def get_status_led(self):
        fan_led_device = "FAN_LED"
        if (not fan_led_device in self.pddf_obj.data.keys()):
            # Implement a generic status_led color scheme
            if self.get_status():
                return self.STATUS_LED_COLOR_GREEN
            else:
                return self.STATUS_LED_COLOR_OFF

        result, color = self.pddf_obj.get_system_led_color(fan_led_device)
        return (color)

    def set_status_led(self, color):
        result = False
        # led color descriptions are not same with BSP driver, so converts here
        color_dict = {
            self.STATUS_LED_COLOR_GREEN : "green",
            self.STATUS_LED_COLOR_RED   : "red",
            self.STATUS_LED_COLOR_AMBER : "yellow",
            self.STATUS_LED_COLOR_OFF   : "off"
        }
        led_device_name = "FAN_LED"
        present_fans = 0
        try:
            num_fantrays = self.pddf_obj.data['PLATFORM']['num_fantrays'] if 'num_fantrays' in self.pddf_obj.data['PLATFORM'] else 5
            num_fans_pertray = self.pddf_obj.data['PLATFORM']['num_fans_pertray'] if 'num_fans_pertray' in self.pddf_obj.data['PLATFORM'] else 2
            total_fans = num_fantrays * num_fans_pertray
            for dev_name in self.pddf_obj.data.keys():
                if dev_name == 'PLATFORM':
                    continue
                dev_type = self.pddf_obj.get_device_type(dev_name)
                if dev_type == 'FAN':
                   for fan_num in range(1, total_fans + 1):
                       attr_name = f"fan{fan_num}_present"
                       output = self.pddf_obj.get_attr_name_output(dev_name, attr_name)
                       if output and output['status'].rstrip() == '1':
                           present_fans += 1
            if present_fans > 8 :
                color = self.STATUS_LED_COLOR_GREEN
            elif present_fans > 6:
                color = self.STATUS_LED_COLOR_AMBER
            else:
                color = self.STATUS_LED_COLOR_RED
        except Exception as e:
            print(f"set_status_led Exception: {e}.")
        finally:
            old_color = self.get_status_led()
            if old_color == color_dict[color]:
                return True
            result, msg = self.pddf_obj.set_system_led_color(led_device_name, color_dict[color])
            return (result)
