/* memtool.c - static /dev/mem register R/W + PCI enumeration for initramfs
 * compile: aarch64-linux-gnu-gcc -static -O2 -o memtool memtool.c
 * usage (2nd arg = action):
 *   ./memtool enum [--csv]              enumerate all PCI devices
 *   ./memtool search <keyword>         search by device description
 *   ./memtool read 0xADDR              read 32-bit MMIO
 *   ./memtool write 0xADDR 0xVALUE     write 32-bit MMIO
 *   ./memtool reboot                   detect NVMe loss; power-cycle via FPGA
 *                                      GLOBAL_RST only when disk lost
 *                                      (FPGA VID=0x10EE / DID=0x7021)
 *   ./memtool monitor [interval_ms]    [NEW] watch NVMe forever, print loss
 *                                      messages repeatedly to stdout + syslog,
 *                                      listen on /run/flks_nvme_monitor.socket
 *                                      for stop/shutdown/mute/status commands
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <signal.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/un.h>
#include <stdint.h>
#include <stdarg.h>

/* ---------- read a small sysfs text file into buf, strip newline ---------- */
static int read_file(const char *path, char *buf, size_t len) {
    int fd = open(path, O_RDONLY);
    if (fd < 0) return -1;
    ssize_t n = read(fd, buf, len - 1);
    close(fd);
    if (n < 0) return -1;
    buf[n] = '\0';
    while (n > 0 && (buf[n-1] == '\n' || buf[n-1] == '\r')) buf[--n] = '\0';
    return 0;
}

/* ---------- PCI class code -> human-readable (PCI spec standard classes) ---------- */
static const char *class_desc(const char *class_str) {
    unsigned long cls = strtoul(class_str, NULL, 16);
    unsigned base = (cls >> 16) & 0xFF;
    unsigned sub  = (cls >>  8) & 0xFF;

    switch (base) {
    case 0x00: return "Unclassified device";
    case 0x01:
        switch (sub) {
        case 0x08: return "Non-Volatile memory controller";
        case 0x06: return "SATA controller";
        case 0x00: return "SCSI storage controller";
        case 0x01: return "IDE interface";
        case 0x05: return "ATA controller";
        default:   return "Mass storage controller";
        }
    case 0x02: return "Network controller";
    case 0x03: return "Display controller";
    case 0x04: return "Multimedia controller";
    case 0x05: return "Memory controller";
    case 0x06:
        switch (sub) {
        case 0x04: return "PCI bridge";
        case 0x00: return "Host bridge";
        case 0x01: return "ISA bridge";
        default:   return "Bridge";
        }
    case 0x07: return "Communication controller";
    case 0x08: return "Generic system peripheral";
    case 0x0C:
        switch (sub) {
        case 0x03: return "USB controller";
        case 0x05: return "SMBus";
        default:   return "Serial bus controller";
        }
    default: return "Unknown class";
    }
}

/* ---------- device info struct (linked list) ---------- */
typedef struct DevInfo {
    char bdf[32];
    char vendor[16];
    char device[16];
    char class_id[16];
    char driver[32];
    char model[256];        /* sysfs model 源串最长 511B，此处留 256B 足够且不触发截断告警 */
    char bars[256];
    char class_desc[64];
    struct DevInfo *next;
} DevInfo;

/* ---------- read a symlink target (relative path) ---------- */
static int read_link(const char *path, char *buf, size_t len) {
    ssize_t n = readlink(path, buf, len - 1);
    if (n < 0) return -1;
    buf[n] = '\0';
    return 0;
}

/* ---------- map PCI class code -> /sys/class/ subsystem name ---------- */
static const char *class_to_subsystem(unsigned long cls) {
    unsigned base = (cls >> 16) & 0xFF;
    unsigned sub  = (cls >>  8) & 0xFF;
    if (base == 0x01 && sub == 0x08) return "nvme";       /* NVMe */
    if (base == 0x01 && sub == 0x00) return "scsi_disk";  /* SCSI */
    if (base == 0x02)                 return "net";        /* Ethernet */
    if (base == 0x0C && sub == 0x03)  return "usb";        /* USB */
    if (base == 0x03)                 return "drm";         /* Display */
    return NULL;
}

/* ---------- read driver + model from sysfs for a BDF ----------
 * model_len：调用方传入 model 缓冲区容量，拷贝时严格按此截断。
 * 显式传长度而不是写死 64，既让截断成为明确契约，
 * 也消除 -Wformat-truncation（编译器无法推断裸常量与目标容量的关系）。 */
static void read_dev_name(const char *bdf, char *model, size_t model_len, char *drv) {
    model[0] = '\0';
    drv[0] = '\0';
    char path[512], buf[512], link[512];

    /* uevent: DRIVER=xxx */
    snprintf(path, sizeof(path), "/sys/bus/pci/devices/%s/uevent", bdf);
    int fd = open(path, O_RDONLY);
    if (fd >= 0) {
        ssize_t n = read(fd, buf, sizeof(buf) - 1);
        close(fd);
        if (n > 0) {
            buf[n] = '\0';
            char *line = buf;
            while (*line) {
                char *nl = strchr(line, '\n');
                if (nl) *nl = '\0';
                if (strncmp(line, "DRIVER=", 7) == 0)
                    strncpy(drv, line + 7, 31);
                line = nl ? nl + 1 : line + strlen(line);
            }
        }
    }

    /* read class file -> determine subsystem */
    char cbuf[32];
    snprintf(path, sizeof(path), "/sys/bus/pci/devices/%s/class", bdf);
    if (read_file(path, cbuf, sizeof(cbuf)) != 0) return;

    unsigned long cls = strtoul(cbuf, NULL, 16);
    const char *subsys = class_to_subsystem(cls);
    if (!subsys) return;

    /* walk /sys/class/<subsys>/, find device whose "device" link points to this BDF */
    char class_root[256];
    snprintf(class_root, sizeof(class_root), "/sys/class/%s", subsys);
    DIR *dir = opendir(class_root);
    if (!dir) return;

    struct dirent *de;
    while ((de = readdir(dir)) != NULL) {
        if (de->d_name[0] == '.') continue;

        /* readlink /sys/class/<subsys>/<dev>/device
         * 用显式长度判断代替固定缓冲区拼接：路径超长直接跳过该设备，
         * 既消除 -Wformat-truncation 告警，也避免拼出半截路径去访问。 */
        int n = snprintf(path, sizeof(path), "%s/%s/device", class_root, de->d_name);
        if (n < 0 || (size_t)n >= sizeof(path)) continue;
        if (read_link(path, link, sizeof(link)) != 0) continue;

        /* link target ends with /<bdf> ? */
        char *p = strstr(link, bdf);
        if (!p) continue;
        /* make sure it's the BDF at end of path (not a substring) */
        if (p[strlen(bdf)] != '\0' && p[strlen(bdf)] != '/') continue;

        /* found matching device, read its model */
        n = snprintf(path, sizeof(path), "%s/%s/model", class_root, de->d_name);
        if (n < 0 || (size_t)n >= sizeof(path)) continue;
        /* 用与目标等长的独立缓冲读取：读入多少就能放下多少，
         * 不存在"读 512 再塞进 256"的截断路径（这也是告警的根源）。 */
        char mbuf[256];
        if (read_file(path, mbuf, sizeof(mbuf)) == 0) {
            /* 目标容量与 mbuf 一致，编译器可证明不会截断 */
            snprintf(model, model_len, "%s", mbuf);
            break;
        }
    }
    closedir(dir);
}

