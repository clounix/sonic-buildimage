#!/usr/bin/env python
#
# Name: sfp_8472.py, version: 1.0 (Patched for SONiC xcvrd/sfputil compatibility)
#
# Description: Module contains the definitions of SONiC platform APIs
#

import os
import sys
import time
import logging
from ctypes import create_string_buffer

try:
    from sonic_platform_base.sfp_base import SfpBase
    from sonic_platform_base.sonic_sfp.sff8472 import sff8472Dom
    from sonic_platform_base.sonic_sfp.sff8472 import sff8472InterfaceId
    from sonic_platform_base.sonic_sfp.sff8472 import sffbase
    from sonic_platform_base.sonic_sfp.sfputilhelper import SfpUtilHelper
    from sonic_py_common import device_info
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

INFO_OFFSET = 0
DOM_OFFSET  = 256

XCVR_INTFACE_BULK_OFFSET    = 0
XCVR_INTFACE_BULK_WIDTH_SFP = 21
XCVR_VENDOR_NAME_OFFSET     = 20
XCVR_VENDOR_NAME_WIDTH      = 16
XCVR_VENDOR_OUI_OFFSET      = 37
XCVR_VENDOR_OUI_WIDTH       = 3
XCVR_VENDOR_PN_OFFSET       = 40
XCVR_VENDOR_PN_WIDTH        = 16
XCVR_HW_REV_OFFSET          = 56
XCVR_HW_REV_WIDTH_SFP       = 4
XCVR_VENDOR_SN_OFFSET       = 68
XCVR_VENDOR_SN_WIDTH        = 16
XCVR_VENDOR_DATE_OFFSET     = 84
XCVR_VENDOR_DATE_WIDTH      = 8
XCVR_DOM_CAPABILITY_OFFSET  = 92
XCVR_DOM_CAPABILITY_WIDTH   = 1

# Offset for values in SFP eeprom
SFP_TEMPE_OFFSET            = 96
SFP_TEMPE_WIDTH             = 2
SFP_VOLT_OFFSET             = 98
SFP_VOLT_WIDTH              = 2
SFP_CHANNL_MON_OFFSET       = 100
SFP_CHANNL_MON_WIDTH        = 6
SFP_MODULE_THRESHOLD_OFFSET = 0
SFP_MODULE_THRESHOLD_WIDTH  = 40
SFP_CHANNL_THRESHOLD_OFFSET = 112
SFP_CHANNL_THRESHOLD_WIDTH  = 2
SFP_STATUS_CONTROL_OFFSET   = 110
SFP_STATUS_CONTROL_WIDTH    = 1
SFP_TX_DISABLE_HARD_BIT     = 7
SFP_TX_DISABLE_SOFT_BIT     = 6
TRANSCEIVER_PATH = '/sys/s3ip/transceiver/'

class Sfp_8472(SfpBase):

    port_start = 0
    port_end = 55
 
    __port_to_i2c_mapping = {}
    def __init__(self, index):
        self.__index = index

        self.__platform = str(device_info.get_platform())
        self.__hwsku    = str(device_info.get_hwsku())

        self.__attr_path_prefix = '/sys/s3ip/transceiver/eth{}/'.format(self.__index+1)
        self.__presence_attr = None
        self.__eeprom_path = None
     
        self.__eeprom_path = TRANSCEIVER_PATH+'eth{}/'.format(self.__index+1)+'eeprom'
        self.__presence_attr = TRANSCEIVER_PATH+'eth{}/'.format(self.__index+1)+'present'

        SfpBase.__init__(self)

    def __get_attr_value(self, attr_path):
        retval = 'ERR'
        if (not os.path.isfile(attr_path)):
            return retval

        try:
            with open(attr_path, 'r') as fd:
                retval = fd.read()
        except Exception as error:
            logging.error("Unable to open %s file !", attr_path)

        retval = retval.rstrip(' \t\n\r')
        return retval

    def __is_host(self):
        return os.system("docker > /dev/null 2>&1") == 0

    def __get_path_to_port_config_file(self):
        host_platform_root_path = '/usr/share/sonic/device'
        docker_hwsku_path = '/usr/share/sonic/hwsku'

        host_platform_path = "/".join([host_platform_root_path, self.__platform])
        hwsku_path = "/".join([host_platform_path, self.__hwsku]) if self.__is_host() else docker_hwsku_path

        return "/".join([hwsku_path, "port_config.ini"])

    def __read_eeprom_specific_bytes(self, offset, num_bytes):
        sysfsfile_eeprom = None
        eeprom_raw = []

        for i in range(0, num_bytes):
            eeprom_raw.append("0x00")

        sysfs_eeprom_path = self.__eeprom_path
        try:
            sysfsfile_eeprom = open(sysfs_eeprom_path, mode="rb", buffering=0)
            sysfsfile_eeprom.seek(offset)
            raw = sysfsfile_eeprom.read(num_bytes)
            raw_len = len(raw)
            for n in range(0, raw_len):
                eeprom_raw[n] = hex(raw[n])[2:].zfill(2)
        except:
            pass
        finally:
            if sysfsfile_eeprom:
                sysfsfile_eeprom.close()

        return eeprom_raw

    def __convert_string_to_num(self, value_str):
        if "-inf" in value_str:
            return 'N/A'
        elif "Unknown" in value_str:
            return 'N/A'
        elif 'dBm' in value_str:
            t_str = value_str.rstrip('dBm')
            return float(t_str)
        elif 'mA' in value_str:
            t_str = value_str.rstrip('mA')
            return float(t_str)
        elif 'C' in value_str:
            t_str = value_str.rstrip('C')
            return float(t_str)
        elif 'Volts' in value_str:
            t_str = value_str.rstrip('Volts')
            return float(t_str)
        else:
            return 'N/A'

