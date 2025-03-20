#!/bin/python2.7

import os
import re
import inspect
import time
import random
import sys
import logging
import traceback

class Flash:
    ZBUSYBOX_SET = 'busybox devmem 0x%x 32 0x%x'
    ZBUSYBOX_GET = 'busybox devmem 0x%x'
    ILAST_CHECK_SUM = 0xBABA
    FWAIT_READ = 0.1
    FWAIT_WRITE = 0.2
    IOFFSET_SWSM = 0x5b50
    IOFFSET_SW_FW_SYNC = 0x5b5c

    def __init__(self):
        self.init_log()
        self.ipci_addr = self.pci()

    def init_log(self):
        # log_config = {  
        #     'version': 1,  
        #     'disable_existing_loggers': False,  
        #     'handlers': {  
        #         'console': {  
        #             'class': 'logging.StreamHandler',  
        #             'stream': 'ext://sys.stdout',  
        #         },  
        #         'file': {  
        #             'class': 'logging.FileHandler',  
        #             'filename': 'mac.log',  
        #         },  
        #     },  
        #     'root': {  
        #         'handlers': ['console', 'file'],  
        #         'level': 'DEBUG',  
        #     },  
        # }  
        # logging.config.dictConfig(log_config)  
        logging.basicConfig(level=logging.DEBUG, stream=sys.stdout)
        self.log = logging.getLogger()  

    def pci(self):
        str = os.popen('lspci | grep I210').read()
        zdtb = str.split()[0]
        str = os.popen('lspci -v -s %s | grep size=512K' % zdtb).read()
        span = re.search('[a-fA-F0-9]{8,}', str).span()
        zpci_addr = str[span[0]:span[1]]
        ipci_addr = int(zpci_addr, 16)
        self.log.info('pci address: 0x%x' % ipci_addr)
        if ipci_addr <= 0:
            raise Exception('get pci address failed.')
        return ipci_addr

    def semaphore_common(self, imask):
        itime = time.time()
        iswsm_addr = self.ipci_addr + self.IOFFSET_SWSM
        while True:
            str = os.popen(self.ZBUSYBOX_SET % (iswsm_addr, imask)).read()
            time.sleep(self.FWAIT_WRITE)
            str = os.popen(self.ZBUSYBOX_GET % iswsm_addr).read()
            time.sleep(self.FWAIT_READ)
            iswsm_value = int(str[2:], 16)
            if imask == 0:
                self.log.info('release semaphore ownership %08X' % iswsm_value)
                return True
            if iswsm_value & imask:
                self.log.info('acquiring semaphore ownership success %08X' % iswsm_value)
                return True
            ielapse = time.time()
            if ielapse - itime > 30:
                raise Exception('acquiring ownership %08x' % iswsm_value)
        return False

    def semaphore_set(self):
        self.semaphore_common(0x1)
        self.semaphore_common(0x2)

    def semaphore_clear(self):
        self.semaphore_common(0x0)

    def sw_fw_sync_check(self, braise = False):
        isw_fw_sync_addr = self.ipci_addr + self.IOFFSET_SW_FW_SYNC
        str = os.popen(self.ZBUSYBOX_GET % isw_fw_sync_addr).read()
        time.sleep(self.FWAIT_READ)
        isw_fw_sync_value = int(str[2:], 16)
        bstatus = False
        if isw_fw_sync_value & 0x1:
            # software already had ownership
            if isw_fw_sync_value & 0x00010000:
                self.log.info('ownership conflicts. %08x' % isw_fw_sync_value)
            else:
                self.log.debug('software got ownership. %08x' % isw_fw_sync_value)
                bstatus = True
        else:
            # software does not had ownership
            if isw_fw_sync_value & 0x00010000:
                self.log.info('ownership lost. %08x' % isw_fw_sync_value)
            else:
                self.log.info('software can acquire ownership. %08x' % isw_fw_sync_value)
        if braise == True and bstatus == False:
            raise Exception('acquire semaphore ownership failed.')
        return bstatus

    def sw_fw_sync_set(self):
        isw_fw_sync_addr = self.ipci_addr + self.IOFFSET_SW_FW_SYNC
        str = os.popen(self.ZBUSYBOX_SET % (isw_fw_sync_addr, 0x1)).read()
        time.sleep(self.FWAIT_WRITE)