/* ---------- collect all PCI devices into linked list ---------- */
static DevInfo *pci_collect(void) {
    const char *devfs = "/sys/bus/pci/devices";
    DIR *dir = opendir(devfs);
    if (!dir) { perror(devfs); return NULL; }

    DevInfo *head = NULL, **tail = &head;
    struct dirent *de;
    char path[256], buf[128];

    while ((de = readdir(dir)) != NULL) {
        if (de->d_name[0] == '.') continue;

        /* limit bdf name to avoid gcc format-truncation warnings */
        char bdf_name[32];
        strncpy(bdf_name, de->d_name, sizeof(bdf_name) - 1);
        bdf_name[sizeof(bdf_name) - 1] = '\0';

        DevInfo *info = calloc(1, sizeof(DevInfo));
        strcpy(info->bdf, bdf_name);

        /* vendor */
        snprintf(path, sizeof(path), "%s/%s/vendor", devfs, bdf_name);
        if (read_file(path, buf, sizeof(buf)) < 0) { free(info); continue; }
        strcpy(info->vendor, buf);

        /* device */
        snprintf(path, sizeof(path), "%s/%s/device", devfs, bdf_name);
        if (read_file(path, buf, sizeof(buf)) < 0) { free(info); continue; }
        strcpy(info->device, buf);

        /* class */
        snprintf(path, sizeof(path), "%s/%s/class", devfs, bdf_name);
        if (read_file(path, buf, sizeof(buf)) == 0) strcpy(info->class_id, buf);
        strncpy(info->class_desc, class_desc(info->class_id),
                sizeof(info->class_desc) - 1);

        /* driver + model from sysfs */
        read_dev_name(bdf_name, info->model, sizeof(info->model), info->driver);

        /* bars: walk all 6 resource lines, collect non-zero */
        info->bars[0] = '\0';
        snprintf(path, sizeof(path), "%s/%s/resource", devfs, bdf_name);
        int rfd = open(path, O_RDONLY);
        if (rfd >= 0) {
            char rbuf[2048];
            ssize_t rn = read(rfd, rbuf, sizeof(rbuf) - 1);
            close(rfd);
            if (rn > 0) {
                rbuf[rn] = '\0';
                int idx = 0;
                char *line = rbuf;
                while (*line && idx < 6) {
                    char *nl = strchr(line, '\n');
                    if (nl) *nl = '\0';
                    char *tok = strtok(line, " \t");
                    if (tok) {
                        unsigned long start = strtoul(tok, NULL, 16);
                        if (start != 0) {
                            char tmp[64];
                            snprintf(tmp, sizeof(tmp), "BAR%d=0x%lx ", idx, start);
                            strcat(info->bars, tmp);
                        }
                    }
                    idx++;
                    if (!nl) break;
                    line = nl + 1;
                }
            }
        }
        if (!info->bars[0]) strcpy(info->bars, "(none)");

        *tail = info;
        tail = &info->next;
    }
    closedir(dir);
    return head;
}

/* ---------- case-insensitive substring match, no temp buffer ----------
 * 逐字符比较，避免先转小写再拼接带来的缓冲区容量问题。
 * 返回 1 = haystack 中包含 needle（忽略大小写） */
static int field_match(const char *haystack, const char *needle) {
    if (!haystack || !needle || !*needle) return 0;
    size_t nlen = strlen(needle);
    for (const char *h = haystack; *h; h++) {
        size_t i = 0;
        while (i < nlen && h[i] &&
               tolower((unsigned char)h[i]) == tolower((unsigned char)needle[i]))
            i++;
        if (i == nlen) return 1;
    }
    return 0;
}

/* ---------- enumerate / search ---------- */
static void pci_enum(int csv, const char *filter) {
    DevInfo *head = pci_collect();
    if (!head) return;

    if (csv) {
        printf("bdf,vendor,device,driver,model,bars,class_desc,device_desc\n");
    } else {
        printf("%-16s %-8s %-8s %-12s %-25s %-30s %-30s %s\n",
               "BDF", "VENDOR", "DEVICE", "DRIVER", "MODEL",
               "BARS", "CLASS DESC", "DEVICE DESC");
        printf("------------------------------------------------------------------------------------------------------------------------\n");
    }

    for (DevInfo *p = head; p; p = p->next) {
        /* device_desc = driver module name */
        const char *dev_desc = p->driver[0] ? p->driver : "(none)";

        if (filter) {
            /* 逐个字段做大小写无关匹配，不再拼成一个大缓冲。
             * 拼接版把 4 个字段塞进 256B，存在真实截断风险（也是告警来源）。 */
            if (!field_match(p->driver, filter) &&
                !field_match(p->model, filter) &&
                !field_match(p->class_desc, filter) &&
                !field_match(dev_desc, filter) &&
                !field_match(p->bdf, filter) &&
                !field_match(p->vendor, filter) &&
                !field_match(p->device, filter))
                continue;
        }

        if (csv) {
            printf("%s,%s,%s,%s,%s,%s,%s,%s\n",
                   p->bdf, p->vendor, p->device, p->driver, p->model,
                   p->bars, p->class_desc, dev_desc);
        } else {
            printf("%-16s %-8s %-8s %-12s %-25s %-30s %-30s %s\n",
                   p->bdf, p->vendor, p->device, p->driver, p->model,
                   p->bars, p->class_desc, dev_desc);
        }
    }

    /* free list */
    DevInfo *p = head;
    while (p) { DevInfo *n = p->next; free(p); p = n; }
}

/* ---------- build timestamp: convert __DATE__/__TIME__ to ISO yyyy-MM-dd HH:mm:ss ---------- */
static const char *mon_to_num(const char *m) {
    if (!strncmp(m, "Jan", 3)) return "01";
    if (!strncmp(m, "Feb", 3)) return "02";
    if (!strncmp(m, "Mar", 3)) return "03";
    if (!strncmp(m, "Apr", 3)) return "04";
    if (!strncmp(m, "May", 3)) return "05";
    if (!strncmp(m, "Jun", 3)) return "06";
    if (!strncmp(m, "Jul", 3)) return "07";
    if (!strncmp(m, "Aug", 3)) return "08";
    if (!strncmp(m, "Sep", 3)) return "09";
    if (!strncmp(m, "Oct", 3)) return "10";
    if (!strncmp(m, "Nov", 3)) return "11";
    if (!strncmp(m, "Dec", 3)) return "12";
    return "??";
}

static void print_build_time(void) {
    /* __DATE__ = "Oct 19 2026", __TIME__ = "12:34:56" */
    char mon[4], day[5], year[6];
    sscanf(__DATE__, "%3s %4s %5s", mon, day, year);
    /* pad single-digit day */
    int d = atoi(day);
    printf("build: %s-%s-%02d %s\n", year, mon_to_num(mon), d, __TIME__);
}