##############################################
# Device methods
##############################################
    def get_presence(self):
        """
        Retrieves the presence of the device
        Returns: bool: True if device is present, False if not
        """
        try:
            attr_rv = self.__get_attr_value(self.__attr_path_prefix + 'present')
            if attr_rv in ('ERR', None, '', 'NA', 'n/a'):
                return False
            val = attr_rv.strip().lower()
            base = 16 if val.startswith('0x') else 10
            return (int(val, base) & 0x1) == 1
        except (ValueError, TypeError, Exception):
            return False

##############################################
# SFP methods
##############################################

    def get_transceiver_info(self):
        """
        Retrieves transceiver info of this SFP
        """
        transceiver_info_dict_keys = ['type',                      'hardware_rev',
                                      'serialnum',                 'manufacturename',
                                      'modelname',                 'Connector',
                                      'encoding',                  'ext_identifier',
                                      'ext_rateselect_compliance', 'cable_type',
                                      'cable_length',              'nominal_bit_rate',
                                      'specification_compliance',  'vendor_date',
                                      'vendor_oui']

        sfp_cable_length_tup = ('LengthSMFkm-UnitsOfKm',  'LengthSMF(UnitsOf100m)',
                                'Length50um(UnitsOf10m)', 'Length62.5um(UnitsOfm)',
                                'LengthCable(UnitsOfm)',  'LengthOM3(UnitsOf10m)')

        sfp_compliance_code_tup = ('10GEthernetComplianceCode',    'InfinibandComplianceCode',
                                   'ESCONComplianceCodes',         'SONETComplianceCodes',
                                   'EthernetComplianceCodes',      'FibreChannelLinkLength',
                                   'FibreChannelTechnology',        'SFP+CableTechnology',
                                   'FibreChannelTransmissionMedia', 'FibreChannelSpeed')

        sfpi_obj = sff8472InterfaceId()
        if not self.get_presence() or not sfpi_obj:
            return {}

        offset = INFO_OFFSET

        sfp_interface_bulk_raw  = self.__read_eeprom_specific_bytes((offset + XCVR_INTFACE_BULK_OFFSET), XCVR_INTFACE_BULK_WIDTH_SFP)
        sfp_interface_bulk_data = sfpi_obj.parse_sfp_info_bulk(sfp_interface_bulk_raw, 0)

        sfp_vendor_name_raw  = self.__read_eeprom_specific_bytes((offset + XCVR_VENDOR_NAME_OFFSET), XCVR_VENDOR_NAME_WIDTH)
        sfp_vendor_name_data = sfpi_obj.parse_vendor_name(sfp_vendor_name_raw, 0)

        sfp_vendor_pn_raw  = self.__read_eeprom_specific_bytes((offset + XCVR_VENDOR_PN_OFFSET), XCVR_VENDOR_PN_WIDTH)
        sfp_vendor_pn_data = sfpi_obj.parse_vendor_pn(sfp_vendor_pn_raw, 0)

        sfp_vendor_rev_raw  = self.__read_eeprom_specific_bytes((offset + XCVR_HW_REV_OFFSET), XCVR_HW_REV_WIDTH_SFP)
        sfp_vendor_rev_data = sfpi_obj.parse_vendor_rev(sfp_vendor_rev_raw, 0)

        sfp_vendor_sn_raw  = self.__read_eeprom_specific_bytes((offset + XCVR_VENDOR_SN_OFFSET), XCVR_VENDOR_SN_WIDTH)
        sfp_vendor_sn_data = sfpi_obj.parse_vendor_sn(sfp_vendor_sn_raw, 0)

        sfp_vendor_oui_raw = self.__read_eeprom_specific_bytes((offset + XCVR_VENDOR_OUI_OFFSET), XCVR_VENDOR_OUI_WIDTH)
        if sfp_vendor_oui_raw is not None:
            sfp_vendor_oui_data = sfpi_obj.parse_vendor_oui(sfp_vendor_oui_raw, 0)

        sfp_vendor_date_raw  = self.__read_eeprom_specific_bytes((offset + XCVR_VENDOR_DATE_OFFSET), XCVR_VENDOR_DATE_WIDTH)
        sfp_vendor_date_data = sfpi_obj.parse_vendor_date(sfp_vendor_date_raw, 0)

        transceiver_info_dict = dict.fromkeys(transceiver_info_dict_keys, 'N/A')

        if sfp_interface_bulk_data:
            transceiver_info_dict['type']                      = sfp_interface_bulk_data['data']['type']['value']
            transceiver_info_dict['connector']                 = sfp_interface_bulk_data['data']['Connector']['value']
            transceiver_info_dict['encoding']                  = sfp_interface_bulk_data['data']['EncodingCodes']['value']
            transceiver_info_dict['ext_identifier']            = sfp_interface_bulk_data['data']['Extended Identifier']['value']
            transceiver_info_dict['ext_rateselect_compliance'] = sfp_interface_bulk_data['data']['RateIdentifier']['value']
            transceiver_info_dict['type_abbrv_name']           = sfp_interface_bulk_data['data']['type_abbrv_name']['value']
            transceiver_info_dict['nominal_bit_rate']          = str(sfp_interface_bulk_data['data']['NominalSignallingRate(UnitsOf100Mbd)']['value'])

        transceiver_info_dict['manufacturer'] = sfp_vendor_name_data['data']['Vendor Name']['value'] if sfp_vendor_name_data else 'N/A'
        transceiver_info_dict['model']       = sfp_vendor_pn_data['data']['Vendor PN']['value'] if sfp_vendor_pn_data else 'N/A'
        transceiver_info_dict['hardware_rev']     = sfp_vendor_rev_data['data']['Vendor Rev']['value'] if sfp_vendor_rev_data else 'N/A'
        transceiver_info_dict['serial']       = sfp_vendor_sn_data['data']['Vendor SN']['value'] if sfp_vendor_sn_data else 'N/A'
        transceiver_info_dict['vendor_oui']      = sfp_vendor_oui_data['data']['Vendor OUI']['value'] if sfp_vendor_oui_data else 'N/A'
        transceiver_info_dict['vendor_date']     = sfp_vendor_date_data['data']['VendorDataCode(YYYY-MM-DD Lot)']['value'] if sfp_vendor_date_data else 'N/A'

        transceiver_info_dict['cable_type']      = "Unknown"
        transceiver_info_dict['cable_length']    = "Unknown"
        for key in sfp_cable_length_tup:
            if key in sfp_interface_bulk_data['data']:
                transceiver_info_dict['cable_type']   = key
                transceiver_info_dict['cable_length'] = str(sfp_interface_bulk_data['data'][key]['value'])

        compliance_code_dict  = dict()
        for key in sfp_compliance_code_tup:
            if key in sfp_interface_bulk_data['data']['Specification compliance']['value']:
                compliance_code_dict[key] = sfp_interface_bulk_data['data']['Specification compliance']['value'][key]['value']

        transceiver_info_dict['specification_compliance'] = str(compliance_code_dict)

        transceiver_info_dict['application_advertisement'] = 'N/A'
        if 'type_abbrv_name' not in transceiver_info_dict or transceiver_info_dict['type_abbrv_name'] == 'N/A':
            transceiver_info_dict['type_abbrv_name'] = transceiver_info_dict.get('type', 'SFP')

        mandatory_keys = {
            'application_advertisement': 'N/A',
            'type_abbrv_name': transceiver_info_dict.get('type', 'N/A'),
            'cable_type': transceiver_info_dict.get('cable_type', 'Unknown'),
            'nominal_bit_rate': transceiver_info_dict.get('nominal_bit_rate', 'N/A')
        }
        for k, v in mandatory_keys.items():
            if k not in transceiver_info_dict:
                transceiver_info_dict[k] = v

        return transceiver_info_dict

    def get_transceiver_bulk_status(self):
        """
        Retrieves transceiver bulk status of this SFP
        """
        transceiver_dom_info_dict_keys = ['rx_los',       'tx_fault',
                                          'reset_status', 'power_lpmode',
                                          'tx_disable',   'tx_disable_channel',
                                          'temperature',  'voltage',
                                          'rx1power',     'rx2power',
                                          'rx3power',     'rx4power',
                                          'tx1bias',      'tx2bias',
                                          'tx3bias',      'tx4bias',
                                          'tx1power',     'tx2power',
                                          'tx3power',     'tx4power']

        sfpd_obj = sff8472Dom()
        if not self.get_presence() or not sfpd_obj:
            return {}

        sysfsfile_eeprom = None
        bdata=''
        sysfsfile_eeprom = open(self.__eeprom_path, mode="rb", buffering=0)
        bdata = sysfsfile_eeprom.read()
        if len(bdata) <= 256:
            sysfsfile_eeprom.close()
            return None
        sysfsfile_eeprom.close()

        eeprom_ifraw = self.__read_eeprom_specific_bytes(0, DOM_OFFSET)
        sfpi_obj = sff8472InterfaceId(eeprom_ifraw)
        cal_type = sfpi_obj.get_calibration_type()
        sfpd_obj._calibration_type = cal_type

        offset = DOM_OFFSET
        transceiver_dom_info_dict = dict.fromkeys(transceiver_dom_info_dict_keys, 'N/A')

        dom_temperature_raw = self.__read_eeprom_specific_bytes((offset + SFP_TEMPE_OFFSET), SFP_TEMPE_WIDTH)
        if dom_temperature_raw is not None:
            dom_temperature_data = sfpd_obj.parse_temperature(dom_temperature_raw, 0)
            transceiver_dom_info_dict['temperature'] = dom_temperature_data['data']['Temperature']['value']

        dom_voltage_raw = self.__read_eeprom_specific_bytes((offset + SFP_VOLT_OFFSET), SFP_VOLT_WIDTH)
        if dom_voltage_raw is not None:
            dom_voltage_data = sfpd_obj.parse_voltage(dom_voltage_raw, 0)
            transceiver_dom_info_dict['voltage'] = dom_voltage_data['data']['Vcc']['value']

        dom_channel_monitor_raw = self.__read_eeprom_specific_bytes((offset + SFP_CHANNL_MON_OFFSET), SFP_CHANNL_MON_WIDTH)
        if dom_channel_monitor_raw is not None:
            dom_channel_monitor_data = sfpd_obj.parse_channel_monitor_params(dom_channel_monitor_raw, 0)
            transceiver_dom_info_dict['tx1power'] = dom_channel_monitor_data['data']['TXPower']['value']
            transceiver_dom_info_dict['rx1power'] = dom_channel_monitor_data['data']['RXPower']['value']
            transceiver_dom_info_dict['tx1bias'] = dom_channel_monitor_data['data']['TXBias']['value']

        for key in transceiver_dom_info_dict:
            transceiver_dom_info_dict[key] = self.__convert_string_to_num(transceiver_dom_info_dict[key])

        transceiver_dom_info_dict['reset_status']       = self.get_reset_status()
        transceiver_dom_info_dict['rx_los']             = self.get_rx_los()
        transceiver_dom_info_dict['tx_fault']           = self.get_tx_fault()
        transceiver_dom_info_dict['tx_disable']         = self.get_tx_disable()
        transceiver_dom_info_dict['tx_disable_channel'] = self.get_tx_disable_channel()
        transceiver_dom_info_dict['lp_mode']            = self.get_lpmode()

        return transceiver_dom_info_dict

    def get_transceiver_threshold_info(self):
        """
        Retrieves transceiver threshold info of this SFP
        """
        transceiver_dom_threshold_info_dict_keys = ['temphighalarm',    'temphighwarning',    'templowalarm',    'templowwarning',
                                                    'vcchighalarm',     'vcchighwarning',     'vcclowalarm',     'vcclowwarning',
                                                    'rxpowerhighalarm', 'rxpowerhighwarning', 'rxpowerlowalarm', 'rxpowerlowwarning',
                                                    'txpowerhighalarm', 'txpowerhighwarning', 'txpowerlowalarm', 'txpowerlowwarning',
                                                    'txbiashighalarm',  'txbiashighwarning',  'txbiaslowalarm',  'txbiaslowwarning']

        sfpd_obj = sff8472Dom()

        if not self.get_presence() or not sfpd_obj:
            return {}

        sysfsfile_eeprom = None
        bdata=''
        sysfsfile_eeprom = open(self.__eeprom_path, mode="rb", buffering=0)
        bdata = sysfsfile_eeprom.read()
        if len(bdata) <= 256:
            sysfsfile_eeprom.close()
            return None
        sysfsfile_eeprom.close()

        eeprom_ifraw = self.__read_eeprom_specific_bytes(0, DOM_OFFSET)
        sfpi_obj = sff8472InterfaceId(eeprom_ifraw)
        cal_type = sfpi_obj.get_calibration_type()
        sfpd_obj._calibration_type = cal_type

        offset = DOM_OFFSET
        transceiver_dom_threshold_info_dict = dict.fromkeys(transceiver_dom_threshold_info_dict_keys, 'N/A')
        dom_module_threshold_raw = self.__read_eeprom_specific_bytes((offset + SFP_MODULE_THRESHOLD_OFFSET), SFP_MODULE_THRESHOLD_WIDTH)
        if dom_module_threshold_raw is not None:
            dom_module_threshold_data = sfpd_obj.parse_alarm_warning_threshold(dom_module_threshold_raw, 0)

            transceiver_dom_threshold_info_dict['temphighalarm']      = dom_module_threshold_data['data']['TempHighAlarm']['value']
            transceiver_dom_threshold_info_dict['templowalarm']       = dom_module_threshold_data['data']['TempLowAlarm']['value']
            transceiver_dom_threshold_info_dict['temphighwarning']    = dom_module_threshold_data['data']['TempHighWarning']['value']
            transceiver_dom_threshold_info_dict['templowwarning']     = dom_module_threshold_data['data']['TempLowWarning']['value']

            transceiver_dom_threshold_info_dict['vcchighalarm']       = dom_module_threshold_data['data']['VoltageHighAlarm']['value']
            transceiver_dom_threshold_info_dict['vcclowalarm']        = dom_module_threshold_data['data']['VoltageLowAlarm']['value']
            transceiver_dom_threshold_info_dict['vcchighwarning']     = dom_module_threshold_data['data']['VoltageHighWarning']['value']
            transceiver_dom_threshold_info_dict['vcclowwarning']      = dom_module_threshold_data['data']['VoltageLowWarning']['value']

            transceiver_dom_threshold_info_dict['txbiashighalarm']    = dom_module_threshold_data['data']['BiasHighAlarm']['value']
            transceiver_dom_threshold_info_dict['txbiaslowalarm']     = dom_module_threshold_data['data']['BiasLowAlarm']['value']
            transceiver_dom_threshold_info_dict['txbiashighwarning']  = dom_module_threshold_data['data']['BiasHighWarning']['value']
            transceiver_dom_threshold_info_dict['txbiaslowwarning']   = dom_module_threshold_data['data']['BiasLowWarning']['value']

            transceiver_dom_threshold_info_dict['txpowerhighalarm']   = dom_module_threshold_data['data']['TXPowerHighAlarm']['value']
            transceiver_dom_threshold_info_dict['txpowerlowalarm']    = dom_module_threshold_data['data']['TXPowerLowAlarm']['value']
            transceiver_dom_threshold_info_dict['txpowerhighwarning'] = dom_module_threshold_data['data']['TXPowerHighWarning']['value']
            transceiver_dom_threshold_info_dict['txpowerlowwarning']  = dom_module_threshold_data['data']['TXPowerLowWarning']['value']

            transceiver_dom_threshold_info_dict['rxpowerhighalarm']   = dom_module_threshold_data['data']['RXPowerHighAlarm']['value']
            transceiver_dom_threshold_info_dict['rxpowerlowalarm']    = dom_module_threshold_data['data']['RXPowerLowAlarm']['value']
            transceiver_dom_threshold_info_dict['rxpowerhighwarning'] = dom_module_threshold_data['data']['RXPowerHighWarning']['value']
            transceiver_dom_threshold_info_dict['rxpowerlowwarning']  = dom_module_threshold_data['data']['RXPowerLowWarning']['value']

        for key in transceiver_dom_threshold_info_dict:
            transceiver_dom_threshold_info_dict[key] = self.__convert_string_to_num(transceiver_dom_threshold_info_dict[key])

        return transceiver_dom_threshold_info_dict

    def get_reset_status(self):
        return False

    def get_rx_los(self):
        rx_los = False
        status_control_raw = self.__read_eeprom_specific_bytes(SFP_STATUS_CONTROL_OFFSET, SFP_STATUS_CONTROL_WIDTH)
        if status_control_raw:
            data = int(status_control_raw[0], 16)
            rx_los = (sffbase().test_bit(data, 1) != 0)
        return rx_los

    def get_tx_fault(self):
        tx_fault = False
        status_control_raw = self.__read_eeprom_specific_bytes(SFP_STATUS_CONTROL_OFFSET, SFP_STATUS_CONTROL_WIDTH)
        if status_control_raw:
            data = int(status_control_raw[0], 16)
            tx_fault = (sffbase().test_bit(data, 2) != 0)
        return tx_fault

    def get_tx_disable(self):
        tx_disable = False
        status_control_raw = self.__read_eeprom_specific_bytes(SFP_STATUS_CONTROL_OFFSET, SFP_STATUS_CONTROL_WIDTH)
        if status_control_raw:
            data = int(status_control_raw[0], 16)
            tx_disable_hard = (sffbase().test_bit(data, SFP_TX_DISABLE_HARD_BIT) != 0)
            tx_disable_soft = (sffbase().test_bit(data, SFP_TX_DISABLE_SOFT_BIT) != 0)
            tx_disable = tx_disable_hard | tx_disable_soft
        return tx_disable

    def get_tx_disable_channel(self):
        return 0

    def get_lpmode(self):
        return False

    def get_power_override(self):
        return False

    def get_temperature(self):
        transceiver_dom_info_dict = self.get_transceiver_bulk_status()
        return transceiver_dom_info_dict.get("temperature", "N/A")

    def get_voltage(self):
        transceiver_dom_info_dict = self.get_transceiver_bulk_status()
        return transceiver_dom_info_dict.get("voltage", "N/A")

    def get_tx_bias(self):
        transceiver_dom_info_dict = self.get_transceiver_bulk_status()
        tx1_bs = transceiver_dom_info_dict.get("tx1bias", "N/A")
        return [tx1_bs, "N/A", "N/A", "N/A"] if transceiver_dom_info_dict else []

    def get_rx_power(self):
        transceiver_dom_info_dict = self.get_transceiver_bulk_status()
        rx1_pw = transceiver_dom_info_dict.get("rx1power", "N/A")
        return [rx1_pw, "N/A", "N/A", "N/A"] if transceiver_dom_info_dict else []

    def get_tx_power(self):
        transceiver_dom_info_dict = self.get_transceiver_bulk_status()
        tx1_pw = transceiver_dom_info_dict.get("tx1power", "N/A")
        return [tx1_pw, "N/A", "N/A", "N/A"] if transceiver_dom_info_dict else []

    def reset(self):
        return False

    def tx_disable(self, tx_disable):
        sysfs_eeprom_path = self.__eeprom_path
        status_control_raw = self.__read_eeprom_specific_bytes(SFP_STATUS_CONTROL_OFFSET, SFP_STATUS_CONTROL_WIDTH)
        if status_control_raw is not None:
            tx_disable_bit = 64 if tx_disable else 191
            status_control = int(status_control_raw[0], 16)
            tx_disable_ctl = (status_control | tx_disable_bit) if tx_disable else (status_control & tx_disable_bit)
            try:
                sysfsfile_eeprom = open(sysfs_eeprom_path, mode="r+b", buffering=0)
                buffer = create_string_buffer(1)
                buffer[0] = chr(tx_disable_ctl)
                sysfsfile_eeprom.seek(SFP_STATUS_CONTROL_OFFSET)
                sysfsfile_eeprom.write(buffer[0])
            except:
                return False
            finally:
                if sysfsfile_eeprom:
                    sysfsfile_eeprom.close()
                    time.sleep(0.01)
            return True
        return False

    def tx_disable_channel(self, channel, disable):
        return False

    def set_lpmode(self, lpmode):
        return False

    def set_power_override(self, power_override, power_set):
        return False
