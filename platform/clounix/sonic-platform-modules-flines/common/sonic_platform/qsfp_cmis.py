#!/usr/bin/env python
#
# Name: qsfp_cmis.py, version: 1.0
#
# Description: Module contains the definitions of SONiC platform APIs
#

import os
import time
import logging
import struct
import datetime
from ctypes import create_string_buffer

try:
    from sonic_platform_base.sfp_base import SfpBase
    from sonic_platform_base.sonic_sfp.qsfp_dd import qsfp_dd_InterfaceId
    from sonic_platform_base.sonic_sfp.qsfp_dd import qsfp_dd_Dom
    from sonic_py_common.logger import Logger
    from sonic_py_common import device_info
    from sonic_platform_base.sonic_sfp.sfputilhelper import SfpUtilHelper
    from .helper import APIHelper
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

XCVR_INTFACE_BULK_OFFSET = 0
XCVR_INTFACE_BULK_WIDTH_QSFP = 20
XCVR_INTFACE_BULK_WIDTH_SFP = 21
XCVR_TYPE_OFFSET = 0
XCVR_TYPE_WIDTH = 1
XCVR_EXT_TYPE_OFFSET = 1
XCVR_EXT_TYPE_WIDTH = 1
XCVR_CONNECTOR_OFFSET = 2
XCVR_CONNECTOR_WIDTH = 1
XCVR_COMPLIANCE_CODE_OFFSET = 3
XCVR_COMPLIANCE_CODE_WIDTH = 8
XCVR_ENCODING_OFFSET = 11
XCVR_ENCODING_WIDTH = 1
XCVR_NBR_OFFSET = 12
XCVR_NBR_WIDTH = 1
XCVR_EXT_RATE_SEL_OFFSET = 13
XCVR_EXT_RATE_SEL_WIDTH = 1
XCVR_CABLE_LENGTH_OFFSET = 14
XCVR_CABLE_LENGTH_WIDTH_QSFP = 5
XCVR_CABLE_LENGTH_WIDTH_SFP = 6
XCVR_VENDOR_NAME_OFFSET = 20
XCVR_VENDOR_NAME_WIDTH = 16
XCVR_VENDOR_OUI_OFFSET = 37
XCVR_VENDOR_OUI_WIDTH = 3
XCVR_VENDOR_PN_OFFSET = 40
XCVR_VENDOR_PN_WIDTH = 16
XCVR_HW_REV_OFFSET = 56
XCVR_HW_REV_WIDTH_OSFP = 2
XCVR_HW_REV_WIDTH_QSFP = 2
XCVR_HW_REV_WIDTH_SFP = 4
XCVR_EXT_SPECIFICATION_COMPLIANCE_OFFSET = 64
XCVR_EXT_SPECIFICATION_COMPLIANCE_WIDTH = 1
XCVR_VENDOR_SN_OFFSET = 68
XCVR_VENDOR_SN_WIDTH = 16
XCVR_VENDOR_DATE_OFFSET = 84
XCVR_VENDOR_DATE_WIDTH = 8
XCVR_DOM_CAPABILITY_OFFSET = 92
XCVR_DOM_CAPABILITY_WIDTH = 2

XCVR_EXT_TYPE_OFFSET_QSFP_DD = 72
XCVR_EXT_TYPE_WIDTH_QSFP_DD = 2
XCVR_CONNECTOR_OFFSET_QSFP_DD = 75
XCVR_CONNECTOR_WIDTH_QSFP_DD = 1
XCVR_CABLE_LENGTH_OFFSET_QSFP_DD = 74
XCVR_CABLE_LENGTH_WIDTH_QSFP_DD = 1
XCVR_HW_REV_OFFSET_QSFP_DD = 36
XCVR_HW_REV_WIDTH_QSFP_DD = 2
XCVR_VENDOR_DATE_OFFSET_QSFP_DD = 54
XCVR_VENDOR_DATE_WIDTH_QSFP_DD = 8
XCVR_DOM_CAPABILITY_OFFSET_QSFP_DD = 2
XCVR_DOM_CAPABILITY_WIDTH_QSFP_DD = 1
XCVR_MEDIA_TYPE_OFFSET_QSFP_DD = 85
XCVR_MEDIA_TYPE_WIDTH_QSFP_DD = 1
XCVR_FIRST_APPLICATION_LIST_OFFSET_QSFP_DD = 86
XCVR_FIRST_APPLICATION_LIST_WIDTH_QSFP_DD = 32
XCVR_SECOND_APPLICATION_LIST_OFFSET_QSFP_DD = 351
XCVR_SECOND_APPLICATION_LIST_WIDTH_QSFP_DD = 28

XCVR_INTERFACE_DATA_START = 0
XCVR_INTERFACE_DATA_SIZE = 92
SFP_MODULE_ADDRA2_OFFSET = 256
SFP_MODULE_THRESHOLD_OFFSET = 0
SFP_MODULE_THRESHOLD_WIDTH = 56

QSFP_DOM_BULK_DATA_START = 22
QSFP_DOM_BULK_DATA_SIZE = 36
SFP_DOM_BULK_DATA_START = 96
SFP_DOM_BULK_DATA_SIZE = 10

QSFP_DD_DOM_BULK_DATA_START = 14
QSFP_DD_DOM_BULK_DATA_SIZE = 4

OSFP_TYPE_OFFSET = 0
OSFP_VENDOR_NAME_OFFSET = 129
OSFP_VENDOR_PN_OFFSET = 148
OSFP_HW_REV_OFFSET = 164
OSFP_VENDOR_SN_OFFSET = 166

QSFP_DD_TYPE_OFFSET = 0
QSFP_DD_VENDOR_NAME_OFFSET = 1
QSFP_DD_VENDOR_PN_OFFSET = 20
QSFP_DD_VENDOR_SN_OFFSET = 38
QSFP_DD_VENDOR_OUI_OFFSET = 17