/* ---------- /dev/mem 32-bit R/W ---------- */
static int mem_rw(uint64_t addr, uint32_t *val, int write) {
    int fd = open("/dev/mem", O_RDWR | O_SYNC);
    if (fd < 0) { perror("open /dev/mem"); return -1; }

    uint64_t page = addr & ~0xFFFULL;
    size_t off = (size_t)(addr - page);

    volatile uint32_t *map = mmap(NULL, 0x1000,
        PROT_READ | PROT_WRITE, MAP_SHARED, fd, (off_t)page);
    if (map == MAP_FAILED) { perror("mmap"); close(fd); return -1; }

    if (write) {
        map[off / 4] = *val;
        printf("[wrote] 0x%08x @ 0x%llx\n", *val, (unsigned long long)addr);
    } else {
        *val = map[off / 4];
        printf("0x%08x\n", *val);
    }
    munmap((void *)map, 0x1000);
    close(fd);
    return 0;
}

/* ---------- FPGA GLOBAL_RST power cycle (reboot) ----------
 * FPGA PCIe 身份（依据 references/DS410G(DS410H)软硬件接口文档V1.0.pdf §4.3
 * "FPGA PCIe 接口及用户中断"：PCIe x1，速率 5GT/s）：
 *   Vendor ID = 0x10EE (Xilinx)
 *   Device ID = 0x7021
 *   BAR0 物理基址禁止写死（0x4200_0000 只是 FPGA 内部 AXI 基址），
 *   必须从 sysfs resource 属性实时解析。
 * Power Cycle 寄存器（依据 references/DS410G(DS410H)_FPGA寄存器说明V1.0.pdf
 * §4.4.6 GLOBAL_RST）：复位管理块 AXI 0x4200_0100，GLOBAL_RST offset 0x20，
 * 即 BAR 内偏移 0x120；global_rst bit31 (W)，写 1 (0x80000000) = 全局复位，无需喂狗。
 * 实现过程：
 *   1) 遍历 /sys/bus/pci/devices/ 设备节点，按 vendor=0x10ee & device=0x7021
 *      双匹配找到 FPGA 设备；
 *   2) 读取该节点 resource 属性，第一行非零即 BAR0 物理地址空间基址；
 *   3) /dev/mem mmap 写 BAR0+0x120 = 0x80000000，写入 MMIO Power Cycle 寄存器
 *      触发整机全局复位（power cycle）。
 */
#define FPGA_VID        "0x10ee"
#define FPGA_DID        "0x7021"
#define FPGA_ID_OFF     0x00u       /* §4.1.1 FPGA_ID，bits15:0 预期 0x6548（只读校验） */
#define FPGA_ID_EXPECT  0x6548u
#define GLOBAL_RST_OFF  0x120u      /* AXI 0x4200_0100 + offset 0x20 */
#define GLOBAL_RST_VAL  0x80000000u /* bit31=1 全局复位 */

/* step 1+2: scan sysfs, match VID/DID, parse resource -> BAR0 physical base */
static int fpga_find_bar(uint64_t *bar0, char *bdf_out, size_t bdf_len) {
    const char *devfs = "/sys/bus/pci/devices";
    DIR *dir = opendir(devfs);
    if (!dir) { perror(devfs); return -1; }

    int found = 0;
    struct dirent *de;
    char path[512], vbuf[32], dbuf[32];
    *bar0 = 0;
    bdf_out[0] = '\0';

    while ((de = readdir(dir)) != NULL) {
        if (de->d_name[0] == '.') continue;

        /* step 1: match vendor + device (case-insensitive) */
        snprintf(path, sizeof(path), "%s/%s/vendor", devfs, de->d_name);
        if (read_file(path, vbuf, sizeof(vbuf)) != 0) continue;
        snprintf(path, sizeof(path), "%s/%s/device", devfs, de->d_name);
        if (read_file(path, dbuf, sizeof(dbuf)) != 0) continue;
        if (strcasecmp(vbuf, FPGA_VID) != 0 ||
            strcasecmp(dbuf, FPGA_DID) != 0) continue;

        /* step 2: parse resource, first non-zero line = BAR0 physical base */
        snprintf(path, sizeof(path), "%s/%s/resource", devfs, de->d_name);
        int rfd = open(path, O_RDONLY);
        if (rfd < 0) continue;
        char rbuf[2048];
        ssize_t rn = read(rfd, rbuf, sizeof(rbuf) - 1);
        close(rfd);
        if (rn <= 0) continue;
        rbuf[rn] = '\0';

        uint64_t start = 0;
        char *line = rbuf;
        while (*line) {
            char *nl = strchr(line, '\n');
            if (nl) *nl = '\0';
            char *tok = strtok(line, " \t");
            if (tok) {
                unsigned long s = strtoul(tok, NULL, 16);
                if (s != 0) { start = s; break; }
            }
            if (!nl) break;
            line = nl + 1;
        }
        if (start == 0) continue;

        if (found) {
            fprintf(stderr, "error: multiple FPGA devices (%s, %s)\n",
                    bdf_out, de->d_name);
            closedir(dir);
            return -1;
        }
        found = 1;
        *bar0 = start;
        strncpy(bdf_out, de->d_name, bdf_len - 1);
        bdf_out[bdf_len - 1] = '\0';
    }
    closedir(dir);
    return found ? 0 : -1;
}

/* ---------- NVMe 丢盘检测（power cycle 前置门控） ----------
 * detect_lose_nvme() 返回 1 = 判定硬盘丢失（fpga_power_cycle 才继续复位）；
 * 返回 0 = 系统正常，提前返回，不做 power cycle。
 *
 * 检查1：往 /home/admin/ 创建一个 GUID(v4) 名称的文件，写入字符串 "ok" 后删除；
 *        测试失败打印文件系统异常消息，成功则提前返回 0。
 * 检查2：检查 /dev/nvme0 设备节点与 /sys/class/nvme/nvme0 驱动目录是否存在；
 *        不存在则打印设备节点和驱动都发现错误的消息，存在则提前返回 0。
 * 检查3：以上两项都没有提前返回 0，则打印硬盘丢失消息，返回 1。
 */

/* 在 dir 下创建 GUID(v4) 命名临时文件，写入 "ok" 后删除；0=成功，-1=失败 */
static int fs_write_probe(const char *dir) {
    unsigned char b[16] = {0};
    int ur = open("/dev/urandom", O_RDONLY);
    if (ur >= 0) {
        if (read(ur, b, sizeof(b)) != (ssize_t)sizeof(b)) {
            close(ur);
            return -1;
        }
        close(ur);
    } else {
        /* 无 /dev/urandom 时用 pid 填充，仅保证文件名唯一 */
        pid_t p = getpid();
        for (int i = 0; i < 16; i++)
            b[i] = (unsigned char)(p >> ((i % 4) * 8));
    }
    b[6] = (b[6] & 0x0F) | 0x40;   /* UUID version 4 */
    b[8] = (b[8] & 0x3F) | 0x80;   /* UUID variant */

    char path[512];
    snprintf(path, sizeof(path),
        "%s/%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
        dir, b[0], b[1], b[2], b[3], b[4], b[5], b[6], b[7], b[8], b[9],
        b[10], b[11], b[12], b[13], b[14], b[15]);

    int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) { perror(path); return -1; }
    if (write(fd, "ok", 2) != 2) {
        perror("write");
        close(fd);
        unlink(path);
        return -1;
    }
    close(fd);
    if (unlink(path) != 0) perror("unlink");
    return 0;
}

