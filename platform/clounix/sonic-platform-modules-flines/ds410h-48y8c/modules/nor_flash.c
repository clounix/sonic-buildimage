#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/io.h>
#include <linux/ioport.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/file.h>
#include <linux/sched.h>
#include <linux/string.h>
#include <linux/mutex.h>

#define MMIO_BASE 0x28014000
#define MMIO_SIZE 0x1000
#define MMIO_BUF_BASE 0x0
#define MMIO_BUF_LEN (1024 * 1024 * 16)
#define FLASH_ERASE_SIZE (64 * 1024)
#define PROC_NAME "nor_flash_mmio"
#define READ_BUF_LEN_MAX (4096)
#define FLASH_SIZE (MMIO_BUF_LEN)

static void __iomem *mmio_base = NULL;
static void __iomem *mmio_buf = NULL;
static struct proc_dir_entry *proc_entry = NULL;
static struct mutex flash_op_mtx;


static struct resource mmio_res = 
{
    .start = MMIO_BASE,
    .end   = MMIO_BASE + MMIO_SIZE - 1,
    .name  = "flash-custom-mmio"
};

static struct resource mmio_buf_res = 
{
    .start = MMIO_BUF_BASE,
    .end   = MMIO_BUF_LEN + MMIO_BUF_LEN - 1,
    .name  = "flash-custom-mmio-buf"
};


unsigned int mCmd_Write = 0x2;
unsigned int mCmd_Eares = 0xD8;
unsigned int mCmd_Pp = 0x6;

unsigned int mCmd_Eares16 = 0xDC;
unsigned int mCmd_Write16 = 0x12;


#define FLASH_PP_CMD_OFFSET 0x18
#define CMD_WRITE 0x0
#define CMD_ERASER 0x1
#define CMD_PP 0x2

#define QSPI_REG_ADDR_BASE 0x28014000
#define QSPI_REG_ADDR_LEN 0X1000
#define QSPI_BUF_ADDR_BASE 0x0
#define QSPI_BUF_ADDR_LEN (1024 * 1024 * 16)

#define REG_RD_CFG 0x4
#define REG_WR_CFG 0x8
#define REG_FLUSH_REG 0xc
#define REG_CMD_PORT 0x10
#define REG_ADDR_PORT 0x14
#define REG_LD_PORT 0x1c


int FlushFlash(int Address, unsigned int BufferSizeInBytes)
{
    //WriteBackDataCacheRange ((void *)(int)Address, (int)BufferSizeInBytes);
    return 0;
}

void mmio_write_bit32(unsigned int *addr, unsigned int value)
{
    iowrite32(value, addr);
    return;
}


static inline int SpiWrite(int Address, void *Buffer, unsigned int BufferSizeInBytes)
{
    unsigned int cmd_id = 0;
    unsigned int Index;
    unsigned int *TemBuffer = Buffer;
    int Status;


    if(BufferSizeInBytes > 256) 
    {
        printk("Max Len is 256\n");
        return -1;
    }

    if(BufferSizeInBytes % 4 != 0) 
    {
        printk("Len must aligne 4 bytes\n");
        return -1;
    }

    if(Address % 4 != 0)
    {
        printk("Address not aligne 4 byte \n");
        return -1;
    }

    cmd_id = mCmd_Pp;
    if(Address >= 0x1000000)
    {
        mmio_write_bit32(mmio_base + REG_CMD_PORT, 0x400000 | (cmd_id << 24) | (0x1 << 12));
    }
    else
    {
        mmio_write_bit32(mmio_base + REG_CMD_PORT, 0x400000 | (cmd_id << 24));
    }
    mmio_write_bit32(mmio_base + REG_LD_PORT, 0x1);
    asm volatile ("isb sy":::"cc");
    asm volatile ("dsb sy":::"cc");

    cmd_id = mCmd_Write;
    if(Address >= 0x1000000)
    {
        cmd_id = mCmd_Write16;
        mmio_write_bit32(mmio_base + REG_WR_CFG, 0x000208 | (cmd_id << 24) | (0x1 << 4));
    }
    else
    {
        mmio_write_bit32(mmio_base + REG_WR_CFG, 0x000208 | (cmd_id << 24));
    }

    for(Index = 0; Index < BufferSizeInBytes / 4; Index++)
    {
        mmio_write_bit32(Address + Index * 4 + mmio_buf, TemBuffer[Index]);
    }
    Status = FlushFlash(Address, BufferSizeInBytes);
    mmio_write_bit32(mmio_base + REG_LD_PORT, 0x1);
    asm volatile ("isb sy":::"cc");
    asm volatile ("dsb sy":::"cc");

    mmio_write_bit32(mmio_base + REG_FLUSH_REG, 0x1);

    asm volatile ("isb sy":::"cc");
    asm volatile ("dsb sy":::"cc");

    mmio_write_bit32(mmio_base + REG_WR_CFG, 0x0);

    return Status;
}


