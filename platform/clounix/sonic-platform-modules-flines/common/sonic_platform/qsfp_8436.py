#!/usr/bin/env python
#
# Name: qsfp_8436.py, version: 1.0 (Final Patched)
#
# Description: Module contains the definitions of SONiC platform APIs 
#

import os
import logging
from ctypes import create_string_buffer

try:
    from sonic_platform_base.sfp_base import SfpBase
    from sonic_platform_base.sonic_sfp.sff8436 import sff8436Dom
    from sonic_platform_base.sonic_sfp.sff8436 import sff8436InterfaceId
    from sonic_platform_base.sonic_sfp.sff8024 import ext_specification_compliance
    from sonic_platform_base.sonic_sfp.sfputilhelper import SfpUtilHelper
    from .helper import APIHelper
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

INFO_OFFSET = 128
THRE_OFFSET = 384 #128*3
DOM_OFFSET  = 0

XCVR_INTFACE_BULK_OFFSET     = 0
XCVR_INTFACE_BULK_WIDTH_QSFP = 20
XCVR_HW_REV_WIDTH_QSFP       = 2
XCVR_CABLE_LENGTH_WIDTH_QSFP = 5
XCVR_VENDOR_NAME_OFFSET      = 20
XCVR_VENDOR_NAME_WIDTH       = 16
XCVR_VENDOR_OUI_OFFSET       = 37
XCVR_VENDOR_OUI_WIDTH        = 3
XCVR_VENDOR_PN_OFFSET        = 40
XCVR_VENDOR_PN_WIDTH         = 16
XCVR_HW_REV_OFFSET           = 56
XCVR_HW_REV_WIDTH_OSFP       = 2
XCVR_SPECIFICATION_ETH_COMPLIANCE_OFFSET = 3
XCVR_SPECIFICATION_ETH_COMPLIANCE_WIDTH = 1
XCVR_EXT_SPECIFICATION_COMPLIANCE_OFFSET = 64
XCVR_EXT_SPECIFICATION_COMPLIANCE_WIDTH = 1
XCVR_VENDOR_SN_OFFSET        = 68
XCVR_VENDOR_SN_WIDTH         = 16
XCVR_VENDOR_DATE_OFFSET      = 84
XCVR_VENDOR_DATE_WIDTH       = 8
XCVR_DOM_CAPABILITY_OFFSET   = 92
XCVR_DOM_CAPABILITY_WIDTH    = 1

# Offset for values in QSFP eeprom
QSFP_DOM_REV_OFFSET                 = 1
QSFP_DOM_REV_WIDTH                  = 1
QSFP_TEMPE_OFFSET                   = 22
QSFP_TEMPE_WIDTH                    = 2
QSFP_VOLT_OFFSET                    = 26
QSFP_VOLT_WIDTH                     = 2
QSFP_CHANNL_MON_OFFSET              = 34
QSFP_CHANNL_MON_WIDTH               = 16
QSFP_CHANNL_MON_WITH_TX_POWER_WIDTH = 24
QSFP_CONTROL_OFFSET                 = 86
QSFP_CONTROL_WIDTH                  = 8
QSFP_CHANNL_RX_LOS_STATUS_OFFSET    = 3
QSFP_CHANNL_RX_LOS_STATUS_WIDTH     = 1
QSFP_CHANNL_TX_FAULT_STATUS_OFFSET  = 4
QSFP_CHANNL_TX_FAULT_STATUS_WIDTH   = 1
QSFP_POWEROVERRIDE_OFFSET           = 93
QSFP_POWEROVERRIDE_WIDTH            = 1
QSFP_MODULE_THRESHOLD_OFFSET        = 128
QSFP_MODULE_THRESHOLD_WIDTH         = 24
QSFP_CHANNEL_THRESHOLD_OFFSET       = 176
QSFP_CHANNEL_THRESHOLD_WIDTH        = 16

QSFP_REG_VALUE_ENABLE  = "0x1"
QSFP_REG_VALUE_DISABLE = "0x0"
TRANSCEIVER_PATH = '/sys/s3ip/transceiver/'

class QSfp_SFF8436(SfpBase):

    def __init__(self, index):
        self.__index = index
        self.__api_helper = APIHelper()

        self.__attr_path_prefix = '/sys/s3ip/transceiver/eth{}/'.format(self.__index+1)
        self.__presence_attr = None
        self.__eeprom_path = None
     
        self.__eeprom_path = TRANSCEIVER_PATH+'eth{}/'.format(self.__index+1)+'eeprom'
        self.__presence_attr = TRANSCEIVER_PATH+'eth{}/'.format(self.__index+1)+'present'
        self.__reset_attr = TRANSCEIVER_PATH+'eth{}/'.format(self.__index+1)+'reset'
        self.__lpmode_attr = TRANSCEIVER_PATH+'eth{}/'.format(self.__index+1)+'low_power_mode'

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

    def __set_attr_value(self, attr_path, value):
        try:
            with open(attr_path, 'r+') as reg_file:
                reg_file.write(value)
        except IOError as e:
            logging.error("Error: unable to open file: '%s'" % str(e))
            return False
        return True

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

    def __write_eeprom_specific_bytes(self, offset, buffer):
        sysfs_eeprom_path = self.__eeprom_path
        try:
            with open(sysfs_eeprom_path, "r+b") as sysfsfile_eeprom:
                sysfsfile_eeprom.seek(offset)
                sysfsfile_eeprom.write(buffer[0])
        except IOError as e:
            logging.error("Error: unable to open file: '%s'" % str(e))
            return False
        return True

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
        try:
            attr_rv = self.__api_helper.read_one_line_file(self.__attr_path_prefix + 'present')
            if attr_rv is None:
                return False
            val = attr_rv.strip().lower()
            if val in ('na', 'n/a', 'none', ''):
                return False
            base = 16 if val.startswith('0x') else 10
            return (int(val, base) & 0x1) == 1
        except (ValueError, TypeError, Exception):
            return False

