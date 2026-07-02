#include  "update_header.h"



int parse_data_header(uint8_t *image, size_t size, uint32_t region)
{
    int offset = 0;
    struct header_s *header = NULL;
    struct item_s *item_info = NULL;

    if (image == NULL || size <= 0)
        return -EINVAL;

    header = (struct header_s *)image;
    if (header->magic != UPGRADE_MAGIC) {
        printf("please review this upgrade package!\n");
        return -EFAULT;
    }
    if (header->len + sizeof(struct header_s) != size)
        return -EFAULT;
    //目前item个数只有fpga的数据，支持后续扩展。
    if (header->num < 1) {
        printf("upgrade package data error!\n");
        return -EFAULT;
    }

    offset = sizeof(struct header_s);
    //crc校验预留

    //只有一个item，直接解析。
    item_info = (struct item_s *)(header->data);
    if (header->len != item_info->size + sizeof(struct item_s))
        return -EFAULT;
    //判断item的offset
    if (item_info->offset != offset)
        return -EFAULT;
    //是否是fpga升级包
    if (item_info->tag < TAG_FPGA_PRIMARY || item_info->tag > TAG_FPGA_BACKUP) {
        printf("not support this upgrade package!\n");
        return -EFAULT;
    }
    //判断主备是否一致
    if (!((region == 1 && item_info->tag == TAG_FPGA_PRIMARY) || (region == 0 && item_info->tag == TAG_FPGA_BACKUP))) {
        printf("not match, region:%s, file:%s\n", region ? "top" : "base", item_info->tag == 21 ? "top": "base");
        return -EFAULT;
    }
    offset += sizeof(struct item_s);

    return offset;
}