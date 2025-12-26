#############################################################################
# PDDF
#
# PDDF sfp base class inherited from the base class
#############################################################################

import binascii

try:
    from sonic_platform_base.sonic_xcvr.sfp_optoe_base import SfpOptoeBase
    from sonic_platform_base.sonic_sfp.qsfp_dd import qsfp_dd_Dom
    from sonic_platform_base.sonic_xcvr.api.public.cmis import CmisApi
    from sonic_platform_base.sonic_xcvr.api.public.sff8636 import Sff8636Api
    from sonic_platform_base.sonic_xcvr.api.public.sff8436 import Sff8436Api
    from sonic_platform_base.sonic_xcvr.api.public.sff8472 import Sff8472Api
    from sonic_platform_base.sonic_xcvr.codes.public.cmis import CmisCodes
    from sonic_platform_base.sonic_xcvr.fields import consts
    from sonic_platform_base.sfp_base import SfpBase
    import time
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

QSFP_PWR_CTRL_ADDR = 93


# capabilities
LOOPBACK_CAPA = 128
PRBS_PATTERN_CAPABILITIES_OFFSET = 129
DIAGNOSTIC_REPORTING_CAP_OFFSET = 130
MEDIA_GENERATOR_PATTERN_SUPPORT_OFFSET = 134
MEDIA_CHECKER_PATTERN_SUPPORT_OFFSET = 138

#host generator
HOST_SIDE_GENERATOR_ENABLE_OFFSET = 144
HOST_SIDE_GENERATOR_PATTERN_BEGIN = 148

#media generator
MEDIA_SIDE_GENERATOR_ENABLE_OFFSET = 152
MEDIA_SIDE_GENERATOR_PATTERN_BEGIN = 156

#HOST checker
HOST_SIDE_CHECKER_ENABLE_OFFSET = 160
HOST_SIDE_CHECKER_PATTERN_BEGIN = 164

#media checker
MEDIA_SIDE_CHECKER_ENABLE_OFFSET = 168
MEDIA_SIDE_CHECKER_PATTERN_BEGIN = 172


QSFP_DD_DIAG_SELECTOR_OFFSET = 128
QSFP_DD_HOST_BER_OFFSET = 192
QSFP_DD_BER_WIDTH = 16
QSFP_DD_MEDIA_BER_OFFSET = 208
QSFP_DD_MEDIA_BER_COUNT_OFFSET = 192
QSFP_DD_MEDIA_BER_COUNT_WIDTH = 64
LOOPBACK_CONTROL_OFFSET = 180
LOOPBACK_CONTROL_WIDTH = 4

MEDIA_OUTPUT_LOOPBACK = 180
MEDIA_INPUT_LOOPBACK = 181
HOST_OUTPUT_LOOPBACK = 182
HOST_INPUT_LOOPBACK = 183

BER_CHECK_DELAY_TIME_MS = 300

SFP_PAGE_SIZE = 128
PAGE13 = 0x13
PAGE14 = 0x14

QSFP_DD_TYPE = 'QSFP-DD'
DSFP_TYPE = 'DSFP'

