#!/usr/bin/env python
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

import common, device

HOST = ''
SOCKET_LIST = []
SOCKET_MAX_CLIENT = 10
QUEUES = []
THREADS = []
FUNCS = {}

class GlobalThread(common.threading.Thread):
	def __init__(self,threadname, q):
		common.threading.Thread.__init__(self,name = threadname)
		self.queue = q

	def run(self):
		while common.RUN:
			message = self.queue.get()
			self.onMessage(message)
	def onMessage(self, message):
		"""
		Commands:
			uninstall	: Uninstall platform drivers
		"""
		if len(message.command) < 1:
			result = self.onMessage.__doc__
		else:
			if message.command[0] == 'uninstall':
				common.RUN = False
				doUninstall()
				result = 'Success'
			else:
				result = self.onMessage.__doc__
		if (message.callback is not None):
			message.callback(result)

class messageObject(object):
	def __init__(self, command, callback):
		super(messageObject, self).__init__()
		self.command = command
		self.callback = callback

def callback(sock, result):
	sock.sendall(result)

def messageHandler():
	server_socket = common.socket.socket(common.socket.AF_INET, common.socket.SOCK_STREAM)
	server_socket.setsockopt(common.socket.SOL_SOCKET, common.socket.SO_REUSEADDR, 1)
	server_socket.bind((HOST, common.SOCKET_PORT))
	server_socket.listen(SOCKET_MAX_CLIENT)
	SOCKET_LIST.append(server_socket)

	while(common.RUN):
		ready_to_read,ready_to_write,in_error = common.select.select(SOCKET_LIST,[],[],0)
		for sock in ready_to_read:
			if sock == server_socket:
				sockfd, addr = server_socket.accept()
				SOCKET_LIST.append(sockfd)
			else:
				try:
					data = sock.recv(common.SOCKET_RECV_BUFFER)
					if data:
						cb = common.partial(callback, sock)
						cmdlist = data.split()

						if cmdlist[0] not in common.CMD_TYPE:
							callback(sock, 'Fail')
							continue
						
						msg = messageObject(cmdlist[1:], cb)
						FUNCS[cmdlist[0]].put(msg)
						continue
					else:
						if sock in SOCKET_LIST:
							SOCKET_LIST.remove(sock)
				except:
					raise
					continue
		common.time.sleep(0.2)

	server_socket.close()

KERNEL_MODULE = [
'i2c_dev',
'i2c-mux', 
'i2c-mux-pca9541', 
'i2c-mux-pca954x force_deselect_on_exit=1', 
'at24', 
'lm75', 
'pegatron_fn8032_bnf_cpld',
'pegatron_hwmon_mcu', 
'pegatron_fn8032_bnf_psu', 
'pegatron_fn8032_bnf_sfp', 
'i2c-mux-pca9641']
#'pegatron_fn8032_bnf_ixgbe']

def checkDriver():
    for i in range(0, len(KERNEL_MODULE)):
        status, output = common.doBash("lsmod | grep " + KERNEL_MODULE[i])
        if status:
            status, output = common.doBash("modprobe " + KERNEL_MODULE[i])
    return

i2c_topology_dict=[
    {'bus': "i2c-0", 'driver' : "pca9641", 'address': "0x71"},
    {'bus': "i2c-1", 'driver' : "fn8032_bnf_cpld", 'address': "0x18"},
    {'bus': "i2c-1", 'driver' : "pca9544", 'address': "0x72"},
    {'bus': "i2c-1", 'driver' : "pca9548", 'address': "0x73"},
    {'bus': "i2c-1", 'driver' : "lm75",    'address': "0x4a"},
    {'bus': "i2c-2", 'driver' : "fn8032_bnf_psu", 'address': "0x58"},
    {'bus': "i2c-3", 'driver' : "fn8032_bnf_psu", 'address': "0x59"},
    {'bus': "i2c-4", 'driver' : "pega_hwmon_mcu", 'address': "0x70"},
    {'bus': "i2c-6", 'driver' : "fn8032_bnf_cpld", 'address': "0x74"},
    {'bus': "i2c-7", 'driver' : "fn8032_bnf_cpld", 'address': "0x75"},
    {'bus': "i2c-7", 'driver' : "fn8032_bnf_sfpB", 'address': "0x50"},
    {'bus': "i2c-8", 'driver' : "fn8032_bnf_cpld", 'address': "0x76"},
    {'bus': "i2c-8", 'driver' : "fn8032_bnf_sfpC", 'address': "0x50"},
    {'bus': "i2c-9", 'driver' : "24c02", 'address': "0x54"}
]

def doInstall():
    status, output = common.doBash("depmod -a")
    checkDriver()
    for o in i2c_topology_dict:
        status, output = common.doBash("echo " + o['driver'] + " " + o['address'] + " > " + common.I2C_PREFIX + o['bus'] + "/new_device")
    return

def setupThreads():
	global THREADS, QUEUES

	# Queues
	# Global
	queueGlobal = common.Queue.Queue()
	QUEUES.append(queueGlobal)
	FUNCS['global'] = queueGlobal

	# Device
	queueDevice = common.Queue.Queue()
	QUEUES.append(queueDevice)
	FUNCS['device'] = queueDevice

	# Threads
	# Global
	threadGlobal = GlobalThread('Global Handler', queueGlobal)
	THREADS.append(threadGlobal)	

	# Device
	threadDevice = device.DeviceThread('Device Handler', queueDevice)
	THREADS.append(threadDevice)

	# Check platform status
	threadPlatformStatus = device.PlatformStatusThread('Platform Status Handler', 0.3)
	THREADS.append(threadPlatformStatus)
def functionInit():
	setupThreads()
	for thread in THREADS:
                thread.start()
	return

def deviceInit():
	msg = messageObject(['init'], None)
	FUNCS['device'].put(msg)
	return

def do_platformApiInstall():
    print "Platform API Install...."
    status, output = common.doBash("/usr/local/bin/platform_api_mgnt.sh install")
    return

# Platform uninitialize
def doUninstall():
    for i in range(0, len(KERNEL_MODULE)):
        status, output = common.doBash("modprobe -rq " + KERNEL_MODULE[i])
    for o in i2c_topology_dict:
        status, output = common.doBash("echo " + o['address'] + " > " + common.I2C_PREFIX + o['bus'] + "/delete_device")
    return

def main(): #start 20200219
	args = common.sys.argv[1:]

	if len(args[0:]) < 1:
		common.sys.exit(0)

	if args[0] == 'install':
		common.RUN = True
		doInstall()
		do_platformApiInstall()
		functionInit()
		deviceInit()
		messageHandler()
	
	common.sys.exit(0)

if __name__ == "__main__":
	main()