class Eeprom(Flash):
    IOFFSET_EEC = 0x12010
    IOFFSET_FLA = 0x1201c
    IOFFSET_EERD = 0x12014
    IOFFSET_EEWR = 0x12018
    IOFFSET_EELOADCTL = 0x12020
    IEEC_FLUPD = 0x800000
    IEEC_UNLOCK = 0xffffffff
    IEEC_VALID = 0x40
    IEEC_DONE = 0x4000000
    IFLA_UNLOCK = 0x11f
    IFLA_LOCK = 0x100
    IFLA_WRITABLE = 0x10
    IEELOADCTL_UNLOCK = 0xe7
    IEELOADCTL_LOCK = 0x40000
    IEELOADCTL_WRITABLE = 0x80
    IEELOADCTL_VALID = 0x100

    def __init__(self):
        Flash.__init__(self)

    def eec_check(self, imask):
        ieec_addr = self.ipci_addr + self.IOFFSET_EEC
        str = os.popen(self.ZBUSYBOX_GET % ieec_addr).read()
        time.sleep(self.FWAIT_READ)
        ieec_value = int(str[2:], 16)
        if ieec_value & imask:
            self.log.debug('EEC cehck passed %08x' % ieec_value)
            return True
        if imask == self.IEEC_VALID:
            raise Exception('EEC check failed %08x' % ieec_value)
        return False

    def eec_set(self, ival):
        ieec_addr = self.ipci_addr + self.IOFFSET_EEC
        str = os.popen(self.ZBUSYBOX_SET % (ieec_addr, ival)).read()
        time.sleep(self.FWAIT_WRITE)

    def fla_check(self, braise = False):
        ifla_addr = self.ipci_addr + self.IOFFSET_FLA
        str = os.popen(self.ZBUSYBOX_GET % ifla_addr).read()
        time.sleep(self.FWAIT_READ)
        ifla_value = int(str[2:], 16)
        if ifla_value & self.IFLA_WRITABLE:
            self.log.debug('FLA check passwd %08x' % ifla_value)
            return True
        self.log.info('FLA check blocked %08x' % ifla_value)
        if braise == True:
            raise Exception('FLA check failed')
        return False

    def fla_set(self, icontrol):
        ifla_addr = self.ipci_addr + self.IOFFSET_FLA
        str = os.popen(self.ZBUSYBOX_SET % (ifla_addr, icontrol)).read()
        time.sleep(self.FWAIT_WRITE)

    def read_mac_rom(self, ieerd_addr, irom_op):
        str = os.popen(self.ZBUSYBOX_SET % (ieerd_addr, irom_op)).read()
        time.sleep(self.FWAIT_WRITE)
        str = os.popen(self.ZBUSYBOX_GET % (ieerd_addr)).read()
        time.sleep(self.FWAIT_READ)
        ieerd_value = int(str[2:], 16)
        if ieerd_value & 0x2:
            iret_value = ieerd_value >> 16
            return iret_value
        raise Exception('EERD failed %08x' % ieerd_value)

    def write_mac_rom(self, ieewr_addr, irom_op):
        str = os.popen(self.ZBUSYBOX_SET % (ieewr_addr, irom_op)).read()
        time.sleep(self.FWAIT_WRITE)
        str = os.popen(self.ZBUSYBOX_GET % (ieewr_addr)).read()
        time.sleep(self.FWAIT_READ)
        ieewr_value = int(str[2:], 16)
        if ieewr_value & 0x2:
            iret_value = ieewr_value >> 16
            return iret_value
        raise Exception('EEWR failed %08x' % ieewr_value)

    def to_human(self, ibig_endian):
        idw2 = ibig_endian & 0xFFFF0000
        iw1 = idw2 >> 16
        ib1 = iw1 & 0xFF
        ib2 = (iw1 & 0xFF00) >> 8
        iw2 = (ib1 << 8) | ib2
        return iw2

    def to_machine(self, ilittle_endian):
        if ilittle_endian > 0xFFFF:
            raise Exception('invalid parameter %08X' % ilittle_endian)
        ib1 = ilittle_endian & 0xFF
        ib2 = (ilittle_endian & 0xFF00) >> 8
        iw2 = (ib1 << 8) | ib2
        idw1 = iw2 << 16
        return idw1

    def show_mac(self):
        ieerd_addr = self.ipci_addr + self.IOFFSET_EERD
        macs = []
        zmacs = ''
        for i in range(0, 3):
            irom_op = (i << 2) | 1
            iw1 = self.read_mac_rom(ieerd_addr, irom_op)
            iw2 = self.to_human(iw1)
            macs.append(iw2)
        for i in macs:
            zmacs += '%04X-' % i
        zmacs = zmacs[:-2]
        self.log.info('MAC: %s', zmacs)

    def check_summary(self):
        ieerd_addr = self.ipci_addr + self.IOFFSET_EERD
        isum = 0
        for i in range(0, 0x40):
            irom_op = (i << 2) | 1
            iw1 = self.read_mac_rom(ieerd_addr, irom_op)
            isum += iw1
            if isum > 0xFFFF:
                isum &= 0xFFFF
            self.log.debug('addr: %02X value: %04X sum: %04X' % (i, iw1, isum))
        if isum == self.ILAST_CHECK_SUM:
            self.log.debug('CHECKSUM is right.')
        else:
            self.log.warn('CHECKSUM is wrong.')

    def update_mac(self):
        if sys.argv[1] != '-u':
            raise Exception('invalid parameters.')
        zmac = sys.argv[2]
        if len(zmac) != 12:
            raise Exception('invalid mac address: %s' % sys.argv[2])
        zw = ''
        macs = []
        for c in zmac:
            zw += c
            if len(zw) == 4:
                macs.append(int(zw, 16))
                zw = ''
        if len(macs) != 3:
            raise Exception('mac address error')
        ieewr_addr = self.ipci_addr + self.IOFFSET_EEWR 
        for i in range(0, 3):
            imac = macs[i]
            irom_op = (i << 2) | 0x1
            idw1 = self.to_machine(imac)
            idw2 = idw1 | irom_op
            self.write_mac_rom(ieewr_addr, idw2)

    def update_summary(self):
        ieerd_addr = self.ipci_addr + self.IOFFSET_EERD
        ieewr_addr = self.ipci_addr + self.IOFFSET_EEWR
        icheck_sum = 0
        i3f = 0
        for i in range(0, 0x3F):
            irom_op = (i << 2) | 1
            iw1 = self.read_mac_rom(ieerd_addr, irom_op)
            icheck_sum += iw1
            if icheck_sum > 0xFFFF:
                icheck_sum &= 0xFFFF
            # self.log.debug('addr: %02X value: %04X sum: %04X' % (i, iw1, icheck_sum))
        for itest in range(0, 0xFFFF):
            iverify = itest + icheck_sum
            if iverify > 0xFFFF:
                iverify &= 0xFFFF
            if iverify == self.ILAST_CHECK_SUM:
                i3f = itest
                break
        irom_op = (0x3F << 2) | 1
        idw1 = (i3f << 16) | irom_op
        self.write_mac_rom(ieewr_addr, idw1)