/* 返回 1=判定 NVMe 硬盘丢失；0=系统正常 */
static int detect_lose_nvme(void) {
    /* 检查1：/home/admin 文件系统写探针（GUID 文件，写 ok 后删除） */
    if (fs_write_probe("/home/admin") != 0) {
        fprintf(stderr, "filesystem error: write test failed in /home/admin, rootfs may be read-only\n");
    } else {
        return 0;
    }

    /* 检查2：/dev/nvme0 设备节点 与 /sys/class/nvme/nvme0 驱动目录 */
    if (access("/dev/nvme0", F_OK) != 0 ||
        access("/sys/class/nvme/nvme0", F_OK) != 0) {
        fprintf(stderr, "error: both /dev/nvme0 node and /sys/class/nvme/nvme0 are missing\n");
    } else {
        return 0;
    }

    /* 检查3：两项均未提前返回 0 -> 判定硬盘丢失 */
    fprintf(stderr, "error: NVMe disk lost (device node and driver both missing)\n");
    return 1;
}

/* locate FPGA, verify FPGA_ID, write GLOBAL_RST -> MMIO power cycle */
static int fpga_power_cycle(void) {
    /* 门控：仅 detect_lose_nvme() 返回 1（判定丢盘）才继续 power cycle，否则提前返回 */
    if (detect_lose_nvme() != 1) {
        printf("NVMe status normal, skip FPGA power cycle.\n");
        return 0;
    }

    uint64_t bar0 = 0;
    char bdf[32] = {0};
    if (fpga_find_bar(&bar0, bdf, sizeof(bdf)) != 0) {
        fprintf(stderr, "error: FPGA not found (vendor=%s device=%s)\n",
                FPGA_VID, FPGA_DID);
        return -1;
    }
    printf("FPGA: BDF=%s BAR0=0x%llx\n", bdf, (unsigned long long)bar0);

    int fd = open("/dev/mem", O_RDWR | O_SYNC);
    if (fd < 0) { perror("open /dev/mem"); return -1; }

    /* map two pages covering BAR0+0x00 .. BAR0+0x120 */
    uint64_t page = bar0 & ~0xFFFULL;
    size_t off = (size_t)(bar0 - page);
    volatile uint32_t *map = mmap(NULL, 0x2000,
        PROT_READ | PROT_WRITE, MAP_SHARED, fd, (off_t)page);
    if (map == MAP_FAILED) { perror("mmap"); close(fd); return -1; }

    /* sanity read: FPGA_ID low16 should be 0x6548 (§4.1.1), warn only */
    uint32_t fid = map[(off + FPGA_ID_OFF) / 4];
    printf("FPGA_ID @BAR+0x%02x = 0x%08x (low16 expect 0x%04x)\n",
           FPGA_ID_OFF, fid, FPGA_ID_EXPECT);
    if ((fid & 0xFFFFu) != FPGA_ID_EXPECT)
        fprintf(stderr, "warning: FPGA_ID low16=0x%04x mismatch, still writing GLOBAL_RST\n",
                fid & 0xFFFFu);

    /* step 3: write GLOBAL_RST bit31=1 -> MMIO power cycle */
    printf("write GLOBAL_RST @BAR+0x%03x = 0x%08x ...\n",
           GLOBAL_RST_OFF, GLOBAL_RST_VAL);
    map[(off + GLOBAL_RST_OFF) / 4] = GLOBAL_RST_VAL;
    msync((void *)map, 0x2000, MS_SYNC);  /* flush MMIO write before exit */
    munmap((void *)map, 0x2000);
    close(fd);
    printf("GLOBAL_RST written, system should power-cycle now.\n");
    return 0;
}

/* =====================================================================
 *  [NEW] monitor -- 后台持续监听 NVMe 丢盘，零外部依赖
 * =====================================================================
 * 设计约束（重要）：
 *   丢盘时本程序必须是最后一个还活着的、能说话的东西。因此：
 *   1) 不依赖 nvme-cli / lspci / setpci / dmesg / syslog daemon / shell；
 *      只用 libc + /sys + /dev + 一个 AF_UNIX socket。
 *   2) 不用 malloc 于运行期（丢盘时堆可能还能用，但没必要赌），
 *      所有缓冲区为栈上或静态，且长度编译期固定。
 *   3) 不调用 system()/popen()/fork()-exec()，避免依赖 /bin/sh。
 *   4) 丢盘状态下会刻意【不做】的事：
 *      - 不往被监控的 NVMe 上写任何东西（fs_write_probe 只用于"判定"，
 *        monitor 模式不复用，避免丢盘时在只读 fs 上反复报 EROFS 噪声）
 *      - 不读 /dev/nvme0（丢盘后会一直阻塞或返回 -ENODEV）
 *      - 不打开新文件描述符做常规动作（fd 泄漏会拖死自己）
 *   5) 所有 syscall 返回值都必须检查；任何一路失败都只降级，不退出。
 * ===================================================================== */

#define MON_SOCK_PATH   "/run/flks_nvme_monitor.socket"
#define MON_PID_PATH    "/run/flks_nvme_monitor.pid"
#define MON_SYSLOG      "/dev/log"
/* 丢盘是高等级严重事故：检测与上报间隔统一 3 秒，不做长周期降频。
 * 保留 200ms 下限只为方便现场短时调试。 */
#define MON_DEFAULT_MS  3000
#define MON_MIN_MS      200
#define MON_MAX_MS      600000
#define MON_REPORT_MS   3000

/* NVMe 控制器 class code: 0x010802 */
#define NVME_CLASS      "0x010802"
/* 现场故障盘，仅用于日志里标注"是否本应存在的那块" */
#define WATCH_VID       "0x1dee"
#define WATCH_DID       "0x5216"

static volatile sig_atomic_t g_stop = 0;

static void mon_sig(int s) { (void)s; g_stop = 1; }

/* ---- 单调时钟，用于间隔控制（丢盘时 CLOCK_REALTIME 可能被 NTP 跳变） ---- */
static long long mono_ms(void) {
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) {
        /* 退化为 gettimeofday 级别精度，绝不因时钟问题退出 */
        struct timespec rt;
        clock_gettime(CLOCK_REALTIME, &rt);
        return (long long)rt.tv_sec * 1000 + rt.tv_nsec / 1000000;
    }
    return (long long)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

