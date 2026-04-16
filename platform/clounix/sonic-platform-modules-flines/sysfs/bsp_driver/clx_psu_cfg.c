#include <linux/i2c.h>
#include "pmbus.h"

struct pmbus_driver_info clx_psu_list[2] = {
    [0] = {
        .pages = 1,
        .format = {0},
        .m = {0},
        .b = {0},
        .R = {0},
        .func[0] = PMBUS_HAVE_VIN | PMBUS_HAVE_VOUT | PMBUS_HAVE_IIN | PMBUS_HAVE_IOUT | PMBUS_HAVE_PIN |
                        PMBUS_HAVE_POUT | PMBUS_HAVE_FAN12 | PMBUS_HAVE_TEMP | PMBUS_HAVE_TEMP2 | PMBUS_HAVE_TEMP3 |
                        PMBUS_HAVE_STATUS_VOUT | PMBUS_HAVE_STATUS_IOUT | PMBUS_HAVE_STATUS_INPUT | PMBUS_HAVE_STATUS_TEMP | PMBUS_HAVE_STATUS_FAN12,
    },
    [1] = {
        .pages = 1,
        .format = {0},
        .m = {0},
        .b = {0},
        .R = {0},
        .func[0] = PMBUS_HAVE_VIN | PMBUS_HAVE_VOUT | PMBUS_HAVE_IIN | PMBUS_HAVE_IOUT | PMBUS_HAVE_PIN |
                        PMBUS_HAVE_POUT | PMBUS_HAVE_FAN12 | PMBUS_HAVE_TEMP | PMBUS_HAVE_TEMP2 | PMBUS_HAVE_TEMP3 |
                        PMBUS_HAVE_STATUS_VOUT | PMBUS_HAVE_STATUS_IOUT | PMBUS_HAVE_STATUS_INPUT | PMBUS_HAVE_STATUS_TEMP | PMBUS_HAVE_STATUS_FAN12,
    },
};
