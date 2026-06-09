#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
Host fan speed control service (migrated from thermal_control.normalization).
"""

import signal
import sys
import syslog
import time
import os
import json

try:
    import sonic_platform
    from sonic_platform.thermal_manager import ThermalManager
except ImportError as e:
    raise ImportError('%s - required module not found' % str(e))

FUNCTION_NAME = 'fan-control'
SLEEP_TIME = 10

SYSLOG_IDENTIFIER = FUNCTION_NAME


def sys_log(level, msg):
    syslog.openlog(SYSLOG_IDENTIFIER, syslog.LOG_PID, syslog.LOG_USER)
    syslog.syslog(level, msg)
    syslog.closelog()


def log_info(msg):
    sys_log(syslog.LOG_INFO, msg)


def log_warning(msg):
    sys_log(syslog.LOG_WARNING, msg)


def log_error(msg):
    sys_log(syslog.LOG_ERR, msg)


def log_critical(msg):
    sys_log(syslog.LOG_CRIT, msg)

def set_all_fans_max_speed():
    """
    Set all fans to 100% speed when fan-control.service is stopped.
    """
    try:
        chassis = sonic_platform.platform.Platform().get_chassis()
        for fan in chassis.get_all_fans():
            try:
                fan.set_speed(100)
            except Exception as e:
                log_warning('Failed to set fan %s speed to 100: %s' % (fan.get_name(), e))
    except Exception as e:
        log_warning('Failed to set all fans to max speed: %s' % e)

#wait for database config
def fan_control_init():
    db_config_path = '/var/run/redis/sonic-db/database_config.json'
    max_retries = 30
    retry_interval = 1

    for i in range(max_retries):
        if os.path.exists(db_config_path) and os.path.getsize(db_config_path) > 0:
            try:
                with open(db_config_path, 'r') as f:
                    json.load(f)
                return True
            except json.JSONDecodeError as e:
                print(f"{e}, retry ({i+1}/{max_retries})")
        time.sleep(retry_interval)

class ThermalControl(object):

    POLICY_FILE = '/usr/share/sonic/platform/thermal_policy.json'

    def __init__(self):
        try:
            self.chassis = sonic_platform.platform.Platform().get_chassis()
        except Exception as e:
            log_error("Failed to get chassis due to {}".format(repr(e)))
        self.thermal_manager = ThermalManager
        self.thermal_manager.initialize()
        self.thermal_manager.load(ThermalControl.POLICY_FILE)
        self.thermal_manager.init_thermal_algorithm(self.chassis)

    def deinit(self):
        self.thermal_manager.stop()
        try:
            if self.thermal_manager:
                self.thermal_manager.deinitialize()
        except Exception as e:
            log_error('Caught exception while destroying thermal manager - {}'.format(repr(e)))

    def execute(self):
        try:
            if self.thermal_manager:
                self.thermal_manager.run_policy(self.chassis)
        except Exception as e:
            log_critical("Critical error in thermal normalization action: %s. "
                         "Setting all fans to 100%% for safety." % e)
            set_all_fans_max_speed()

def main():
    fan_control_init()
    thermal = ThermalControl()
    def signal_handler(signum, frame):
        signum_name = signal.Signals(signum).name
        log_info('Received signal %s, set fans to 100%% and exit fan-control' % signum_name)
        thermal.deinit()
        set_all_fans_max_speed()
        sys.exit(0)

    signal.signal(signal.SIGTERM, signal_handler)
    signal.signal(signal.SIGINT, signal_handler)

    log_info('Fan control service started, interval=%ds' % SLEEP_TIME)

    while True:
        thermal.execute()
        time.sleep(SLEEP_TIME)

if __name__ == '__main__':
    main()