class PddfSfp(SfpOptoeBase):
    """
    PDDF generic Sfp class
    """

    pddf_obj = {}
    plugin_data = {}
    _port_start = 0
    _port_end = 0


    def __init__(self, index, pddf_data=None, pddf_plugin_data=None):
        if not pddf_data or not pddf_plugin_data:
            raise ValueError('PDDF JSON data error')

        self.pddf_obj = pddf_data
        self.plugin_data = pddf_plugin_data

        self.platform = self.pddf_obj.get_platform()

        # index is 0-based
        self._port_start = 0
        self._port_end = int(self.platform['num_ports'])
        if index < self._port_start or index >= self._port_end:
            print("Invalid port index %d" % index)
            return

	# 1-based port index
        self.port_index = index+1
        self.device = 'PORT{}'.format(self.port_index)
        self.sfp_type = self.pddf_obj.get_device_type(self.device)
        self.eeprom_path = self.pddf_obj.get_path(self.device, 'eeprom')

        SfpOptoeBase.__init__(self)

    def get_eeprom_path(self):
        return self.eeprom_path

    def get_transceiver_loopback(self):
        ret = dict()
        api = self.get_xcvr_api()
        if api is None:
            print("not support xcvr api")
            return ret

        # looback only support on cmis
        if not isinstance(api, CmisApi):
            print("Loopback functionality is not implemented on this module")
            return ret

        loopback_rule = self.read_eeprom(PAGE13 * SFP_PAGE_SIZE + LOOPBACK_CAPA, 1)
        if loopback_rule is None:
            print("not support loopback capabilities")
            return ret

        loopback_rule = int(loopback_rule[0])
        if loopback_rule & 0xf == 0:
            print("not get loopback capabilities")
            return ret

        loopback_raw = self.read_eeprom(PAGE13 * SFP_PAGE_SIZE + LOOPBACK_CONTROL_OFFSET, LOOPBACK_CONTROL_WIDTH)
        if loopback_raw is None:
            print("not support loopback-control")
            return ret

        if len(loopback_raw) != LOOPBACK_CONTROL_WIDTH:
            print("not get loopback_control data")
            return ret

        #media_side_output_loopback
        if ((loopback_rule & (1 << 0)) != 0):
            if (int(loopback_raw[0]) != 0):
                ret['media_output_loopback'] = '1'
            else:
                ret['media_output_loopback'] = '0'
        else:
            print('media_output_loopback not support on this port')

        #media_side_input_loopback
        if ((loopback_rule & (1 << 1)) != 0):
            if (int(loopback_raw[1]) != 0):
                ret['media_input_loopback'] = '1'
            else:
                ret['media_input_loopback'] = '0'
        else:
            print('media_intput_loopback not support on this port')

        #host_side_output_loopback
        if ((loopback_rule & (1 << 2)) != 0):
            if (int(loopback_raw[2]) != 0):
                ret['host_output_loopback'] = '1'
            else:
                ret['host_output_loopback'] = '0'
        else:
            print('host_output_loopback not support on this port')

        #host_side_input_loopback
        if ((loopback_rule & (1 << 3)) != 0):
            if (int(loopback_raw[3]) != 0):
                ret['host_input_loopback'] = '1'
            else:
                ret['host_input_loopback'] = '0'
        else:
            print('host_input_loopback not support on this port')

        return ret

    def set_loopback_mode(self, loopback_mode):
        '''
        Loopback mode has to be one of the five:
        1. "none" (default)
        2. "host-side-input"
        3. "host-side-output"
        4. "media-side-input"
        5. "media-side-output"
        '''
        api = self.get_xcvr_api()
        if api is None:
            print("not support xcvr api")
            return False
        if not isinstance(api, CmisApi):
            print("Loopback functionality is not implemented on this module")
            return False

        loopback_rule = int(self.read_eeprom(PAGE13 * SFP_PAGE_SIZE + LOOPBACK_CAPA, 1)[0])
        if loopback_rule & 0xf == 0:
            print("not get loopback capabilities")
            return False

        if loopback_mode == 'none':
            media_output = self.write_eeprom(PAGE13 * SFP_PAGE_SIZE + MEDIA_OUTPUT_LOOPBACK, 1, bytearray(b'\x00'))
            media_input = self.write_eeprom(PAGE13 * SFP_PAGE_SIZE + MEDIA_INPUT_LOOPBACK, 1, bytearray(b'\x00'))
            host_output = self.write_eeprom(PAGE13 * SFP_PAGE_SIZE + HOST_OUTPUT_LOOPBACK, 1, bytearray(b'\x00'))
            host_input = self.write_eeprom(PAGE13 * SFP_PAGE_SIZE + HOST_INPUT_LOOPBACK, 1, bytearray(b'\x00'))
            return all([media_output, media_input, host_output, host_input])

        if loopback_mode == 'media-side-output':
            if (loopback_rule & (1 << 0)) != 0:
                media_output = self.write_eeprom(PAGE13 * SFP_PAGE_SIZE + MEDIA_OUTPUT_LOOPBACK, 1, bytearray(b'\xff'))
                return media_output
            else:
                print('port not support this feature')
                return False

        if loopback_mode == 'media-side-input':
            if (loopback_rule & (1 << 1)) != 0:
                media_input = self.write_eeprom(PAGE13 * SFP_PAGE_SIZE + MEDIA_INPUT_LOOPBACK, 1, bytearray(b'\xff'))
                return media_input
            else:
                print('port not support this feature')
                return False

        if loopback_mode == 'host-side-output':
            if (loopback_rule & (1 << 2)) != 0:
                host_output = self.write_eeprom(PAGE13 * SFP_PAGE_SIZE + HOST_OUTPUT_LOOPBACK, 1, bytearray(b'\xff'))
                return host_output
            else:
                print('port not support this feature')
                return False

        if loopback_mode == 'host-side-input':
            if (loopback_rule & (1 << 3)) != 0:
                host_input = self.write_eeprom(PAGE13 * SFP_PAGE_SIZE + HOST_INPUT_LOOPBACK, 1, bytearray(b'\xff'))
                return host_input
            else:
                print('port not support this feature')
                return False

        return False

    def prbs_is_supported(self):
        data = self.read_eeprom(PAGE13 * SFP_PAGE_SIZE + DIAGNOSTIC_REPORTING_CAP_OFFSET, 1)
        if data is not None:
            if (data[0] & 0x20) != 0:
                print("media side input SNR measurement support")
            if (data[0] & 0x10) != 0:
                print("host side input SNR measurement support")
            if (data[0] & 0x02) != 0:
                print("Bits And Errors Counting Supported")
            if (data[0] & 0x01) != 0:
                print("Bit Error Ratio Results Supported")
            print('')
            return (data[0] & 0x33)
        else:
            return False

    def prbs_media_checker_status(self):
        data = self.read_eeprom(PAGE13 * SFP_PAGE_SIZE + MEDIA_SIDE_CHECKER_ENABLE_OFFSET, 1)
        if len(data) > 0:
            return (data[0] & 0xff)
        else:
            return False

    def prbs_media_generator_status(self):
        data = self.read_eeprom(PAGE13 * SFP_PAGE_SIZE + MEDIA_SIDE_GENERATOR_ENABLE_OFFSET, 1)
        if len(data) > 0:
            return (data[0] & 0xff)
        else:
            return False


    def prbs_host_checker_status(self):
        data = self.read_eeprom(PAGE13 * SFP_PAGE_SIZE + HOST_SIDE_CHECKER_ENABLE_OFFSET, 1)
        if len(data) > 0:
            return (data[0] & 0xff)
        else:
            return False

    def prbs_host_generator_status(self):
        data = self.read_eeprom(PAGE13 * SFP_PAGE_SIZE + HOST_SIDE_GENERATOR_ENABLE_OFFSET, 1)
        if len(data) > 0:
            return (data[0] & 0xff)
        else:
            return False

    def string_to_bytes(self, data):
        try:
            if data.startswith('0x'):
                data = data[len('0x'):]
            convert_data = binascii.a2b_hex(data)
        except Exception as e:
            print("Error: read port failed: {}".format(str(e)))
            return None

        return convert_data

    def get_prbs_pattern_id(self, pattern_name):
        pattern_dict = {
            "prbs31q": 0,
            "prbs31": 1,
            "prbs23q": 2,
            "prbs23": 3,
            "prbs15q": 4,
            "prbs15": 5,
            "prbs13q": 6,
            "prbs13": 7,
            "prbs9q": 8,
            "prbs9": 9,
            "prbs7q": 10,
            "prbs7": 11,
            "SSPRq": 12,
            "reserved": 13,
            "custom": 14,
            "user": 15
        }
        return pattern_dict.get(pattern_name, None)

    def get_prbs_err_count(self, pos, low_lanes):
        """
        Retrieves the Media BER count list of this SFP
        Returns:
            A list of numbers, representing Media BER count
            for channel 0 to channel 3 or channel 4 to channel 7.
        """
        default = ['00'] * QSFP_DD_MEDIA_BER_COUNT_WIDTH
        ret_list = []
        # page 14
        sfpd_obj = qsfp_dd_Dom()
        if sfpd_obj is None:
            return default

        offset = PAGE14 * SFP_PAGE_SIZE
        data = 0
        if pos == 'host':
            data = b'\x02' if low_lanes else b'\x03'
        elif pos == 'media':
            data = b'\x04' if low_lanes else b'\x05'

        self.write_eeprom(offset + QSFP_DD_DIAG_SELECTOR_OFFSET, 1, bytearray(data))
        time.sleep(BER_CHECK_DELAY_TIME_MS / 1000)
        dom_raw = self.read_eeprom(offset + QSFP_DD_MEDIA_BER_COUNT_OFFSET, QSFP_DD_MEDIA_BER_COUNT_WIDTH)
        if dom_raw is not None:
            for item in dom_raw:
                item = str(item)
                ret_list.append(item)
            return ret_list
        else:
            print("get prbs err count err")
            return default

    def _calc_ber(self, data1, data2):
        s = data1 >> 3 & 0x1f
        m = data1 & 0x07
        m = m << 8 | data2
        return m * pow(10, s - 24)

    def get_media_host_ber(self, pos):
        ber_list = []
        if not self.write_eeprom(PAGE14 * SFP_PAGE_SIZE + QSFP_DD_DIAG_SELECTOR_OFFSET, 1, bytearray(b'\x01')):
            print('Diagnostics Select failed')
            return ber_list

        time.sleep(3)
        if pos == 'media':
            dom_raw = self.read_eeprom(PAGE14 * SFP_PAGE_SIZE + QSFP_DD_MEDIA_BER_OFFSET, QSFP_DD_BER_WIDTH)
        else:
            dom_raw = self.read_eeprom(PAGE14 * SFP_PAGE_SIZE + QSFP_DD_HOST_BER_OFFSET, QSFP_DD_BER_WIDTH)
        if len(dom_raw) <= 0:
            return ber_list
        if self.sfp_type == QSFP_DD_TYPE:
            ber_list.append(str(self._calc_ber(dom_raw[0], dom_raw[1])))
            ber_list.append(str(self._calc_ber(dom_raw[2], dom_raw[3])))
            ber_list.append(str(self._calc_ber(dom_raw[4], dom_raw[5])))
            ber_list.append(str(self._calc_ber(dom_raw[6], dom_raw[7])))
            ber_list.append(str(self._calc_ber(dom_raw[8], dom_raw[9])))
            ber_list.append(str(self._calc_ber(dom_raw[10], dom_raw[11])))
            ber_list.append(str(self._calc_ber(dom_raw[12], dom_raw[13])))
            ber_list.append(str(self._calc_ber(dom_raw[14], dom_raw[15])))
        elif self.sfp_type == DSFP_TYPE:
            ber_list.append(str(self._calc_ber(dom_raw[0], dom_raw[1])))
            ber_list.append(str(self._calc_ber(dom_raw[2], dom_raw[3])))
        else:
            return ['0.0'] * 8
        return ber_list

    def prbs_gen_check_set(self, pos, control, pattern, ops_type):
        print('')
        convert_ctl = self.string_to_bytes(control)
        pattern_id = self.get_prbs_pattern_id(pattern)
        if pattern_id is None:
            print("Pattern name not found")
            return False

        pattern_offset_map = {
            ('host', 'gen'): (PAGE13 * SFP_PAGE_SIZE + HOST_SIDE_GENERATOR_PATTERN_BEGIN),
            ('host', 'check'): (PAGE13 * SFP_PAGE_SIZE + HOST_SIDE_CHECKER_PATTERN_BEGIN),
            ('media', 'gen'): (PAGE13 * SFP_PAGE_SIZE + MEDIA_SIDE_GENERATOR_PATTERN_BEGIN),
            ('media', 'check'): (PAGE13 * SFP_PAGE_SIZE + MEDIA_SIDE_CHECKER_PATTERN_BEGIN)
        }
        control_offset_map = {
            ('host', 'gen'): (PAGE13 * SFP_PAGE_SIZE + HOST_SIDE_GENERATOR_ENABLE_OFFSET),
            ('host', 'check'): (PAGE13 * SFP_PAGE_SIZE + HOST_SIDE_CHECKER_ENABLE_OFFSET),
            ('media', 'gen'): (PAGE13 * SFP_PAGE_SIZE + MEDIA_SIDE_GENERATOR_ENABLE_OFFSET),
            ('media', 'check'): (PAGE13 * SFP_PAGE_SIZE + MEDIA_SIDE_CHECKER_ENABLE_OFFSET)
        }
        key = (pos, ops_type)
        offset = pattern_offset_map.get(key)
        pattern_id |= (pattern_id << 4)
        pattern_id = pattern_id.to_bytes(1, 'little')
        if offset is not None:
             for i in range(4):
                 if not self.write_eeprom(offset + i, 1, bytearray(pattern_id)):
                    return False
        else:
            print("Faled to get pattern offset for {}".format(key))
            return False

        offset = control_offset_map.get(key)
        if offset:
            if not self.write_eeprom(offset, 1, bytearray(convert_ctl)):
                return False
        else:
            print("Faled to get control offset for {}".format(key))
            return False
        return True

    def get_name(self):
        """
        Retrieves the name of the device
            Returns:
            string: The name of the device
        """
        # Name of the port/sfp ?
        return 'PORT{}'.format(self.port_index)

    def get_presence(self):
        """
        Retrieves the presence of the PSU
        Returns:
            bool: True if PSU is present, False if not
        """
        output = self.pddf_obj.get_attr_name_output(self.device, 'xcvr_present')
        if not output:
            return False

        mode = output['mode']
        modpres = output['status'].rstrip()
        if 'XCVR' in self.plugin_data:
            if 'xcvr_present' in self.plugin_data['XCVR']:
                ptype = self.sfp_type
                vtype = 'valmap-'+ptype
                if vtype in self.plugin_data['XCVR']['xcvr_present'][mode]:
                    vmap = self.plugin_data['XCVR']['xcvr_present'][mode][vtype]
                    if modpres in vmap:
                        return vmap[modpres]
                    else:
                        return False
        # if self.plugin_data doesn't specify anything regarding Transceivers
        if modpres == '1':
            return True
        else:
            return False

    def get_reset_status(self):
        """
        Retrieves the reset status of SFP
        Returns:
            A Boolean, True if reset enabled, False if disabled
        """
        reset_status = None
        device = 'PORT{}'.format(self.port_index)
        output = self.pddf_obj.get_attr_name_output(device, 'xcvr_reset')

        if output:
            status = int(output['status'].rstrip())

            if status == 1:
                reset_status = True
            else:
                reset_status = False

        return reset_status

    def get_rx_los(self):
        """
        Retrieves the RX LOS (lost-of-signal) status of SFP
        Returns:
            A Boolean, True if SFP has RX LOS, False if not.
            Note : RX LOS status is latched until a call to get_rx_los or a reset.
        """
        rx_los = None
        device = 'PORT{}'.format(self.port_index)
        output = self.pddf_obj.get_attr_name_output(device, 'xcvr_rxlos')

        if output:
            status = int(output['status'].rstrip())

            if status == 1:
                rx_los = True
            else:
                rx_los = False
        else:
            # Use common SfpOptoeBase implementation for get_rx_los
            rx_los = super().get_rx_los()

        return rx_los

    def get_tx_fault(self):
        """
        Retrieves the TX fault status of SFP
        Returns:
            A Boolean, True if SFP has TX fault, False if not
            Note : TX fault status is lached until a call to get_tx_fault or a reset.
        """
        tx_fault = None
        device = 'PORT{}'.format(self.port_index)
        output = self.pddf_obj.get_attr_name_output(device, 'xcvr_txfault')

        if output:
            status = int(output['status'].rstrip())

            if status == 1:
                tx_fault = True
            else:
                tx_fault = False
        else:
            # Use common SfpOptoeBase implementation for get_tx_fault
            tx_fault = super().get_tx_fault()

        return tx_fault

    def get_tx_disable(self):
        """
        Retrieves the tx_disable status of this SFP
        Returns:
            A Boolean, True if tx_disable is enabled, False if disabled
        """
        tx_disable = False
        device = 'PORT{}'.format(self.port_index)
        output = self.pddf_obj.get_attr_name_output(device, 'xcvr_txdisable')

        if output:
            status = int(output['status'].rstrip())

            if status == 1:
                tx_disable = True
            else:
                tx_disable = False
        else:
            # Use common SfpOptoeBase implementation for get_tx_disable
            tx_disable = super().get_tx_disable()

        return tx_disable

    def get_lpmode(self):
        """
        Retrieves the lpmode (low power mode) status of this SFP
        Returns:
            A Boolean, True if lpmode is enabled, False if disabled
        """
        lpmode = False
        device = 'PORT{}'.format(self.port_index)
        output = self.pddf_obj.get_attr_name_output(device, 'xcvr_lpmode')

        if output:
            status = int(output['status'].rstrip())

            if status == 1:
                lpmode = True
            else:
                lpmode = False
        else:
            xcvr_id = self._xcvr_api_factory._get_id()
            if xcvr_id is not None:
                if xcvr_id == 0x18 or xcvr_id == 0x19 or xcvr_id == 0x1e:
                    # QSFP-DD or OSFP
                    # Use common SfpOptoeBase implementation for get_lpmode
                    lpmode = super().get_lpmode()
                elif xcvr_id == 0x11 or xcvr_id == 0x0d or xcvr_id == 0x0c:
                    # QSFP28, QSFP+, QSFP
                    # get_power_set() is not defined in the optoe_base class
                    api = self.get_xcvr_api()
                    power_set = api.get_power_set()
                    power_override = self.get_power_override()
                    # By default the lpmode pin is pulled high as mentioned in the sff community
                    return power_set if power_override else True

        return lpmode

    def get_intr_status(self):
        """
        Retrieves the interrupt status for this transceiver
        Returns:
            A Boolean, True if there is interrupt, False if not
        """
        intr_status = False

        # Interrupt status can be checked for absent ports too
        device = 'PORT{}'.format(self.port_index)
        output = self.pddf_obj.get_attr_name_output(device, 'xcvr_intr_status')

        if output:
            status = int(output['status'].rstrip())

            if status == 1:
                intr_status = True
            else:
                intr_status = False

        return intr_status

    def reset(self):
        """
        Reset SFP and return all user module settings to their default srate.
        Returns:
            A boolean, True if successful, False if not
        """
        status = False
        device = 'PORT{}'.format(self.port_index)
        path = self.pddf_obj.get_path(device, 'xcvr_reset')
        if path:
            try:
                with open(path, 'r+') as f:
                    f.seek(0)
                    f.write('1')
                    time.sleep(1)
                    f.seek(0)
                    f.write('0')
                    status = True
            except (IOError, OSError):
                status = False
        else:
            # Use common SfpOptoeBase implementation for reset
            status = super().reset()

        return status

    def tx_disable(self, tx_disable):
        """
        Disable SFP TX for all channels
        Args:
            tx_disable : A Boolean, True to enable tx_disable mode, False to disable
                         tx_disable mode.
        Returns:
            A boolean, True if tx_disable is set successfully, False if not
        """
        # find out a generic implementation of tx_disable for SFP, QSFP and OSFP
        status = False
        device = 'PORT{}'.format(self.port_index)
        path = self.pddf_obj.get_path(device, 'xcvr_txdisable')

        # TODO: put the optic based reset logic using EEPROM
        if path:
            try:
                f = open(path, 'r+')
            except IOError as e:
                return False

            try:
                if tx_disable:
                    f.write('1')
                else:
                    f.write('0')
                f.close()
                status = True
            except IOError as e:
                status = False
        else:
            # Use common SfpOptoeBase implementation for tx_disable
            status = super().tx_disable(tx_disable)


        return status

    def set_lpmode(self, lpmode):
        """
        Sets the lpmode (low power mode) of SFP
        Args:
            lpmode: A Boolean, True to enable lpmode, False to disable it
            Note  : lpmode can be overridden by set_power_override
        Returns:
            A boolean, True if lpmode is set successfully, False if not
        """
        status = False
        device = 'PORT{}'.format(self.port_index)
        path = self.pddf_obj.get_path(device, 'xcvr_lpmode')

        if path:
            try:
                f = open(path, 'r+')
            except IOError as e:
                return False

            try:
                if lpmode:
                    f.write('1')
                else:
                    f.write('0')

                f.close()
                status = True
            except IOError as e:
                status = False
        else:
            xcvr_id = self._xcvr_api_factory._get_id()
            if xcvr_id is not None:
                if xcvr_id == 0x18 or xcvr_id == 0x19 or xcvr_id == 0x1e:
                    # QSFP-DD or OSFP
                    # Use common SfpOptoeBase implementation for set_lpmode
                    status = super().set_lpmode(lpmode)
                elif xcvr_id == 0x11 or xcvr_id == 0x0d or xcvr_id == 0x0c:
                    # QSFP28, QSFP+, QSFP
                    if lpmode is True:
                        status = self.set_power_override(True, True)
                    else:
                        status = self.set_power_override(True, False)

        return status

    def get_position_in_parent(self):
        """
        Retrieves 1-based relative physical position in parent device.
        Returns:
            integer: The 1-based relative physical position in parent
            device or -1 if cannot determine the position
        """
        return self.port_index

    def is_replaceable(self):
        """
        Indicate whether the SFP is replaceable.
        Returns:
            bool: True if it is replaceable.
        """
        return True

    def dump_sysfs(self):
        return self.pddf_obj.cli_dump_dsysfs('xcvr')

    def get_sfp_error(self):
        """
        Checks the status of the SFP
        Returns:
            A tuple, (error_code, error_desc)
            error_code: A integer, the error code of the SFP
            error_desc: A string, the error description of the SFP
        """
        error_code = SfpBase.SFP_CLX_ERROR_CODE_RESERVED
        error_desc_dict = SfpBase.SFP_CLX_ERROR_CODE_TO_ERROR_DESC_DICT
        error_desc = []
        if not self.get_presence():
            return SfpBase.SFP_CLX_ERROR_CODE_RESERVED, ''

        # bad eeprom
        buf = self.read_eeprom(0x00, 1)
        if buf is None or buf[0] == 00:
            error_code = SfpBase.SFP_CLX_ERROR_CODE_BAD_EEPROM
            return error_code, error_desc_dict[error_code]

        # unknown module
        api = self.get_xcvr_api()
        if api is None:
            error_code = SfpBase.SFP_CLX_ERROR_CODE_UNKNOWN_MODULE
            return error_code, error_desc_dict[error_code]

        # module error and datapath error
        if isinstance(api, CmisApi):
            status = api.get_transceiver_status()
            if status['module_state'] != CmisCodes.MODULE_STATE[3]:
                error_code = SfpBase.SFP_CLX_ERROR_CODE_MODULE_ERROR
                error_desc.append(status['module_state'])
                if status['module_fault_cause'] != CmisCodes.MODULE_FAULT_CAUSE[0]:
                    error_desc.append(status['module_fault_cause'])
                return error_code, "|".join(error_desc)

            if status['datapath_firmware_fault']:
                return SfpBase.SFP_CLX_ERROR_CODE_MODULE_ERROR, "Datapath firmware fault"
            if status['module_firmware_fault']:
                return SfpBase.SFP_CLX_ERROR_CODE_MODULE_ERROR, "Module firmware fault"

            if status['temphighalarm_flag']:
                error_desc.append("High temperature")
            if status['templowalarm_flag']:
                error_desc.append("Low temperature")
            if status['vcchighalarm_flag']:
                error_desc.append("High supply voltage")
            if status['vcclowalarm_flag']:
                error_desc.append("Low supply voltage")
            if status['lasertemphighalarm_flag']:
                error_desc.append("High laser temperature")
            if status['lasertemplowalarm_flag']:
                error_desc.append("Low laser temperature")
            if len(error_desc) > 0:
                error_code = SfpBase.SFP_CLX_ERROR_CODE_MODULE_ERROR
                return error_code, "|".join(error_desc)

            if api.is_flat_memory():
                return SfpBase.SFP_CLX_ERROR_CODE_RESERVED, ''

            # datapath error check for cmis
            dp_state = api.get_datapath_state()
            config_lane_status = api.get_config_datapath_hostlane_status()
            config_lane = 0
            for lane in range(api.NUM_CHANNELS):
                name = "{}_{}_{}".format(consts.STAGED_CTRL_APSEL_FIELD, 0, lane + 1)
                appl = api.xcvr_eeprom.read(name)
                if (appl is None) or ((appl >> 4) == 0):
                    continue
                config_lane |= (1 << lane)

            for lane in range(api.NUM_CHANNELS):
                name = "{}_{}_{}".format(consts.STAGED_CTRL_APSEL_FIELD, 0, lane + 1)
                appl = api.xcvr_eeprom.read(name)
                if (appl is None) or ((appl >> 4) == 0):
                    continue

                if (config_lane & (1 << lane)) != 0:
                    continue

                name = "ConfigStatusLane{}".format(lane + 1)
                if conf_state[name] != CmisCodes.CONFIG_STATUS[1]:
                    error_desc = conf_state[name]
                    break

                name = "DP{}State".format(lane + 1)
                if dp_state[name] != CmisCodes.DATAPATH_STATE[4]:
                    error_desc = dp_state[name]
                    break

            if len(error_desc) > 0:
                error_code = SfpBase.SFP_CLX_ERROR_CODE_MODULE_DATAPATH_ERROR
                return error_code, "|".join(error_desc)

            config_lanes_list = []
            for lane in range(api.NUM_CHANNELS):
                if (config_lane & (1 << lane)) != 0:
                    config_lanes_list.append(lane + 1)

            for lane in config_lanes_list:
                if status['txoutput_status%d' % lane] != 'N/A' and status['txoutput_status%d' % lane] != True:
                    error_desc.append('TX output signal muted or invalid')
                    break
            for lane in config_lanes_list:
                if status['rxoutput_status_hostlane%d' % lane] != 'N/A' and status['rxoutput_status_hostlane%d' % lane] != True:
                    error_desc.append('RX output signal muted or invalid')
                    break
            try:
                for lane in config_lanes_list:
                    if status['tx%ddisable' % lane] != 'N/A' and status['tx%ddisable' % lane] != False:
                        error_desc.append('TX disable')
                        break
            except Exception:
                pass
            for lane in config_lanes_list:
                if status['txfault%d' % lane] != 'N/A' and status['txfault%d' % lane] != False:
                    error_desc.append('TX fault')
                    break
            for lane in config_lanes_list:
                if status['txlos_hostlane%d' % lane] != 'N/A' and status['txlos_hostlane%d' % lane] != False:
                    error_desc.append('TX loss of signal')
                    break
            for lane in config_lanes_list:
                if status['txcdrlol_hostlane%d' % lane] != 'N/A' and status['txcdrlol_hostlane%d' % lane] != False:
                    error_desc.append('TX clock and data recovery loss of lock')
                    break
            for lane in config_lanes_list:
                if status['rxlos%d' % lane] != 'N/A' and status['rxlos%d' % lane] != False:
                    error_desc.append('RX loss of signal')
                    break
            for lane in config_lanes_list:
                if status['rxcdrlol%d' % lane] != 'N/A' and status['rxcdrlol%d' % lane] != False:
                    error_desc.append('RX clock and data recovery loss of lock')
                    break
            try:
                for lane in config_lanes_list:
                    if status['tx_eq_fault%d' % lane] != 'N/A' and status['tx_eq_fault%d' % lane] != False:
                        error_desc.append('TX adaptive input equalization fault')
                        break
            except Exception:
                pass
            if api.get_tx_power_support():
                for lane in config_lanes_list:
                    if status['txpowerhighalarm_flag%d' % lane] != 'N/A' and status['txpowerhighalarm_flag%d' % lane] != False:
                        error_desc.append('TX power high alarm')
                        break
                for lane in config_lanes_list:
                    if status['txpowerlowalarm_flag%d' % lane] != 'N/A' and status['txpowerlowalarm_flag%d' % lane] != False:
                        error_desc.append('TX power low alarm')
                        break
            if api.get_tx_bias_support():
                for lane in config_lanes_list:
                    if status['txbiashighalarm_flag%d' % lane] != 'N/A' and status['txbiashighalarm_flag%d' % lane] != False:
                        error_desc.append('TX bias high alarm')
                        break
                for lane in config_lanes_list:
                    if status['txbiaslowalarm_flag%d' % lane] != 'N/A' and status['txbiaslowalarm_flag%d' % lane] != False:
                        error_desc.append('TX bias low alarm')
                        break
            if api.get_rx_power_support():
                for lane in config_lanes_list:
                    if status['rxpowerhighalarm_flag%d' % lane] != 'N/A' and status['rxpowerhighalarm_flag%d' % lane] != False:
                        error_desc.append('RX power high alarm')
                        break
                for lane in config_lanes_list:
                    if status['rxpowerlowalarm_flag%d' % lane] != 'N/A' and status['rxpowerlowalarm_flag%d' % lane] != False:
                        error_desc.append('RX power low alarm')
                        break
            if len(error_desc) > 0:
                error_code = SfpBase.SFP_CLX_ERROR_CODE_MODULE_DATAPATH_ERROR
                return error_code, "|".join(error_desc)

            return SfpBase.SFP_CLX_ERROR_CODE_RESERVED, ''

        elif isinstance(api, Sff8636Api) or isinstance(api, Sff8436Api) or isinstance(api, Sff8472Api):
            status = api.get_transceiver_status()
            for lane in range(1, api.NUM_CHANNELS + 1):
                if status['rxlos%d' % lane] != 'N/A' and status['rxlos%d' % lane] != False:
                    error_desc.append('RX loss of signal')
                    break
                if status['txfault%d' % lane] != 'N/A' and status['txfault%d' % lane] != False:
                    error_desc.append('TX fault')
                    break
                if status['tx%ddisable' % lane] != 'N/A' and status['tx%ddisable' % lane] != False:
                    error_desc.append('TX disable')
                    break
            if len(error_desc) > 0:
                error_code = SfpBase.SFP_CLX_ERROR_CODE_MODULE_DATAPATH_ERROR
                return error_code, "|".join(error_desc)

            return SfpBase.SFP_CLX_ERROR_CODE_RESERVED, ''
        else:
            return SfpBase.SFP_CLX_ERROR_CODE_RESERVED, ''


    def get_error_description(self):
        """
        Retrieves the error description of the SFP
        Returns:
            A string, the error description of the SFP
        """
        if not self.get_presence():
            return SfpBase.SFP_STATUS_UNPLUGGED

        try:
            error_code, error_desc = self.get_sfp_error()
            if error_code != SfpBase.SFP_CLX_ERROR_CODE_RESERVED:
                return error_desc
        except Exception as e:
            print("Error checking SFP error: {}".format(e))
        return SfpBase.SFP_STATUS_OK
