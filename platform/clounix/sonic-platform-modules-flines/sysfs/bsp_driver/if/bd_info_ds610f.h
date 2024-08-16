#ifndef _BD_INFO_DS610F_H_
#define _BD_INFO_DS610F_H_

#include "clx_driver.h"

#define SYSEEPROM_BUS_DS610F 0
#define SYSEEPROM_ADDR_DS610F 0x50
#define SYSEEPROM_SIZE_DS610F 256

#define MUX_ADDR_DS610F 0x70
#define MUX_CH_SEL_DS610F 0x40

#define PORT_MAX_DS610F 56
#define PORT_PLATFORM_DS610F 610
#define PORT_CLK_DIV_DS610F (0x19)

#define FAN_MAX_DS610F 6
#define FAN_MAX_SPEED_DS610F 27800
#define MOTOR_NUM_PER_FAN_DS610F 1

#define FAN_BUS_DS610F 10
#define FAN_ADDR_DS610F 0x60

#define CLX_DS610F_REBOOT_EEPROM_BUS 14
#define CLX_DS610F_REBOOT_EEPROM_ADDR 0x50

#define DS610F_VOL_TOTAL_SENSOR_NUM (8)
#define DS610F_CURR_TOTAL_SENSOR_NUM (4)

/*
    [0]:addr
    [1]:location in sensor_arry
    [2]:sensor offset
    [3]:scaling factor
*/
short ds610f_vol_sensor_map[SENSOR_INDEX_MAX][SENSOR_COL_MAX] = {
    {0x20, 0, -1, 1},
    {0x21, 1, 3, 1},
    {0x0, 0, 0, 0},
    {0x0, 0, 0, 0},
};
/*
    [0]: range
    [1]: location in sensor_arry
*/
unsigned char ds610f_vol_index_range_map[SENSOR_ROW_MAX][SENSOR_RANGE_MAX] = {
    {4, 0},
    {8, 1},
    {0, 0},
    {0, 0},
};

/*
    [0]:addr
    [1]:location in sensor_arry
    [2]:sensor offse
    [3]:scaling factor
*/
short ds610f_curr_sensor_map[SENSOR_INDEX_MAX][SENSOR_COL_MAX] = {
    {0x20, 0, -1, 1},
    {0x21, 1, 1, 1},
    {0x0, 0, 0, 0},
    {0x0, 0, 0, 0},
};
/*
    [0]: range
    [1]: location in sensor_arry
*/
unsigned char ds610f_curr_index_range_map[SENSOR_ROW_MAX][SENSOR_RANGE_MAX] = {
    {2, 0},
    {4, 1},
    {0, 0},
    {0, 0},
};

struct sensor_descript ds610f_sensor_map_index[] = {
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

#endif
