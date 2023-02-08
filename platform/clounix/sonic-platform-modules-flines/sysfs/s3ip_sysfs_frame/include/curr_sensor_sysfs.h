#ifndef _CURR_SENSOR_SYSFS_H_
#define _CURR_SENSOR_SYSFS_H_

struct s3ip_sysfs_curr_sensor_drivers_s {
    int (*get_main_board_curr_number)(void);
    //share with temp sensors
    ssize_t (*get_loglevel)(char *buf, size_t count);
    ssize_t (*set_loglevel)(char *buf, size_t count);
    ssize_t (*get_debug)(char *buf, size_t count);
    ssize_t (*set_debug)(char *buf, size_t count);
    ssize_t (*get_main_board_curr_alias)(unsigned int curr_index, char *buf, size_t count);
    ssize_t (*get_main_board_curr_type)(unsigned int curr_index, char *buf, size_t count);
    ssize_t (*get_main_board_curr_max)(unsigned int curr_index, char *buf, size_t count);
    int (*set_main_board_curr_max)(unsigned int curr_index, const char *buf, size_t count);
    ssize_t (*get_main_board_curr_min)(unsigned int curr_index, char *buf, size_t count);
    int (*set_main_board_curr_min)(unsigned int curr_index, const char *buf, size_t count);
    ssize_t (*get_main_board_curr_crit)(unsigned int curr_index, char *buf, size_t count);
    int (*set_main_board_curr_crit)(unsigned int curr_index, const char *buf, size_t count);
    ssize_t (*get_main_board_curr_average)(unsigned int curr_index, char *buf, size_t count);
    ssize_t (*get_main_board_curr_value)(unsigned int curr_index, char *buf, size_t count);
};

extern int s3ip_sysfs_curr_sensor_drivers_register(struct s3ip_sysfs_curr_sensor_drivers_s *drv);
extern void s3ip_sysfs_curr_sensor_drivers_unregister(void);
#endif /*_CURR_SENSOR_SYSFS_H_ */