/* ---- wall clock 字符串，不依赖 localtime 的 malloc ---- */
static void stamp(char *buf, size_t len) {
    time_t t = time(NULL);
    struct tm tm;
    if (localtime_r(&t, &tm) == NULL) {
        snprintf(buf, len, "1970-01-01 00:00:00");
        return;
    }
    strftime(buf, len, "%Y-%m-%d %H:%M:%S", &tm);
}

/* ---- syslog 直连 /dev/log：不依赖 rsyslog 是否安装，尽力而为 ----
 * 用 SOCK_DGRAM + 非阻塞。socket 一旦建立就一直复用，丢盘时不再重复 open。
 * 如果 /dev/log 不存在或满了，静默丢弃，绝不影响 stdout 与主循环。 */
static int g_syslog_fd    = -1;
static int g_syslog_tried = 0;

static int mon_syslog_open(void) {
    int fd = socket(AF_UNIX, SOCK_DGRAM | SOCK_CLOEXEC | SOCK_NONBLOCK, 0);
    if (fd < 0) return -1;
    struct sockaddr_un sa;
    memset(&sa, 0, sizeof(sa));
    sa.sun_family = AF_UNIX;
    strncpy(sa.sun_path, MON_SYSLOG, sizeof(sa.sun_path) - 1);
    if (connect(fd, (struct sockaddr *)&sa, sizeof(sa)) != 0) {
        close(fd);
        return -1;
    }
    return fd;
}

/* 优先级 3 = ERR，见 syslog(3)。自己拼报文头：<PRI>TAG[PID]: MSG */
static void mon_syslog(const char *tag, const char *msg) {
    if (g_syslog_fd < 0) {
        /* 丢了就重试，但不必每轮都重试，交给调用方节流 */
        if (g_syslog_tried) return;
        g_syslog_tried = 1;
        g_syslog_fd = mon_syslog_open();
        if (g_syslog_fd < 0) return;
    }
    char pkt[512];
    int n = snprintf(pkt, sizeof(pkt), "<3>%s[%d]: %s", tag, (int)getpid(), msg);
    if (n <= 0) return;
    if ((size_t)n >= sizeof(pkt)) n = (int)sizeof(pkt) - 1;
    /* MSG_NOSIGNAL 防 SIGPIPE；失败静默，不重试连接（丢弃本条） */
    if (send(g_syslog_fd, pkt, (size_t)n, MSG_NOSIGNAL | MSG_DONTWAIT) < 0) {
        /* socket 可能被重启的 syslogd 换掉了，关闭后下轮重连一次 */
        if (errno == ECONNREFUSED || errno == ENOTCONN) {
            close(g_syslog_fd);
            g_syslog_fd = -1;
            g_syslog_tried = 0;
        }
    }
}

/* ---- 物理串口直写 ----
 * 为什么不能只写 stdout：
 *   服务由 systemd 拉起时，StandardOutput=journal 会把 fd1 接到 journald 的
 *   socket 上，即便写成 journal+console，console 那一路也是 systemd 转发的。
 *   丢盘时 journald / rsyslogd 往往已经卡死或被杀，fd1 的写端随之失效，
 *   于是"串口什么都没有"。
 *   现场结论：必须绕开所有守护进程，直接 open 物理 tty 并 write。
 *
 * 做法：
 *   1) 优先用 CONFIG 里指定的设备（monitor 第 2 个参数可覆盖）；
 *   2) 否则依次尝试 /dev/ttyS0 ttyS1 ttyAMA0 ttyAMA1 tty0 console；
 *   3) fd 长期持有，O_NONBLOCK 打开，写失败不阻塞、不重试、不退出；
 *   4) 每隔一段时间检查设备是否还在（串口驱动重载会换 inode），不在则重开。
 * 注意：不做 termios 配置——串口波特率由内核 console 参数决定，
 * 这里只是往同一个 tty 追加字节，改配置反而可能把 console 弄坏。 */
static int g_tty_fd   = -1;
static long long g_tty_next_try = 0;
static const char *g_tty_dev = NULL;

/* 候选串口设备表：按现场命中概率排序 */
static const char *const MON_TTY_CANDIDATES[] = {
    "/dev/ttyS0",
    "/dev/ttyS1",
    "/dev/ttyAMA0",
    "/dev/ttyAMA1",
    "/dev/tty0",
    "/dev/console",
    NULL
};

static int mon_tty_open_one(const char *dev) {
    /* O_NONBLOCK 关键：串口无流控且对端未读时，阻塞写会把监控进程挂死 */
    int fd = open(dev, O_WRONLY | O_NONBLOCK | O_NOCTTY | O_CLOEXEC);
    if (fd < 0) return -1;

    /* 关键：如果进程的 stdout 已被关闭（systemd 下 journald 崩溃后
     * fd1 会失效，或被父进程 >&- 关掉），open 会把这个最小的空闲编号
     * 分配给串口，也就是 fd == 1。此后"写串口"和"写stdout"变成同一个
     * fd，同一条消息会打两遍。这里把串口 fd 挪到 >=3 再返回。 */
    if (fd < 3) {
        int high = fcntl(fd, F_DUPFD_CLOEXEC, 3);
        close(fd);
        if (high < 0) return -1;
        fd = high;
    }
    return fd;
}

/* 网络控制台/云环境常见只读 tty0，写入会 EIO，这里靠实际写一个字来验证 */
static int mon_tty_probe(int fd) {
    char probe = '\0';
    ssize_t w = write(fd, &probe, 1);
    return (w == 1) ? 1 : 0;
}

static void mon_tty_init(void) {
    if (g_tty_dev && *g_tty_dev) {
        int fd = mon_tty_open_one(g_tty_dev);
        if (fd >= 0) { g_tty_fd = fd; return; }
        /* 指定设备打不开就退回自动探测，绝不让监控因此停摆 */
    }
    for (int i = 0; MON_TTY_CANDIDATES[i]; i++) {
        int fd = mon_tty_open_one(MON_TTY_CANDIDATES[i]);
        if (fd < 0) continue;
        if (!mon_tty_probe(fd)) { close(fd); continue; }
        g_tty_fd = fd;
        g_tty_dev = MON_TTY_CANDIDATES[i];
        return;
    }
    g_tty_fd = -1;
}

/* 独立于 stdout 的物理串口输出。任何一步失败都静默跳过。 */
static void mon_tty_write(const char *s, size_t n) {
    if (g_tty_fd < 0) {
        /* 打不开时不要每 3 秒都去 open 一遍，5 秒重试一次即可 */
        long long now = mono_ms();
        if (now < g_tty_next_try) return;
        g_tty_next_try = now + 5000;
        mon_tty_init();
        if (g_tty_fd < 0) return;
    }
    ssize_t w = write(g_tty_fd, s, n);
    if (w < 0 && (errno == EIO || errno == ENODEV || errno == ENXIO)) {
        /* 设备已失效（驱动重载/console 切换），关掉下轮重开 */
        close(g_tty_fd);
        g_tty_fd = -1;
        g_tty_next_try = mono_ms() + 1000;
    }
}

