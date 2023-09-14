#ifndef _BD_INFO_CLX12800_H_
#define _BD_INFO_CLX12800_H_

#include "clx_driver.h"

#define SYSEEPROM_BUS_CLX12800 12
#define SYSEEPROM_ADDR_CLX12800 0x51
#define SYSEEPROM_SIZE_CLX12800 32768

#define MUX_ADDR_CLX12800 0x70
#define MUX_CH_SEL_CLX12800 0x40

#define PORT_MAX_CLX128000 34
#define PORT_CLK_DIV_CLX128000 (0x63)

#define FAN_MAX_CLX128000 6
#define MOTOR_NUM_PER_FAN_CLX128000   1

#define FAN_BUS_CLX128000 10
#define FAN_ADDR_CLX128000 0x60

#define CLX12800_REAL_MAX_VOL_SENSOR_NUM (4)
#define CLX12800_TOTAL_VOL_SENSOR_NODE (14)
#define CLX12800_REAL_MAX_CURR_SENSOR_NUM (4)
#define CLX12800_TOTAL_CURR_SENSOR_NODE (7)

#define CLX12800_REAL_MAX_TEMP_SENSOR_NUM (8)
#define CLX12800_TOTAL_TEMP_SENSOR_NODE (11)
/*
    [0]:addr
    [1]:location in sensor_arry
    [2]:sensor offse
*/
short clx12800_curr_sensor_map[][3] = {
    {0x10, 0, -1},
    {0x20, 1, 0},
    {0x21, 2, 2},
    {0x22, 3, 4},
    {0x0, 0},
};

short clx12800_vol_sensor_map[][3] = {
    {0x10, 0, -1},
    {0x20, 1, 1},
    {0x21, 2, 5},
    {0x22, 3, 9},
    {0x0, 0},
};
/*
    [0]: range
    [1]: location in sensor_arry
*/
unsigned char clx12800_curr_index_range_map[][2] = {
    {1, 0},
    {3, 1},
    {5, 2},
    {CLX12800_TOTAL_CURR_SENSOR_NODE, 3},
};

unsigned char clx12800_vol_index_range_map[][2] = {
    {2, 0},
    {6, 1},
    {10, 2},
    {CLX12800_TOTAL_VOL_SENSOR_NODE, 3},
};

struct sensor_descript clx12800_sensor_map_index[] = {
    {"fpga-temp", 0x48, "BOARD 0x48", -1, "tmp75c"},
    {"fpga-temp", 0x49, "BOARD 0x49", 0, "tmp75c"},
    {"fpga-temp", 0x4a, "BOARD 0x4a", 1, "tmp75c"},
    {"i2c-0-mux (chan_id 4)", 0x48, "FAN 0x48", 2, "tmp75c"},
    {"i2c-0-mux (chan_id 4)", 0x49, "FAN 0x49", 3, "tmp75c"},
    {"fpga-pol", 0x20, "mp2882-rail", 4, "mp2882"},
    {"fpga-pol", 0x21, "mp2882-rail", 6, "mp2882"},
    {"fpga-pol", 0x22, "mp2882-rail", 8, "mp2882"},
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
#endif //_BD_INFO_CLX12800_H_

