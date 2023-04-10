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

#define CLX8000_REAL_MAX_VOL_SENSOR_NUM (4)
#define CLX8000_TOTAL_VOL_SENSOR_NODE (12)
#define CLX8000_REAL_MAX_CURR_SENSOR_NUM (4)
#define CLX8000_TOTAL_CURR_SENSOR_NODE (6)

#define CLX8000_REAL_MAX_TEMP_SENSOR_NUM (7)
#define CLX8000_TOTAL_TEMP_SENSOR_NODE (9)
/*
    [0]:addr
    [1]:location in sensor_arry
    [2]:sensor offse
*/
short clx8000_curr_sensor_map[][3] = {
    {0x10, 0, -1},
    {0x20, 1, 0},
    {0x21, 2, 2},
    {0x29, 3, 4},
    {0x0, 0},
};

short clx8000_vol_sensor_map[][3] = {
    {0x10, 0, -1},
    {0x20, 1, 1},
    {0x21, 2, 5},
    {0x29, 3, 9},
    {0x0, 0},
};
/*
    [0]: range
    [1]: location in sensor_arry
*/
unsigned char clx8000_curr_index_range_map[][2] = {
    {1, 0},
    {3, 1},
    {5, 2},
    {CLX8000_TOTAL_CURR_SENSOR_NODE, 3},
};

unsigned char clx8000_vol_index_range_map[][2] = {
    {2, 0},
    {6, 1},
    {10, 2},
    {CLX8000_TOTAL_VOL_SENSOR_NODE, 3},
};

struct sensor_descript clx8000_sensor_map_index[] = {
    {"fpga-tmp", 0x48, "BOARD 0x48", -1, "tmp75c"},
    {"fpga-tmp", 0x49, "BOARD 0x49", 0, "tmp75c"},
    {"fpga-tmp", 0x4a, "BOARD 0x4a", 1, "tmp75c"},
    {"fpga-fan", 0x48, "FAN 0x48", 2, "tmp75c"},
    {"fpga-fan", 0x49, "FAN 0x49", 3, "tmp75c"},
    {"fpga-pmbus", 0x20, "mp2882-rail", 4, "mp2882"},
    {"fpga-pmbus", 0x21, "mp2882-reil", 6, "mp2882"},
    {"cpu 0", 0, "cpu 0", 0},
    {"cpu 1", 0, "cpu 1", 0},
    {"cpu 2", 0, "cpu 2", 0},
    {"cpu 3", 0, "cpu 3", 0},
    {"cpu 4", 0, "cpu 4", 0},
    {"cpu 5", 0, "cpu 5", 0},
    {"cpu 6", 0, "cpu 6", 0},
    {"cpu 7", 0, "cpu 7", 0},
    {"cpu 8", 0, "cpu 8", 0},
    {"cpu 9", 0, "cpu 9", 0},
    {"cpu 10", 0, "cpu 10", 0},
    {"cpu 11", 0, "cpu 11", 0},
    {"cpu 12", 0, "cpu 12", 0},
    {"cpu 13", 0, "cpu 13", 0},
    {"cpu 14", 0, "cpu 14", 0},
    {"cpu 15", 0, "cpu 15", 0},
    {"cpu 16", 0, "cpu 16", 0},
    {"cpu 17", 0, "cpu 17", 0},
    {"cpu 18", 0, "cpu 18", 0},
    {"cpu 19", 0, "cpu 19", 0},
    {"cpu 20", 0, "cpu 20", 0},
    {"cpu 21", 0, "cpu 21", 0},
    {"cpu 22", 0, "cpu 22", 0},
    {"cpu 23", 0, "cpu 23", 0},
    {"cpu 24", 0, "cpu 24", 0},
    {"cpu 25", 0, "cpu 25", 0},
    {"cpu 26", 0, "cpu 26", 0},
    {"cpu 27", 0, "cpu 27", 0},
    {"cpu 28", 0, "cpu 28", 0},
    {"cpu 29", 0, "cpu 29", 0},
    {"cpu 30", 0, "cpu 30", 0},
    {"cpu 31", 0, "cpu 31", 0},
    {NULL, 0, 0, 0},
};

#endif //_BD_INFO_CLX8000_H_