static unsigned int ReadDeviceId(void)
{
    unsigned int Reg = 0;

    mmio_write_bit32(mmio_base + REG_CMD_PORT, 0x9F002040);
    Reg = ioread32(mmio_base + REG_LD_PORT);
    return Reg;
}


static inline void S25FS128S_Erase_Sector(void)
{
    unsigned int cmd_id, Index;
    int BlockAddress = 0;

    for (Index = 0; Index < 8; Index++) 
    {
        cmd_id = mCmd_Pp;
        mmio_write_bit32(mmio_base + REG_CMD_PORT, 0x400000 | (cmd_id << 24));
        mmio_write_bit32(mmio_base + REG_LD_PORT, 0x1);
        asm volatile ("isb sy":::"cc");
        asm volatile ("dsb sy":::"cc");

        cmd_id = 0x20;
        mmio_write_bit32(mmio_base + REG_CMD_PORT, 0x408000 | (cmd_id << 24));
        mmio_write_bit32(mmio_base + REG_ADDR_PORT, BlockAddress);
        mmio_write_bit32(mmio_base + REG_LD_PORT, 0x1);
        asm volatile ("isb sy":::"cc");
        asm volatile ("dsb sy":::"cc");
        BlockAddress += 0x1000;
    }
}

static inline void SPI_Erase_Sector(int BlockAddress)
{
    unsigned int cmd_id;
    unsigned int FlashId;
    if (BlockAddress == 0) 
    {
        FlashId = ReadDeviceId();
        if (0x4D182001 == FlashId) 
        {
            S25FS128S_Erase_Sector();
        }
    }

    cmd_id = mCmd_Pp;
    if(BlockAddress >= 0x1000000)
    {
        mmio_write_bit32(mmio_base + REG_CMD_PORT, 0x400000 | (cmd_id << 24) | (0x1 << 12));
    }
    else
    {
        mmio_write_bit32(mmio_base + REG_CMD_PORT, 0x400000 | (cmd_id << 24));
    }
    mmio_write_bit32(mmio_base + REG_LD_PORT, 0x1);
    asm volatile ("isb sy":::"cc");
    asm volatile ("dsb sy":::"cc");

    cmd_id = mCmd_Eares;
    if(BlockAddress >= 0x1000000)
    {
        cmd_id = mCmd_Eares16;
        mmio_write_bit32(mmio_base + REG_CMD_PORT, 0x408000 |(cmd_id << 24));
    }
    else
    {
        mmio_write_bit32(mmio_base + REG_CMD_PORT, 0x408000 |(cmd_id << 24));
    }
    mmio_write_bit32(mmio_base + REG_ADDR_PORT, BlockAddress);
    mmio_write_bit32(mmio_base + REG_LD_PORT, 0x1);
    asm volatile ("isb sy":::"cc");
    asm volatile ("dsb sy":::"cc");
}



int NorFlashPlatformRead (int Address)
{
    int RDCFG;

    if(Address >= 0x1000000)
    {
        RDCFG = ioread32(mmio_base + REG_RD_CFG);
        RDCFG |= 0x13080000;
        mmio_write_bit32(mmio_base + REG_RD_CFG, RDCFG);
    }

    if(0x80000 == (ioread32(mmio_base + REG_RD_CFG) & 0x80000))
    {
        RDCFG  = ioread32(mmio_base + REG_RD_CFG);
        RDCFG  = RDCFG & 0x00f7ffff;
        mmio_write_bit32(mmio_base + REG_RD_CFG, RDCFG);
    }
    return 0;
}

int NorFlashPlatformEraseSingleBlock (int BlockAddress)
{
    printk("NorFlashPlatformEraseSingleBlock: BlockAddress: 0x%x, mmio_base:%p\n", BlockAddress, mmio_base);

    SPI_Erase_Sector(BlockAddress);

    return 0;
}

