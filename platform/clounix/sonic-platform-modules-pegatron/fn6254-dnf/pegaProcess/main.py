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

# Platform initialize
KERNEL_MODULE = ['i2c_dev', 'i2c-mux-pca954x force_deselect_on_exit=1', 'at24', 'pegatron_fn_6254_dn_f_cpld', 'pegatron_hwmon_mcu', 'pegatron_fn_6254_dn_f_psu', 'pegatron_fn_6254_dn_f_sfp', 'pegatron_fn_6254_dn_f_ixgbe']
MODULE_ID = ['pca9544', 'pca9544', 'fn_6254_dn_f_psu', 'fn_6254_dn_f_psu', '24c02', 'pega_hwmon_mcu', 'fn_6254_dn_f_cpld', 'fn_6254_dn_f_cpld', 'fn_6254_dn_f_cpld', 'fn_6254_dn_f_sfpA', 'fn_6254_dn_f_sfpB', 'fn_6254_dn_f_sfpC']
I2C_CHECK_NODE = ['i2c-0', 'i2c-1']
UNINSTALL_CHECK_NODE = ['-0072', '-0073']
DEVICE_NODE= ['i2c-2', 'i2c-6', 'i2c-2', 'i2c-3', 'i2c-4', 'i2c-5', 'i2c-6', 'i2c-7', 'i2c-8', 'i2c-6', 'i2c-7', 'i2c-8']
DEVICE_ADDRESS = ['0x72', '0x73', '0x58', '0x59', '0x54', '0x70', '0x74', '0x75', '0x76', '0x50', '0x50', '0x50']
def checkDevicePosition(num):
	for i in range(0, len(I2C_CHECK_NODE)):
		status, output = common.doBash("echo " + MODULE_ID[num] + " " + DEVICE_ADDRESS[num] + " > " + common.I2C_PREFIX + I2C_CHECK_NODE[i] + "/new_device")
		status, output = common.doBash("ls " + common.I2C_PREFIX + DEVICE_NODE[num])
		DEVICE_NODE[num] = I2C_CHECK_NODE[i]

		if status:
			status, output = common.doBash("echo " + DEVICE_ADDRESS[num] + " > " + common.I2C_PREFIX + I2C_CHECK_NODE[i] + "/delete_device")
		else:
			return
	return

def installDevice():
	for i in range(0, len(MODULE_ID)):
		if MODULE_ID[i] == "pca9544":
			checkDevicePosition(i)
		else:
			status, output = common.doBash("echo " + MODULE_ID[i] + " " + DEVICE_ADDRESS[i] + " > " + common.I2C_PREFIX + DEVICE_NODE[i] + "/new_device")
	return

def checkDriver():
	for i in range(0, len(KERNEL_MODULE)):
		status, output = common.doBash("lsmod | grep " + KERNEL_MODULE[i])
		if status:
			status, output = common.doBash("modprobe " + KERNEL_MODULE[i])
	return

def doInstall():
	status, output = common.doBash("depmod -a")
	checkDriver()
	installDevice()
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

# Platform uninitialize
def doUninstall():
	for i in range(0, len(KERNEL_MODULE)):
		status, output = common.doBash("modprobe -rq " + KERNEL_MODULE[i])
	for i in range(0, len(MODULE_ID)):
		if MODULE_ID[i] == "pca9544":
			for node in range(0, len(I2C_CHECK_NODE)):
				status, output = common.doBash("ls " + common.I2C_PREFIX + str(node) + UNINSTALL_CHECK_NODE[i])
				if not status:
					status, output = common.doBash("echo " + DEVICE_ADDRESS[i] + " > " + common.I2C_PREFIX + I2C_CHECK_NODE[node] + "/delete_device")
		else:
			status, output = common.doBash("echo " + DEVICE_ADDRESS[i] + " > " + common.I2C_PREFIX + DEVICE_NODE[i] + "/delete_device")
	return

def main():
	args = common.sys.argv[1:]

	if len(args[0:]) < 1:
		common.sys.exit(0)

	if args[0] == 'install':
		common.RUN = True
		doInstall()
		functionInit()
		deviceInit()
		messageHandler()
	
	common.sys.exit(0)

if __name__ == "__main__":
	main()