QSFP_DOM_REV_OFFSET = 1
QSFP_DOM_REV_WIDTH = 1
QSFP_TEMPE_OFFSET = 22
QSFP_TEMPE_WIDTH = 2
QSFP_VOLT_OFFSET = 26
QSFP_VOLT_WIDTH = 2
QSFP_VERSION_COMPLIANCE_OFFSET = 1
QSFP_VERSION_COMPLIANCE_WIDTH = 2
QSFP_CHANNL_MON_OFFSET = 34
QSFP_CHANNL_MON_WIDTH = 16
QSFP_CHANNL_MON_WITH_TX_POWER_WIDTH = 24
QSFP_CHANNL_DISABLE_STATUS_OFFSET = 86
QSFP_CHANNL_DISABLE_STATUS_WIDTH = 1
QSFP_CHANNL_RX_LOS_STATUS_OFFSET = 3
QSFP_CHANNL_RX_LOS_STATUS_WIDTH = 1
QSFP_CHANNL_TX_FAULT_STATUS_OFFSET = 4
QSFP_CHANNL_TX_FAULT_STATUS_WIDTH = 1
QSFP_CONTROL_OFFSET = 86
QSFP_CONTROL_WIDTH = 8
QSFP_MODULE_MONITOR_OFFSET = 0
QSFP_MODULE_MONITOR_WIDTH = 9
QSFP_POWEROVERRIDE_OFFSET = 93
QSFP_POWEROVERRIDE_WIDTH = 1
QSFP_POWEROVERRIDE_BIT = 0
QSFP_POWERSET_BIT = 1
QSFP_OPTION_VALUE_OFFSET = 192
QSFP_OPTION_VALUE_WIDTH = 4

QSFP_MODULE_THRESHOLD_OFFSET = 128
QSFP_MODULE_THRESHOLD_WIDTH = 24
QSFP_CHANNL_THRESHOLD_OFFSET = 176
QSFP_CHANNL_THRESHOLD_WIDTH = 24

CMIS_LOWER_PAGE_START = 0
CMIS_LOWER_PAGE_REVISION_COMPLIANCE_OFFSET = 1
CMIS_LOWER_PAGE_FLAT_MEM = 2
CMIS_LOWER_PAGE_MODULE_STATE_OFFSET = 3

CMIS_UPPER_PAGE01_START = 128
CMIS_UPPER_PAGE01_TX_DISABLE_IMPLEMENTED = 155

CMIS_UPPER_PAGE02_START = 384
CMIS_UPPER_PAGE10_START = 2048
CMIS_UPPER_PAGE10_DATA_PATH_DEINIT_OFFSET = 128
CMIS_UPPER_PAGE10_TX_DISABLE_OFFSET = 130
CMIS_UPPER_PAGE10_STAGED0_APPLY_DATA_PATH_INIT_OFFSET = 143
CMIS_UPPER_PAGE10_STAGED0_APSEL_CONTROL_OFFSET = 145

CMIS_UPPER_PAGE11_START = 2176
CMIS_UPPER_PAGE11_DATA_PATH_STATE_OFFSET = 128
CMIS_UPPER_PAGE11_CONFIG_ERROR_CODE_OFFSET = 202

SFP_TEMPE_OFFSET = 96
SFP_TEMPE_WIDTH = 2
SFP_VOLT_OFFSET = 98
SFP_VOLT_WIDTH = 2
SFP_CHANNL_MON_OFFSET = 100
SFP_CHANNL_MON_WIDTH = 6
SFP_CHANNL_STATUS_OFFSET = 110
SFP_CHANNL_STATUS_WIDTH = 1
SFP_CHANNL_THRESHOLD_OFFSET = 112
SFP_CHANNL_THRESHOLD_WIDTH = 2
SFP_STATUS_CONTROL_OFFSET = 110
SFP_STATUS_CONTROL_WIDTH = 1
SFP_TX_DISABLE_HARD_BIT = 7
SFP_TX_DISABLE_SOFT_BIT = 6

QSFP_DD_TEMPE_OFFSET = 14
QSFP_DD_TEMPE_WIDTH = 2
QSFP_DD_VOLT_OFFSET = 16
QSFP_DD_VOLT_WIDTH = 2
QSFP_DD_TX_BIAS_OFFSET = 170
QSFP_DD_TX_BIAS_WIDTH = 16
QSFP_DD_RX_POWER_OFFSET = 186
QSFP_DD_RX_POWER_WIDTH = 16
QSFP_DD_TX_POWER_OFFSET = 26
QSFP_DD_TX_POWER_WIDTH = 16
QSFP_DD_CHANNL_MON_OFFSET = 154
QSFP_DD_CHANNL_MON_WIDTH = 48
QSFP_DD_CHANNL_DISABLE_STATUS_OFFSET = 86
QSFP_DD_CHANNL_DISABLE_STATUS_WIDTH = 1
QSFP_DD_CHANNL_RX_LOS_STATUS_OFFSET = 19
QSFP_DD_CHANNL_RX_LOS_STATUS_WIDTH = 1
QSFP_DD_CHANNL_TX_FAULT_STATUS_OFFSET = 7
QSFP_DD_CHANNL_TX_FAULT_STATUS_WIDTH = 1
QSFP_DD_MODULE_THRESHOLD_OFFSET = 128
QSFP_DD_MODULE_THRESHOLD_WIDTH = 72
QSFP_DD_CHANNL_STATUS_OFFSET = 26
QSFP_DD_CHANNL_STATUS_WIDTH = 1
DOM_OFFSET = 0

SFP_TYPE_CODE_LIST = ['03']
QSFP_TYPE_CODE_LIST = ['0d', '11']
QSFP_DD_TYPE_CODE_LIST = ['18', '1b']

qsfp_cable_length_tup = ('Length(km)', 'Length OM3(2m)',
                         'Length OM2(m)', 'Length OM1(m)',
                         'Length Cable Assembly(m)')

sfp_cable_length_tup = ('LengthSMFkm-UnitsOfKm', 'LengthSMF(UnitsOf100m)',
                        'Length50um(UnitsOf10m)', 'Length62.5um(UnitsOfm)',
                        'LengthCable(UnitsOfm)', 'LengthOM3(UnitsOf10m)')

sfp_compliance_code_tup = ('10GEthernetComplianceCode', 'InfinibandComplianceCode',
                            'ESCONComplianceCodes', 'SONETComplianceCodes',
                            'EthernetComplianceCodes','FibreChannelLinkLength',
                            'FibreChannelTechnology', 'SFP+CableTechnology',
                            'FibreChannelTransmissionMedia','FibreChannelSpeed')

qsfp_compliance_code_tup = ('10/40G Ethernet Compliance Code', 'SONET Compliance codes',
                            'SAS/SATA compliance codes', 'Gigabit Ethernet Compliant codes',
                            'Fibre Channel link length/Transmitter Technology',
                            'Fibre Channel transmission media', 'Fibre Channel Speed')

SFP_TYPE = "SFP"
QSFP_TYPE = "QSFP"
OSFP_TYPE = "OSFP"
QSFP_DD_TYPE = "QSFP_DD"

TRANSCEIVER_PATH = '/sys/s3ip/transceiver/'