int NorFlashPlatformWrite (int Address, void *Buffer, unsigned int BufferSizeInBytes)
{
    unsigned int Index = 0, Remainder = 0, Quotient = 0;
    int Status;
    int TmpAddress = Address;

    printk("NorFlashPlatformWrite: Address: 0x%x Len:0x%x\n", Address, BufferSizeInBytes);
    Remainder = BufferSizeInBytes % 256;
    Quotient = BufferSizeInBytes / 256;

    if(BufferSizeInBytes <= 256) 
    {
        Status = SpiWrite(TmpAddress, Buffer, BufferSizeInBytes);
    }
    else 
    {
        for(Index = 0; Index < Quotient; Index++) 
        {
            Status = SpiWrite(TmpAddress, Buffer, 256);
            TmpAddress += 256;
            Buffer += 256;
        }

        if(Remainder != 0) 
        {
            Status = SpiWrite(TmpAddress, Buffer, Remainder);
        }
    }

    if(Status) 
    {
        printk("%s() Line=%d\n", __FUNCTION__, __LINE__);
    }

    return 0;
}

static int kernel_write_file(const char *path, unsigned int address, size_t len)
{
#define BUF_LEN (64 * 1024)
    struct file *filp;
    loff_t pos = 0;
    ssize_t ret;
    char *buf = NULL;
    unsigned int remain_len = 0;
    unsigned int i = 0;
    unsigned int read_len = 0;
    unsigned int curr_addr = 0;

    filp = filp_open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (IS_ERR(filp)) 
    {
        printk("open file %s failed, err:%ld\n", path, PTR_ERR(filp));
        return PTR_ERR(filp);
    }
    
    buf = kmalloc(BUF_LEN, GFP_KERNEL);
    if(buf == NULL)
    {
        filp_close(filp, current->files);
        return ret;
    }

    remain_len = len;
    pos = 0;
    curr_addr = address;
    while(remain_len > 0)
    {
        NorFlashPlatformRead(curr_addr);
        read_len = remain_len > BUF_LEN ? BUF_LEN : remain_len;
        for(i = 0; i < read_len; i++)
        {
            buf[i] = ioread8(mmio_buf + curr_addr + i);
        }

        ret = kernel_write(filp, buf, read_len, &pos);
        if (ret < 0) 
        {
            printk("write file failed, ret:%zd\n", ret);
            filp_close(filp, current->files);
            kfree(buf);
            return ret;
        }
        else if(ret == 0)
        {
            break;
        }
        remain_len -= ret;
        curr_addr += ret;
    }

    printk("write success, write %zd bytes\n", len);

    filp_close(filp, current->files);
    kfree(buf);
    return 0;
}


static int flash_write_file(const char *path, unsigned int addr)
{
#define BUF_LEN (4 * 1024)
    struct file *filp;
    loff_t pos = 0;
    struct inode *inode;
    loff_t size;
    char buf[BUF_LEN] = {0};
    ssize_t ret;
    int remain_len = 0;
    int i = 0;

    printk("file_name :%s\n", path);
    filp = filp_open(path, O_RDONLY, 0);
    if(IS_ERR(filp)) 
    {
        printk("ERR open %s: %ld", path, PTR_ERR(filp));
        return PTR_ERR(filp);
    }

    inode = file_inode(filp);
    size = i_size_read(inode);
    printk("file_size : 0x%x byte\n", size);

    for(i = 0; i < FLASH_SIZE / FLASH_ERASE_SIZE; i++)
    {
        NorFlashPlatformEraseSingleBlock(addr + FLASH_ERASE_SIZE * i);
        cond_resched();
    }

    remain_len = size;
    pos = 0;
    while(pos < size)
    {
        memset(buf, 0, sizeof(buf));
        i = pos;
        ret = kernel_read(filp, buf, remain_len > BUF_LEN ? BUF_LEN : remain_len, &pos);
        if(ret < 0)
        {
            filp_close(filp, current->files);
            printk("read %s error!\n", path);
            return ret;
        }
        else if(ret == 0)
        {
            break;
        }
        remain_len = size - ret;

        NorFlashPlatformWrite(addr + i, buf, ret);
        cond_resched();
    }

    filp_close(filp, current->files);
    return 0;
}

