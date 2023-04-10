#ifndef _BD_INFO_DS410_H_
#define _BD_INFO_DS410_H_

#include "clx_driver.h"

#define SYSEEPROM_BUS_DS410 0
#define SYSEEPROM_ADDR_DS410 0x50
#define SYSEEPROM_SIZE_DS410 256

#define MUX_ADDR_DS410 0x70
#define MUX_CH_SEL_DS410 0x40

#define PORT_MAX_DS410 56
#define PORT_CLK_DIV_DS410 (0x19)

#define FAN_MAX_DS410 5
#define FAN_MAX_SPEED_DS410 20000
#define MOTOR_NUM_PER_FAN_DS410 1

#define FAN_BUS_DS410 6
#define FAN_ADDR_DS410 0x60

#define CLX_DS410_REBOOT_EEPROM_BUS 10
#define CLX_DS410_REBOOT_EEPROM_ADDR 0x50

#define DS410_VOL_TOTAL_SENSOR_NUM (2)
#define DS410_CURR_TOTAL_SENSOR_NUM (2)

/*
    [0]:addr
    [1]:location in sensor_arry
    [2]:sensor offse
*/
short ds410_vol_sensor_map[3][3] = {
    {0x2B, 0, -3},
    {0x27, 1, -3},
    {0x0, 0},
};
/*
    [0]: range
    [1]: location in sensor_arry
*/
unsigned char ds410_vol_index_range_map[3][2] = {
    {1, 0},
    {2, 1},
    {0, 0},
};
/*
    [0]:addr
    [1]:location in sensor_arry
    [2]:sensor offse
*/
short ds410_curr_sensor_map[3][3] = {
    {0x2B, 0, -1},
    {0x27, 1, -1},
    {0x0, 0},
};
/*
    [0]: range
    [1]: location in sensor_arry
*/
unsigned char ds410_curr_index_range_map[][2] = {
    {1, 0},
    {2, 1},
    {0, 0},
};
struct sensor_descript ds410_sensor_map_index[] = {
    {"fpga-tmp", 0x48, "BOARD 0x48"},
    {"fpga-psu0", 0x49, "BOARD 0x49"},
    {"fpga-psu1", 0x4a, "BOARD 0x4a"},
    {"fpga-fan", 0x4b, "FAN 0x4b"},
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

#endif //_BD_INFO_DS410_H_