class Burst(Flash):
    IOFFSET_FLSWCTL = 0x12048
    IOFFSET_FLSWDATA = 0x1204c
    IOFFSET_FLSWCNT = 0x12050
    IFLSWCTL_DONE = 0x40000000
    IFLSWCTL_CMD = 0x01000000
    IFLSWCTL_CMDV = 0x10000000
    def __init__(self):
        super().__init__()

    def unlock(self):
        pass


def main():
    v = Eeprom()
    try:
        v.log.info('tool started.')
        # acquiring semaphore ownership
        v.semaphore_set()
        if v.sw_fw_sync_check() == False:
            v.sw_fw_sync_set()
        v.sw_fw_sync_check(True)
        # acquiring flash access
        v.eec_set(v.IEEC_UNLOCK)
        v.eec_check(v.IEEC_VALID)
        # if v.fla_check() == True:
        #     v.fla_set(v.IFLA_UNLOCK)
        # v.fla_check()
        # v.show_mac()
        # update mac address 
        if len(sys.argv) == 3:
            v.update_mac()
            v.update_summary()
            v.check_summary()
            # v.show_mac()
            # synchronize to flash
            # v.fla_set(v.IFLA_LOCK)
            # v.fla_check()
            v.semaphore_clear()
            v.eec_set(v.IEEC_FLUPD)
            v.eec_check(v.IEEC_DONE)
        else:
            v.check_summary()
    except Exception as e:
        # v.fla_set(v.IFLA_LOCK)
        # v.fla_check()
        v.semaphore_clear()
        traceback.print_exc()
        print e
    finally:
        pass

if __name__ == '__main__':
    os.system('networkctl down eth0')
    main()
    os.system('networkctl up eth0')
    os.system('ip link show dev eth0')