static ssize_t mmio_proc_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos)
{
    char kbuf[128] = {0};
    unsigned long offset, value, len;
    int ret;
    mutex_lock(&flash_op_mtx);

    if (count >= sizeof(kbuf))
    {
        ret = -EINVAL;
        goto out;
    }

    if(copy_from_user(kbuf, buf, count))
    {
        ret = -EINVAL;
        goto out;
    }

    printk("cmd = %s\n", kbuf);
    if(strstr(kbuf, "reg-read"))
    {
        ret = sscanf(kbuf, "reg-read %lx", &offset);
        if (offset >= MMIO_SIZE || (offset & 3))
        {
            ret = -EINVAL;
            goto out;
        }
        value = ioread32(mmio_base + offset);
        printk("reg value = 0x%x\n", value);
    }
    else if(strstr(kbuf, "reg-write"))
    {
        ret = sscanf(kbuf, "reg-write %lx %lx", &offset, &value);
        if (offset >= MMIO_SIZE || (offset & 3))
        {
            ret = -EINVAL;
            goto out;
        }
        mmio_write_bit32(mmio_base + offset, value);
    }
    else if(strstr(kbuf, "buf-read"))
    {
        char path[128] = {0};
        ret = sscanf(kbuf, "buf-read %s %lx %lx", path, &offset, &len);
        if (offset >= MMIO_BUF_LEN || (offset & 3))
        {
            ret = -EINVAL;
            goto out;
        }
        kernel_write_file(path, offset, len);
    }
    else if(strstr(kbuf, "buf-write"))
    {
        char tmp_buf[64] = {0};
        ret = sscanf(kbuf, "buf-write %s %lx", tmp_buf, &offset);
        if (offset >= MMIO_BUF_LEN || (offset & 3))
        {
            ret = -EINVAL;
            goto out;
        }

        if(offset & 0x3)
        {
            offset += 0x3;
            offset &= ~0x3;
        }

        flash_write_file(tmp_buf, offset);
    }
    ret = count;
out:
    mutex_unlock(&flash_op_mtx);
    return ret;
}


static const struct proc_ops mmio_proc_ops = {
    .proc_write = mmio_proc_write,
};



static int __init mmio_init(void)
{
    mutex_init(&flash_op_mtx);
    if (!request_mem_region(mmio_res.start, resource_size(&mmio_res), mmio_res.name)) 
    {
        pr_err("request_mem_region failed\n");
        return -EBUSY;
    }
    pr_info("register mmio 0x%lx~0x%lx success\n", mmio_res.start, mmio_res.end);

    if (!request_mem_region(mmio_buf_res.start, resource_size(&mmio_buf_res), mmio_buf_res.name)) 
    {
        pr_err("request_mem_region failed\n");
        return -EBUSY;
    }
    pr_info("register mmio buf 0x%lx~0x%lx success\n", mmio_buf_res.start, mmio_buf_res.end);

    mmio_base = ioremap(mmio_res.start, MMIO_SIZE);
    pr_info("mmap mmio reg 0x%lx->0x%lx len = 0x%x success\n", mmio_res.start, mmio_base, MMIO_SIZE);

    mmio_buf = ioremap(mmio_buf_res.start, MMIO_BUF_LEN);
    pr_info("mmap mmio buf 0x%lx->0x%lx len = 0x%x success\n", mmio_buf_res.start, mmio_buf, MMIO_BUF_LEN);

    proc_entry = proc_create(PROC_NAME, 0644, NULL, &mmio_proc_ops);
    if(!proc_entry)
    {
        pr_err("proc_create failed\n");
        iounmap(mmio_base);
        iounmap(mmio_buf);
        release_mem_region(mmio_res.start, resource_size(&mmio_res));
        release_mem_region(mmio_buf_res.start, resource_size(&mmio_buf_res));
        return -ENOMEM;
    }

    return 0;
}

static void __exit mmio_exit(void)
{
    proc_remove(proc_entry);
    iounmap(mmio_base);
    iounmap(mmio_buf);
    release_mem_region(mmio_res.start, resource_size(&mmio_res));
    release_mem_region(mmio_buf_res.start, resource_size(&mmio_buf_res));
    pr_info("unregister mmio\n");
}

module_init(mmio_init);
module_exit(mmio_exit);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Register flash custom MMIO 0x28014000");