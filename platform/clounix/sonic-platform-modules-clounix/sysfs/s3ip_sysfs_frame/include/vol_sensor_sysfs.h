#ifndef _VOL_SENSOR_SYSFS_H_
#define _VOL_SENSOR_SYSFS_H_

struct s3ip_sysfs_vol_sensor_drivers_s {
    int (*get_main_board_vol_number)(void);
    //share with temp sensors    
    ssize_t (*get_loglevel)(char *buf, size_t count);
    ssize_t (*set_loglevel)(char *buf, size_t count);
    ssize_t (*get_debug)(char *buf, size_t count);
    ssize_t (*set_debug)(char *buf, size_t count);
    ssize_t (*get_main_board_vol_alias)(unsigned int vol_index, char *buf, size_t count);
    ssize_t (*get_main_board_vol_type)(unsigned int vol_index, char *buf, size_t count);
    ssize_t (*get_main_board_vol_max)(unsigned int vol_index, char *buf, size_t count);
    int (*set_main_board_vol_max)(unsigned int vol_index, const char *buf, size_t count);
    ssize_t (*get_main_board_vol_min)(unsigned int vol_index, char *buf, size_t count);
    int (*set_main_board_vol_min)(unsigned int vol_index, const char *buf, size_t count);
    ssize_t (*get_main_board_vol_crit)(unsigned int vol_index, char *buf, size_t count);
    int (*set_main_board_vol_crit)(unsigned int vol_index, const char *buf, size_t count);
    ssize_t (*get_main_board_vol_range)(unsigned int vol_index, char *buf, size_t count);
    ssize_t (*get_main_board_vol_nominal_value)(unsigned int vol_index, char *buf, size_t count);
    ssize_t (*get_main_board_vol_value)(unsigned int vol_index, char *buf, size_t count);
};

extern int s3ip_sysfs_vol_sensor_drivers_register(struct s3ip_sysfs_vol_sensor_drivers_s *drv);
extern void s3ip_sysfs_vol_sensor_drivers_unregister(void);
#endif /*_VOL_SENSOR_SYSFS_H_ */
