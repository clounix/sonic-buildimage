#ifndef _XCVR_INTERFACE_H_
#define _XCVR_INTERFACE_H_


#include "device_driver_common.h"
#include "clx_driver.h"

//interface for xcvr
struct xcvr_fn_if {
    int (*get_eth_number)(void * driver);
    ssize_t (*get_transceiver_power_on_status)(void * driver, char *buf, size_t count);
    int (*set_transceiver_power_on_status)(void * driver, int status);
    ssize_t (*get_transceiver_presence_status)(void * driver, char *buf, size_t count);
    ssize_t (*get_eth_power_on_status)(void * driver, unsigned int eth_index, char *buf, size_t count);
    int (*set_eth_power_on_status)(void * driver, unsigned int eth_index, int status);
    ssize_t (*get_eth_tx_fault_status)(void * driver, unsigned int eth_index, char *buf, size_t count);
    ssize_t (*get_eth_tx_disable_status)(void * driver, unsigned int eth_index, char *buf, size_t count);
    int (*set_eth_tx_disable_status)(void * driver, unsigned int eth_index, int status);
    ssize_t (*get_eth_present_status)(void * driver, unsigned int eth_index, char *buf, size_t count);
    ssize_t (*get_eth_rx_los_status)(void * driver, unsigned int eth_index, char *buf, size_t count);
    ssize_t (*get_eth_reset_status)(void * driver, unsigned int eth_index, char *buf, size_t count);
    int (*set_eth_reset_status)(void * driver, unsigned int eth_index, int status);
    ssize_t (*get_eth_low_power_mode_status)(void * driver, unsigned int eth_index, char *buf, size_t count);
    ssize_t (*get_eth_interrupt_status)(void * driver, unsigned int eth_index, char *buf, size_t count);
    int (*get_eth_eeprom_size)(void * driver, unsigned int eth_index);
    ssize_t (*read_eth_eeprom_data)(void * driver, unsigned int eth_index, char *buf, loff_t offset, size_t count);
    ssize_t (*write_eth_eeprom_data)(void * driver, unsigned int eth_index, char *buf, loff_t offset, size_t count);
};

#define XCVR_DEV_VALID(dev) \
    if (dev == NULL) \
        return (-1);

//xcvr ERROR CODE
#define ERR_XCVR_INIT_FAIL ((ERR_MODULLE_XCVR << 16) | 0x1)
#define ERR_XCVR_REG_FAIL ((ERR_MODULLE_XCVR << 16) | 0x2)

extern int g_dev_loglevel[];
#define XCVR_LOG_INFO(_prefix, fmt, args...) do { \
    dev_log(_prefix, g_dev_loglevel[CLX_DRIVER_TYPES_XCVR], INFO, fmt, ##args); \
} while (0)

#define XCVR_LOG_ERR(_prefix, fmt, args...) do { \
    dev_log(_prefix, g_dev_loglevel[CLX_DRIVER_TYPES_XCVR], ERR, fmt, ##args); \
} while (0)

#define XCVR_LOG_DBG(_prefix, fmt, args...) do { \
    dev_log(_prefix, g_dev_loglevel[CLX_DRIVER_TYPES_XCVR], DBG, fmt, ##args); \
} while (0)

#define XCVR_INFO(fmt, args...) XCVR_LOG_INFO("transceiver@INFO ", fmt, ##args)
#define XCVR_ERR(fmt, args...)  XCVR_LOG_ERR("transceiver@ERR ", fmt, ##args)
#define XCVR_DBG(fmt, args...)  XCVR_LOG_DBG("transceiver@DBG ", fmt, ##args)

//start with 0
#define XCVR_ETH_INDEX_MAPPING(eth_index) \
    eth_index = eth_index - 1;

struct xcvr_fn_if *get_xcvr(void);
void xcvr_if_create_driver(void);
void xcvr_if_delete_driver(void);
#endif //_XCVR_INTERFACE_H_


