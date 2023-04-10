#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/ide.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/gpio.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/of_gpio.h>
#include <linux/semaphore.h>
#include <linux/timer.h>
#include <linux/i2c.h>
/*
#include <asm/mach/map.h>
#include <asm/uaccess.h>
#include <asm/io.h>
*/
/***************************************************************
文件名		: pca9555.c
作者	  	: 
版本	   	: V1.0
描述	   	: PCA9555驱动程序
其他	   	: 无
日志	   	: 初版V1.0 2022/12/20 
***************************************************************/
#define PCA9555_CNT	     1
#define PCA9555_NAME	"pca9555"
#define MAX_REG_INDEX   8
#define PCA9555_ADDR    	0X26	/* PCA9555器件地址  */

struct pca9555_dev {
	dev_t devid;			/* 设备号 	 */
	struct cdev cdev;		/* cdev 	*/
	struct class *class;	/* 类 		*/
	struct device *device;	/* 设备 	 */
	struct device_node	*nd; /* 设备节点 */
	int major;			/* 主设备号 */
	void *private_data;	/* 私有数据 */
	unsigned char regdata[MAX_REG_INDEX];		/* 保存寄存器数据 */
};

struct data_msg {
       u8 reg;
       u8 val;
};


static struct pca9555_dev pca9555dev;

/*
 * @description	: 从pca9555读取多个寄存器数据
 * @param - dev:  pca9555设备
 * @param - reg:  要读取的寄存器首地址
 * @param - val:  读取到的数据
 * @param - len:  要读取的数据长度
 * @return 		: 操作结果
 */
static int pca9555_read_regs(struct pca9555_dev *dev, u8 reg, void *val, int len)
{
	int ret;
	struct i2c_msg msg[2];
	struct i2c_client *client = (struct i2c_client *)dev->private_data;
    
	/* msg[0]为发送要读取的首地址 */
	msg[0].addr = client->addr;			/* pca9555地址 */
	msg[0].flags = 0;					/* 标记为发送数据 */
	msg[0].buf = &reg;					/* 读取的首地址 */
	msg[0].len = 1;						/* reg长度*/

	/* msg[1]读取数据 */
	msg[1].addr = client->addr;			/* pca9555地址 */
	msg[1].flags = I2C_M_RD;			/* 标记为读取数据*/
	msg[1].buf = val;					/* 读取数据缓冲区 */
	msg[1].len = len;					/* 要读取的数据长度*/

	ret = i2c_transfer(client->adapter, msg, 2);
	if(ret == 2) {
		ret = 0;
	} else {
		printk("i2c rd failed=%d reg=%06x len=%d\n",ret, reg, len);
		ret = -EREMOTEIO;
	}
	return ret;
}

/*
 * @description	: 向pca9555多个寄存器写入数据
 * @param - dev:  pca9555设备
 * @param - reg:  要写入的寄存器首地址
 * @param - val:  要写入的数据缓冲区
 * @param - len:  要写入的数据长度
 * @return 	  :   操作结果
 */
static s32 pca9555_write_regs(struct pca9555_dev *dev, u8 reg, u8 *buf, u8 len)
{
	u8 b[256];
	struct i2c_msg msg;
	struct i2c_client *client = (struct i2c_client *)dev->private_data;
	
	b[0] = reg;					/* 寄存器首地址 */
	memcpy(&b[1],buf,len);		/* 将要写入的数据拷贝到数组b里面 */
		
	msg.addr = client->addr;	/* pca9555地址 */
	msg.flags = 0;				/* 标记为写数据 */

	msg.buf = b;				/* 要写入的数据缓冲区 */
	msg.len = len + 1;			/* 要写入的数据长度 */

	return i2c_transfer(client->adapter, &msg, 1);
}

/*
 * @description	: 读取pca9555指定寄存器值，读取一个寄存器
 * @param - dev:  pca9555设备
 * @param - reg:  要读取的寄存器
 * @return 	  :   读取到的寄存器值
 */
static unsigned char pca9555_read_reg(struct pca9555_dev *dev, u8 reg)
{
	u8 data = 0;
#if 0
	pca9555_read_regs(dev, reg, &data, 1);
	printk("pca9555:pca9555_read_reg ret data = 0x%x \r\n",data);
	return data;
#endif
#if 1
	struct i2c_client *client = (struct i2c_client *)dev->private_data;
	data = i2c_smbus_read_byte_data(client, reg);
	/*printk("pca9555:pca9555_read_reg ret data = 0x%x \r\n",data);*/
	return data;
#endif
}