##############################################
# SFP methods
##############################################
    def is_copper(self):
        ETHERNET_10_40G_COMPLIANCE = {
            1: "40G Active Cable (XLPPI)", 2: "40GBASE-LR4", 4: "40GBASE-SR4",
            8: "40GBASE-CR4", 16: "10GBASE-SR", 32: "10GBASE-LR",
            64: "10GBASE-LRM", 128: "Extended",
        }
        eth_compliance_raw = self.__read_eeprom_specific_bytes(INFO_OFFSET + XCVR_SPECIFICATION_ETH_COMPLIANCE_OFFSET, XCVR_SPECIFICATION_ETH_COMPLIANCE_WIDTH)
        try:
            eth_compliance = ETHERNET_10_40G_COMPLIANCE[int(eth_compliance_raw[0], 16)]
        except:
            logging.error("cannot specify ethernet10_40G compliance")
            return None
        ext_spec_compliance_raw = self.__read_eeprom_specific_bytes(INFO_OFFSET + XCVR_EXT_SPECIFICATION_COMPLIANCE_OFFSET, XCVR_EXT_SPECIFICATION_COMPLIANCE_WIDTH)
        ext_spec_compliance = ext_specification_compliance[(ext_spec_compliance_raw[0])]
       
        if eth_compliance is None or ext_spec_compliance is None:
            return None

        return eth_compliance == "40GBASE-CR4" or \
               "CR" in ext_spec_compliance or \
               "ACC" in ext_spec_compliance or \
               "Copper" in ext_spec_compliance

    def get_transceiver_info(self):
        transceiver_info_dict_keys = ['type',                      'hardware_rev',
                                      'serialnum',                 'manufacturename',
                                      'modelname',                 'Connector',
                                      'encoding',                  'ext_identifier',
                                      'ext_rateselect_compliance', 'cable_type',
                                      'cable_length',              'nominal_bit_rate',
                                      'specification_compliance',  'vendor_date',
                                      'vendor_oui']

        qsfp_cable_length_tup = ('Length(km)',    'Length OM3(2m)',
                                 'Length OM2(m)', 'Length OM1(m)',  'Length Cable Assembly(m)')

        qsfp_compliance_code_tup = ('10/40G Ethernet Compliance Code',                  'SONET Compliance codes',
                                    'SAS/SATA compliance codes',                        'Gigabit Ethernet Compliant codes',
                                    'Fibre Channel link length/Transmitter Technology', 'Fibre Channel transmission media',
                                    'Fibre Channel Speed')

        sfpi_obj = sff8436InterfaceId()
        if not self.get_presence() or not sfpi_obj:
            return {}

        offset = INFO_OFFSET
        sfp_interface_bulk_raw  = self.__read_eeprom_specific_bytes((offset + XCVR_INTFACE_BULK_OFFSET), XCVR_INTFACE_BULK_WIDTH_QSFP)
        sfp_interface_bulk_data = sfpi_obj.parse_sfp_info_bulk(sfp_interface_bulk_raw, 0)

        sfp_vendor_name_raw  = self.__read_eeprom_specific_bytes((offset + XCVR_VENDOR_NAME_OFFSET), XCVR_VENDOR_NAME_WIDTH)
        sfp_vendor_name_data = sfpi_obj.parse_vendor_name(sfp_vendor_name_raw, 0)
        sfp_vendor_pn_raw  = self.__read_eeprom_specific_bytes((offset + XCVR_VENDOR_PN_OFFSET), XCVR_VENDOR_PN_WIDTH)
        sfp_vendor_pn_data = sfpi_obj.parse_vendor_pn(sfp_vendor_pn_raw, 0)
        sfp_vendor_rev_raw  = self.__read_eeprom_specific_bytes((offset + XCVR_HW_REV_OFFSET), XCVR_HW_REV_WIDTH_QSFP)
        sfp_vendor_rev_data = sfpi_obj.parse_vendor_rev(sfp_vendor_rev_raw, 0)
        sfp_vendor_sn_raw  = self.__read_eeprom_specific_bytes((offset + XCVR_VENDOR_SN_OFFSET), XCVR_VENDOR_SN_WIDTH)
        sfp_vendor_sn_data = sfpi_obj.parse_vendor_sn(sfp_vendor_sn_raw, 0)
        sfp_vendor_oui_raw = self.__read_eeprom_specific_bytes((offset + XCVR_VENDOR_OUI_OFFSET), XCVR_VENDOR_OUI_WIDTH)
        sfp_vendor_oui_data = sfpi_obj.parse_vendor_oui(sfp_vendor_oui_raw, 0) if sfp_vendor_oui_raw is not None else None
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
            transceiver_info_dict['nominal_bit_rate']          = str(sfp_interface_bulk_data['data']['Nominal Bit Rate(100Mbs)']['value'])

        transceiver_info_dict['manufacturer'] = sfp_vendor_name_data['data']['Vendor Name']['value'] if sfp_vendor_name_data else 'N/A'
        transceiver_info_dict['model']       = sfp_vendor_pn_data['data']['Vendor PN']['value'] if sfp_vendor_pn_data else 'N/A'
        transceiver_info_dict['vendor_rev']  = sfp_vendor_rev_data['data']['Vendor Rev']['value'] if sfp_vendor_rev_data else 'N/A'
        transceiver_info_dict['serial']      = sfp_vendor_sn_data['data']['Vendor SN']['value'] if sfp_vendor_sn_data else 'N/A'
        transceiver_info_dict['vendor_oui']  = sfp_vendor_oui_data['data']['Vendor OUI']['value'] if sfp_vendor_oui_data else 'N/A'
        transceiver_info_dict['vendor_date'] = sfp_vendor_date_data['data']['VendorDataCode(YYYY-MM-DD Lot)']['value'] if sfp_vendor_date_data else 'N/A'

        transceiver_info_dict['cable_type']   = "Unknown"
        transceiver_info_dict['cable_length'] = "Unknown"
        for key in qsfp_cable_length_tup:
            if key in sfp_interface_bulk_data['data']:
                transceiver_info_dict['cable_type']   = key
                transceiver_info_dict['cable_length'] = str(sfp_interface_bulk_data['data'][key]['value'])

        compliance_code_dict  = dict()
        for key in qsfp_compliance_code_tup:
            if key in sfp_interface_bulk_data['data']['Specification compliance']['value']:
                compliance_code_dict[key] = sfp_interface_bulk_data['data']['Specification compliance']['value'][key]['value']
        sfp_ext_specification_compliance_raw = self.__read_eeprom_specific_bytes(offset + XCVR_EXT_SPECIFICATION_COMPLIANCE_OFFSET, XCVR_EXT_SPECIFICATION_COMPLIANCE_WIDTH)
        if sfp_ext_specification_compliance_raw is not None:
            sfp_ext_specification_compliance_data = sfpi_obj.parse_ext_specification_compliance(sfp_ext_specification_compliance_raw[0 : 1], 0)
            if sfp_ext_specification_compliance_data['data']['Extended Specification compliance']['value'] != "Unspecified":
                compliance_code_dict['Extended Specification compliance'] = sfp_ext_specification_compliance_data['data']['Extended Specification compliance']['value']
        
        transceiver_info_dict['specification_compliance'] = str(compliance_code_dict)
        transceiver_info_dict['nominal_bit_rate'] = str(sfp_interface_bulk_data['data']['Nominal Bit Rate(100Mbs)']['value'])

        if self.is_copper():
            transceiver_info_dict['specification_compliance'] = "{'media_interface':'DAC'}"
            transceiver_info_dict['type_abbrv_name']         = "{'media_interface':'DAC'}"
        else:
            transceiver_info_dict['specification_compliance'] = "{'media_interface':'AOC'}"
            transceiver_info_dict['type_abbrv_name']         = "{'media_interface':'AOC'}"

        # ========================================================================
        # KEY FIX: Synchronize aliases to ensure both old and new SONiC keys work
        # ========================================================================
        # Note: Since keys were pre-filled with 'N/A' by dict.fromkeys, we must check 
        # for 'N/A' specifically, not just 'not in'.
        
        if transceiver_info_dict.get('hardware_rev') == 'N/A':
            transceiver_info_dict['hardware_rev'] = transceiver_info_dict.get('vendor_rev', 'N/A')
            
        if transceiver_info_dict.get('manufacturename') == 'N/A':
            transceiver_info_dict['manufacturename'] = transceiver_info_dict.get('manufacturer', 'N/A')
            
        if transceiver_info_dict.get('modelname') == 'N/A':
            transceiver_info_dict['modelname'] = transceiver_info_dict.get('model', 'N/A')
            
        if transceiver_info_dict.get('serialnum') == 'N/A':
            transceiver_info_dict['serialnum'] = transceiver_info_dict.get('serial', 'N/A')

        if 'application_advertisement' not in transceiver_info_dict:
            transceiver_info_dict['application_advertisement'] = 'N/A'

        return transceiver_info_dict

    def get_transceiver_bulk_status(self):
        transceiver_dom_info_dict_keys = ['temperature', 'voltage', 'rx1power', 'rx2power',
                              'rx3power', 'rx4power', 'rx5power', 'rx6power',
                              'rx7power', 'rx8power', 'tx1bias', 'tx2bias',
                              'tx3bias', 'tx4bias', 'tx5bias', 'tx6bias',
                              'tx7bias', 'tx8bias', 'tx1power', 'tx2power',
                              'tx3power', 'tx4power', 'tx5power', 'tx6power',
                              'tx7power', 'tx8power']
        transceiver_dom_info_dict = dict.fromkeys(transceiver_dom_info_dict_keys, 'N/A')

        if not self.get_presence():
            return {}

        sfpi_obj = sff8436InterfaceId()
        sfpd_obj = sff8436Dom()

        if not sfpi_obj or not sfpd_obj:
            return transceiver_dom_info_dict

        offset = DOM_OFFSET
        offset_xcvr = INFO_OFFSET

        qsfp_dom_capability_raw = self.__read_eeprom_specific_bytes((offset_xcvr + XCVR_DOM_CAPABILITY_OFFSET), XCVR_DOM_CAPABILITY_WIDTH)
        if qsfp_dom_capability_raw is None:
            return transceiver_dom_info_dict
        qspf_dom_capability_data = sfpi_obj.parse_dom_capability(qsfp_dom_capability_raw, 0)

        dom_temperature_raw = self.__read_eeprom_specific_bytes((offset + QSFP_TEMPE_OFFSET), QSFP_TEMPE_WIDTH)
        if dom_temperature_raw is not None:
            dom_temperature_data = sfpd_obj.parse_temperature(dom_temperature_raw, 0)
            transceiver_dom_info_dict['temperature'] = dom_temperature_data['data']['Temperature']['value']

        dom_voltage_raw = self.__read_eeprom_specific_bytes((offset + QSFP_VOLT_OFFSET), QSFP_VOLT_WIDTH)
        if dom_voltage_raw is not None:
            dom_voltage_data = sfpd_obj.parse_voltage(dom_voltage_raw, 0)
            transceiver_dom_info_dict['voltage'] = dom_voltage_data['data']['Vcc']['value']

        qsfp_dom_rev_raw = self.__read_eeprom_specific_bytes((offset + QSFP_DOM_REV_OFFSET), QSFP_DOM_REV_WIDTH)
        if qsfp_dom_rev_raw is not None:
            qsfp_dom_rev_data = sfpd_obj.parse_sfp_dom_rev(qsfp_dom_rev_raw, 0)
            qsfp_dom_rev = qsfp_dom_rev_data['data']['dom_rev']['value']
        else:
            qsfp_dom_rev = ""

        qsfp_tx_power_support = qspf_dom_capability_data['data']['Tx_power_support']['value']
        dom_channel_monitor_data = {}
        dom_channel_monitor_raw = None

        if (qsfp_dom_rev[0:8] != 'SFF-8636' or (qsfp_dom_rev[0:8] == 'SFF-8636' and qsfp_tx_power_support != 'on')):
            dom_channel_monitor_raw = self.__read_eeprom_specific_bytes((offset + QSFP_CHANNL_MON_OFFSET), QSFP_CHANNL_MON_WIDTH)
            if dom_channel_monitor_raw is not None:
                dom_channel_monitor_data = sfpd_obj.parse_channel_monitor_params(dom_channel_monitor_raw, 0)
        else:
            dom_channel_monitor_raw = self.__read_eeprom_specific_bytes((offset + QSFP_CHANNL_MON_OFFSET), QSFP_CHANNL_MON_WITH_TX_POWER_WIDTH)
            if dom_channel_monitor_raw is not None:
                dom_channel_monitor_data = sfpd_obj.parse_channel_monitor_params_with_tx_power(dom_channel_monitor_raw, 0)
                transceiver_dom_info_dict['tx1power'] = dom_channel_monitor_data['data']['TX1Power']['value']
                transceiver_dom_info_dict['tx2power'] = dom_channel_monitor_data['data']['TX2Power']['value']
                transceiver_dom_info_dict['tx3power'] = dom_channel_monitor_data['data']['TX3Power']['value']
                transceiver_dom_info_dict['tx4power'] = dom_channel_monitor_data['data']['TX4Power']['value']

        if dom_channel_monitor_raw:
            transceiver_dom_info_dict['rx1power'] = dom_channel_monitor_data['data']['RX1Power']['value']
            transceiver_dom_info_dict['rx2power'] = dom_channel_monitor_data['data']['RX2Power']['value']
            transceiver_dom_info_dict['rx3power'] = dom_channel_monitor_data['data']['RX3Power']['value']
            transceiver_dom_info_dict['rx4power'] = dom_channel_monitor_data['data']['RX4Power']['value']
            transceiver_dom_info_dict['tx1bias']  = dom_channel_monitor_data['data']['TX1Bias']['value']
            transceiver_dom_info_dict['tx2bias']  = dom_channel_monitor_data['data']['TX2Bias']['value']
            transceiver_dom_info_dict['tx3bias']  = dom_channel_monitor_data['data']['TX3Bias']['value']
            transceiver_dom_info_dict['tx4bias']  = dom_channel_monitor_data['data']['TX4Bias']['value']

        for key in transceiver_dom_info_dict:
            transceiver_dom_info_dict[key] = self.__convert_string_to_num(transceiver_dom_info_dict[key])

        transceiver_dom_info_dict['rx_los']             = self.get_rx_los()
        transceiver_dom_info_dict['tx_fault']           = self.get_tx_fault()
        transceiver_dom_info_dict['reset_status']       = self.get_reset_status()
        transceiver_dom_info_dict['tx_disable']         = self.get_tx_disable()
        transceiver_dom_info_dict['tx_disable_channel'] = self.get_tx_disable_channel()
        transceiver_dom_info_dict['lp_mode']            = self.get_lpmode()

        return transceiver_dom_info_dict

    def get_transceiver_threshold_info(self):
        transceiver_dom_threshold_info_dict_keys = ['temphighalarm',    'temphighwarning',    'templowalarm',    'templowwarning',
                                                    'vcchighalarm',     'vcchighwarning',     'vcclowalarm',     'vcclowwarning',
                                                    'rxpowerhighalarm', 'rxpowerhighwarning', 'rxpowerlowalarm', 'rxpowerlowwarning',
                                                    'txpowerhighalarm', 'txpowerhighwarning', 'txpowerlowalarm', 'txpowerlowwarning',
                                                    'txbiashighalarm',  'txbiashighwarning',  'txbiaslowalarm',  'txbiaslowwarning']

        transceiver_dom_threshold_dict = dict.fromkeys(transceiver_dom_threshold_info_dict_keys, 'N/A')

        if not self.get_presence():
            return {}

        sfpd_obj = sff8436Dom()
        if not sfpd_obj:
            return transceiver_dom_threshold_dict

        offset = THRE_OFFSET
        dom_module_threshold_raw = self.__read_eeprom_specific_bytes((offset + QSFP_MODULE_THRESHOLD_OFFSET), QSFP_MODULE_THRESHOLD_WIDTH)
        if dom_module_threshold_raw:
            module_threshold_values = sfpd_obj.parse_module_threshold_values(dom_module_threshold_raw, 0)
            module_threshold_data = module_threshold_values.get('data')
            if module_threshold_data:
                transceiver_dom_threshold_dict['temphighalarm']   = module_threshold_data['TempHighAlarm']['value']
                transceiver_dom_threshold_dict['templowalarm']    = module_threshold_data['TempLowAlarm']['value']
                transceiver_dom_threshold_dict['temphighwarning'] = module_threshold_data['TempHighWarning']['value']
                transceiver_dom_threshold_dict['templowwarning']  = module_threshold_data['TempLowWarning']['value']
                transceiver_dom_threshold_dict['vcchighalarm']    = module_threshold_data['VccHighAlarm']['value']
                transceiver_dom_threshold_dict['vcclowalarm']     = module_threshold_data['VccLowAlarm']['value']
                transceiver_dom_threshold_dict['vcchighwarning']  = module_threshold_data['VccHighWarning']['value']
                transceiver_dom_threshold_dict['vcclowwarning']   = module_threshold_data['VccLowWarning']['value']

        dom_channel_thres_raw = self.__read_eeprom_specific_bytes((offset + QSFP_CHANNEL_THRESHOLD_OFFSET), QSFP_CHANNEL_THRESHOLD_WIDTH)
        channel_threshold_values = sfpd_obj.parse_channel_threshold_values(dom_channel_thres_raw, 0)
        channel_threshold_data = channel_threshold_values.get('data')
        if channel_threshold_data:
            transceiver_dom_threshold_dict['rxpowerhighalarm']   = channel_threshold_data['RxPowerHighAlarm']['value']
            transceiver_dom_threshold_dict['rxpowerlowalarm']    = channel_threshold_data['RxPowerLowAlarm']['value']
            transceiver_dom_threshold_dict['rxpowerhighwarning'] = channel_threshold_data['RxPowerHighWarning']['value']
            transceiver_dom_threshold_dict['rxpowerlowwarning']  = channel_threshold_data['RxPowerLowWarning']['value']
            transceiver_dom_threshold_dict['txpowerhighalarm']   = "0.0dBm"
            transceiver_dom_threshold_dict['txpowerlowalarm']    = "0.0dBm"
            transceiver_dom_threshold_dict['txpowerhighwarning'] = "0.0dBm"
            transceiver_dom_threshold_dict['txpowerlowwarning']  = "0.0dBm"
            transceiver_dom_threshold_dict['txbiashighalarm']    = channel_threshold_data['TxBiasHighAlarm']['value']
            transceiver_dom_threshold_dict['txbiaslowalarm']     = channel_threshold_data['TxBiasLowAlarm']['value']
            transceiver_dom_threshold_dict['txbiashighwarning']  = channel_threshold_data['TxBiasHighWarning']['value']
            transceiver_dom_threshold_dict['txbiaslowwarning']   = channel_threshold_data['TxBiasLowWarning']['value']

        for key in transceiver_dom_threshold_dict:
            transceiver_dom_threshold_dict[key] = self.__convert_string_to_num(transceiver_dom_threshold_dict[key])

        return transceiver_dom_threshold_dict

    def get_reset_status(self):
        try:
            attr_rv = self.__api_helper.read_one_line_file(self.__attr_path_prefix + 'reset')
            if attr_rv is None:
                return False
            val = attr_rv.strip().lower()
            if val in ('na', 'n/a', 'none', ''):
                return False
            base = 16 if val.startswith('0x') else 10
            return (int(val, base) & 0x1) == 1
        except (ValueError, TypeError, Exception):
            return False

    def get_rx_los(self):
        rx_los_list = []
        dom_channel_monitor_raw = self.__read_eeprom_specific_bytes(QSFP_CHANNL_RX_LOS_STATUS_OFFSET, QSFP_CHANNL_RX_LOS_STATUS_WIDTH)
        if dom_channel_monitor_raw is not None:
            try:
                rx_los_data = int(dom_channel_monitor_raw[0], 16)
                rx_los_list.append(rx_los_data & 0x01 != 0)
                rx_los_list.append(rx_los_data & 0x02 != 0)
                rx_los_list.append(rx_los_data & 0x04 != 0)
                rx_los_list.append(rx_los_data & 0x08 != 0)
            except:
                return [False, False, False, False]
        return rx_los_list if rx_los_list else [False, False, False, False]

    def get_tx_fault(self):
        tx_fault_list = []
        dom_channel_monitor_raw = self.__read_eeprom_specific_bytes(QSFP_CHANNL_TX_FAULT_STATUS_OFFSET, QSFP_CHANNL_TX_FAULT_STATUS_WIDTH)
        if dom_channel_monitor_raw is not None:
            try:
                tx_fault_data = int(dom_channel_monitor_raw[0], 16)
                tx_fault_list.append(tx_fault_data & 0x01 != 0)
                tx_fault_list.append(tx_fault_data & 0x02 != 0)
                tx_fault_list.append(tx_fault_data & 0x04 != 0)
                tx_fault_list.append(tx_fault_data & 0x08 != 0)
            except:
                return [False, False, False, False]
        return tx_fault_list if tx_fault_list else [False, False, False, False]

    def get_tx_disable(self):
        tx_disable = False
        tx_disable_list = []
        sfpd_obj = sff8436Dom()
        if sfpd_obj is None:
            return tx_disable

        dom_control_raw = self.__read_eeprom_specific_bytes(QSFP_CONTROL_OFFSET, QSFP_CONTROL_WIDTH)
        if dom_control_raw is not None:
            dom_control_data = sfpd_obj.parse_control_bytes(dom_control_raw, 0)
            tx_disable_list.append('On' == dom_control_data['data']['TX1Disable']['value'])
            tx_disable_list.append('On' == dom_control_data['data']['TX2Disable']['value'])
            tx_disable_list.append('On' == dom_control_data['data']['TX3Disable']['value'])
            tx_disable_list.append('On' == dom_control_data['data']['TX4Disable']['value'])
            tx_disable = tx_disable_list[0] or tx_disable_list[1] or tx_disable_list[2] or tx_disable_list[3]

        return tx_disable

    def get_tx_disable_channel(self):
        tx_disable_channel = 0
        tx_disable_list = []
        sfpd_obj = sff8436Dom()
        if sfpd_obj is None:
            return tx_disable_channel

        dom_control_raw = self.__read_eeprom_specific_bytes(QSFP_CONTROL_OFFSET, QSFP_CONTROL_WIDTH)
        if dom_control_raw is not None:
            dom_control_data = sfpd_obj.parse_control_bytes(dom_control_raw, 0)
            tx_disable_list.append('On' == dom_control_data['data']['TX1Disable']['value'])
            tx_disable_list.append('On' == dom_control_data['data']['TX2Disable']['value'])
            tx_disable_list.append('On' == dom_control_data['data']['TX3Disable']['value'])
            tx_disable_list.append('On' == dom_control_data['data']['TX4Disable']['value'])

        for i in range(len(tx_disable_list)):
            if tx_disable_list[i]:
                tx_disable_channel |= 1 << i

        return tx_disable_channel

    def get_lpmode(self):
        try:
            attr_rv = self.__get_attr_value(self.__lpmode_attr)
            if attr_rv == 'ERR':
                return False
            val = attr_rv.strip().lower()
            if val in ('na', 'n/a', 'none', ''):
                return False
            base = 16 if val.startswith('0x') else 10
            return (int(val, base) & 0x1) == 1
        except (ValueError, TypeError, Exception):
            return False

    def get_power_override(self):
        power_override = False
        sfpd_obj = sff8436Dom()
        if sfpd_obj is None:
            return power_override

        dom_control_raw = self.__read_eeprom_specific_bytes(QSFP_CONTROL_OFFSET, QSFP_CONTROL_WIDTH)
        if dom_control_raw is not None:
            dom_control_data = sfpd_obj.parse_control_bytes(dom_control_raw, 0)
            power_override = ('On' == dom_control_data['data']['PowerOverride']['value'])

        return power_override

    def get_temperature(self):
        temp = "N/A"
        sfpd_obj = sff8436Dom()
        if not self.get_presence() or not sfpd_obj:
            return temp

        dom_temperature_raw = self.__read_eeprom_specific_bytes((DOM_OFFSET + QSFP_TEMPE_OFFSET), QSFP_TEMPE_WIDTH)
        if dom_temperature_raw is not None:
            dom_temperature_data = sfpd_obj.parse_temperature(dom_temperature_raw, 0)
            temp = self.__convert_string_to_num(dom_temperature_data['data']['Temperature']['value'])
        return temp

    def get_voltage(self):
        voltage = "N/A"
        sfpd_obj = sff8436Dom()
        if not self.get_presence() or not sfpd_obj:
            return voltage

        dom_voltage_raw = self.__read_eeprom_specific_bytes((DOM_OFFSET + QSFP_VOLT_OFFSET), QSFP_VOLT_WIDTH)
        if dom_voltage_raw is not None:
            dom_voltage_data = sfpd_obj.parse_voltage(dom_voltage_raw, 0)
            voltage = self.__convert_string_to_num(dom_voltage_data['data']['Vcc']['value'])
        return voltage

    def get_tx_bias(self):
        tx_bias_list = []
        sfpd_obj = sff8436Dom()
        sfpi_obj = sff8436InterfaceId()
        if not self.get_presence() or not sfpd_obj:
            return []

        qsfp_dom_capability_raw = self.__read_eeprom_specific_bytes((INFO_OFFSET + XCVR_DOM_CAPABILITY_OFFSET), XCVR_DOM_CAPABILITY_WIDTH)
        if qsfp_dom_capability_raw is None:
            return None
        qspf_dom_capability_data = sfpi_obj.parse_dom_capability(qsfp_dom_capability_raw, 0)

        qsfp_dom_rev_raw = self.__read_eeprom_specific_bytes((DOM_OFFSET + QSFP_DOM_REV_OFFSET), QSFP_DOM_REV_WIDTH)
        if qsfp_dom_rev_raw is None:
            return []
        qsfp_dom_rev_data = sfpd_obj.parse_sfp_dom_rev(qsfp_dom_rev_raw, 0)
        qsfp_dom_rev = qsfp_dom_rev_data['data']['dom_rev']['value']

        qsfp_tx_power_support = qspf_dom_capability_data['data']['Tx_power_support']['value']
        dom_channel_monitor_data = {}
        dom_channel_monitor_raw = None

        if (qsfp_dom_rev[0:8] != 'SFF-8636' or (qsfp_dom_rev[0:8] == 'SFF-8636' and qsfp_tx_power_support != 'on')):
            dom_channel_monitor_raw = self.__read_eeprom_specific_bytes((DOM_OFFSET + QSFP_CHANNL_MON_OFFSET), QSFP_CHANNL_MON_WIDTH)
            if dom_channel_monitor_raw is not None:
                dom_channel_monitor_data = sfpd_obj.parse_channel_monitor_params(dom_channel_monitor_raw, 0)
        else:
            dom_channel_monitor_raw = self.__read_eeprom_specific_bytes((DOM_OFFSET + QSFP_CHANNL_MON_OFFSET), QSFP_CHANNL_MON_WITH_TX_POWER_WIDTH)
            if dom_channel_monitor_raw is not None:
                dom_channel_monitor_data = sfpd_obj.parse_channel_monitor_params_with_tx_power(dom_channel_monitor_raw, 0)

        if dom_channel_monitor_raw:
            tx_bias_list.append(self.__convert_string_to_num(dom_channel_monitor_data['data']['TX1Bias']['value']))
            tx_bias_list.append(self.__convert_string_to_num(dom_channel_monitor_data['data']['TX2Bias']['value']))
            tx_bias_list.append(self.__convert_string_to_num(dom_channel_monitor_data['data']['TX3Bias']['value']))
            tx_bias_list.append(self.__convert_string_to_num(dom_channel_monitor_data['data']['TX4Bias']['value']))

        return tx_bias_list

    def get_rx_power(self):
        rx_power_list = []
        sfpd_obj = sff8436Dom()
        sfpi_obj = sff8436InterfaceId()
        if not self.get_presence() or not sfpd_obj:
            return []

        qsfp_dom_capability_raw = self.__read_eeprom_specific_bytes((INFO_OFFSET + XCVR_DOM_CAPABILITY_OFFSET), XCVR_DOM_CAPABILITY_WIDTH)
        if qsfp_dom_capability_raw is None:
            return None
        qspf_dom_capability_data = sfpi_obj.parse_dom_capability(qsfp_dom_capability_raw, 0)

        qsfp_dom_rev_raw = self.__read_eeprom_specific_bytes((DOM_OFFSET + QSFP_DOM_REV_OFFSET), QSFP_DOM_REV_WIDTH)
        if qsfp_dom_rev_raw is None:
            return []
        qsfp_dom_rev_data = sfpd_obj.parse_sfp_dom_rev(qsfp_dom_rev_raw, 0)
        qsfp_dom_rev = qsfp_dom_rev_data['data']['dom_rev']['value']

        qsfp_tx_power_support = qspf_dom_capability_data['data']['Tx_power_support']['value']
        dom_channel_monitor_data = {}
        dom_channel_monitor_raw = None

        if (qsfp_dom_rev[0:8] != 'SFF-8636' or (qsfp_dom_rev[0:8] == 'SFF-8636' and qsfp_tx_power_support != 'on')):
            dom_channel_monitor_raw = self.__read_eeprom_specific_bytes((DOM_OFFSET + QSFP_CHANNL_MON_OFFSET), QSFP_CHANNL_MON_WIDTH)
            if dom_channel_monitor_raw is not None:
                dom_channel_monitor_data = sfpd_obj.parse_channel_monitor_params(dom_channel_monitor_raw, 0)
        else:
            dom_channel_monitor_raw = self.__read_eeprom_specific_bytes((DOM_OFFSET + QSFP_CHANNL_MON_OFFSET), QSFP_CHANNL_MON_WITH_TX_POWER_WIDTH)
            if dom_channel_monitor_raw is not None:
                dom_channel_monitor_data = sfpd_obj.parse_channel_monitor_params_with_tx_power(dom_channel_monitor_raw, 0)

        if dom_channel_monitor_raw:
            rx_power_list.append(self.__convert_string_to_num(dom_channel_monitor_data['data']['RX1Power']['value']))
            rx_power_list.append(self.__convert_string_to_num(dom_channel_monitor_data['data']['RX2Power']['value']))
            rx_power_list.append(self.__convert_string_to_num(dom_channel_monitor_data['data']['RX3Power']['value']))
            rx_power_list.append(self.__convert_string_to_num(dom_channel_monitor_data['data']['RX4Power']['value']))

        return rx_power_list

    def get_tx_power(self):
        tx_power_list = []
        sfpd_obj = sff8436Dom()
        sfpi_obj = sff8436InterfaceId()
        if not self.get_presence() or not sfpd_obj:
            return []

        qsfp_dom_capability_raw = self.__read_eeprom_specific_bytes((INFO_OFFSET + XCVR_DOM_CAPABILITY_OFFSET), XCVR_DOM_CAPABILITY_WIDTH)
        if qsfp_dom_capability_raw is None:
            return None
        qspf_dom_capability_data = sfpi_obj.parse_dom_capability(qsfp_dom_capability_raw, 0)

        qsfp_dom_rev_raw = self.__read_eeprom_specific_bytes((DOM_OFFSET + QSFP_DOM_REV_OFFSET), QSFP_DOM_REV_WIDTH)
        if qsfp_dom_rev_raw is None:
            return []
        qsfp_dom_rev_data = sfpd_obj.parse_sfp_dom_rev(qsfp_dom_rev_raw, 0)
        qsfp_dom_rev = qsfp_dom_rev_data['data']['dom_rev']['value']

        qsfp_tx_power_support = qspf_dom_capability_data['data']['Tx_power_support']['value']
        dom_channel_monitor_data = {}
        dom_channel_monitor_raw = None

        if qsfp_dom_rev[0:8] == 'SFF-8636' and qsfp_tx_power_support == 'on':
            dom_channel_monitor_raw = self.__read_eeprom_specific_bytes((DOM_OFFSET + QSFP_CHANNL_MON_OFFSET), QSFP_CHANNL_MON_WITH_TX_POWER_WIDTH)
            if dom_channel_monitor_raw is not None:
                dom_channel_monitor_data = sfpd_obj.parse_channel_monitor_params_with_tx_power(dom_channel_monitor_raw, 0)
                tx_power_list.append(self.__convert_string_to_num(dom_channel_monitor_data['data']['TX1Power']['value']))
                tx_power_list.append(self.__convert_string_to_num(dom_channel_monitor_data['data']['TX2Power']['value']))
                tx_power_list.append(self.__convert_string_to_num(dom_channel_monitor_data['data']['TX3Power']['value']))
                tx_power_list.append(self.__convert_string_to_num(dom_channel_monitor_data['data']['TX4Power']['value']))

        return tx_power_list

    def reset(self):
        try:
            # Changed hex() to str() for better sysfs compatibility
            with open(self.__reset_attr, "w") as reg_file:
                reg_file.write('1')
        except IOError as e:
            print("Error: unable to open file: %s" % str(e))
            return False

        import time
        time.sleep(2)

        try:
            with open(self.__reset_attr, "w") as reg_file:
                reg_file.write('0')
        except IOError as e:
            print("Error: unable to open file: %s" % str(e))
            return False

        return True

    def tx_disable(self, tx_disable):
        tx_disable_ctl = 0xf if tx_disable else 0x0
        buffer = create_string_buffer(1)
        buffer[0] = chr(tx_disable_ctl)
        return self.__write_eeprom_specific_bytes(QSFP_CONTROL_OFFSET, buffer)

    def tx_disable_channel(self, channel, disable):
        channel_state = self.get_tx_disable_channel()
        if disable:
            tx_disable_ctl = channel_state | channel
        else:
            tx_disable_ctl = channel_state & (~channel & 0xf)
        buffer = create_string_buffer(1)
        buffer[0] = chr(tx_disable_ctl)
        return self.__write_eeprom_specific_bytes(QSFP_CONTROL_OFFSET, buffer)
    
    def set_lpmode(self, lpmode):
        try:
            with open(self.__lpmode_attr, 'w') as f:
                f.write('1' if lpmode else '0')
            return True
        except (IOError, OSError, Exception):
            return False

    def set_power_override(self, power_override, power_set):
        power_override_bit = 1 if power_override else 0
        power_set_bit = 1 if power_set else 0
        buffer = create_string_buffer(1)
        buffer[0] = chr(power_override_bit | power_set_bit)
        return self.__write_eeprom_specific_bytes(QSFP_POWEROVERRIDE_OFFSET, buffer)
    