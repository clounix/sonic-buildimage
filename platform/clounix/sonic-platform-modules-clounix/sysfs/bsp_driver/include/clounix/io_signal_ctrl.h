enum {
    PSU_PRST = 1,
    PSU_ACOK,
    PSU_PWOK,

    PSU_LED_G,
    PSU_LED_R,

    SYS_LED_G,
    SYS_LED_R,

    FAN_LED_G,
    FAN_LED_R,

    ID_LED_B,
};

struct io_sig_desc {
    struct list_head node;
    int type;
    int num;
    int (*read)(int, int);
    int (*write)(int, int, int);
};

extern void del_io_sig_desc(int type, int num);
extern int add_io_sig_desc(int type, int num, int (*read)(int, int), int (*write)(int, int, int));
extern int read_io_sig_desc(int type, int num);
extern int write_io_sig_desc(int type, int num, int val);
