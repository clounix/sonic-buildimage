#!/usr/bin/env python3
#
# Name: eeprom.py, version: 1.1
#
# Description: Module contains the definitions of SONiC platform APIs
#

import os
import syslog
import binascii

try:
    from sonic_eeprom import eeprom_tlvinfo
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

CACHE_ROOT = '/var/cache/sonic/decode-syseeprom'
CACHE_FILE = 'syseeprom_cache'

class Eeprom(eeprom_tlvinfo.TlvInfoDecoder):
    def __init__(self):
        self.__eeprom_path = "/sys/s3ip/syseeprom/syseeprom"
        super(Eeprom, self).__init__(self.__eeprom_path, 0, '', True)
        self.__eeprom_tlv_dict = {}
        self.__eeprom_data = b''
     
        cache_path = os.path.join(CACHE_ROOT, CACHE_FILE)
        if os.path.isfile(cache_path):
            try:
                os.remove(cache_path)
            except Exception:
                pass
            
        if not os.path.exists(CACHE_ROOT):
            try:
                os.makedirs(CACHE_ROOT)
            except Exception:
                pass

        try:
            self.set_cache_name(cache_path)
        except Exception as e:
            syslog.syslog(syslog.LOG_WARNING, f"Failed to set cache name: {e}")

        try:
            raw_data = self.read_eeprom()
            if not raw_data:
                raise ValueError("EEPROM read returned empty data")
            self.__eeprom_data = bytes(raw_data)
            self.update_cache(self.__eeprom_data)
        except Exception as e:
            syslog.syslog(syslog.LOG_ERR, f"EEPROM read or cache update failed: {e}")
            self.__eeprom_data = b''
            return

        if not self.is_valid_tlvinfo_header(self.__eeprom_data):
            syslog.syslog(syslog.LOG_ERR, "Invalid TLV info header in EEPROM")
            return

        total_length = (self.__eeprom_data[9] << 8) | self.__eeprom_data[10]
        tlv_index = self._TLV_INFO_HDR_LEN
        tlv_end = self._TLV_INFO_HDR_LEN + total_length

        while tlv_index < tlv_end and (tlv_index + 2) <= len(self.__eeprom_data):
            tlv_code = self.__eeprom_data[tlv_index]
            tlv_len = self.__eeprom_data[tlv_index + 1]
            
            if tlv_index + 2 + tlv_len > len(self.__eeprom_data):
                break

            tlv_data = self.__eeprom_data[tlv_index : tlv_index + 2 + tlv_len]

            try:
                if tlv_code == self._TLV_CODE_VENDOR_EXT and tlv_len >= 5:
                    vid = (tlv_data[2] << 24) | (tlv_data[3] << 16) | (tlv_data[4] << 8) | tlv_data[5]
                    val_str = tlv_data[6:6 + tlv_len - 4].decode('utf-8', errors='ignore')
                    value = f"{vid}{val_str}"
                else:
                    _, value = self.decoder(tlv_code, tlv_data)

                self.__eeprom_tlv_dict[f"0x{tlv_code:02X}"] = value
            except Exception as e:
                syslog.syslog(syslog.LOG_WARNING, f"Failed to decode TLV 0x{tlv_code:02X}: {e}")

            if tlv_code == self._TLV_CODE_CRC_32:
                break

            tlv_index += 2 + tlv_len

    def serial_number_str(self):
        return self.__eeprom_tlv_dict.get("0x23", "N/A")

    def base_mac_address(self):
        return self.__eeprom_tlv_dict.get("0x24", "N/A")

    def modelstr(self):
        return self.__eeprom_tlv_dict.get("0x21", "N/A")

    def part_number_str(self):
        return self.__eeprom_tlv_dict.get("0x22", "N/A")

    def serial_tag_str(self):
        return self.__eeprom_tlv_dict.get("0x25", "N/A")

    def revision_str(self):
        return self.__eeprom_tlv_dict.get("0x26", "N/A")

    def system_eeprom_info(self):
        """
        Returns a dictionary, where keys are the type code defined in
        ONIE EEPROM format and values are their corresponding values
        found in the system EEPROM.
        """
        return dict(self.__eeprom_tlv_dict)
    
