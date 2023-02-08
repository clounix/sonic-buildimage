#ifndef _REBOOT_EEPROM_SYSFS_H_
#define _REBOOT_EEPROM_SYSFS_H_

struct s3ip_sysfs_reboot_eeprom_drivers_s
{
    ssize_t (*get_loglevel)(char *buf, size_t count);
    ssize_t (*set_loglevel)(char *buf, size_t count);
    ssize_t (*get_debug)(char *buf, size_t count);
    ssize_t (*set_debug)(char *buf, size_t count);
    int (*get_reboot_eeprom_size)(void);
    ssize_t (*read_reboot_eeprom_data)(char *buf, loff_t offset, size_t count);
    ssize_t (*write_reboot_eeprom_data)(char *buf, loff_t offset, size_t count);
};

extern int s3ip_sysfs_reboot_eeprom_drivers_register(struct s3ip_sysfs_reboot_eeprom_drivers_s *drv);
extern void s3ip_sysfs_reboot_eeprom_drivers_unregister(void);
#endif /*_REBOOT_EEPROM_SYSFS_H_ */