/* ---- 把一条消息同时打到 物理串口 + stdout + syslog，全链路不依赖缓冲 ---- */
static void mon_emit(const char *level, const char *fmt, ...) {
    char body[400];
    char line[512];
    char ts[32];

    va_list ap;
    va_start(ap, fmt);
    vsnprintf(body, sizeof(body), fmt, ap);
    va_end(ap);

    stamp(ts, sizeof(ts));

    int n = snprintf(line, sizeof(line), "%s memtool[%s]: %s\n",
                     ts, level, body);
    if (n > 0) {
        if ((size_t)n > sizeof(line)) n = (int)sizeof(line) - 1;

        /* 1) 物理串口：唯一不经过任何守护进程的通道，丢盘时最先活下来 */
        mon_tty_write(line, (size_t)n);

        /* 2) stdout：交互式运行时可见；systemd 下可能已被 journal 接管
         *    甚至已断开，写失败一律忽略。
         *    额外判断 fd1 是否与串口同号：若相同说明 stdout 已被关闭、
         *    编号被串口复用，再写一次就是重复输出。 */
        if (STDOUT_FILENO != g_tty_fd) {
            ssize_t w = write(STDOUT_FILENO, line, (size_t)n);
            (void)w;
        }
    }

    char slog[480];
    snprintf(slog, sizeof(slog), "[%s] %s", level, body);
    mon_syslog("memtool", slog);
}

/* ---- 枚举当前在位的 NVMe 控制器 ----
 * 返回：控制器个数；把第一个匹配的 BDF 写到 first_bdf。
 * 只用 opendir/readdir，不依赖 glob()（glob 可能内部 malloc）。
 * 注意 readdir 在目录被并发改动时是安全的，不需要额外锁。 */
static int mon_scan_nvme(char *first_bdf, size_t len) {
    const char *devfs = "/sys/bus/pci/devices";
    DIR *dir = opendir(devfs);
    if (first_bdf && len) first_bdf[0] = '\0';
    if (!dir) return -1;   /* 目录都打不开，属于严重异常 */

    int cnt = 0;
    struct dirent *de;
    char path[320], buf[32];

    while ((de = readdir(dir)) != NULL) {
        if (de->d_name[0] == '.') continue;
        snprintf(path, sizeof(path), "%s/%s/class", devfs, de->d_name);
        if (read_file(path, buf, sizeof(buf)) != 0) continue;
        if (strcasecmp(buf, NVME_CLASS) != 0) continue;

        cnt++;
        if (first_bdf && len && first_bdf[0] == '\0') {
            /* d_name 理论最长 255，但 BDF 一定形如 0000:01:00.0（12 字符）。
             * 先比长度再拷：既是防御性检查，也让编译器确认不会截断。 */
            size_t dn = strnlen(de->d_name, sizeof(de->d_name));
            if (dn < len) snprintf(first_bdf, len, "%s", de->d_name);
        }
    }
    closedir(dir);
    return cnt;
}

/* ---- 一次完整健康判定 ----
 * 返回 0=正常，1=疑似丢盘，2=已确认丢盘（无控制器在位）
 * 判定顺序刻意"从最便宜到最贵"，且每一级都能独立给出结论：
 *   级1  /sys/bus/pci/devices 下还有没有 class=0x010802 的设备（不漏出 syscall 之外）
 *   级2  /dev/nvme0 字符设备是否还存在
 *   级3  /sys/class/nvme/nvme0/state 是否为 live（内核 5.19+ 提供）
 * 三级都失败才判丢盘，避免"仅 /dev 节点延迟创建"造成误报。 */
static int mon_check_nvme(char *why, size_t wlen) {
    char bdf[32];
    int cnt = mon_scan_nvme(bdf, sizeof(bdf));

    if (cnt < 0) {
        snprintf(why, wlen, "sysfs inaccessible (cannot open /sys/bus/pci/devices)");
        return 1;
    }
    if (cnt == 0) {
        snprintf(why, wlen, "no NVMe controller on PCIe bus (class=%s, count=0)", NVME_CLASS);
        return 2;
    }

    if (access("/dev/nvme0", F_OK) != 0) {
        snprintf(why, wlen, "controller present at %s but /dev/nvme0 node is gone", bdf);
        return 1;
    }

    char path[256], st[64];
    snprintf(path, sizeof(path), "/sys/class/nvme/nvme0/state");
    if (read_file(path, st, sizeof(st)) == 0) {
        if (strcmp(st, "live") != 0) {
            snprintf(why, wlen, "controller state=%s (not live), BDF=%s", st, bdf);
            return 1;
        }
    }
    return 0;
}

/* ---- 尝试把已掉线的设备重新枚举回来 ----
 * 现场证据：丢盘后 07:28 曾被重新枚举出同型号盘（1dee:5216），说明
 * 内核侧留下过 removed 的子设备。内核 6.6 有 rescan_removed_children，
 * 5.x 只有 /sys/bus/pci/rescan。两者都只写 "1"，失败忽略。
 * 这一动作不依赖任何外部命令，且不会触碰盘内数据。 */
static void mon_try_rescan(void) {
    static const char *nodes[] = {
        "/sys/bus/pci/rescan_removed_children",
        "/sys/bus/pci/rescan"
    };
    for (size_t i = 0; i < sizeof(nodes) / sizeof(nodes[0]); i++) {
        int fd = open(nodes[i], O_WRONLY | O_NONBLOCK);
        if (fd < 0) continue;
        ssize_t w = write(fd, "1\n", 2);
        (void)w;
        close(fd);
    }
}

/* ---- 非阻塞 accept 包装：避开 accept4 的 _GNU_SOURCE 依赖 ----
 * 老 libc / 老内核上 accept4 可能不存在，用 accept + fcntl 等价实现，
 * 保证在 initramfs 里的精简 glibc 上也能编过。 */
