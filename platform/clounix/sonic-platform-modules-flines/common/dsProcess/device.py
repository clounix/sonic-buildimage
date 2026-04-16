#!/usr/bin/env python3
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

import common


def deviceInit():

    # disalbe eeprom write-protect
    path = "/sys/s3ip/fan/eepromwp"
    common.writeFile(path, "1")

    SFP_PATH = "/sys/s3ip/transceiver"
    sfp_num = int(common.readFile(SFP_PATH + "/number"))

    # Set SFP& QSFP reset to normal
    for x in range(sfp_num):
        path = SFP_PATH + '/eth' + str(x + 1) + '/reset'
        result = common.writeFile(path, "0")

    # Set SFP && QSFP  high power mode  according to the present signal
    for x in range(sfp_num):
        path = SFP_PATH + '/eth' + str(x + 1) + '/low_power_mode'
        result = common.writeFile(path, "0")

    for x in range(sfp_num):
        path = SFP_PATH + '/eth' + str(x + 1) + '/tx_disable'
        result = common.writeFile(path, "0")

    return


if __name__ == '__main__':
    print('standalone init')
    deviceInit()