/*
 * @description	: 向pca9555指定寄存器写入指定的值，写一个寄存器
 * @param - dev:  pca9555设备
 * @param - reg:  要写的寄存器
 * @param - data: 要写入的值
 * @return   :    无
 */
/*
static void pca9555_write_reg(struct pca9555_dev *dev, u8 reg, u8 data)
{
	u8 buf = 0;
	buf = data;
	pca9555_write_regs(dev, reg, &buf, 1);
}
*/
/*
 * @description	: 读取pca9555的数据，读取原始数据，包括ALS,PS和IR, 注意！
 *				: 如果同时打开ALS,IR+PS的话两次数据读取的时间间隔要大于112.5ms
 * @param - ir	: ir数据
 * @param - ps 	: ps数据
 * @param - ps 	: als数据 
 * @return 		: 无。
 */
void pca9555_readdata(struct pca9555_dev *dev)
{
	unsigned char i =0;
    /*unsigned char buf[MAX_REG_INDEX];*/

   
	
	/* 循环读取所有传感器数据 */
    for(i = 0; i < MAX_REG_INDEX; i++)	
    {
        dev->regdata[i] = pca9555_read_reg(dev, i);	
    }

    
}

/*
 * @description		: 打开设备
 * @param - inode 	: 传递给驱动的inode
 * @param - filp 	: 设备文件，file结构体有个叫做private_data的成员变量
 * 					  一般在open的时候将private_data指向设备结构体。
 * @return 			: 0 成功;其他 失败
 */
static int pca9555_open(struct inode *inode, struct file *filp)
{
    
    filp->private_data = &pca9555dev;

	return 0;
}

/*
 * @description		: 从设备读取数据 
 * @param - filp 	: 要打开的设备文件(文件描述符)
 * @param - buf 	: 返回给用户空间的数据缓冲区
 * @param - cnt 	: 要读取的数据长度
 * @param - offt 	: 相对于文件首地址的偏移
 * @return 			: 读取的字节数，如果为负值，表示读取失败
 */
static ssize_t pca9555_read(struct file *filp, char __user *buf, size_t cnt, loff_t *off)
{
	/*short data[3];*/
	long err = 0;

	struct pca9555_dev *dev = (struct pca9555_dev *)filp->private_data;
	/*printk("pca9555:kernel pca9555_read \r\n");*/
   

	pca9555_readdata(dev);
	err = copy_to_user(buf, dev->regdata, cnt);
	return err;
}


/*
 * @description		: 向设备写数据 
 * @param - filp 	: 设备文件，表示打开的文件描述符
 * @param - buf 	: 要写给设备写入的数据, 偶数index，寄存器地址，奇数index，写入的值
 * @param - cnt 	: 要写入的数据长度
 * @param - offt 	: 相对于文件首地址的偏移
 * @return 			: 写入的字节数，如果为负值，表示写入失败
 */
static ssize_t pca9555_write(struct file *filp, const char __user *buf, size_t cnt, loff_t *offt)
{
	int retvalue;
	/*u8 reg;*/
	unsigned int i= 0;
	/*unsigned char databuf[8];*/  /*databuf[]:偶数index，寄存器地址，奇数index，写入的值*/
    struct data_msg databuf[MAX_REG_INDEX];
	struct pca9555_dev *dev = (struct pca9555_dev *)filp->private_data;
    struct i2c_client *client = (struct i2c_client *)dev->private_data;

    if(cnt > MAX_REG_INDEX)
	{
		printk("kernel param cnt = 0x%lx, exceeds the max register number!\r\n",cnt);
		return -EFAULT;
	}
	retvalue = copy_from_user(databuf, buf, cnt*sizeof(databuf[0]));
	if(retvalue < 0) {
		printk("kernel write failed!\r\n");
		return -EFAULT;
	}

    /*databuf[]:偶数index，寄存器地址，奇数index，写入的值*/
    for(i=0; i< cnt; i++)
	{
		/*pca9555_write_regs(dev, reg, &databuf[i+1], 1);*/
		/*printk("pca9555:pca9555_write databuf[%d].reg = 0x%x,databuf[%d].val = 0x%x.\n",i,databuf[i].reg,i,databuf[i].val);*/
		retvalue = i2c_smbus_write_byte_data(client, databuf[i].reg, databuf[i].val);
		if (retvalue < 0) {
			printk("pca9555:i2c_smbus_write_byte_data return failed.\n");
			return -EFAULT;
		}
	}

	return i;
}

