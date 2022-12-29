#ifndef _BD_INFO_CLX8000_H_
#define _BD_INFO_CLX8000_H_

#include "clx_driver.h"

#define SYSEEPROM_BUS_CLX8000 13
#define SYSEEPROM_ADDR_CLX8000 0x50
#define SYSEEPROM_SIZE_CLX8000 256

#define MUX_ADDR_CLX8000 0x70
#define MUX_CH_SEL_CLX8000 0x40

#define PORT_MAX_CLX8000 56
#define PORT_CLK_DIV_CLX8000 (0xc8)

#define FAN_MAX_CLX8000 5
#define MOTOR_NUM_PER_FAN_CLX8000   1

#define FAN_BUS_CLX8000 6
#define FAN_ADDR_CLX8000 0x60

struct sensor_descript clx8000_sensor_map_index[] = {
    {"fpga-tmp", 0x48, "BOARD 0x48"},
    {"fpga-tmp", 0x49, "BOARD 0x49"},
    {"fpga-tmp", 0x4a, "BOARD 0x4a"},
    {"fpga-fan", 0x48, "FAN 0x48"},
    {"fpga-fan", 0x49, "FAN 0x49"},
    {"cpu 0", 0, "cpu 0"},
    {"cpu 1", 0, "cpu 1"},
    {"cpu 2", 0, "cpu 2"},
    {"cpu 3", 0, "cpu 3"},
    {"cpu 4", 0, "cpu 4"},
    {"cpu 5", 0, "cpu 5"},
    {"cpu 6", 0, "cpu 6"},
    {"cpu 7", 0, "cpu 7"},
    {"cpu 8", 0, "cpu 8"},
    {"cpu 9", 0, "cpu 9"},
    {"cpu 10", 0, "cpu 10"},
    {"cpu 11", 0, "cpu 11"},
    {"cpu 12", 0, "cpu 12"},
    {"cpu 13", 0, "cpu 13"},
    {"cpu 14", 0, "cpu 14"},
    {"cpu 15", 0, "cpu 15"},
    {"cpu 16", 0, "cpu 16"},
    {"cpu 17", 0, "cpu 17"},
    {"cpu 18", 0, "cpu 18"},
    {"cpu 19", 0, "cpu 19"},
    {"cpu 20", 0, "cpu 20"},
    {"cpu 21", 0, "cpu 21"},
    {"cpu 22", 0, "cpu 22"},
    {"cpu 23", 0, "cpu 23"},
    {"cpu 24", 0, "cpu 24"},
    {"cpu 25", 0, "cpu 25"},
    {"cpu 26", 0, "cpu 26"},
    {"cpu 27", 0, "cpu 27"},
    {"cpu 28", 0, "cpu 28"},
    {"cpu 29", 0, "cpu 29"},
    {"cpu 30", 0, "cpu 30"},
    {"cpu 31", 0, "cpu 31"},
    {NULL, 0, 0},
};

#endif //_BD_INFO_CLX8000_H_

