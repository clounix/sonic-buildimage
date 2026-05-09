#!/usr/bin/env python

try:
    import os
    import sys
    import syslog
    import signal
    import time
    # import glob
    from datetime import datetime
except ImportError as e:
    raise ImportError('%s - required module not found' % str(e))

###############################################################################

# # read SONiC immutable variables
# [ -f /etc/sonic/sonic-environment ] && . /etc/sonic/sonic-environment
# PLATFORM=${PLATFORM:-$(sonic-db-cli CONFIG_DB HGET 'DEVICE_METADATA|localhost' platform)}
PRODUCT_NAME  = ''
FUNCTION_NAME = 'watchdog'

DEBUG_MODE = False

###############################################################################

WDT_TIMEOUT = 90             # Timeout value, in second.
WDT_KICK_DOG_INTERVAL = 5   # Kick watchdog interval, in second.
WDT_TIMEOUT_LAST = WDT_TIMEOUT

CHECK_WDT_TIMEOUT_ERR = -1
RET_OK  = 0
RET_NOK = -1
WATCHDOG_ENABLE = 1

###############################################################################
# LOG.

# priorities (these are ordered)

LOG_EMERG     = 0       #  system is unusable
LOG_ALERT     = 1       #  action must be taken immediately
LOG_CRIT      = 2       #  critical conditions
LOG_ERR       = 3       #  error conditions
LOG_WARNING   = 4       #  warning conditions
LOG_NOTICE    = 5       #  normal but significant condition
LOG_INFO      = 6       #  informational
LOG_DEBUG     = 7       #  debug-level messages

priority_name = {
    LOG_CRIT    : "critical",
    LOG_DEBUG   : "debug",
    LOG_WARNING : "warning",
    LOG_INFO    : "info",
}


def SYS_LOG(level, msg):
    syslog.syslog(level, msg)

def DBG_LOG(msg):
    if DEBUG_MODE:
        level = syslog.LOG_DEBUG
        x = PRODUCT_NAME + ' ' + priority_name[level].upper() + ' : ' + msg
        SYS_LOG(level, x)

def SYS_LOG_INFO(msg):
    level = syslog.LOG_INFO
    x = PRODUCT_NAME + ' ' + priority_name[level].upper() + ' : ' + msg
    SYS_LOG(level, x)

def SYS_LOG_WARN(msg):
    level = syslog.LOG_WARNING
    x = PRODUCT_NAME + ' ' + priority_name[level].upper() + ' : ' + msg
    SYS_LOG(level, x)

def SYS_LOG_CRITICAL(msg):
    level = syslog.LOG_CRIT
    x = PRODUCT_NAME + ' ' + priority_name[level].upper() + ' : ' + msg
    SYS_LOG(level, x)

###############################################################################

def getstatusoutput(cmd):
    if sys.version_info.major == 2:
        # python2
        import commands
        ret, output = commands.getstatusoutput( cmd )
    else:
        # python3
        import subprocess
        ret, output = subprocess.getstatusoutput( cmd )
    DBG_LOG('CMD [{}]: return [{}], detail:{}.'.format(cmd, ret, output.splitlines()))
    return ret, output


###############################################################################

def check_debug_flag():
    path_prefix = '/run/'
    return os.path.isfile(path_prefix + "wdt-control.debug")

###############################################################################
def wdt_initially():
    cmd = 'echo {} > /sys_switch/watchdog/timeout'.format(WDT_TIMEOUT)
    ret, output = getstatusoutput(cmd)
    if ret != RET_OK:
        SYS_LOG_WARN('Failed to configure watchdog, {}'.format(output))
        return False
    SYS_LOG_INFO("Watchdog_init: {}".format(cmd))
    cmd = 'cat /sys_switch/watchdog/enable'
    ret, output = getstatusoutput(cmd)
    if ret != RET_OK or output == 'NA':
        SYS_LOG_WARN('Failed to get watchdog enable, {}'.format(output))
        return False
    SYS_LOG_INFO("Watchdog enable: {} is {}".format(cmd, output))
    if int(output) != WATCHDOG_ENABLE:
        cmd = 'echo {} > /sys_switch/watchdog/enable'.format(WATCHDOG_ENABLE)
        ret, output = getstatusoutput(cmd)
        if ret != RET_OK:
            SYS_LOG_WARN('Failed to set watchdog enable, {}'.format(output))
            return False
        SYS_LOG_INFO("Watchdog enable: {}".format(cmd))

    return True

