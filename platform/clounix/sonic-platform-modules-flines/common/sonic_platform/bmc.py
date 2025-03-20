from datetime import datetime
import time
import subprocess
import os

_BMC_FLAG = "/tmp/flines-bmc-exists"
_THERMAL_ZONE0 = "/sys/class/thermal/thermal_zone0/temp"


def ipmi_shell(str):
    if not os.path.exists(_THERMAL_ZONE0):
        raise UserWarning("FLKS: BMC device is not exists.")
    proc = subprocess.Popen(
        str, stdout=subprocess.PIPE, stderr=subprocess.PIPE, shell=True
    )
    _, zerr = proc.communicate()
    if proc.returncode != 0:
        raise UserWarning("[%d]{%s}: %s" % (proc.returncode, str, zerr.decode()))
    pass


def update_flag(znow):
    with open(_BMC_FLAG, "w") as f:
        f.write(znow)
        f.flush()
        f.close()
        os.utime(_BMC_FLAG, None)


# Returns true represent that IPMI command executed failed.
def test_cpu_temperature(znow):
    try:
        itemp = 0
        with open(_THERMAL_ZONE0, "r") as f:
            itemp = int(f.read().strip())
        ipmi_shell("ipmitool -I open raw 0x2e 0x11 %d" % (itemp / 1000))
        update_flag(znow)
        print("%s FLKS: The BMC synchronized the temperature through IPMI." % znow)
        return True
    except UserWarning as w:
        if os.path.exists(_BMC_FLAG):
            os.remove(_BMC_FLAG)
        print(znow + " FLKS: " + str(w))
        return False
    finally:
        pass
    pass


def test_time(znow):
    try:
        ipmi_shell("ipmitool -I open sel time set now")
        ipmi_shell("ipmitool -I open raw 0x2e 0x09 0x01")
        update_flag(znow)
        print("%s FLKS: The BMC synchronized the time through IPMI." % znow)
        return True
    except UserWarning as w:
        if os.path.exists(_BMC_FLAG):
            os.remove(_BMC_FLAG)
        print(znow + " FLKS: " + str(w))
        return False
    finally:
        pass
    pass


def test_heart_beat(znow):
    try:
        ipmi_shell("ipmitool -I open raw 0x2e 0x09 0x01")
        print("%s FLKS: The BMC heart beat to SONiC through IPMI." % znow)
        return True
    except UserWarning as w:
        print(znow + " FLKS: " + str(w))
        return False
    finally:
        pass
    pass

if __name__ == "__main__":
    test_count = 0
    worked = False
    while True:
        worked = False
        zdt = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        if not os.path.exists(_BMC_FLAG):
            if test_cpu_temperature(zdt):
                worked = True
            if test_time(zdt):
                worked = True
        else:
            if test_count % 5 == 0:
                if test_cpu_temperature(zdt):
                    worked = True
                pass
            if test_count % 3600 == 0:
                if test_time(zdt):
                    worked = True
                pass
        
        test_heart_beat(zdt)
        test_count += 1
        if worked:
            print(zdt + " worked.")
        else:
            print(zdt + " Idling...")
        time.sleep(1)