MODULE_STATE = {
    1: 'ModuleLowPwr', 2: 'ModulePwrUp', 3: 'ModuleReady',
    4: 'ModulePwrDn', 5: 'ModuleFault'
}
DATAPATH_STATE = {
    1: 'DataPathDeactivated', 2: 'DataPathInit', 3: 'DataPathDeinit',
    4: 'DataPathActivated', 5: 'DataPathTxTurnOn', 6: 'DataPathTxTurnOff',
    7: 'DataPathInitialized',
}
CONFIG_STATUS = {
    0: 'ConfigUndefined', 1: 'ConfigSuccess', 2: 'ConfigRejected',
    3: 'ConfigRejectedInvalidAppSel', 4: 'ConfigRejectedInvalidDataPath',
    5: 'ConfigRejectedInvalidSI', 6: 'ConfigRejectedLaneInUse',
    7: 'ConfigRejectedPartialDataPath', 12: 'ConfigInProgress',
}

CMIS_STATE_UNKNOWN = 'UNKNOWN'
CMIS_STATE_INSERTED = 'INSERTED'
CMIS_STATE_DP_DEINIT = 'DP_DEINIT'
CMIS_STATE_AP_CONF = 'AP_CONFIGURED'
CMIS_STATE_DP_INIT = 'DP_INIT'
CMIS_STATE_DP_TXON = 'DP_TXON'
CMIS_STATE_READY = 'READY'
CMIS_STATE_REMOVED = 'REMOVED'
CMIS_STATE_FAILED = 'FAILED'

CMIS_MAX_RETRIES = 3
CMIS_DEF_EXPIRED = 1

