#ifndef _FAN_SYSFS_H_
#define _FAN_SYSFS_H_

struct s3ip_sysfs_fan_drivers_s {
    int (*get_fan_number)(void);
    ssize_t (*get_loglevel)(char *buf, size_t count);
    ssize_t (*set_loglevel)(const char *buf, size_t count);
    ssize_t (*get_debug)(char *buf, size_t count);
    ssize_t (*set_debug)(const char *buf, size_t count);
    ssize_t (*get_fan_eeprom_wp)(char *buf, size_t count);
    int (*set_fan_eeprom_wp)(unsigned int enable);
    int (*get_fan_motor_number)(unsigned int fan_index);
    ssize_t (*get_fan_vendor_name)(unsigned int fan_index, char *buf, size_t count);
    ssize_t (*get_fan_model_name)(unsigned int fan_index, char *buf, size_t count);
    ssize_t (*get_fan_serial_number)(unsigned int fan_index, char *buf, size_t count);
    ssize_t (*get_fan_part_number)(unsigned int fan_index, char *buf, size_t count);
    ssize_t (*get_fan_hardware_version)(unsigned int fan_index, char *buf, size_t count);
    ssize_t (*get_fan_status)(unsigned int fan_index, char *buf, size_t count);
    ssize_t (*get_fan_led_status)(unsigned int fan_index, char *buf, size_t count);
    int (*set_fan_led_status)(unsigned int fan_index, int status);
    ssize_t (*get_fan_vmon)(unsigned int fan_index, char *buf, size_t count);
    ssize_t (*get_fan_direction)(unsigned int fan_index, char *buf, size_t count);
    ssize_t (*get_fan_motor_speed)(unsigned int fan_index, unsigned int motor_index, char *buf, size_t count);
    ssize_t (*get_fan_motor_speed_tolerance)(unsigned int fan_index, unsigned int motor_index, char *buf, size_t count);
    ssize_t (*get_fan_motor_speed_target)(unsigned int fan_index, unsigned int motor_index, char *buf, size_t count);
    ssize_t (*get_fan_motor_speed_max)(unsigned int fan_index, unsigned int motor_index, char *buf, size_t count);
    ssize_t (*get_fan_motor_speed_min)(unsigned int fan_index, unsigned int motor_index, char *buf, size_t count);
    ssize_t (*get_fan_motor_ratio)(unsigned int fan_index, unsigned int motor_index, char *buf, size_t count);
    int (*set_fan_motor_ratio)(unsigned int fan_index, unsigned int motor_index, int ratio);
    int (*get_fan_eeprom_size)(unsigned int fan_index);
    ssize_t (*read_fan_eeprom_data)(unsigned int fan_index, char *buf, loff_t offset, size_t count);
    ssize_t (*write_fan_eeprom_data)(unsigned int fan_index, char *buf, loff_t offset, size_t count);

};

extern int s3ip_sysfs_fan_drivers_register(struct s3ip_sysfs_fan_drivers_s *drv);
extern void s3ip_sysfs_fan_drivers_unregister(void);
#endif /*_FAN_SYSFS_H_ */
