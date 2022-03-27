#!/usr/bin/env python

try:
    import binascii
    import time
    import optparse
    import warnings
    import os
    import sys
    from sonic_eeprom import eeprom_base
    from sonic_eeprom import eeprom_tlvinfo
    #from sonic_eeprom import eeprom_fruinfo
    import subprocess
except ImportError as e:
    raise ImportError (str(e) + "- required module not found")

class board(eeprom_tlvinfo.TlvInfoDecoder):
    _TLV_INFO_MAX_LEN = 256
    def __init__(self, name, path, cpld_root, ro):
        self.eeprom_path = "/sys/switch/syseeprom/eeprom"
        super(board, self).__init__(self.eeprom_path, 0, '', True)
"""
class psu(eeprom_fruinfo.ipmifru):
    _FRU_INFO_MAX_LEN = 256
    def __init__(self, name, index, cpld_root, ro):
        if (index == 1):
            self.eeprom_path = "/sys/bus/i2c/devices/1-0050/eeprom"
        else:
            self.eeprom_path = "/sys/bus/i2c/devices/1-0052/eeprom"
        super(psu, self).__init__(self.eeprom_path, 0, '', True)

class fanboard(eeprom_fruinfo.ipmifru):
    _FRU_INFO_MAX_LEN = 256
    def __init__(self, name, path, cpld_root, ro):
        self.eeprom_path = "/sys/bus/i2c/devices/6-0051/eeprom"
        super(fanboard, self).__init__(self.eeprom_path, 0, '', True)
"""