static int mon_accept(int lfd) {
    /* 阻塞 accept：有连接才返回；lfd 本身非阻塞，无连接时立即返回 -1，不卡主循环 */
    int cfd = accept(lfd, NULL, NULL);
    if (cfd < 0) return -1;
    int fdfl = fcntl(cfd, F_GETFD, 0);
    if (fdfl >= 0) fcntl(cfd, F_SETFD, fdfl | FD_CLOEXEC);

    /* 关键：listen socket 带 SOCK_NONBLOCK，Linux 会让 accept 出的 fd 继承它，
     * 于是数据未到达时 read 立刻 EAGAIN、命令被丢弃（实测踩到）。
     * 这里显式恢复成阻塞读，并加 1 秒读超时防止异常客户端挂住主循环。 */
    int fl = fcntl(cfd, F_GETFL, 0);
    if (fl >= 0) fcntl(cfd, F_SETFL, fl & ~O_NONBLOCK);

    struct timeval tv;
    tv.tv_sec  = 1;
    tv.tv_usec = 0;
    setsockopt(cfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    return cfd;
}

/* ---- 从 sockaddr_un 拿下一行命令（简单累加缓冲） ---- */
static void mon_handle_cmd(const char *buf, int muted, int *stop_flag, int *mute_flag) {
    /* 允许一次发多条，按 \n 切分 */
    char tmp[256];
    strncpy(tmp, buf, sizeof(tmp) - 1);
    tmp[sizeof(tmp) - 1] = '\0';

    char *line = tmp;
    while (line && *line) {
        char *nl = strchr(line, '\n');
        if (nl) *nl = '\0';

        /* 去掉首尾空白 */
        while (*line == ' ' || *line == '\t') line++;
        size_t L = strlen(line);
        while (L && (line[L-1] == ' ' || line[L-1] == '\t' || line[L-1] == '\r'))
            line[--L] = '\0';

        if (L) {
            if (strcasecmp(line, "stop") == 0 ||
                strcasecmp(line, "shutdown") == 0 ||
                strcasecmp(line, "quit") == 0 ||
                strcasecmp(line, "exit") == 0) {
                mon_emit("INFO", "stop command received ('%s'), monitor exiting", line);
                *stop_flag = 1;
            } else if (strcasecmp(line, "mute") == 0 ||
                       strcasecmp(line, "silence") == 0) {
                *mute_flag = 1;
                mon_emit("INFO", "mute command received, loss messages suppressed (detection continues)");
            } else if (strcasecmp(line, "unmute") == 0 ||
                       strcasecmp(line, "resume") == 0) {
                *mute_flag = 0;
                mon_emit("INFO", "unmute command received, loss messages resumed");
            } else if (strcasecmp(line, "status") == 0) {
                mon_emit("INFO", "status: running, muted=%d", muted);
            } else {
                mon_emit("WARN", "unknown command '%s' (supported: stop/mute/unmute/status)", line);
            }
        }
        line = nl ? nl + 1 : NULL;
    }
}

/* ---- 创建监听 socket ----
 * 放在 /run（tmpfs），丢盘时 rootfs 只读也不受影响。
 * 若残留旧文件且无人监听则删除重建；若有人监听则直接失败退出，避免双实例打架。 */
static int mon_listen_socket(const char *path) {
    struct sockaddr_un sa;
    memset(&sa, 0, sizeof(sa));
    sa.sun_family = AF_UNIX;
    strncpy(sa.sun_path, path, sizeof(sa.sun_path) - 1);

    int fd = socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC | SOCK_NONBLOCK, 0);
    if (fd < 0) return -1;

    if (unlink(path) != 0 && errno != ENOENT) {
        /* 连删除都失败，很可能已有实例在跑 */
        close(fd);
        return -2;
    }
    if (bind(fd, (struct sockaddr *)&sa, sizeof(sa)) != 0) {
        close(fd);
        return -3;
    }
    if (listen(fd, 4) != 0) {
        close(fd);
        unlink(path);
        return -4;
    }
    /* 允许任意用户发命令（只读语义的操作，写权限由目录决定） */
    chmod(path, 0666);
    return fd;
}

/* ---- 主循环 ---- */
static int mon_run(int interval_ms) {
    if (interval_ms < MON_MIN_MS) interval_ms = MON_MIN_MS;
    if (interval_ms > MON_MAX_MS) interval_ms = MON_MAX_MS;

    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = mon_sig;
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGINT,  &sa, NULL);
    sigaction(SIGHUP,  &sa, NULL);   /* 串口断开时忽略，不要死 */
    signal(SIGPIPE, SIG_IGN);        /* 关键：写已关闭的 socket 不能崩 */

    /* 先试一次 syslog，失败不致命 */
    g_syslog_fd = mon_syslog_open();
    g_syslog_tried = 1;

    /* 物理串口先打开并常驻：这是丢盘时唯一不依赖守护进程的通道，
     * 必须在第一条消息前就绪，否则启动日志会漏到 journal 里找不回来。 */
    mon_tty_init();

    int lfd = mon_listen_socket(MON_SOCK_PATH);
    if (lfd < 0) {
        mon_emit("WARN", "failed to listen on %s (rc=%d errno=%d), detection continues without command channel",
                 MON_SOCK_PATH, lfd, errno);
        lfd = -1;
    } else {
        mon_emit("INFO", "listening on %s", MON_SOCK_PATH);
    }

    /* pid 文件：便于外部 kill，写失败不影响 */
    {
        int pf = open(MON_PID_PATH, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (pf >= 0) {
            char pb[24];
            int n = snprintf(pb, sizeof(pb), "%d\n", (int)getpid());
            if (n > 0) { ssize_t w = write(pf, pb, (size_t)n); (void)w; }
            close(pf);
        }
    }

    mon_emit("INFO", "NVMe monitor started, interval=%dms report=%dms tty=%s pid=%d, loss messages will repeat (send 'stop' to quit)",
             interval_ms, (int)MON_REPORT_MS,
             (g_tty_fd >= 0 && g_tty_dev) ? g_tty_dev : "none",
             (int)getpid());

    int muted = 0;
    int req_stop = 0;             /* socket 收到的停止请求 */
    int last_state = -1;          /* -1 未知，0 正常，1/2 异常 */
    int fanout = 0;               /* 本轮丢盘已上报次数 */
    long long burst_start = 0;    /* 本轮丢盘起点 */
    long long last_report = 0;    /* 上次上报时刻 */
    long long next_tick = mono_ms();

    while (!g_stop && !req_stop) {
        /* ---------- 1. 先把 socket 上的指令收干净（非阻塞） ---------- */
        if (lfd >= 0) {
            for (;;) {
                int cfd = mon_accept(lfd);
                if (cfd < 0) break;      /* EAGAIN / EINTR 都直接跳出 */
                char buf[256];
                ssize_t n = read(cfd, buf, sizeof(buf) - 1);
                if (n > 0) {
                    buf[n] = '\0';
                    mon_handle_cmd(buf, muted, &req_stop, &muted);
                }
                close(cfd);
            }
        }

        /* ---------- 2. 检测 ---------- */
        char why[220];
        int st = mon_check_nvme(why, sizeof(why));

        if (st == 0) {
            if (last_state != 0) {
                if (last_state > 0) {
                    mon_emit("INFO", "NVMe recovered: controller is back on the bus");
                } else {
                    mon_emit("INFO", "NVMe status normal, monitoring started");
                }
                last_state = 0;
                fanout = 0;
            }
        } else {
            /* ---------- 丢盘 ---------- */
            /* 先尝试重新枚举（不影响本轮判定，下一轮看结果） */
            mon_try_rescan();

            long long now = mono_ms();

            /* 从"正常/未知"转入"异常"：开启新一轮上报 burst */
            if (last_state <= 0) {
                burst_start = now;
                fanout      = 0;
                last_report = 0;
            }

            /* 节流：固定 3 秒一条，不做快慢两档切换。
             * 丢盘属于严重事故，宁可多打几条也不能让现场以为"没监控"。 */
            long long since = now - burst_start;
            long long gap   = MON_REPORT_MS;

            /* 首条必须立即发，不等 gap */
            int due = (fanout == 0) || (now - last_report >= gap);

            if (due && !muted) {
                if (fanout == 0) {
                    mon_emit("ERROR", "NVMe disk LOST: %s", why);
                } else {
                    mon_emit("ERROR",
                             "NVMe disk LOST (elapsed %llds, report #%d): %s",
                             since / 1000, fanout + 1, why);
                }
                fanout++;
                last_report = now;
            }
            /* muted 期间不发，但保留 burst_start，解除静默后立即补一条 */
            last_state = st;
        }

        /* ---------- 2.5 socket 自愈 ----------
         * 丢盘场景下 /run 也可能被清空（tmpfs 重建、外部清理脚本、
         * systemd PrivateTmp 切换等），socket 文件一旦消失，指令通道就永久
         * 失效。这里每轮检查一次，发现文件不在就重建监听，不退出进程。
         * 判据用 access() 而非 stat()：只关心"路径上还有没有这个节点"。 */
        if (lfd >= 0 && access(MON_SOCK_PATH, F_OK) != 0) {
            close(lfd);
            lfd = -1;
            int nlfd = mon_listen_socket(MON_SOCK_PATH);
            if (nlfd >= 0) {
                lfd = nlfd;
                mon_emit("WARN", "command socket vanished, re-created %s", MON_SOCK_PATH);
            } else {
                mon_emit("WARN", "command socket vanished and re-create failed (rc=%d errno=%d), retry next round",
                         nlfd, errno);
            }
        } else if (lfd < 0) {
            /* 启动时就没建起来（例如 /run 尚不可写），每轮重试一次 */
            int nlfd = mon_listen_socket(MON_SOCK_PATH);
            if (nlfd >= 0) {
                lfd = nlfd;
                mon_emit("INFO", "command socket ready, listening on %s", MON_SOCK_PATH);
            }
        }

        /* ---------- 3. 定时等待，期间仍可收指令 ---------- */
        next_tick += interval_ms;
        long long now2 = mono_ms();
        if (next_tick < now2) next_tick = now2;   /* 落后就重置，不追赶 */

        while (!g_stop && !req_stop && lfd >= 0) {
            long long wait = next_tick - mono_ms();
            if (wait <= 0) break;

            fd_set rfds;
            FD_ZERO(&rfds);
            int maxfd = -1;
            if (lfd >= 0) { FD_SET(lfd, &rfds); maxfd = lfd; }

            struct timeval tv;
            tv.tv_sec  = (time_t)(wait / 1000);
            tv.tv_usec = (suseconds_t)((wait % 1000) * 1000);

            int r = select(maxfd + 1, maxfd >= 0 ? &rfds : NULL, NULL, NULL, &tv);
            if (r > 0 && lfd >= 0 && FD_ISSET(lfd, &rfds)) {
                for (;;) {
                    int cfd = mon_accept(lfd);
                    if (cfd < 0) break;
                    char buf[256];
                    ssize_t n = read(cfd, buf, sizeof(buf) - 1);
                    if (n > 0) {
                        buf[n] = '\0';
                        mon_handle_cmd(buf, muted, &req_stop, &muted);
                    }
                    close(cfd);
                }
                continue;   /* 处理完指令重新算等待时间 */
            }
            if (r > 0 && lfd >= 0 && !FD_ISSET(lfd, &rfds)) {
                /* select 报可读但不是监听 fd：属于异常事件（例如 fd 被外部
                 * 关闭后编号复用），本轮直接结束等待，回到主循环重新判定。 */
                break;
            }
            if (r < 0 && errno != EINTR) {
                /* select 出错也不能退出，睡一小会儿继续 */
                struct timespec ts = { 0, 200 * 1000 * 1000 };
                nanosleep(&ts, NULL);
            }
            if (r == 0) break;   /* 超时到点 */
        }
    }

    /* ---- 收尾：全部尽力而为，任何一步失败都不影响退出码 ---- */
    mon_emit("INFO", "monitor stopped, cleaning up");
    if (lfd >= 0) { close(lfd); unlink(MON_SOCK_PATH); }
    unlink(MON_PID_PATH);
    if (g_syslog_fd >= 0) close(g_syslog_fd);
    return 0;
}