class QSfp_CMIS(SfpBase):

    port_start = 0
    port_end = 55
    NUM_CHANNELS = 8

    dom_supported = True
    dom_temp_supported = True
    dom_volt_supported = True
    dom_rx_power_supported = True
    dom_tx_power_supported = True
    dom_tx_disable_supported = True
    calibration = 1

    def __init__(self, index):
        self.__index = index
        self.__api_helper = APIHelper()
        self.__attr_path_prefix = '/sys/s3ip/transceiver/eth{}/'.format(self.__index+1)
        self.__platform = str(device_info.get_platform())
        self.__hwsku = str(device_info.get_hwsku())
        self.__presence_attr = None
        self.__eeprom_path = None
        self.__eeprom_path = TRANSCEIVER_PATH+'eth{}/'.format(self.__index+1)+'eeprom'
        self.__presence_attr = TRANSCEIVER_PATH+'eth{}/'.format(self.__index+1)+'present'
        self.__reset_attr = TRANSCEIVER_PATH+'eth{}/'.format(self.__index+1)+'reset'
        self.__lpmode_attr = TRANSCEIVER_PATH+'eth{}/'.format(self.__index+1)+'low_power_mode'
        SfpBase.__init__(self)
        self.cmis_state = CMIS_STATE_UNKNOWN 
        self.cmis_retries = 0
        self.cmis_expired = None
        self.info_dict_keys = ['type', 'hardware_rev', 'serial', 'manufacturer',
            'model', 'connector', 'encoding', 'ext_identifier',
            'ext_rateselect_compliance', 'cable_type', 'cable_length',
            'nominal_bit_rate', 'specification_compliance', 'vendor_date', 'vendor_oui', 'application_advertisement']

    def __get_attr_value(self, attr_path):
        retval = 'ERR'
        if not os.path.isfile(attr_path):
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

    def _read_eeprom_specific_bytes(self, offset, num_bytes):
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
        
    def _write_eeprom_specific_bytes(self, offset, data):
        sysfs_eeprom_path = self.__eeprom_path
        buffer = create_string_buffer(1)
        buffer[0] = struct.pack('B', data)
        try:
            with open(sysfs_eeprom_path, mode='rb+', buffering=0) as f:
                f.seek(offset)
                f.write(buffer[0])
        except OSError as e:
            logging.error(" %s", str(e))
            return False
        return True

    def read_eeprom(self, offset, num_bytes):
        sysfsfile_eeprom = None
        eeprom_read_bytes = []
        eeprom_raw = []
        for i in range(0, num_bytes):
            eeprom_raw.append("0x00")
        sysfs_eeprom_path = self.__eeprom_path
        try:
            sysfsfile_eeprom = open(sysfs_eeprom_path, mode="rb", buffering=0)
            sysfsfile_eeprom.seek(offset)
            raw = sysfsfile_eeprom.read(num_bytes)
            raw_len = len(raw)
            eeprom_read_bytes = bytearray(raw)
            for n in range(0, raw_len):
                eeprom_raw[n] = hex(raw[n])[2:].zfill(2)
        except:
            pass
        finally:
            if sysfsfile_eeprom:
                sysfsfile_eeprom.close()
        return eeprom_read_bytes

    def write_eeprom(self, offset, num_bytes, write_buffer):
        try:
            with open(self.__eeprom_path, mode='r+b', buffering=0) as f:
                f.seek(offset)
                f.write(write_buffer[0:num_bytes])
        except (OSError, IOError):
            return False
        return True

    def _convert_string_to_num(self, value_str):
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

    def _dom_capability_detect(self):
        if not self.get_presence():
            self.dom_supported = False
            self.dom_temp_supported = False
            self.dom_volt_supported = False
            self.dom_rx_power_supported = False
            self.dom_tx_bias_power_supported = False
            self.dom_tx_power_supported = False
            self.calibration = 0
            return
        sfpi_obj = qsfp_dd_InterfaceId()
        if sfpi_obj is None:
            self.dom_supported = False
        offset = 0
        qsfp_dom_capability_raw = self._read_eeprom_specific_bytes((offset + XCVR_DOM_CAPABILITY_OFFSET_QSFP_DD), XCVR_DOM_CAPABILITY_WIDTH_QSFP_DD)
        if qsfp_dom_capability_raw is not None:
            self.dom_temp_supported = True
            self.dom_volt_supported = True
            dom_capability = sfpi_obj.parse_dom_capability(qsfp_dom_capability_raw, 0)
            if dom_capability['data']['Flat_MEM']['value'] == 'Off':
                self.dom_supported = True
                self.second_application_list = True
                self.dom_rx_power_supported = True
                self.dom_tx_power_supported = True
                self.dom_tx_bias_power_supported = True
                self.dom_thresholds_supported = True
                self.dom_rx_tx_power_bias_supported = True
            else:
                self.dom_supported = False
                self.second_application_list = False
                self.dom_rx_power_supported = False
                self.dom_tx_power_supported = False
                self.dom_tx_bias_power_supported = False
                self.dom_thresholds_supported = False
                self.dom_rx_tx_power_bias_supported = False
        else:
            self.dom_supported = False
            self.dom_temp_supported = False
            self.dom_volt_supported = False
            self.dom_rx_power_supported = False
            self.dom_tx_power_supported = False
            self.dom_tx_bias_power_supported = False
            self.dom_thresholds_supported = False
            self.dom_rx_tx_power_bias_supported = False

    def get_presence(self):
        try:
            attr_rv = self.__api_helper.read_one_line_file(self.__attr_path_prefix + 'present')
            if attr_rv is None:
                return False
            val = attr_rv.strip().lower()
            if val in ('na', 'n/a', 'none', '', 'err'):
                return False
            base = 16 if val.startswith('0x') else 10
            return (int(val, base) & 0x1) == 1
        except (ValueError, TypeError, Exception):
            return False

    def get_transceiver_type(self, origin_transceiver_type):
        transceiver_type = origin_transceiver_type
        if transceiver_type != 'Unknown':
            return transceiver_type
        type_of_transceiver = {
            '00': 'Unknown or unspecified', '01': 'GBIC', '02': 'Module/connector soldered to motherboard',
            '03': 'SFP/SFP+/SFP28', '04': '300 pin XBI', '05': 'XENPAK', '06': 'XFP', '07': 'XFF',
            '08': 'XFP-E', '09': 'XPAK', '0a': 'X2', '0b': 'DWDM-SFP/SFP+', '0c': 'QSFP',
            '0d': 'QSFP+ or later', '0e': 'CXP or later', '0f': 'Shielded Mini Multilane HD 4X',
            '10': 'Shielded Mini Multilane HD 8X', '11': 'QSFP28 or later', '12': 'CXP2 (aka CXP28) or later',
            '13': 'CDFP (Style 1/Style2)', '14': 'Shielded Mini Multilane HD 4X Fanout Cable',
            '15': 'Shielded Mini Multilane HD 8X Fanout Cable', '16': 'CDFP (Style 3)', '17': 'microQSFP',
            '18': 'QSFP-DD Double Density 8X Pluggable Transceiver', '19': 'OSFP 8X Pluggable Transceiver',
            '1a': 'SFP-DD Double Density 2X Pluggable Transceiver', '1b': 'DSFP Dual Small Form Factor Pluggable Transceiver',
            '1c': 'x4 MiniLink/OcuLink', '1d': 'x8 MiniLink', '1e': 'QSFP+ or later with CMIS'
        }
        offset = 128
        sfp_type_raw = self._read_eeprom_specific_bytes((offset + QSFP_DD_TYPE_OFFSET), XCVR_TYPE_WIDTH)
        return type_of_transceiver.get(sfp_type_raw[0], 'Unknown')

    def get_transceiver_type_abbr_name(self, origin_transceiver_type):
        transceiver_type_abbr_name = origin_transceiver_type
        if transceiver_type_abbr_name != 'Unknown':
            return transceiver_type_abbr_name
        type_abbrv_name = {
            '00': 'Unknown', '01': 'GBIC', '02': 'Soldered', '03': 'SFP', '04': 'XBI300',
            '05': 'XENPAK', '06': 'XFP', '07': 'XFF', '08': 'XFP-E', '09': 'XPAK', '0a': 'X2',
            '0b': 'DWDM-SFP', '0c': 'QSFP', '0d': 'QSFP+', '0e': 'CXP', '0f': 'HD4X',
            '10': 'HD8X', '11': 'QSFP28', '12': 'CXP2', '13': 'CDFP-1/2', '14': 'HD4X-Fanout',
            '15': 'HD8X-Fanout', '16': 'CDFP-3', '17': 'MicroQSFP', '18': 'QSFP-DD',
            '19': 'OSFP-8X', '1a': 'SFP-DD', '1b': 'DSFP', '1c': 'Link-x4', '1d': 'Link-x8', '1e': 'QSFP+C'
        }
        offset = 128
        sfp_type_raw = self._read_eeprom_specific_bytes((offset + QSFP_DD_TYPE_OFFSET), XCVR_TYPE_WIDTH)
        return type_abbrv_name.get(sfp_type_raw[0], 'Unknown')

    def get_transceiver_info(self):
        transceiver_info_dict = {}
        compliance_code_dict = {}
        transceiver_info_dict = dict.fromkeys(self.info_dict_keys, 'N/A')
        transceiver_info_dict['specification_compliance'] = '{}'

        if not self.get_presence():
            return transceiver_info_dict

        self._dom_capability_detect()
        sfpi_obj = qsfp_dd_InterfaceId()
        if not sfpi_obj:
            return None

        offset = 128
        sfp_type_raw = self._read_eeprom_specific_bytes((offset + QSFP_DD_TYPE_OFFSET), XCVR_TYPE_WIDTH)
        sfp_type_data = sfpi_obj.parse_sfp_type(sfp_type_raw, 0)
        sfp_type_abbrv_name = sfpi_obj.parse_sfp_type_abbrv_name(sfp_type_raw, 0)

        sfp_vendor_name_raw = self._read_eeprom_specific_bytes((offset + QSFP_DD_VENDOR_NAME_OFFSET), XCVR_VENDOR_NAME_WIDTH)
        sfp_vendor_name_data = sfpi_obj.parse_vendor_name(sfp_vendor_name_raw, 0)
        sfp_vendor_pn_raw = self._read_eeprom_specific_bytes((offset + QSFP_DD_VENDOR_PN_OFFSET), XCVR_VENDOR_PN_WIDTH)
        sfp_vendor_pn_data = sfpi_obj.parse_vendor_pn(sfp_vendor_pn_raw, 0)
        sfp_vendor_rev_raw = self._read_eeprom_specific_bytes((offset + XCVR_HW_REV_OFFSET_QSFP_DD), XCVR_HW_REV_WIDTH_QSFP_DD)
        sfp_vendor_rev_data = sfpi_obj.parse_vendor_rev(sfp_vendor_rev_raw, 0)
        sfp_vendor_sn_raw = self._read_eeprom_specific_bytes((offset + QSFP_DD_VENDOR_SN_OFFSET), XCVR_VENDOR_SN_WIDTH)
        sfp_vendor_sn_data = sfpi_obj.parse_vendor_sn(sfp_vendor_sn_raw, 0)
        sfp_vendor_oui_raw = self._read_eeprom_specific_bytes((offset + QSFP_DD_VENDOR_OUI_OFFSET), XCVR_VENDOR_OUI_WIDTH)
        sfp_vendor_oui_data = sfpi_obj.parse_vendor_oui(sfp_vendor_oui_raw, 0)
        sfp_vendor_date_raw = self._read_eeprom_specific_bytes((offset + XCVR_VENDOR_DATE_OFFSET_QSFP_DD), XCVR_VENDOR_DATE_WIDTH_QSFP_DD)
        sfp_vendor_date_data = sfpi_obj.parse_vendor_date(sfp_vendor_date_raw, 0)
        sfp_connector_raw = self._read_eeprom_specific_bytes((offset + XCVR_CONNECTOR_OFFSET_QSFP_DD), XCVR_CONNECTOR_WIDTH_QSFP_DD)
        sfp_connector_data = sfpi_obj.parse_connector(sfp_connector_raw, 0)
        sfp_ext_identifier_raw = self._read_eeprom_specific_bytes((offset + XCVR_EXT_TYPE_OFFSET_QSFP_DD), XCVR_EXT_TYPE_WIDTH_QSFP_DD)
        sfp_ext_identifier_data = sfpi_obj.parse_ext_iden(sfp_ext_identifier_raw, 0)
        sfp_cable_len_raw = self._read_eeprom_specific_bytes((offset + XCVR_CABLE_LENGTH_OFFSET_QSFP_DD), XCVR_CABLE_LENGTH_WIDTH_QSFP_DD)
        sfp_cable_len_data = sfpi_obj.parse_cable_len(sfp_cable_len_raw, 0)

        transceiver_info_dict['manufacturer'] = str(sfp_vendor_name_data['data']['Vendor Name']['value'])
        transceiver_info_dict['model'] = str(sfp_vendor_pn_data['data']['Vendor PN']['value'])
        transceiver_info_dict['hardware_rev'] = str(sfp_vendor_rev_data['data']['Vendor Rev']['value'])
        transceiver_info_dict['vendor_rev'] = transceiver_info_dict['hardware_rev']
        transceiver_info_dict['serial'] = sfp_vendor_sn_data['data']['Vendor SN']['value']
        transceiver_info_dict['vendor_oui'] = str(sfp_vendor_oui_data['data']['Vendor OUI']['value'])
        transceiver_info_dict['vendor_date'] = str(sfp_vendor_date_data['data']['VendorDataCode(YYYY-MM-DD Lot)']['value'])
        transceiver_info_dict['connector'] = str(sfp_connector_data['data']['Connector']['value'])
        transceiver_info_dict['encoding'] = "Not supported for CMIS cables"
        transceiver_info_dict['ext_identifier'] = str(sfp_ext_identifier_data['data']['Extended Identifier']['value'])
        transceiver_info_dict['ext_rateselect_compliance'] = "Not supported for CMIS cables"
        transceiver_info_dict['specification_compliance'] = "Not supported for CMIS cables"
        transceiver_info_dict['cable_type'] = "Length Cable Assembly(m)"
        transceiver_info_dict['cable_length'] = str(sfp_cable_len_data['data']['Length Cable Assembly(m)']['value'])
        transceiver_info_dict['nominal_bit_rate'] = "Not supported for CMIS cables"
        transceiver_info_dict['type'] = self.get_transceiver_type(str(sfp_type_data['data']['type']['value']))
        transceiver_info_dict['type_abbrv_name'] = self.get_transceiver_type_abbr_name(str(sfp_type_abbrv_name['data']['type_abbrv_name']['value']))

        sfp_media_type_raw = self._read_eeprom_specific_bytes(XCVR_MEDIA_TYPE_OFFSET_QSFP_DD, XCVR_MEDIA_TYPE_WIDTH_QSFP_DD)
        if sfp_media_type_raw is not None:
            sfp_media_type_dict = sfpi_obj.parse_media_type(sfp_media_type_raw, 0)
            if sfp_media_type_dict is None:
                return transceiver_info_dict
            host_media_list = ""
            sfp_application_type_first_list = self._read_eeprom_specific_bytes((XCVR_FIRST_APPLICATION_LIST_OFFSET_QSFP_DD), XCVR_FIRST_APPLICATION_LIST_WIDTH_QSFP_DD)
            if self.second_application_list:
                possible_application_count = 15
                sfp_application_type_second_list = self._read_eeprom_specific_bytes((XCVR_SECOND_APPLICATION_LIST_OFFSET_QSFP_DD), XCVR_SECOND_APPLICATION_LIST_WIDTH_QSFP_DD)
                if sfp_application_type_first_list is not None and sfp_application_type_second_list is not None:
                    sfp_application_type_list = sfp_application_type_first_list + sfp_application_type_second_list
                else:
                    return transceiver_info_dict
            else:
                possible_application_count = 8
                if sfp_application_type_first_list is not None:
                    sfp_application_type_list = sfp_application_type_first_list
                else:
                    return transceiver_info_dict
            for i in range(0, possible_application_count):
                if sfp_application_type_list[i * 4] == 'ff':
                    break
                host_electrical, media_interface = sfpi_obj.parse_application(sfp_media_type_dict, sfp_application_type_list[i * 4], sfp_application_type_list[i * 4 + 1])
                host_media_list = host_media_list + host_electrical + ' - ' + media_interface + '\n\t\t\t\t   '
        else:
            return transceiver_info_dict
        transceiver_info_dict['application_advertisement'] = host_media_list

        if sfp_media_type_raw[0] == '03':
            transceiver_info_dict['specification_compliance'] = "{'media_interface':'DAC'}"
            transceiver_info_dict['type_abbrv_name'] = "{'media_interface':'DAC'}"
        elif sfp_media_type_raw[0] in ('04', '01', '02'):
            transceiver_info_dict['specification_compliance'] = "{'media_interface':'AOC'}"
            transceiver_info_dict['type_abbrv_name'] = "{'media_interface':'AOC'}"

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
        transceiver_dom_info_dict = {}
        dom_info_dict_keys = ['temperature', 'voltage', 'rx1power', 'rx2power', 'rx3power', 'rx4power',
                              'rx5power', 'rx6power', 'rx7power', 'rx8power', 'tx1bias', 'tx2bias',
                              'tx3bias', 'tx4bias', 'tx5bias', 'tx6bias', 'tx7bias', 'tx8bias',
                              'tx1power', 'tx2power', 'tx3power', 'tx4power', 'tx5power', 'tx6power',
                              'tx7power', 'tx8power']
        transceiver_dom_info_dict = dict.fromkeys(dom_info_dict_keys, 'N/A')

        if not self.get_presence():
            return {}

        self._dom_capability_detect()
        offset = 0
        sfpd_obj = qsfp_dd_Dom()
        if sfpd_obj is None:
            return transceiver_dom_info_dict

        dom_data_raw = self._read_eeprom_specific_bytes((offset + QSFP_DD_DOM_BULK_DATA_START), QSFP_DD_DOM_BULK_DATA_SIZE)
        if dom_data_raw is None:
            return transceiver_dom_info_dict

        if self.dom_temp_supported:
            start = QSFP_DD_TEMPE_OFFSET - QSFP_DD_DOM_BULK_DATA_START
            end = start + QSFP_DD_TEMPE_WIDTH
            dom_temperature_data = sfpd_obj.parse_temperature(dom_data_raw[start : end], 0)
            temp = self._convert_string_to_num(dom_temperature_data['data']['Temperature']['value'])
            if temp is not None:
                transceiver_dom_info_dict['temperature'] = temp

        if self.dom_volt_supported:
            start = QSFP_DD_VOLT_OFFSET - QSFP_DD_DOM_BULK_DATA_START
            end = start + QSFP_DD_VOLT_WIDTH
            dom_voltage_data = sfpd_obj.parse_voltage(dom_data_raw[start : end], 0)
            volt = self._convert_string_to_num(dom_voltage_data['data']['Vcc']['value'])
            if volt is not None:
                transceiver_dom_info_dict['voltage'] = volt

        if self.dom_rx_tx_power_bias_supported:
            offset = CMIS_UPPER_PAGE11_START
            dom_data_raw = self._read_eeprom_specific_bytes(offset + QSFP_DD_CHANNL_MON_OFFSET, QSFP_DD_CHANNL_MON_WIDTH)
            if dom_data_raw is None:
                return transceiver_dom_info_dict
            dom_channel_monitor_data = sfpd_obj.parse_channel_monitor_params(dom_data_raw, 0)

            if self.dom_tx_power_supported:
                for i in range(1, 9):
                    key = f'tx{i}power'
                    val = dom_channel_monitor_data['data'].get(f'TX{i}Power', {}).get('value', 'N/A')
                    transceiver_dom_info_dict[key] = str(self._convert_string_to_num(val))

            if self.dom_rx_power_supported:
                for i in range(1, 9):
                    key = f'rx{i}power'
                    val = dom_channel_monitor_data['data'].get(f'RX{i}Power', {}).get('value', 'N/A')
                    transceiver_dom_info_dict[key] = str(self._convert_string_to_num(val))

            if self.dom_tx_bias_power_supported:
                for i in range(1, 9):
                    key = f'tx{i}bias'
                    val = dom_channel_monitor_data['data'].get(f'TX{i}Bias', {}).get('value', 'N/A')
                    transceiver_dom_info_dict[key] = str(val)

        return transceiver_dom_info_dict

    def get_transceiver_threshold_info(self):
        transceiver_dom_threshold_info_dict = {}
        dom_info_dict_keys = ['temphighalarm', 'temphighwarning', 'templowalarm', 'templowwarning',
                              'vcchighalarm', 'vcchighwarning', 'vcclowalarm', 'vcclowwarning',
                              'rxpowerhighalarm', 'rxpowerhighwarning', 'rxpowerlowalarm', 'rxpowerlowwarning',
                              'txpowerhighalarm', 'txpowerhighwarning', 'txpowerlowalarm', 'txpowerlowwarning',
                              'txbiashighalarm', 'txbiashighwarning', 'txbiaslowalarm', 'txbiaslowwarning']
        transceiver_dom_threshold_info_dict = dict.fromkeys(dom_info_dict_keys, 'N/A')
        
        if not self.get_presence():
            return {}
        if not self.dom_supported or not self.dom_thresholds_supported:
            return transceiver_dom_threshold_info_dict

        sfpd_obj = qsfp_dd_Dom()
        if sfpd_obj is None:
            return transceiver_dom_threshold_info_dict

        offset = CMIS_UPPER_PAGE02_START
        dom_module_threshold_raw = self._read_eeprom_specific_bytes((offset + QSFP_DD_MODULE_THRESHOLD_OFFSET), QSFP_DD_MODULE_THRESHOLD_WIDTH)
        if dom_module_threshold_raw is None:
            return transceiver_dom_threshold_info_dict

        dom_module_threshold_data = sfpd_obj.parse_module_threshold_values(dom_module_threshold_raw, 0)
        transceiver_dom_threshold_info_dict['temphighalarm'] = dom_module_threshold_data['data']['TempHighAlarm']['value']
        transceiver_dom_threshold_info_dict['temphighwarning'] = dom_module_threshold_data['data']['TempHighWarning']['value']
        transceiver_dom_threshold_info_dict['templowalarm'] = dom_module_threshold_data['data']['TempLowAlarm']['value']
        transceiver_dom_threshold_info_dict['templowwarning'] = dom_module_threshold_data['data']['TempLowWarning']['value']
        transceiver_dom_threshold_info_dict['vcchighalarm'] = dom_module_threshold_data['data']['VccHighAlarm']['value']
        transceiver_dom_threshold_info_dict['vcchighwarning'] = dom_module_threshold_data['data']['VccHighWarning']['value']
        transceiver_dom_threshold_info_dict['vcclowalarm'] = dom_module_threshold_data['data']['VccLowAlarm']['value']
        transceiver_dom_threshold_info_dict['vcclowwarning'] = dom_module_threshold_data['data']['VccLowWarning']['value']
        transceiver_dom_threshold_info_dict['rxpowerhighalarm'] = dom_module_threshold_data['data']['RxPowerHighAlarm']['value']
        transceiver_dom_threshold_info_dict['rxpowerhighwarning'] = dom_module_threshold_data['data']['RxPowerHighWarning']['value']
        transceiver_dom_threshold_info_dict['rxpowerlowalarm'] = dom_module_threshold_data['data']['RxPowerLowAlarm']['value']
        transceiver_dom_threshold_info_dict['rxpowerlowwarning'] = dom_module_threshold_data['data']['RxPowerLowWarning']['value']
        transceiver_dom_threshold_info_dict['txbiashighalarm'] = dom_module_threshold_data['data']['TxBiasHighAlarm']['value']
        transceiver_dom_threshold_info_dict['txbiashighwarning'] = dom_module_threshold_data['data']['TxBiasHighWarning']['value']
        transceiver_dom_threshold_info_dict['txbiaslowalarm'] = dom_module_threshold_data['data']['TxBiasLowAlarm']['value']
        transceiver_dom_threshold_info_dict['txbiaslowwarning'] = dom_module_threshold_data['data']['TxBiasLowWarning']['value']
        transceiver_dom_threshold_info_dict['txpowerhighalarm'] = dom_module_threshold_data['data']['TxPowerHighAlarm']['value']
        transceiver_dom_threshold_info_dict['txpowerhighwarning'] = dom_module_threshold_data['data']['TxPowerHighWarning']['value']
        transceiver_dom_threshold_info_dict['txpowerlowalarm'] = dom_module_threshold_data['data']['TxPowerLowAlarm']['value']
        transceiver_dom_threshold_info_dict['txpowerlowwarning'] = dom_module_threshold_data['data']['TxPowerLowWarning']['value']

        return transceiver_dom_threshold_info_dict

    def get_reset_status(self):
        try:
            attr_rv = self.__api_helper.read_one_line_file(self.__attr_path_prefix + 'reset')
            if attr_rv is None:
                return False
            val = attr_rv.strip().lower()
            if val in ('na', 'n/a', 'none', '', 'err'):
                return False
            base = 16 if val.startswith('0x') else 10
            return (int(val, base) & 0x1) == 1
        except (ValueError, TypeError, Exception):
            return False

    def get_rx_los(self):
        if not self.dom_supported:
            return None
        rx_los_list = []
        if self.dom_rx_tx_power_bias_supported:
            offset = 512
            dom_channel_monitor_raw = self._read_eeprom_specific_bytes((offset + QSFP_DD_CHANNL_RX_LOS_STATUS_OFFSET), QSFP_DD_CHANNL_RX_LOS_STATUS_WIDTH)
            if dom_channel_monitor_raw is not None:
                try:
                    rx_los_data = int(dom_channel_monitor_raw[0], 8)
                    for i in range(8):
                        rx_los_list.append((rx_los_data & (1 << i)) != 0)
                except (ValueError, TypeError):
                    return [False] * 8
        return rx_los_list if rx_los_list else [False] * 8

    def get_tx_fault(self):
        if not self.dom_supported:
            return None
        return [False] * 8

    def get_lpmode(self):
        try:
            attr_rv = self.__get_attr_value(self.__lpmode_attr)
            if attr_rv == 'ERR':
                return False
            val = attr_rv.strip().lower()
            if val in ('na', 'n/a', 'none', '', 'err'):
                return False
            base = 16 if val.startswith('0x') else 10
            return (int(val, base) & 0x1) == 1
        except (ValueError, TypeError, Exception):
            return False

    def get_power_override(self):
        return None

    def get_temperature(self):
        if not self.dom_temp_supported:
            return None
        offset = 0
        sfpd_obj = qsfp_dd_Dom()
        if sfpd_obj is None:
            return None
        dom_temperature_raw = self._read_eeprom_specific_bytes((offset + QSFP_DD_TEMPE_OFFSET), QSFP_DD_TEMPE_WIDTH)
        if dom_temperature_raw is not None:
            dom_temperature_data = sfpd_obj.parse_temperature(dom_temperature_raw, 0)
            return self._convert_string_to_num(dom_temperature_data['data']['Temperature']['value'])
        return None

    def get_voltage(self):
        if not self.dom_volt_supported:
            return None
        offset = 128
        sfpd_obj = qsfp_dd_Dom()
        if sfpd_obj is None:
            return None
        dom_voltage_raw = self._read_eeprom_specific_bytes((offset + QSFP_DD_VOLT_OFFSET), QSFP_DD_VOLT_WIDTH)
        if dom_voltage_raw is not None:
            dom_voltage_data = sfpd_obj.parse_voltage(dom_voltage_raw, 0)
            return self._convert_string_to_num(dom_voltage_data['data']['Vcc']['value'])
        return None

    def get_tx_bias(self):
        tx_bias_list = []
        if self.dom_rx_tx_power_bias_supported and self.dom_tx_bias_power_supported:
            offset = CMIS_UPPER_PAGE11_START
            sfpd_obj = qsfp_dd_Dom()
            if sfpd_obj is None:
                return None
            dom_tx_bias_raw = self._read_eeprom_specific_bytes((offset + QSFP_DD_TX_BIAS_OFFSET), QSFP_DD_TX_BIAS_WIDTH)
            if dom_tx_bias_raw is not None:
                dom_tx_bias_data = sfpd_obj.parse_dom_tx_bias(dom_tx_bias_raw, 0)
                for i in range(1, 9):
                    val = dom_tx_bias_data['data'].get(f'TX{i}Bias', {}).get('value', 'N/A')
                    tx_bias_list.append(self._convert_string_to_num(val))
        return tx_bias_list if tx_bias_list else ['N/A'] * 8

    def get_rx_power(self):
        rx_power_list = []
        if self.dom_rx_tx_power_bias_supported and self.dom_rx_power_supported:
            offset = CMIS_UPPER_PAGE11_START
            sfpd_obj = qsfp_dd_Dom()
            if sfpd_obj is None:
                return None
            dom_rx_power_raw = self._read_eeprom_specific_bytes((offset + QSFP_DD_RX_POWER_OFFSET), QSFP_DD_RX_POWER_WIDTH)
            if dom_rx_power_raw is not None:
                dom_rx_power_data = sfpd_obj.parse_dom_rx_power(dom_rx_power_raw, 0)
                for i in range(1, 9):
                    val = dom_rx_power_data['data'].get(f'RX{i}Power', {}).get('value', 'N/A')
                    rx_power_list.append(self._convert_string_to_num(val))
        return rx_power_list if rx_power_list else ['N/A'] * 8

    def get_tx_power(self):
        return ['N/A'] * 8

    def reset(self):
        try:
            with open(self.__reset_attr, 'w') as f:
                f.write('1')
            time.sleep(2)
            with open(self.__reset_attr, 'w') as f:
                f.write('0')
            return True
        except (IOError, OSError, Exception):
            return False

    def is_mem_flat(self):
        offset = CMIS_LOWER_PAGE_START + CMIS_LOWER_PAGE_FLAT_MEM
        data = self._read_eeprom_specific_bytes(offset, 1)
        try:
            return (int(data[0], 16) & 0xff) >> 7
        except (ValueError, TypeError):
            return True

    def get_tx_disable_support(self):
        offset = CMIS_UPPER_PAGE01_START + CMIS_UPPER_PAGE01_TX_DISABLE_IMPLEMENTED
        data = self._read_eeprom_specific_bytes(offset, 1)
        try:
            tx_disable_implemented = ((int(data[0], 16) & 0xff) & (0x1 << 1)) >> 1
            return not self.is_mem_flat() and tx_disable_implemented
        except (ValueError, TypeError):
            return False
    
    def get_tx_disable(self):
        tx_disable_support = self.get_tx_disable_support()
        if tx_disable_support is None or not tx_disable_support:
            return ["N/A" for _ in range(self.NUM_CHANNELS)]
        offset = CMIS_UPPER_PAGE10_START + CMIS_UPPER_PAGE10_TX_DISABLE_OFFSET
        tx_disable = self._read_eeprom_specific_bytes(offset, 1)
        try:
            return [bool(int(tx_disable[0], 16) & (1 << i)) for i in range(self.NUM_CHANNELS)]
        except (ValueError, TypeError):
            return [False] * self.NUM_CHANNELS

    def get_tx_disable_channel(self):
        tx_disable_support = self.get_tx_disable_support()
        if tx_disable_support is None or not tx_disable_support:
            return 'N/A'
        offset = CMIS_UPPER_PAGE10_START + CMIS_UPPER_PAGE10_TX_DISABLE_OFFSET
        tx_disable = self._read_eeprom_specific_bytes(offset, 1)
        try:
            return int(tx_disable[0], 16)
        except (ValueError, TypeError):
            return 0

    def tx_disable(self, tx_disable):
        if not self.get_presence():
            return False
        if self.dom_tx_disable_supported:
            channel_mask = 0xff
            return self.tx_disable_channel(channel_mask, tx_disable)
        return False

    def tx_disable_channel(self, channel, disable):
        channel_state = self.get_tx_disable_channel()
        if channel_state is None or channel_state == 'N/A':
            return False
        try:
            channel_state = int(channel_state) if isinstance(channel_state, str) else channel_state
        except (ValueError, TypeError):
            return False
        for i in range(self.NUM_CHANNELS):
            mask = (1 << i)
            if not (channel & mask):
                continue
            if disable:
                channel_state |= mask
            else:
                channel_state &= ~mask
        offset = CMIS_UPPER_PAGE10_START + CMIS_UPPER_PAGE10_TX_DISABLE_OFFSET
        return self._write_eeprom_specific_bytes(offset, channel_state)
    
    def set_datapath_deinit(self, channel):
        offset = CMIS_LOWER_PAGE_START + CMIS_LOWER_PAGE_REVISION_COMPLIANCE_OFFSET
        cmis_major = self._read_eeprom_specific_bytes(offset, 1)
        offset = CMIS_UPPER_PAGE10_START + CMIS_UPPER_PAGE10_DATA_PATH_DEINIT_OFFSET
        data = self._read_eeprom_specific_bytes(offset, 1)
        try:
            deinit_value = int(data[0], 16)
            cmis_ver = int(cmis_major[0], 16) if cmis_major else 3
            for lane in range(self.NUM_CHANNELS):
                if ((1 << lane) & channel) == 0:
                    continue
                if cmis_ver >= 4:
                    deinit_value |= (1 << lane)
                else:
                    deinit_value &= ~(1 << lane)
            return self._write_eeprom_specific_bytes(offset, deinit_value)
        except (ValueError, TypeError):
            return False

    def set_datapath_init(self, channel):
        offset = CMIS_LOWER_PAGE_START + CMIS_LOWER_PAGE_REVISION_COMPLIANCE_OFFSET
        cmis_major = self._read_eeprom_specific_bytes(offset, 1)
        offset = CMIS_UPPER_PAGE10_START + CMIS_UPPER_PAGE10_DATA_PATH_DEINIT_OFFSET
        data = self._read_eeprom_specific_bytes(offset, 1)
        try:
            deinit_value = int(data[0], 16)
            cmis_ver = int(cmis_major[0], 16) if cmis_major else 3
            for lane in range(self.NUM_CHANNELS):
                if ((1 << lane) & channel) == 0:
                    continue
                if cmis_ver >= 4:
                    deinit_value &= ~(1 << lane)
                else:
                    deinit_value |= (1 << lane)
            return self._write_eeprom_specific_bytes(offset, deinit_value)
        except (ValueError, TypeError):
            return False

    def get_module_state(self):
        offset = CMIS_LOWER_PAGE_START + CMIS_LOWER_PAGE_MODULE_STATE_OFFSET
        data = self._read_eeprom_specific_bytes(offset, 1)
        try:
            state = (int(data[0], 16) & 0xe) >> 1
            return MODULE_STATE.get(state, 'Unknown')
        except (ValueError, TypeError, KeyError):
            return 'Unknown'

    def test_module_state(self, states):
        return self.get_module_state() in states

    def get_datapath_state(self, channel):
        for lane in range(self.NUM_CHANNELS):
            if ((1 << lane) & channel) == 0:
                continue
            offset = CMIS_UPPER_PAGE11_START + CMIS_UPPER_PAGE11_DATA_PATH_STATE_OFFSET + (lane//2)
            data = self._read_eeprom_specific_bytes(offset, 1)
            try:
                tmp_data = int(data[0], 16)
                state = tmp_data >> 4 if tmp_data % 2 else tmp_data & 0xf
                return DATAPATH_STATE.get(state, 'Unknown')
            except (ValueError, TypeError, KeyError):
                return 'Unknown'
        return 'Unknown'

    def test_datapath_state(self, channel, states):
        return self.get_datapath_state(channel) in states

    def set_application(self, channel, appl_code):
        lane_first = -1
        for lane in range(self.NUM_CHANNELS):
            if ((1 << lane) & channel) == 0:
                continue
            if lane_first < 0:
                lane_first = lane
            offset = CMIS_UPPER_PAGE10_START + CMIS_UPPER_PAGE10_STAGED0_APSEL_CONTROL_OFFSET + lane
            data = (appl_code << 4) | (lane_first << 1)
            self._write_eeprom_specific_bytes(offset, data)
        offset = CMIS_UPPER_PAGE10_START + CMIS_UPPER_PAGE10_STAGED0_APPLY_DATA_PATH_INIT_OFFSET
        return self._write_eeprom_specific_bytes(offset, channel)

    def get_config_datapath_hostlane_status(self, channel):
        for lane in range(self.NUM_CHANNELS):
            if ((1 << lane) & channel) == 0:
                continue
            offset = CMIS_UPPER_PAGE11_START + CMIS_UPPER_PAGE11_CONFIG_ERROR_CODE_OFFSET + (lane//2)
            data = self._read_eeprom_specific_bytes(offset, 1)
            try:
                tmp_data = int(data[0], 16)
                state = tmp_data >> 4 if tmp_data % 2 else tmp_data & 0xf
                return CONFIG_STATUS.get(state, 'Unknown')
            except (ValueError, TypeError, KeyError):
                return 'Unknown'
        return 'Unknown'

    def test_config_error(self, channel, states):
        return self.get_config_datapath_hostlane_status(channel) in states

    def reset_cmis_init(self, retries):
        self.cmis_state = CMIS_STATE_INSERTED
        self.cmis_retries = retries
        self.cmis_expired = None

    def init_sequence(self):
        return self.cmis_state

    def set_lpmode(self, lpmode):
        try:
            with open(self.__lpmode_attr, 'w') as f:
                f.write('1' if lpmode else '0')
            return True
        except (IOError, OSError, Exception):
            return False

    def set_power_override(self, power_override, power_set):
        return True