def set_timeout(wdt_timeout):
    cmd = 'echo {} > /sys_switch/watchdog/timeout'.format(wdt_timeout)
    ret, output = getstatusoutput(cmd)
    if ret != RET_OK:
        SYS_LOG_WARN('Failed to configure watchdog, {}'.format(output))
        return False

    return True

def get_timeout():
    ret, output = getstatusoutput('cat /sys_switch/watchdog/timeout')
    if ret != 0 or not output.isdigit():
        SYS_LOG_WARN('Failed to get the watchdogutil arm time [{}]'.format(output))
        return RET_NOK
    DBG_LOG('watchdog time [{}]'.format(output))
    return int(output)

def feed_wdt():
    cmd = 'echo 1 > /sys_switch/watchdog/reset'
    ret, output = getstatusoutput(cmd)
    if ret != RET_OK:
        SYS_LOG_WARN('Failed to configure watchdog reset, {}'.format(output))
        return False

    cmd = 'cat /sys_switch/watchdog/enable'
    ret, output = getstatusoutput(cmd)
    if ret != RET_OK or output == 'NA':
        SYS_LOG_WARN('Failed to get watchdog enable, {}'.format(output))
        return False
    if int(output) != WATCHDOG_ENABLE:
        cmd = 'echo {} > /sys_switch/watchdog/enable'.format(WATCHDOG_ENABLE)
        ret, output = getstatusoutput(cmd)
        if ret != RET_OK:
            SYS_LOG_WARN('Failed to set watchdog enable, {}'.format(output))
            return False
        SYS_LOG_INFO("Re-enable the watchdog:{} Reason: The watchdog disabled by external program.".format(cmd))

    return True

def signal_handler(signal, frame):
    SYS_LOG_WARN('Terminating watchdog program due to SIGINT!')
    sys.exit(1)

def signal_handler_term(signal, frame):
    SYS_LOG_INFO('Terminating watchdog program due to SIGTERM!')
    sys.exit(0)


def main(argv):
    global DEBUG_MODE
    global WDT_TIMEOUT_LAST
    time_last = time.time()
    time_now = time_last

    SYS_LOG_INFO('Starting watchdog program...')

    # Register signal of SIGINT, e.g. "Ctrl+C".
    signal.signal(signal.SIGINT, signal_handler)
    # Register signal of SIGTERM.
    signal.signal(signal.SIGTERM, signal_handler_term)

    while not wdt_initially():
        SYS_LOG_WARN("Failed to set WDT timeout initially")
        time.sleep(1)

    # Main loop.
    while True:
        DEBUG_MODE = check_debug_flag()
        time_now = time.time()
        if DEBUG_MODE:
            DBG_LOG('Passing time since last kick: [{}]'.format(time_now - time_last))

        if time_now >= (time_last + WDT_KICK_DOG_INTERVAL + 2):
            time_now_str  = time.strftime("%Y-%m-%d %H:%M:%S", time.localtime(time_now))
            time_last_str = time.strftime("%Y-%m-%d %H:%M:%S", time.localtime(time_last))
            SYS_LOG_WARN('Pass too much time since last kick, last[{}], now[{}]'.format(time_last_str, time_now_str))
        time_last = time_now

        DBG_LOG('Setting WDT, date = [{}]'.format(datetime.now()))

        if not feed_wdt():
            SYS_LOG_WARN("Failed to feed WDT timeout")

        DBG_LOG('Setting WDT, date = [{}]'.format(datetime.now()))
        # Sleep
        time.sleep(WDT_KICK_DOG_INTERVAL)

    SYS_LOG_INFO('Exiting watchdog program...')


if __name__ == '__main__':
    main(sys.argv)


