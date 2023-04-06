#include <linux/netlink.h>

#define NETLINK_S3IP (MAX_LINKS - 1)
#define S3IP_PORT (6666)
#define NETLINK_TO_KERNEL (0)

#define MAX_DATA_LEN (128)

enum {
    PSU_INFO,
    PSU_TEMP_INFO,
    PSU_FAN_INFO,

    TEMP_INFO,
    CURR_INFO,
    VOL_INFO,
};

enum {
    // PMBUS_TYPE 
    MODEL_NAME,
    SERIAL_NUM,
    DATE,
    VENDOR,
    HW_VERSION,

    IN_CURR,
    IN_POWER,
    IN_VOL,
    OUT_CURR,
    OUT_POWER,
    OUT_VOL,

    IN_ALIAS,
    IN_TYPE,
    IN_MAX,
    IN_MIN,
    IN_ALARM, 
    
    CURR_ALIAS,
    CURR_TYPE,
    CURR_MAX,
    CURR_MIN,
    CURR_AVERAGE,

    // TEMP_SENSOR
    TEMP_ALIAS,
    TEMP_TYPE,
    IN_TEMP,
    TEMP_MAX,
    TEMP_MAX_HYST,
    TEMP_MIN,
};

extern int send_s3ip_msg(char *pbuf, unsigned short len);

struct s3ip_msg {
    unsigned short type;
    unsigned short index;
    unsigned char attr_type;
};

union s3ip_data {
    struct s3ip_msg msg;
    unsigned char buff[MAX_DATA_LEN];
};