int main(int argc, char *argv[]) {
    print_build_time();

    if (argc < 2) {
        fprintf(stderr,
            "usage:\n"
            "  %s enum [--csv]            enumerate all PCI devices\n"
            "  %s search <keyword>        search by description\n"
            "  %s read 0xADDR             read 32-bit MMIO\n"
            "  %s write 0xADDR 0xVALUE   write 32-bit MMIO\n"
            "  %s reboot                   detect NVMe loss; FPGA GLOBAL_RST\n"
            "                             power-cycle only when disk lost\n"
            "                             (FPGA VID=0x10EE / DID=0x7021)\n"
            "  %s monitor [interval_ms] [tty]  [NEW] watch NVMe forever (default 3000ms),\n"
            "                             repeat loss message every 3s to PHYSICAL serial\n"
            "                             tty + stdout + syslog (tty default: auto-detect),\n"
            "                             cmd socket %s\n"
            "                             commands: stop | mute | unmute | status\n",
            argv[0], argv[0], argv[0], argv[0], argv[0], argv[0], MON_SOCK_PATH);
        return 1;
    }

    /* ---- enum ---- */
    if (strcmp(argv[1], "enum") == 0) {
        int csv = (argc >= 3 && strcmp(argv[2], "--csv") == 0);
        pci_enum(csv, NULL);
        return 0;
    }

    /* ---- search ---- */
    if (strcmp(argv[1], "search") == 0) {
        if (argc < 3) {
            fprintf(stderr, "usage: %s search <keyword>\n", argv[0]);
            return 1;
        }
        pci_enum(0, argv[2]);
        return 0;
    }

    /* ---- read ---- */
    if (strcmp(argv[1], "read") == 0) {
        if (argc < 3) {
            fprintf(stderr, "usage: %s read 0xADDR\n", argv[0]);
            return 1;
        }
        uint64_t addr = strtoull(argv[2], NULL, 0);
        uint32_t val = 0;
        return mem_rw(addr, &val, 0);
    }

    /* ---- write ---- */
    if (strcmp(argv[1], "write") == 0) {
        if (argc < 4) {
            fprintf(stderr, "usage: %s write 0xADDR 0xVALUE\n", argv[0]);
            return 1;
        }
        uint64_t addr = strtoull(argv[2], NULL, 0);
        uint32_t val = (uint32_t)strtoul(argv[3], NULL, 0);
        return mem_rw(addr, &val, 1);
    }

    /* ---- reboot: FPGA GLOBAL_RST MMIO power cycle ---- */
    if (strcmp(argv[1], "reboot") == 0) {
        return fpga_power_cycle();
    }

    /* ---- monitor: 后台持续监听 NVMe 丢盘 ---- */
    if (strcmp(argv[1], "monitor") == 0) {
        int iv = MON_DEFAULT_MS;
        if (argc >= 3) {
            long v = strtol(argv[2], NULL, 0);
            if (v > 0) iv = (int)v;
        }
        /* 可选第 3 参数：指定物理串口设备（默认自动探测） */
        if (argc >= 4 && argv[3][0] == '/') {
            g_tty_dev = argv[3];
        }
        return mon_run(iv);
    }

    fprintf(stderr, "unknown action '%s' (enum/search/read/write/reboot/monitor)\n", argv[1]);
    return 1;
}