/*
 * @description		: 关闭/释放设备
 * @param - filp 	: 要关闭的设备文件(文件描述符)
 * @return 			: 0 成功;其他 失败
 */
static int pca9555_release(struct inode *inode, struct file *filp)
{
	return 0;
}

/* PCA9555操作函数 */
static const struct file_operations pca9555_ops = {
	.owner = THIS_MODULE,
	.open = pca9555_open,
	.read = pca9555_read,
	.write = pca9555_write,
	.release = pca9555_release,
};

 /*
  * @description     : i2c驱动的probe函数，当驱动与
  *                    设备匹配以后此函数就会执行
  * @param - client  : i2c设备
  * @param - id      : i2c设备ID
  * @return          : 0，成功;其他负值,失败
  */
static int pca9555_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	/* 1、构建设备号 */
	if (pca9555dev.major) {
		pca9555dev.devid = MKDEV(pca9555dev.major, 0);
		register_chrdev_region(pca9555dev.devid, PCA9555_CNT, PCA9555_NAME);
	} else {
		alloc_chrdev_region(&pca9555dev.devid, 0, PCA9555_CNT, PCA9555_NAME);
		pca9555dev.major = MAJOR(pca9555dev.devid);
	}

	/* 2、注册设备 */
	cdev_init(&pca9555dev.cdev, &pca9555_ops);
	cdev_add(&pca9555dev.cdev, pca9555dev.devid, PCA9555_CNT);

	/* 3、创建类 */
	pca9555dev.class = class_create(THIS_MODULE, PCA9555_NAME);
	if (IS_ERR(pca9555dev.class)) {
		return PTR_ERR(pca9555dev.class);
	}

	/* 4、创建设备 */
	pca9555dev.device = device_create(pca9555dev.class, NULL, pca9555dev.devid, NULL, PCA9555_NAME);
	if (IS_ERR(pca9555dev.device)) {
		return PTR_ERR(pca9555dev.device);
	}

	pca9555dev.private_data = client;

	return 0;
}

/*
 * @description     : i2c驱动的remove函数，移除i2c驱动的时候此函数会执行
 * @param - client 	: i2c设备
 * @return          : 0，成功;其他负值,失败
 */
static int pca9555_remove(struct i2c_client *client)
{
	/* 删除设备 */
	cdev_del(&pca9555dev.cdev);
	unregister_chrdev_region(pca9555dev.devid, PCA9555_CNT);

	/* 注销掉类和设备 */
	device_destroy(pca9555dev.class, pca9555dev.devid);
	class_destroy(pca9555dev.class);
	return 0;
}

/* 传统匹配方式ID列表 */
static const struct i2c_device_id pca9555_id[] = {
	{"alientek,pca9555", 0},  
	{}
};

/* 设备树匹配列表 */
static const struct of_device_id pca9555_of_match[] = {
	{ .compatible = "alientek,pca9555" },
	{ /* Sentinel */ }
};

/* i2c驱动结构体 */	
static struct i2c_driver pca9555_driver = {
	.probe = pca9555_probe,
	.remove = pca9555_remove,
	.driver = {
			.owner = THIS_MODULE,
		   	.name = "pca9555",
		   	.of_match_table = pca9555_of_match, 
		   },
	.id_table = pca9555_id,
};
		   
/*
 * @description	: 驱动入口函数
 * @param 		: 无
 * @return 		: 无
 */
static int __init pca9555_init(void)
{
	int ret = 0;

	ret = i2c_add_driver(&pca9555_driver);
	return ret;
}

/*
 * @description	: 驱动出口函数
 * @param 		: 无
 * @return 		: 无
 */
static void __exit pca9555_exit(void)
{
	i2c_del_driver(&pca9555_driver);
}

/* module_i2c_driver(pca9555_driver) */

module_init(pca9555_init);
module_exit(pca9555_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("zuozhongkai");



