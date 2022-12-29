#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>

struct init_and_exit_func_note
{
    int (*init)(void);
    void (*exit)(void);
};

extern int cpu_coretemp_init(void);
extern void cpu_coretemp_exit(void);
extern int via_cputemp_init(void);
extern void via_cputemp_exit(void);

static void (*exit_ops)(void) = NULL;
static struct init_and_exit_func_note ops_list[] = {
    {cpu_coretemp_init, cpu_coretemp_exit},
    {via_cputemp_init, via_cputemp_exit},
    {NULL, NULL},
};

static int __init common_cputemp_init(void)
{
    int i;
    int ret;

    for (i=0; ops_list[i].init != NULL; i++) {
        ret = ops_list[i].init();
        if (ret == 0) {
            exit_ops = ops_list[i].exit;
            return ret;
        }
    }

    return -ENODATA;
}

static void __exit common_cputemp_exit(void)
{
    if (exit_ops != NULL)
        exit_ops();
    return;
}

module_init(common_cputemp_init);
module_exit(common_cputemp_exit);

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("baohx@clounix.com");
MODULE_DESCRIPTION("x86 cpu temp sensor driver");
