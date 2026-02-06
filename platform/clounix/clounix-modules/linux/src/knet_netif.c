#include "knet_dev.h"
#include "knet_pci.h"
#include "knet_fault_event.h"
#include <linux/slab.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <uapi/linux/ethtool.h>
#include <linux/ethtool.h>
#include <linux/jiffies.h>
#include <linux/rtnetlink.h>
#include <linux/ip.h>
#include <linux/udp.h>
#include <linux/tcp.h>
#include <net/checksum.h>

static const char *action_str[] = {"UNKNOWN",    "ACTION_NETDEV",   "ACTION_NETLINK",
                                   "ACTION_SDK", "ACTION_FAST_FWD", "ACTION_DROP"};

static const char *match_type_str[] = {"Don't Care", "Pattern"};

static unsigned char stp_mac[ETH_ALEN] = {0x01, 0x80, 0xc2, 0x00, 0x00, 0x00};
static unsigned char pvst_mac[ETH_ALEN] = {0x01, 0x00, 0x0c, 0xcc, 0xcc, 0xcd};
static clx_netif_ifa_cfg_t ifa_cfg = {0};

static void
print_profile_reason(clx_rx_rsn_bmp_t reason_bitmap)
{
    uint32_t reason = 0;
    bool first_bit = true;

    CLX_REASON_BITMAP_FOREACH(reason_bitmap, reason)
    {
        if (first_bit) {
            pr_info("            %u", reason);
            first_bit = false;
        } else {
            pr_info("            %u", reason);
        }
    }
    pr_info("\n");
}

static void
print_profile_rule(struct profile_rule *rule)
{
    int i, j;

    if (!(verbosity & DBG_PROFILE)) {
        return;
    }

    if (!rule) {
        pr_err("Invalid profile_rule pointer\n");
        return;
    }

    pr_info("Profile Rule:\n");
    pr_info("  ID: %u\n", rule->id);
    pr_info("  Name: %s\n", rule->name);
    pr_info("  Priority: %u\n", rule->priority);

    pr_info("  Match Reason:\n");
    pr_info("    Match Type: %s\n", match_type_str[rule->match_reason.match_type]);
    pr_info("    Reason: ");
    print_profile_reason(rule->match_reason.reason_bitmap);

    pr_info("  Match Port:\n");
    pr_info("    Match Type: %s\n", match_type_str[rule->match_port.match_type]);
    pr_info("    Port_di: %u\n", rule->match_port.port_di);

    for (i = 0; i < MAX_PATTERN_NUM; i++) {
        pr_info("  Match Pattern [%d]:\n", i);
        pr_info("    Match Type: %s\n", match_type_str[rule->match_pattern[i].match_type]);
        pr_info("    Offset: %u\n", rule->match_pattern[i].offset);

        pr_info("    Pattern: ");
        for (j = 0; j < MAX_PATTERN_LENGTH; j++) {
            pr_cont("%02x ", rule->match_pattern[i].pattern[j]);
        }
        pr_cont("\n");

        pr_info("    Mask: ");
        for (j = 0; j < MAX_PATTERN_LENGTH; j++) {
            pr_cont("%02x ", rule->match_pattern[i].mask[j]);
        }
        pr_cont("\n");
    }

    pr_info("  Action: %s\n", action_str[rule->action]);

    /* print netlink */
    if (rule->action == ACTION_NETLINK) {
        pr_info("  Netlink:\n");
        pr_info("    Family name: %s\n", rule->netlink.family_name);
        pr_info("    Group name: %s\n", rule->netlink.mc_grp_name);
    }
}

void
print_packet(uint32_t loglvl, const unsigned char *data, size_t len)
{
    size_t i, j;
    char ascii[17];
    ascii[16] = '\0';

    if (!(loglvl & verbosity & DBG_RX_PAYLOAD) && !(loglvl & verbosity & DBG_TX_PAYLOAD)) {
        return;
    }
    dbg_print(DBG_INFO, "Dump %s Packet  (len=%zu):\n", (loglvl == DBG_RX_PAYLOAD) ? "Rx" : "Tx",
              len);

    for (i = 0; i < len; i += 16) {
        // print offset
        printk(KERN_CONT "%08zx  ", i);

        // print hex
        for (j = 0; j < 16; j++) {
            if (i + j < len) {
                printk(KERN_CONT "%02x ", data[i + j]);
                ascii[j] = (data[i + j] >= 32 && data[i + j] <= 126) ? data[i + j] : '.';
            } else {
                printk(KERN_CONT "   ");
                ascii[j] = ' ';
            }

            if (j == 7)
                printk(KERN_CONT " ");
        }

        printk(KERN_CONT " |%s|\n", ascii);
    }
}

void
clx_netif_performance_test_rx(uint32_t unit, struct dma_rx_packet *rx_packet)
{
    netif_perf_t *ptr_perf_test = &clx_misc_dev->test_perf;
    unsigned long time_diff;
    uint64_t pps, bps;

    // Start the timer when packet length is TARGET_PACKET_LEN and initialize stats
    if ((rx_packet->packet_len - clx_dma_drv(unit)->dma_hdr_size) != ptr_perf_test->target_len) {
        return;
    }

    if (ptr_perf_test->packet_count == 0) {
        printk("test rx start. unit=%u\n", unit);
        ptr_perf_test->start_time = jiffies;
        ptr_perf_test->interrupt_count = 0;
    }

    // Update performance stats
    ptr_perf_test->packet_count++;
    ptr_perf_test->interrupt_count++; // Track interrupt count

    // Check if the test is complete
    if (ptr_perf_test->packet_count < ptr_perf_test->target_count) {
        return;
    }

    ptr_perf_test->end_time = jiffies; // Stop timer
    printk("test rx done. unit=%u\n", unit);

    // Calculate time difference in milliseconds
    time_diff = jiffies_to_msecs(ptr_perf_test->end_time - ptr_perf_test->start_time);

    ptr_perf_test->byte_count = ptr_perf_test->target_count * ptr_perf_test->target_len;
    // Calculate packets per second (pps) and bits per second (bps)
    pps = (ptr_perf_test->packet_count * 1000) / time_diff;
    bps = (ptr_perf_test->byte_count * 8 * 1000) / time_diff;

    printk("Performance Test Results:\n");
    printk("Packet length: %d\n", ptr_perf_test->target_len);
    printk("Packets per second (pps): %llu\n", pps);
    printk("Bits per second (bps): %llu\n", bps);
    printk("Packet count: %llu\n", ptr_perf_test->packet_count);
    printk("Interrupt count: %llu\n", ptr_perf_test->interrupt_count);

    // Reset stats after test
    memset(ptr_perf_test, 0x0, sizeof(netif_perf_t));

    return;
}

void
clx_netif_performance_test_tx(uint32_t unit, struct sk_buff *ptr_skb)
{
    netif_perf_t *ptr_perf_test = &clx_misc_dev->test_perf;
    unsigned long time_diff;
    uint64_t pps, bps;

    if (ptr_perf_test->tx_enable_test == 0) {
        return;
    }

    if (ptr_perf_test->packet_count == 0) {
        printk("test tx start. unit=%u\n", unit);
        ptr_perf_test->start_time = jiffies;
        ptr_perf_test->interrupt_count = 0;
    }

    // Update performance stats
    ptr_perf_test->packet_count++;
    ptr_perf_test->interrupt_count++; // Track interrupt count

    // Check if the test is complete
    if (ptr_perf_test->packet_count < ptr_perf_test->target_count) {
        return;
    }

    ptr_perf_test->end_time = jiffies; // Stop timer
    printk("test tx done. unit=%u\n", unit);

    // Calculate time difference in milliseconds
    time_diff = jiffies_to_msecs(ptr_perf_test->end_time - ptr_perf_test->start_time);

    ptr_perf_test->byte_count = ptr_perf_test->target_count * ptr_perf_test->target_len;
    // Calculate packets per second (pps) and bits per second (bps)
    pps = (ptr_perf_test->packet_count * 1000) / time_diff;
    bps = (ptr_perf_test->byte_count * 8 * 1000) / time_diff;

    printk("Performance Test Results:\n");
    printk("Packet length: %d\n", ptr_perf_test->target_len);
    printk("Packets per second (pps): %llu\n", pps);
    printk("Bits per second (bps): %llu\n", bps);
    printk("Packet count: %llu\n", ptr_perf_test->packet_count);
    printk("Interrupt count: %llu\n", ptr_perf_test->interrupt_count);

    // Reset stats after test
    memset(ptr_perf_test, 0x0, sizeof(netif_perf_t));

    return;
}

static int
clx_netif_net_dev_init(struct net_device *ptr_net_dev)
{
    return 0;
}

static int
clx_netif_net_dev_open(struct net_device *ptr_net_dev)
{
    netif_start_queue(ptr_net_dev);
    return 0;
}

static int
clx_netif_net_dev_stop(struct net_device *ptr_net_dev)
{
    netif_stop_queue(ptr_net_dev);
    return 0;
}

static int
clx_netif_net_dev_ioctl(struct net_device *ptr_net_dev, struct ifreq *ptr_ifreq, int cmd)
{
    return 0;
}

static netdev_tx_t
clx_netif_net_dev_tx(struct sk_buff *ptr_skb, struct net_device *ptr_net_dev)
{
    struct net_device_priv *ptr_priv = NULL;
    uint32_t unit, channel;
    void *ptr_virt_addr;
    uint32_t pkt_len = 0;
    uint32_t headroom = 0;
    struct sk_buff *new_skb;

    /* check skb */
    if (NULL == ptr_skb) {
        dbg_print(DBG_ERR, "skb is NULL\n");
        return NETDEV_TX_OK;
    }

    if (ptr_net_dev == NULL) {
        dbg_print(DBG_ERR, "net_device is NULL\n");
        dev_kfree_skb_any(ptr_skb);
        return NETDEV_TX_OK;
    }

    ptr_priv = netdev_priv(ptr_net_dev);
    if (ptr_priv == NULL) {
        dbg_print(DBG_ERR, "get netdev_priv failed\n");
        dev_kfree_skb_any(ptr_skb);
        return NETDEV_TX_OK;
    }

    if (!netif_running(ptr_net_dev)) {
        dbg_print(DBG_ERR, "net_device is not running\n");
        dev_kfree_skb_any(ptr_skb);
        return NETDEV_TX_OK;
    }

    unit = ptr_priv->unit;
    channel = ptr_priv->tx_channel;
    new_skb = skb_clone(ptr_skb, GFP_ATOMIC);
    if (!new_skb) {
        pr_err("skb_clone failed. unit=%u\n", unit);
        return NETDEV_TX_BUSY;
    }
    /* pad to 60-bytes if skb_len < 60, see: eth_skb_pad(skb) */
    if (new_skb->len < ETH_ZLEN) {
        skb_pad(new_skb, ETH_ZLEN - ptr_skb->data_len);
        new_skb->len = ETH_ZLEN;
    }

    /* reserve headroom for pdma header */
    headroom = skb_headroom(new_skb);
    if (!IS_ALIGNED(headroom, 4) || (headroom < clx_dma_drv(unit)->dma_hdr_size)) {
        if (headroom < clx_dma_drv(unit)->dma_hdr_size) {
            headroom = clx_dma_drv(unit)->dma_hdr_size - headroom;
        } else {
            headroom = SKB_DATA_ALIGN(headroom) - headroom;
        }
        if (pskb_expand_head(new_skb, headroom, 0, GFP_ATOMIC)) {
            dbg_print(DBG_ERR, "Failed to expand skb headroom:%d. unit=%u\n", headroom, unit);
            ptr_priv->stats.tx_errors++;
            kfree_skb(new_skb);
            return NETDEV_TX_BUSY;
        }
    }

    /* pad 4-bytes for chip-crc */
    skb_pad(new_skb, ETH_FCS_LEN);
    new_skb->len += ETH_FCS_LEN;
    skb_set_tail_pointer(new_skb, new_skb->len);
    pkt_len = new_skb->len;

    /* push pdma header size */
    skb_push(new_skb, clx_dma_drv(unit)->dma_hdr_size);
    ptr_virt_addr = (void *)new_skb->data;
    /* should clear */
    memset(ptr_virt_addr, 0x0, clx_dma_drv(unit)->dma_hdr_size);

    if (!IS_ALIGNED((unsigned long)new_skb->data, 4)) {
        dbg_print(DBG_ERR, "tx err, skb data addr:0x%lx, not align 4bytes. unit=%u\n",
                  (unsigned long)new_skb->data, unit);
    }

    // must prepare pph before mapdma
    clx_dma_drv(unit)->prepare_pph(unit, ptr_priv->port_di, ptr_priv->tc, ptr_virt_addr);

    dbg_print(
        DBG_TX,
        "netdev:%s bind port_di:%d, data_len:%d len:%d pkt_len:%d, new_skb->data:%lx. unit=%u\n",
        ptr_priv->ptr_net_dev->name, ptr_priv->port_di, new_skb->data_len, new_skb->len, pkt_len,
        (unsigned long)new_skb->data, unit);

    print_packet(DBG_TX_PAYLOAD, new_skb->data, new_skb->len);
#if LINUX_VERSION_CODE <= KERNEL_VERSION(4, 6, 7)
    ptr_net_dev->trans_start = jiffies;
#else
    netdev_get_tx_queue(ptr_net_dev, 0)->trans_start = jiffies;
#endif

    if (clx_misc_dev->test_perf.tx_enable_test == 1) {
        clx_netif_performance_test_tx(unit, new_skb);
    }

    if (0 == clx_dma_drv(unit)->tx_packet(unit, channel, new_skb)) {
        ptr_priv->stats.tx_packets += 1;
        ptr_priv->stats.tx_bytes += pkt_len;
    } else {
        ptr_priv->stats.tx_fifo_errors++; /* to record the extreme cases where packets are
                                           * dropped
                                           */
        ptr_priv->stats.tx_dropped++;
        kfree_skb(new_skb);
        return NETDEV_TX_BUSY;
    }

    dev_kfree_skb_any(ptr_skb);
    return NETDEV_TX_OK;
}

netdev_tx_t
clx_netif_fast_tx(uint32_t unit, struct sk_buff *ptr_skb, uint16_t di, uint8_t tc)
{
    uint32_t channel = 4; // default channel
    void *ptr_virt_addr;
    uint32_t pkt_len = 0;
    uint32_t headroom = 0;
    struct sk_buff *new_skb;

    /* check skb */
    if (NULL == ptr_skb) {
        dbg_print(DBG_ERR, "skb is NULL\n");
        return NETDEV_TX_OK;
    }

    new_skb = skb_clone(ptr_skb, GFP_ATOMIC);
    if (!new_skb) {
        pr_err("skb_clone failed\n");
        return NETDEV_TX_BUSY;
    }
    /* pad to 60-bytes if skb_len < 60, see: eth_skb_pad(skb) */
    if (new_skb->len < ETH_ZLEN) {
        skb_pad(new_skb, ETH_ZLEN - ptr_skb->data_len);
        new_skb->len = ETH_ZLEN;
    }

    /* reserve headroom for pdma header */
    headroom = skb_headroom(new_skb);
    if (!IS_ALIGNED(headroom, 4) || (headroom < clx_dma_drv(unit)->dma_hdr_size)) {
        if (headroom < clx_dma_drv(unit)->dma_hdr_size) {
            headroom = clx_dma_drv(unit)->dma_hdr_size - headroom;
        } else {
            headroom = SKB_DATA_ALIGN(headroom) - headroom;
        }
        if (pskb_expand_head(new_skb, headroom, 0, GFP_ATOMIC)) {
            dbg_print(DBG_ERR, "Failed to expand skb headroom:%d\n", headroom);
            kfree_skb(new_skb);
            dev_kfree_skb_any(ptr_skb);
            return NETDEV_TX_BUSY;
        }
    }

    /* pad 4-bytes for chip-crc */
    skb_pad(new_skb, ETH_FCS_LEN);
    new_skb->len += ETH_FCS_LEN;
    skb_set_tail_pointer(new_skb, new_skb->len);
    pkt_len = new_skb->len;

    /* push pdma header size */
    skb_push(new_skb, clx_dma_drv(unit)->dma_hdr_size);
    ptr_virt_addr = (void *)new_skb->data;
    /* should clear */
    memset(ptr_virt_addr, 0x0, clx_dma_drv(unit)->dma_hdr_size);

    if (!IS_ALIGNED((unsigned long)new_skb->data, 4)) {
        dbg_print(DBG_ERR, "tx err, skb data addr:0x%lx, not align 4bytes\n",
                  (unsigned long)new_skb->data);
    }

    // must prepare pph before mapdma
    clx_dma_drv(unit)->prepare_pph(unit, di, tc, ptr_virt_addr);

    dbg_print(DBG_TX,
              "port_di:%d, data_len:%d len:%d pkt_len:%d, new_skb->data:%lx\n",
              di, new_skb->data_len, new_skb->len,
              pkt_len, (unsigned long)new_skb->data);

    print_packet(DBG_TX_PAYLOAD, new_skb->data, new_skb->len);

    if (0 != clx_dma_drv(unit)->tx_packet(unit, channel, new_skb)) {
        kfree_skb(new_skb);
        dev_kfree_skb_any(ptr_skb);
        return NETDEV_TX_BUSY;
    }

    dev_kfree_skb_any(ptr_skb);
    return NETDEV_TX_OK;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0)
static void
clx_netif_net_dev_tx_timeout(struct net_device *ptr_net_dev, unsigned int txqueue)
#else
static void
clx_netif_net_dev_tx_timeout(struct net_device *ptr_net_dev)
#endif
{
    netif_stop_queue(ptr_net_dev);
    usleep_range(100, 1000);
    netif_wake_queue(ptr_net_dev);
}

static struct net_device_stats *
clx_netif_net_dev_get_stats(struct net_device *ptr_net_dev)
{
    struct net_device_priv *ptr_priv = netdev_priv(ptr_net_dev);

    return (&ptr_priv->stats);
}

static int
clx_netif_net_dev_set_mtu(struct net_device *ptr_net_dev, int new_mtu)
{
    struct net_device_priv *ptr_priv = netdev_priv(ptr_net_dev);
    if (new_mtu < 64 || new_mtu > ptr_priv->max_mtu) {
        return -EINVAL;
    }
    ptr_net_dev->mtu = new_mtu;
    return 0;
}
static int
clx_netif_net_dev_set_mac(struct net_device *ptr_net_dev, void *ptr_mac_addr)
{
    struct sockaddr *ptr_addr = ptr_mac_addr;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0)
    eth_hw_addr_set(ptr_net_dev, ptr_addr->sa_data);
#else
    memcpy((void *)ptr_net_dev->dev_addr, ptr_addr->sa_data, ptr_net_dev->addr_len);
#endif
    return 0;
}

static void
clx_netif_net_dev_set_rx_mode(struct net_device *ptr_dev)
{}

static int clx_netif_net_dev_get_link_ksettings(struct net_device *ptr_net_dev,
    struct ethtool_link_ksettings *cmd)
{
    struct net_device_priv *ptr_priv = netdev_priv(ptr_net_dev);

    //ethtool_link_ksettings_add_link_mode(cmd, advertising, 400000baseDR4_Full);
    cmd->base.speed = ptr_priv->speed;

    cmd->base.duplex = ptr_priv->duplex;
    // cmd->base.autoneg = AUTONEG_ENABLE;
    //cmd->base.port = PORT_OTHER;

    return 0;
}

static struct net_device_ops clx_netif_net_dev_ops = {
    .ndo_init = clx_netif_net_dev_init,
    .ndo_open = clx_netif_net_dev_open,
    .ndo_stop = clx_netif_net_dev_stop,
    .ndo_do_ioctl = clx_netif_net_dev_ioctl,
    .ndo_start_xmit = clx_netif_net_dev_tx,
    .ndo_tx_timeout = clx_netif_net_dev_tx_timeout,
    .ndo_get_stats = clx_netif_net_dev_get_stats,
    .ndo_change_mtu = clx_netif_net_dev_set_mtu,
    .ndo_set_mac_address = clx_netif_net_dev_set_mac,
    .ndo_set_rx_mode = clx_netif_net_dev_set_rx_mode,
};

static const struct ethtool_ops clx_netif_net_dev_ethtool_ops = {
    .get_link_ksettings = clx_netif_net_dev_get_link_ksettings,
};

static void
clx_netif_setup(struct net_device *ptr_net_dev)
{
    struct net_device_priv *ptr_priv = netdev_priv(ptr_net_dev);

    /* setup net device */
    ether_setup(ptr_net_dev);
    ptr_net_dev->netdev_ops = &clx_netif_net_dev_ops;
    ptr_net_dev->ethtool_ops = &clx_netif_net_dev_ethtool_ops;
    ptr_net_dev->watchdog_timeo = 30 * HZ;
    ptr_net_dev->mtu = 1500;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 19, 0)
    ptr_net_dev->min_mtu = 64;
    ptr_net_dev->max_mtu = 65535;
#endif
    eth_hw_addr_random(ptr_net_dev);
    // random_ether_addr(ptr_net_dev->dev_addr); /* Please use the mac-addr of interface. */

    /* setup private data */
    ptr_priv->ptr_net_dev = ptr_net_dev;
    memset(&ptr_priv->stats, 0, sizeof(struct net_device_stats));
}

static int
clx_netif_di2id_bind(uint32_t unit, uint32_t port_di, uint32_t netif_id)
{
    clx_netif_drv(unit)->netif_di2id_map[port_di] = netif_id;
    return 0;
}

uint32_t
clx_netif_di2id_lookup(uint32_t unit, uint32_t port_di)
{
    return clx_netif_drv(unit)->netif_di2id_map[port_di];
}

static void
clx_netif_di2id_unbind(uint32_t unit, uint32_t port_di)
{
    clx_netif_drv(unit)->netif_di2id_map[port_di] = -1;
}

int
clx_netif_net_dev_create(uint32_t unit, unsigned long arg)
{
    struct net_device *ptr_net_dev = NULL;
    struct net_device_priv *ptr_priv = NULL;
    struct clx_netif_ioctl_intf netif_cookie;
    uint32_t netif_id = 0;

    if (copy_from_user(&netif_cookie, (void __user *)arg, sizeof(struct clx_netif_ioctl_intf)))
        return -EFAULT;

    if (netif_cookie.port_di >= CLX_NETIF_PORT_DI_MAX_NUM) {
        dbg_print(DBG_INTF, "Invalid port_di%d. unit=%u\n", netif_cookie.port_di, unit);
        return -EINVAL;
    }

    netif_id = clx_netif_di2id_lookup(unit, netif_cookie.port_di);
    if (netif_id != -1) {
        dbg_print(DBG_INTF, "port_di %d already exists. unit=%u\n", netif_cookie.port_di, unit);
        return -EEXIST;
    }

    ptr_net_dev = dev_get_by_name(&init_net, netif_cookie.name);
    if (ptr_net_dev) {
        dev_put(ptr_net_dev);
        return -EEXIST;
    }

    // alloc netif id
    netif_cookie.id = find_first_zero_bit(clx_netif_drv(unit)->netif_id_bitmap, CLX_NETIF_MAX_NUM);
    if (netif_cookie.id == CLX_NETIF_MAX_NUM) {
        dbg_print(DBG_PROFILE, "No available netif id. unit=%u\n", unit);
        unregister_netdev(ptr_net_dev);
        return -ENOSPC;
    }
    set_bit(netif_cookie.id, clx_netif_drv(unit)->netif_id_bitmap);
    clx_netif_di2id_bind(unit, netif_cookie.port_di, netif_cookie.id);

    ptr_net_dev = alloc_netdev(sizeof(struct net_device_priv), netif_cookie.name, NET_NAME_USER,
                               clx_netif_setup);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0)
    eth_hw_addr_set(ptr_net_dev, netif_cookie.mac);
#else
    memcpy((void *)ptr_net_dev->dev_addr, netif_cookie.mac, ptr_net_dev->addr_len);
#endif
    ptr_priv = netdev_priv(ptr_net_dev);
    ptr_net_dev->mtu = clx_netif_drv(unit)->mtu;
    ptr_net_dev->needed_headroom = clx_dma_drv(unit)->dma_hdr_size;

    /* Port info will be used when packet sent from this netdev */
    ptr_priv->port_di = netif_cookie.port_di;
    ptr_priv->id = netif_cookie.id;
    ptr_priv->unit = unit;
    ptr_priv->tx_channel = 4;
    ptr_priv->max_mtu = clx_netif_drv(unit)->mtu;
    ptr_priv->speed = SPEED_1000; // default speed.
    ptr_priv->duplex = DUPLEX_FULL; // default duplex.

    register_netdev(ptr_net_dev);
    netif_carrier_off(ptr_net_dev);

    if (copy_to_user((void __user *)arg, &netif_cookie, sizeof(struct clx_netif_ioctl_intf))) {
        return -EFAULT;
    }

    clx_netif_drv(unit)->netif_db[netif_cookie.id].intf.id = netif_cookie.id;
    clx_netif_drv(unit)->netif_db[netif_cookie.id].ptr_net_dev = ptr_net_dev;
    dbg_print(DBG_INTF, "Network device %s created, port_di:%d, ifindex:%d. unit=%u\n",
              ptr_net_dev->name, netif_cookie.port_di, ptr_net_dev->ifindex, unit);

    return 0;
}

int
clx_netif_net_dev_destroy(uint32_t unit, unsigned long arg)
{
    struct net_device *ptr_net_dev = NULL;
    struct net_device_priv *ptr_priv = NULL;
    uint32_t intf_id;
    int netif_id = 0;
    uint32_t port_di;

    if (copy_from_user(&intf_id, (void __user *)arg, sizeof(uint32_t)))
        return -EFAULT;

    for (netif_id = 0; netif_id < CLX_NETIF_MAX_NUM; netif_id++) {
        if ((clx_netif_drv(unit)->netif_db[netif_id].intf.id == intf_id) &&
            (clx_netif_drv(unit)->netif_db[netif_id].ptr_net_dev != NULL)) {
            dbg_print(DBG_INTF, "Delete network device %s. unit=%u\n",
                      clx_netif_drv(unit)->netif_db[netif_id].ptr_net_dev->name, unit);
            ptr_net_dev = clx_netif_drv(unit)->netif_db[netif_id].ptr_net_dev;
            ptr_priv = netdev_priv(ptr_net_dev);
            port_di = ptr_priv->port_di;

            /* stop traffic and tx quque */
            netif_device_detach(ptr_net_dev);
            netif_tx_disable(ptr_net_dev);
            netif_carrier_off(ptr_net_dev);

            /* ungregister netdev */
            unregister_netdev(ptr_net_dev);
            clx_netif_drv(unit)->netif_db[netif_id].ptr_net_dev = NULL;
            clx_netif_di2id_unbind(unit, port_di);
            clear_bit(netif_id, clx_netif_drv(unit)->netif_id_bitmap);
            break;
        }
    }

    return 0;
}

int
clx_netif_net_dev_set(uint32_t unit, unsigned long arg)
{
    struct clx_netif_ioctl_intf netif_cookie;
    int netif_id = 0;
    struct net_device *ptr_net_dev;
    struct net_device_priv *ptr_priv = NULL;

    if (copy_from_user(&netif_cookie, (void __user *)arg, sizeof(struct clx_netif_ioctl_intf)))
        return -EFAULT;

    for (netif_id = 0; netif_id < CLX_NETIF_MAX_NUM; netif_id++) {
        if ((clx_netif_drv(unit)->netif_db[netif_id].intf.id == netif_cookie.id) &&
            (clx_netif_drv(unit)->netif_db[netif_id].ptr_net_dev != NULL)) {
            ptr_net_dev = clx_netif_drv(unit)->netif_db[netif_id].ptr_net_dev;
            ptr_priv = netdev_priv(ptr_net_dev);

            ether_addr_copy((u8 *)ptr_net_dev->dev_addr, netif_cookie.mac);
            if (netif_cookie.port_di != ptr_priv->port_di) {
                if (netif_cookie.port_di >= CLX_NETIF_PORT_DI_MAX_NUM ||
                    ptr_priv->port_di >= CLX_NETIF_PORT_DI_MAX_NUM) {
                    dbg_print(DBG_ERR, "Invalid port_di%d or %d. unit=%u\n", netif_cookie.port_di,
                              ptr_priv->port_di, unit);
                    return -EINVAL;
                }
                clx_netif_di2id_unbind(unit, ptr_priv->port_di);
                clx_netif_di2id_bind(unit, netif_cookie.port_di, netif_cookie.id);
            }
            ptr_priv->port_di = netif_cookie.port_di;
        }
    }

    if (copy_to_user((void __user *)arg, &netif_cookie, sizeof(struct clx_netif_ioctl_intf)))
        return -EFAULT;
    return 0;
}

int
clx_netif_get_netdev(uint32_t unit, unsigned long arg)
{
    struct clx_netif_ioctl_intf netif_cookie;
    struct net_device *ptr_net_dev;
    struct net_device_priv *ptr_priv = NULL;
    bool found = false;
    int netif_id = 0;

    if (copy_from_user(&netif_cookie, (void __user *)arg, sizeof(struct clx_netif_ioctl_intf)))
        return -EFAULT;

    for (netif_id = 0; netif_id < CLX_NETIF_MAX_NUM; netif_id++) {
        if ((clx_netif_drv(unit)->netif_db[netif_id].intf.id == netif_cookie.id) &&
            (clx_netif_drv(unit)->netif_db[netif_id].ptr_net_dev != NULL)) {
            ptr_net_dev = clx_netif_drv(unit)->netif_db[netif_id].ptr_net_dev;
            ptr_priv = netdev_priv(ptr_net_dev);
            memcpy(netif_cookie.mac, ptr_net_dev->dev_addr, ptr_net_dev->addr_len);
            memcpy(netif_cookie.name, ptr_net_dev->name, CLX_NETIF_NAME_LEN);
            netif_cookie.port_di = ptr_priv->port_di;
            found = true;
            break;
        }
    }
    if (found) {
        if (copy_to_user((void __user *)arg, &netif_cookie, sizeof(struct clx_netif_ioctl_intf)))
            return -EFAULT;
        dbg_print(DBG_INTF, "Found net device %s success. port_di:%u. unit=%u\n", ptr_net_dev->name,
                  ptr_priv->port_di, unit);
    } else {
        dbg_print(DBG_INTF, "Netdevice ID %u not found. unit=%u\n", netif_cookie.id, unit);
        netif_cookie.rc = CLX_IOCTL_E_ENTRY_NOT_FOUND;
        if (copy_to_user((void __user *)arg, &netif_cookie, sizeof(struct clx_netif_ioctl_intf)))
            return -EFAULT;
        return 0;
    }

    return 0;
}

int
clx_netif_get_netdev_cnt(uint32_t unit, unsigned long arg)
{
    struct clx_netif_netdev_cnt cnt_cookie;
    struct net_device *ptr_net_dev;
    struct net_device_priv *ptr_priv = NULL;
    bool found = false;
    int netif_id = 0;

    if (copy_from_user(&cnt_cookie, (void __user *)arg, sizeof(struct clx_netif_netdev_cnt)))
        return -EFAULT;

    for (netif_id = 0; netif_id < CLX_NETIF_MAX_NUM; netif_id++) {
        if ((clx_netif_drv(unit)->netif_db[netif_id].intf.id == cnt_cookie.intf_id) &&
            (clx_netif_drv(unit)->netif_db[netif_id].ptr_net_dev != NULL)) {
            ptr_net_dev = clx_netif_drv(unit)->netif_db[netif_id].ptr_net_dev;
            ptr_priv = netdev_priv(ptr_net_dev);
            cnt_cookie.rx_pkt = ptr_priv->stats.rx_packets;
            cnt_cookie.tx_pkt = ptr_priv->stats.tx_packets;
            cnt_cookie.tx_err = ptr_priv->stats.tx_errors;
            cnt_cookie.tx_queue_full = ptr_priv->stats.rx_fifo_errors;
            found = true;
            break;
        }
    }

    if (found) {
        if (copy_to_user((void __user *)arg, &cnt_cookie, sizeof(struct clx_netif_netdev_cnt))) {
            dbg_print(DBG_ERR, "Failed to copy netdev cnt to user space. unit=%u\n", unit);
            return -EFAULT;
        }
    } else {
        dbg_print(DBG_INTF, "Netdevice ID %u not found. unit=%u\n", cnt_cookie.intf_id, unit);
        cnt_cookie.rc = CLX_IOCTL_E_ENTRY_NOT_FOUND;
        if (copy_to_user((void __user *)arg, &cnt_cookie, sizeof(struct clx_netif_netdev_cnt))) {
            dbg_print(DBG_ERR, "Failed to copy netdev cnt to user space. unit=%u\n", unit);
            return -EFAULT;
        }
        return 0;
    }

    return 0;
}

int
clx_netif_clear_netdev_cnt(uint32_t unit, unsigned long arg)
{
    uint32_t intf_id;
    struct net_device *ptr_net_dev;
    struct net_device_priv *ptr_priv = NULL;
    int netif_id = 0;

    if (copy_from_user(&intf_id, (void __user *)arg, sizeof(uint32_t)))
        return -EFAULT;

    for (netif_id = 0; netif_id < CLX_NETIF_MAX_NUM; netif_id++) {
        if ((clx_netif_drv(unit)->netif_db[netif_id].intf.id == intf_id) &&
            (clx_netif_drv(unit)->netif_db[netif_id].ptr_net_dev != NULL)) {
            ptr_net_dev = clx_netif_drv(unit)->netif_db[netif_id].ptr_net_dev;
            ptr_priv = netdev_priv(ptr_net_dev);
            ptr_priv->stats.rx_packets = 0;
            ptr_priv->stats.tx_packets = 0;
            ptr_priv->stats.tx_errors = 0;
            ptr_priv->stats.rx_fifo_errors = 0;
            break;
        }
    }

    return 0;
}

int
clx_netif_set_ifa_cfg(uint32_t unit, unsigned long arg)
{
    if (copy_from_user(&ifa_cfg, (void __user *)arg, sizeof(clx_netif_ifa_cfg_t))) {
        dbg_print(DBG_ERR, "Failed to copy ifa_cfg from user space\n");
        return -EFAULT;
    }
    return 0;
}

int
clx_netif_get_ifa_cfg(uint32_t unit, unsigned long arg)
{
    if (copy_to_user((void __user *)arg, &ifa_cfg, sizeof(clx_netif_ifa_cfg_t))) {
        dbg_print(DBG_ERR, "Failed to copy ifa_cfg to user space\n");
        return -EFAULT;
    }
    return 0;
}

static void
clx_netif_net_dev_destroy_all(uint32_t unit)
{
    int netif_id = 0;
    struct net_device *ptr_net_dev = NULL;
    struct net_device_priv *ptr_priv = NULL;
    uint32_t port_di;
    for (netif_id = 0; netif_id < CLX_NETIF_MAX_NUM; netif_id++) {
        if (clx_netif_drv(unit)->netif_db[netif_id].ptr_net_dev != NULL) {
            ptr_net_dev = clx_netif_drv(unit)->netif_db[netif_id].ptr_net_dev;
            ptr_priv = netdev_priv(ptr_net_dev);
            port_di = ptr_priv->port_di;

            /* stop DMA */
            clx_dma_drv(unit)->disable_channel(unit, ptr_priv->tx_channel);
            /* stop traffic and tx quque */
            netif_device_detach(ptr_net_dev);
            netif_tx_disable(ptr_net_dev);
            netif_carrier_off(ptr_net_dev);
            /* ungregister netdev */
            unregister_netdev(ptr_net_dev);
            clx_netif_drv(unit)->netif_db[netif_id].ptr_net_dev = NULL;
            clx_netif_di2id_unbind(unit, port_di);
            clear_bit(netif_id, clx_netif_drv(unit)->netif_id_bitmap);
        }
    }
}

int
clx_netif_set_intf_attr(uint32_t unit, unsigned long arg)
{
    struct clx_netif_ioctl_port_attr netif_cookie;
    struct net_device *ptr_net_dev = NULL;
    struct net_device_priv *ptr_priv = NULL;
    uint32_t netif_id = 0;
    int rc = 0;

    if (copy_from_user(&netif_cookie, (void __user *)arg, sizeof(struct clx_netif_ioctl_port_attr)))
        return -EFAULT;

    if (netif_cookie.port_di >= CLX_NETIF_PORT_DI_MAX_NUM) {
        dbg_print(DBG_INTF, "Invalid port_di%d. unit=%u\n", netif_cookie.port_di, unit);
        return -EINVAL;
    }

    netif_id = clx_netif_di2id_lookup(unit, netif_cookie.port_di);
    if (netif_id == -1) {
        dbg_print(DBG_INTF, "invalid port_di,port_di=%d. unit=%u\n", netif_cookie.port_di, unit);
        return -EINVAL;
    }

    ptr_net_dev = clx_netif_drv(unit)->netif_db[netif_id].ptr_net_dev;
    if (NULL == ptr_net_dev) {
        dbg_print(DBG_INFO, "ptr_net_dev is null pointer,port_di=%d. unit=%u\n",
                  netif_cookie.port_di, unit);
        return -EINVAL;
    }

    rtnl_lock();
    if (NETIF_OPER_STATE_UP == netif_cookie.oper_state) {
        dbg_print(DBG_INTF, "set intf %s link up. unit=%u\n", ptr_net_dev->name, unit);
        netif_carrier_on(ptr_net_dev);
    } else {
        dbg_print(DBG_INTF, "set intf %s link down. unit=%u\n", ptr_net_dev->name, unit);
        netif_carrier_off(ptr_net_dev);
    }
    rtnl_unlock();

    /* Link speed config */
    ptr_priv = netdev_priv(ptr_net_dev);
    switch (netif_cookie.speed) {
        case NETIF_PORT_SPEED_1G:
            ptr_priv->speed = SPEED_1000;
            break;
        case NETIF_PORT_SPEED_10G:
            ptr_priv->speed = SPEED_10000;
            break;
        case NETIF_PORT_SPEED_25G:
            ptr_priv->speed = SPEED_25000;
            break;
        case NETIF_PORT_SPEED_40G:
            ptr_priv->speed = SPEED_40000;
            break;
        case NETIF_PORT_SPEED_50G:
            ptr_priv->speed = SPEED_50000;
            break;
        case NETIF_PORT_SPEED_100G:
            ptr_priv->speed = SPEED_100000;
            break;
        case NETIF_PORT_SPEED_200G:
            ptr_priv->speed = 200000;
            break;
        case NETIF_PORT_SPEED_400G:
            ptr_priv->speed = 400000;
            break;
        case NETIF_PORT_SPEED_800G:
            ptr_priv->speed = 800000;
            break;
        default:
            dbg_print(DBG_ERR, "Failed to set speed %d to %s. unit=%u\n", ptr_priv->speed,
                      ptr_net_dev->name, unit);
            rc = -EFAULT;
            break;
    }

    ptr_priv->igr_sample_rate = netif_cookie.igr_sample_rate;
    ptr_priv->egr_sample_rate = netif_cookie.egr_sample_rate;
    ptr_priv->skip_port_state_event = netif_cookie.skip_port_state_event;
    ptr_priv->vlan_tag_type = netif_cookie.vlan_tag_type;
    ptr_priv->tc = netif_cookie.tc;

    return rc;
}

int
clx_netif_get_intf_attr(uint32_t unit, unsigned long arg)
{
    struct clx_netif_ioctl_port_attr netif_cookie;
    struct net_device *ptr_net_dev = NULL;
    struct net_device_priv *ptr_priv = NULL;
    bool status;
    int rc = 0;
    uint32_t netif_id = 0;

    if (copy_from_user(&netif_cookie, (void __user *)arg, sizeof(struct clx_netif_ioctl_port_attr)))
        return -EFAULT;

    if (netif_cookie.port_di >= CLX_NETIF_PORT_DI_MAX_NUM) {
        dbg_print(DBG_INTF, "Invalid port_di%d. unit=%u\n", netif_cookie.port_di, unit);
        return -EINVAL;
    }

    netif_id = clx_netif_di2id_lookup(unit, netif_cookie.port_di);
    if (netif_id == -1) {
        dbg_print(DBG_INTF, "invalid port_di,port_di=%d. unit=%u\n", netif_cookie.port_di, unit);
        return -EINVAL;
    }

    ptr_net_dev = clx_netif_drv(unit)->netif_db[netif_id].ptr_net_dev;
    if (!ptr_net_dev) {
        dbg_print(DBG_INFO, "ptr_net_dev is null pointer,port_di=%d. unit=%u\n",
                  netif_cookie.port_di, unit);
        return -ENODEV;
    }

    status = netif_carrier_ok(ptr_net_dev);
    if (!status) {
        netif_cookie.oper_state = NETIF_OPER_STATE_DOWN;
    } else {
        netif_cookie.oper_state = NETIF_OPER_STATE_UP;
    }

    ptr_priv = netdev_priv(ptr_net_dev);
    switch (ptr_priv->speed) {
        case SPEED_1000:
            netif_cookie.speed = NETIF_PORT_SPEED_1G;
            break;
        case SPEED_10000:
            netif_cookie.speed = NETIF_PORT_SPEED_10G;
            break;
        case SPEED_25000:
            netif_cookie.speed = NETIF_PORT_SPEED_25G;
            break;
        case SPEED_40000:
            netif_cookie.speed = NETIF_PORT_SPEED_40G;
            break;
        case SPEED_50000:
            netif_cookie.speed = NETIF_PORT_SPEED_50G;
            break;
        case SPEED_100000:
            netif_cookie.speed = NETIF_PORT_SPEED_100G;
            break;
        case 200000:
            netif_cookie.speed = NETIF_PORT_SPEED_200G;
            break;
        case 400000:
            netif_cookie.speed = NETIF_PORT_SPEED_400G;
            break;
        case 800000:
            netif_cookie.speed = NETIF_PORT_SPEED_800G;
            break;
        default:
            dbg_print(DBG_INFO, "Failed to get %s speed. unit=%u\n", ptr_net_dev->name, unit);
            break;
    }

    netif_cookie.igr_sample_rate = ptr_priv->igr_sample_rate;
    netif_cookie.egr_sample_rate = ptr_priv->egr_sample_rate;
    netif_cookie.skip_port_state_event = ptr_priv->skip_port_state_event;
    netif_cookie.vlan_tag_type = ptr_priv->vlan_tag_type;
    netif_cookie.tc = ptr_priv->tc;

    if (copy_to_user((void __user *)arg, &netif_cookie, sizeof(struct clx_netif_ioctl_port_attr)))
        return -EFAULT;

    return rc;
}

struct sk_buff *
netif_construct_skb_from_rx_packet(uint32_t unit, uint32_t port_di, struct dma_rx_packet *rx_packet)
{
    struct net_device *ptr_net_dev = NULL;
    struct dma_rx_frag_buffer *rx_frag, *tmp;
    struct sk_buff *ptr_skb, *ptr_merge_skb = NULL;
    struct net_device_priv *ptr_priv = NULL;
    uint32_t copy_len = 0;
    uint32_t netif_id = 0;
    if (port_di >= CLX_NETIF_PORT_DI_MAX_NUM) {
        dbg_print(DBG_INFO, "invalid port_di,port_di=%d. unit=%u\n", port_di, unit);
        return NULL;
    }

    netif_id = clx_netif_di2id_lookup(unit, port_di);
    if (netif_id == -1) {
        dbg_print(DBG_RX, "invalid port_di,port_di=%d. unit=%u\n", port_di, unit);
        return NULL;
    }
    ptr_net_dev = clx_netif_drv(unit)->netif_db[netif_id].ptr_net_dev;
    if (!ptr_net_dev) {
        dbg_print(DBG_RX, "ptr_net_dev is null pointer, netif_id=%d port_di=%d. unit=%u\n", netif_id, port_di, unit);
        return NULL;
    }

    ptr_priv = netdev_priv(ptr_net_dev);

    dbg_print(DBG_RX, "rx_frag, rx_complete:%d, list_count:%d, packet_len:%d. unit=%u\n",
              rx_packet->rx_complete, rx_packet->list_count, rx_packet->packet_len, unit);
    if (rx_packet->list_count > 1) {
        ptr_merge_skb = dev_alloc_skb(rx_packet->packet_len + NET_IP_ALIGN);
        if (NULL == ptr_merge_skb) {
            dbg_print(DBG_CRIT, "Failed to allocate ptr_merge_skb\n");
            knet_fault_event_report(KNET_FAULT_EVENT_KENT_DMA_ALLOC_FAIL);
            return NULL;
        }
        /* Reserve 2 bytes to align IP header */
        skb_reserve(ptr_merge_skb, NET_IP_ALIGN);
        skb_put(ptr_merge_skb, rx_packet->packet_len);

        list_for_each_entry_safe(rx_frag, tmp, &rx_packet->rx_frag, rx_frag)
        {
            ptr_skb = rx_frag->ptr_skb;
            /* Add data to skb and adjust tail and len */
            memcpy(&(((uint8_t *)ptr_merge_skb->data)[copy_len]), ptr_skb->data, ptr_skb->len);
            copy_len += ptr_skb->len;
        }
    } else {
        rx_frag = list_first_entry(&rx_packet->rx_frag, struct dma_rx_frag_buffer, rx_frag);
        ptr_merge_skb = rx_frag->ptr_skb;
    }

    /* Remove DMA header by adjusting data pointer */
    skb_pull(ptr_merge_skb, clx_dma_drv(unit)->dma_hdr_size);
    skb_set_mac_header(ptr_merge_skb, 0);
    ptr_merge_skb->len = rx_packet->packet_len - clx_dma_drv(unit)->dma_hdr_size;

    ptr_merge_skb->dev = ptr_net_dev;
    ptr_merge_skb->pkt_type = PACKET_HOST;
    ptr_merge_skb->ip_summed = CHECKSUM_NONE;

    /* strip CRC padded by asic for the last gpd segment */
    ptr_merge_skb->len -= ETH_FCS_LEN;
    skb_set_tail_pointer(ptr_merge_skb, ptr_merge_skb->len);

    dbg_print(
        DBG_RX,
        "ptr_skb->head:0x%lx, data:0x%lx, end:%d, tail:%d, len:%d, data_len:%d, truesize:%d. unit=%u\n",
        (unsigned long)ptr_merge_skb->head, (unsigned long)ptr_merge_skb->data, ptr_merge_skb->end,
        ptr_merge_skb->tail, ptr_merge_skb->len, ptr_merge_skb->data_len, ptr_merge_skb->truesize,
        unit);

    print_packet(DBG_RX_PAYLOAD, ptr_merge_skb->data, ptr_merge_skb->len);

    /* update netdevice rx counter */
    ptr_priv = netdev_priv(ptr_net_dev);
    ptr_priv->stats.rx_packets++;
    ptr_priv->stats.rx_bytes += ptr_merge_skb->len;

    return ptr_merge_skb;
}

struct sk_buff *
netif_construct_fast_skb_from_rx_packet(uint32_t unit, struct dma_rx_packet *rx_packet)
{
    struct dma_rx_frag_buffer *rx_frag, *tmp;
    struct sk_buff *ptr_skb, *ptr_merge_skb = NULL;
    uint32_t copy_len = 0;

    dbg_print(DBG_RX, "rx_frag, rx_complete:%d, list_count:%d, packet_len:%d\n",
              rx_packet->rx_complete, rx_packet->list_count, rx_packet->packet_len);
    if (rx_packet->list_count > 1) {
        ptr_merge_skb = dev_alloc_skb(rx_packet->packet_len + NET_IP_ALIGN);
        if (NULL == ptr_merge_skb) {
            dbg_print(DBG_CRIT, "Failed to allocate ptr_merge_skb\n");
            knet_fault_event_report(KNET_FAULT_EVENT_KENT_DMA_ALLOC_FAIL);
            return NULL;
        }
        /* Reserve 2 bytes to align IP header */
        skb_reserve(ptr_merge_skb, NET_IP_ALIGN);
        skb_put(ptr_merge_skb, rx_packet->packet_len);

        list_for_each_entry_safe(rx_frag, tmp, &rx_packet->rx_frag, rx_frag)
        {
            ptr_skb = rx_frag->ptr_skb;
            /* Add data to skb and adjust tail and len */
            memcpy(&(((uint8_t *)ptr_merge_skb->data)[copy_len]), ptr_skb->data, ptr_skb->len);
            copy_len += ptr_skb->len;
        }
    } else {
        rx_frag = list_first_entry(&rx_packet->rx_frag, struct dma_rx_frag_buffer, rx_frag);
        ptr_merge_skb = rx_frag->ptr_skb;
    }

    /* Remove DMA header by adjusting data pointer */
    skb_pull(ptr_merge_skb, clx_dma_drv(unit)->dma_hdr_size);
    ptr_merge_skb->len = rx_packet->packet_len - clx_dma_drv(unit)->dma_hdr_size;

    ptr_merge_skb->pkt_type = PACKET_HOST;
    ptr_merge_skb->ip_summed = CHECKSUM_NONE;

    /* strip CRC padded by asic for the last gpd segment */
    ptr_merge_skb->len -= ETH_FCS_LEN;
    skb_set_tail_pointer(ptr_merge_skb, ptr_merge_skb->len);

    dbg_print(
        DBG_RX,
        "ptr_skb->head:0x%lx, data:0x%lx, end:%d, tail:%d, len:%d, data_len:%d, truesize:%d\n",
        (unsigned long)ptr_merge_skb->head, (unsigned long)ptr_merge_skb->data, ptr_merge_skb->end,
        ptr_merge_skb->tail, ptr_merge_skb->len, ptr_merge_skb->data_len, ptr_merge_skb->truesize);

    print_packet(DBG_RX_PAYLOAD, ptr_merge_skb->data, ptr_merge_skb->len);

    return ptr_merge_skb;
}

void
netif_process_skb_from_rx_packet(uint32_t unit,
                                 uint32_t port_di,
                                 struct dma_rx_packet *rx_packet,
                                 struct sk_buff *ptr_merge_skb)
{
    struct ethhdr *ether = NULL;
    struct net_device *ptr_net_dev = NULL;
    struct net_device_priv *ptr_priv = NULL;
    struct rx_pph_info *ptr_pph_info = &rx_packet->pph_info;
    uint32_t netif_id = 0;

    if (port_di >= CLX_NETIF_PORT_DI_MAX_NUM) {
        dbg_print(DBG_INFO, "invalid port_di,port_di=%d. unit=%u\n", port_di, unit);
        return;
    }

    netif_id = clx_netif_di2id_lookup(unit, port_di);
    if (netif_id == -1) {
        dbg_print(DBG_RX, "invalid port_di,port_di=%d. unit=%u\n", port_di, unit);
        return;
    }

    ptr_net_dev = clx_netif_drv(unit)->netif_db[netif_id].ptr_net_dev;
    if (!ptr_net_dev) {
        dbg_print(DBG_ERR, "ptr_net_dev is null pointer,port_di=%d. unit=%u\n", port_di, unit);
        return;
    }

    ptr_merge_skb->protocol = eth_type_trans(ptr_merge_skb, ptr_net_dev);
    ptr_priv = netdev_priv(ptr_net_dev);

    /* handle vlan tag */
    ether = eth_hdr(ptr_merge_skb);
    if (skb_mac_header_was_set(ptr_merge_skb)) {
        if (ether_addr_equal(stp_mac, ether->h_dest) || ether_addr_equal(pvst_mac, ether->h_dest)) {
            if (ETH_P_8021Q == ntohs(ether->h_proto) || ETH_P_8021AD == ntohs(ether->h_proto)) {
                dbg_print(DBG_RX, "unit=%u, frame already have vlan tag, no need insert.\n", unit);
            } else {
                skb_vlan_push(ptr_merge_skb, htons(ETH_P_8021Q), ptr_pph_info->vlan_tag);
                dbg_print(DBG_RX, "unit=%u, force add vlan tag, vlan_tag=%u.\n", unit,
                          ptr_pph_info->vlan_tag);
            }
        } else {
            if (ETH_P_8021Q == ntohs(ether->h_proto) || ETH_P_8021AD == ntohs(ether->h_proto)) {
                if (NETIF_VLAN_TAG_TYPE_STRIP == ptr_priv->vlan_tag_type) {
                    skb_push(ptr_merge_skb, ETH_HLEN);
                    while (ptr_pph_info->vlan_pop_num) {
                        skb_vlan_pop(ptr_merge_skb);
                        ptr_pph_info->vlan_pop_num--;
                    }
                    dbg_print(DBG_RX, "unit=%u, frame have vlan tag, strip all vlan tag.\n", unit);
                }
            } else if (NETIF_VLAN_TAG_TYPE_KEEP == ptr_priv->vlan_tag_type) {
                skb_vlan_push(ptr_merge_skb, htons(ETH_P_8021Q), ptr_pph_info->vlan_tag);
                dbg_print(DBG_RX, "unit=%u, keep vlan tag, vlan_tag=%u.\n", unit,
                          ptr_pph_info->vlan_tag);
            }
        }
    }
    else {
        dbg_print(DBG_WARN, "u=%u, pkt mac not set\n", unit);
    }

    return;
}

int
clx_netif_netdev_receive_skb(uint32_t unit, struct dma_rx_packet *rx_packet, uint32_t port_di)
{
    struct sk_buff *ptr_skb = NULL;

    if (clx_misc_dev->test_perf.rx_enable_test == 1) {
        clx_netif_performance_test_rx(unit, rx_packet);
    }

    ptr_skb = netif_construct_skb_from_rx_packet(unit, port_di, rx_packet);
    if (!ptr_skb) {
        dbg_print(DBG_RX, "Failed to construct skb. unit=%u\n", unit);
        return -EFAULT;
    }

    netif_process_skb_from_rx_packet(unit, port_di, rx_packet, ptr_skb);

    netif_receive_skb(ptr_skb);

    if (rx_packet->list_count > 1) {
        dma_free_rx_packet(unit, rx_packet, true);
    } else {
        dma_free_rx_packet(unit, rx_packet, false);
    }

    return 0;
}

struct net_device *
clx_netif_get_netdev_from_sys_port(uint32_t unit, uint16_t sys_port)
{
    uint8_t port_num = 0;
    uint8_t slice_id = 0;
    uint32_t port_di = 0;
    int32_t netif_id = 0;

    port_num = (sys_port & 0x3F);
    slice_id = ((sys_port >> 6) & 0x7);


    port_di = CLX_NETIF_GET_PORT_DI(unit, slice_id, port_num);
    if (port_di >= CLX_NETIF_PORT_DI_MAX_NUM) {
        dbg_print(DBG_ERR, "Invalid destination port number or slice id, port_num=%d, slice_id=%d\n",
                  port_num, slice_id);
        return NULL;
    }

    netif_id = clx_netif_di2id_lookup(unit, port_di);
    if (netif_id == -1) {
        dbg_print(DBG_ERR, "Invalid netif_id, port_di(%d), port_num(%d), slice_id(%d)\n",
                  port_di, port_num, slice_id);
        return NULL;
    }

    dbg_print(DBG_RX, "sysport(0x%4x) netif_id(%d), port_di(%d)\n", sys_port, netif_id, port_di);
    return clx_netif_drv(unit)->netif_db[netif_id].ptr_net_dev;
}

int
clx_netif_netdev_receive_send_ifa(uint32_t unit, struct dma_rx_packet *rx_packet, uint32_t port_di)
{
    struct sk_buff *ptr_skb = NULL;
    uint8_t *ifa_md_start;
    __be16 eth_proto = 0;
    int ip_offset = ETH_HLEN; // IP header offset in packet
    struct ethhdr *eth = NULL;
    struct iphdr *iph = NULL;
    struct udphdr *udph = NULL;
    struct tcphdr *tcph = NULL;
    struct ifa_header *ifa_hdr = NULL;
    struct ifa_metadata *ifa_md = NULL;
    uint16_t ip_len = 0;
    uint16_t tcp_len = 0;
    uint16_t igr_port_di = 0;
    uint16_t egr_port_di = 0;
    uint8_t ifa_meda_hdr_len = 4;
    uint8_t tc_from_queue_id = 0;
    uint8_t tcp_hdr_len = 0;
    uint16_t min_len = 0;

    if (clx_misc_dev->test_perf.rx_enable_test == 1) {
        clx_netif_performance_test_rx(unit, rx_packet);
    }

    ptr_skb = netif_construct_fast_skb_from_rx_packet(unit, rx_packet);
    if (!ptr_skb) {
        dbg_print(DBG_ERR, "Failed to construct skb.\n");
        return -EFAULT;
    }

    if (ptr_skb->len < ETH_HLEN) {
        dbg_print(DBG_ERR, "Packet too short, len:%d\n", ptr_skb->len);
        goto FREE_MERGED_SKB;
    }
    eth = (struct ethhdr *)ptr_skb->data;
    eth_proto = eth->h_proto;

    if (eth_proto != htons(ETH_P_IP)) {
        dbg_print(DBG_ERR, "Invalid IP packet, proto:0x%x\n", ntohs(eth_proto));
        goto FREE_MERGED_SKB;
    }

    if (ptr_skb->len < ETH_HLEN + sizeof(struct iphdr)) {
        dbg_print(DBG_ERR, "Packet too short for IP header, len:%d\n", ptr_skb->len);
        goto FREE_MERGED_SKB;
    }
    iph = (struct iphdr *)(ptr_skb->data + ip_offset);
    if (iph->ihl < 5 || ptr_skb->len < ETH_HLEN + (iph->ihl * 4)) {
        dbg_print(DBG_ERR, "Invalid IP header length, ihl:%d, len:%d\n", iph->ihl, ptr_skb->len);
        goto FREE_MERGED_SKB;
    }
    if (iph->protocol != ifa_cfg.ip_prot) {
        dbg_print(DBG_ERR, "Invalid IFA packet, proto:0x%x\n", iph->protocol);
        goto FREE_MERGED_SKB;
    }
    ifa_hdr = (struct ifa_header *)((char *)iph + (iph->ihl * 4));
    if (ifa_hdr->version != 2) {
        dbg_print(
            DBG_ERR,
            "Invalid IFA packet, iph->ihl:0x%x, ifa_hdr->version:0x%x, ifa_hdr->next_hdr:0x%x, ifa_hdr->gns:0x%x\n",
            iph->ihl, ifa_hdr->version, ifa_hdr->next_hdr, ifa_hdr->gns);
        goto FREE_MERGED_SKB;
    }
    if (ifa_hdr->next_hdr == IPPROTO_UDP) {
        udph = (struct udphdr *)((char *)ifa_hdr + sizeof(struct ifa_header));
        ifa_md_start = (uint8_t *)((char *)udph + sizeof(struct udphdr) + ifa_meda_hdr_len);
    } else if (ifa_hdr->next_hdr == IPPROTO_TCP) {
        tcph = (struct tcphdr *)((char *)ifa_hdr + sizeof(struct ifa_header));
        if (tcph->doff < 5) {
            dbg_print(DBG_ERR, "Invalid TCP header length, doff:%d\n", tcph->doff);
            goto FREE_MERGED_SKB;
        }
        tcp_hdr_len = tcph->doff * 4;
        if (ptr_skb->len < ETH_HLEN + (iph->ihl * 4) + tcp_hdr_len) {
            dbg_print(DBG_ERR, "Packet too short for TCP header, len:%d, iph_len:%d, tcp_hdr_len:%d\n", ptr_skb->len, iph->ihl * 4, tcp_hdr_len);
            goto FREE_MERGED_SKB;
        }
        ifa_md_start = (uint8_t *)((char *)tcph + tcp_hdr_len + ifa_meda_hdr_len);
    } else {
        dbg_print(DBG_ERR, "Invalid IFA packet, next_hdr:0x%x\n", ifa_hdr->next_hdr);
        goto FREE_MERGED_SKB;
    }
    ifa_md = (struct ifa_metadata *)ifa_md_start;
    dbg_print(DBG_RX, "ifa_md raw data: %02x %02x %02x %02x %02x %02x %02x %02x\n",
              ((uint8_t*)ifa_md)[0], ((uint8_t*)ifa_md)[1], ((uint8_t*)ifa_md)[2], ((uint8_t*)ifa_md)[3],
              ((uint8_t*)ifa_md)[4], ((uint8_t*)ifa_md)[5], ((uint8_t*)ifa_md)[6], ((uint8_t*)ifa_md)[7]);
    
    dbg_print(DBG_RX, "ifa_md first 32bit: 0x%08x, second 32bit: 0x%08x\n",
              ntohl(ifa_md->first_field), ntohl(ifa_md->second_field));

    /* Modify the ingress port and egress port as system interface index */
    igr_port_di = CLX_NETIF_GET_PORT_DI(unit, ((ntohs(ifa_md->igr_sys_port) >> 6) & 0x7), (ntohs(ifa_md->igr_sys_port) & 0x3F));
    if (igr_port_di >= CLX_NETIF_PORT_DI_MAX_NUM) {
        dbg_print(DBG_ERR, "Invalid ingress destination port number %04x\n", ntohs(ifa_md->igr_sys_port));
        goto FREE_MERGED_SKB;
    }
    ifa_md->igr_sys_port =  htons(igr_port_di);
    egr_port_di = CLX_NETIF_GET_PORT_DI(unit, ((ntohs(ifa_md->egr_sys_port) >> 6) & 0x7), (ntohs(ifa_md->egr_sys_port) & 0x3F));
    if (egr_port_di >= CLX_NETIF_PORT_DI_MAX_NUM) {
        dbg_print(DBG_ERR, "Invalid egress destination port number %04x\n", ntohs(ifa_md->egr_sys_port));
        goto FREE_MERGED_SKB;
    }
    ifa_md->egr_sys_port =  htons(egr_port_di);

    /* Modify the device id in the packet */
    ifa_md->node_id = htonl(ifa_cfg.node_id);

    tc_from_queue_id = IFA_GET_QUEUE_ID(ifa_md->second_field); 

    /* Recalculate UDP/TCP checksum */
    if (udph) {
        udph->check = 0;
        udph->check = csum_tcpudp_magic(iph->saddr, iph->daddr, ntohs(udph->len), IPPROTO_UDP,
                                        csum_partial(udph, ntohs(udph->len), 0));
        dbg_print(DBG_RX, "udp checksum recalculated: 0x%04x\n", ntohs(udph->check));
    } else if (tcph) {
        tcph->check = 0;
        ip_len = ntohs(iph->tot_len);
        min_len = (iph->ihl * 4) + sizeof(struct ifa_header) + (tcph->doff * 4);
        if (ip_len < min_len || ip_len > ptr_skb->len - ETH_HLEN) {
            dbg_print(DBG_ERR, "Invalid IP total length: %u, min_len:%u, ptr_skb->len:%d\n", ip_len, min_len, ptr_skb->len);
            goto FREE_MERGED_SKB;
        }
        tcp_len = ip_len - (iph->ihl * 4) - sizeof(struct ifa_header);
        tcph->check = csum_tcpudp_magic(iph->saddr, iph->daddr, tcp_len, IPPROTO_TCP,
                                        csum_partial(tcph, tcp_len, 0));
        dbg_print(DBG_RX, "tcp checksum recalculated: 0x%04x\n", ntohs(tcph->check));
    } else {
        dbg_print(DBG_ERR, "Invalid tcp/udp packet, next_hdr:0x%x\n", ifa_hdr->next_hdr);
        goto FREE_MERGED_SKB;
    }

    dbg_print(
        DBG_RX,
        "After insert device id: Node ID:0x%x, queue id:%d, protocol:0x%x, ptr_skb->head:0x%lx, data:0x%lx, end:%d, tail:%d, len:%d, data_len:%d, truesize:%d, ingress port:%d, egress port:%d\n",
        ifa_cfg.node_id, tc_from_queue_id, htons(ptr_skb->protocol),
        (unsigned long)ptr_skb->head, (unsigned long)ptr_skb->data, ptr_skb->end, ptr_skb->tail,
        ptr_skb->len, ptr_skb->data_len, ptr_skb->truesize,
	ntohs(ifa_md->igr_sys_port), ntohs(ifa_md->egr_sys_port));
    print_packet(DBG_RX_PAYLOAD, ptr_skb->data, ptr_skb->len);

    clx_netif_fast_tx(unit, ptr_skb, egr_port_di, tc_from_queue_id);

    if (rx_packet->list_count > 1) {
        dma_free_rx_packet(unit, rx_packet, true);
    } else {
        dma_free_rx_packet(unit, rx_packet, false);
    }

    return 0;

 FREE_MERGED_SKB:
    if (rx_packet->list_count > 1) {
        dev_kfree_skb_any(ptr_skb);
    }
    return -EINVAL;
}
int
clx_netif_receive_to_sdk(uint32_t unit, unsigned long arg)
{
    struct dma_rx_packet *rx_packet;
    struct dma_rx_frag_buffer *rx_frag, *tmp;
    struct sk_buff *ptr_skb;
    struct clx_netif_ioctl_rx_packet *kpacket;
    struct clx_netif_ioctl_rx_fragment *kfragments;
    struct clx_netif_ioctl_rx_packet __user *user_packet = (void __user *)arg;
    int frag_idx = 0, ret = 0;
    size_t packet_struct_size;
    uint32_t rc = 0;

    ret = wait_event_interruptible_timeout(clx_dma_drv(unit)->rx_wait_queue,
                                           clx_dma_drv(unit)->rx_queue.queue_size,
                                           msecs_to_jiffies(CLX_NETIF_WAIT_RX_TIMEOUT));
    if (ret <= 0) {
        rc = -EAGAIN;
        if (copy_to_user((void __user *)(&user_packet->rc), &rc, sizeof(uint32_t))) {
            dbg_print(DBG_ERR, "copy to user pkt rc failed. unit=%u\n", unit);
            return -EFAULT;
        }
        return 0;
    }

    rx_packet =
        list_first_entry(&clx_dma_drv(unit)->rx_queue.rx_packet, struct dma_rx_packet, rx_packet);
    if (!rx_packet) {
        dbg_print(DBG_ERR, "No packer in the rx queue. queue_size:%u. unit=%u\n",
                  clx_dma_drv(unit)->rx_queue.queue_size, unit);
        return -EAGAIN;
    }

    if (clx_misc_dev->test_perf.rx_enable_test == 1) {
        clx_netif_performance_test_rx(unit, rx_packet);
    }

    packet_struct_size = sizeof(struct clx_netif_ioctl_rx_packet) +
        rx_packet->list_count * sizeof(struct clx_netif_ioctl_rx_fragment);

    kpacket = kmalloc(packet_struct_size, GFP_ATOMIC);
    if (!kpacket) {
        dbg_print(DBG_CRIT, "Failed to allocate kpacket\n");
        knet_fault_event_report(KNET_FAULT_EVENT_KENT_DMA_ALLOC_FAIL);
        return -ENOMEM;
    }

    if (copy_from_user(kpacket, user_packet, sizeof(struct clx_netif_ioctl_rx_packet))) {
        dbg_print(DBG_ERR, "copy from user failed. unit=%u\n", unit);
        ret = -EFAULT;
        goto out_free;
    }
    dbg_print(DBG_RX, "unit:%u, num_fragments:%d\n", kpacket->unit, kpacket->num_fragments);

    kfragments = kpacket->fragments;
    dbg_print(DBG_RX, "rx_packet->list_count:%d. unit=%u\n", rx_packet->list_count, unit);

    list_for_each_entry_safe(rx_frag, tmp, &rx_packet->rx_frag, rx_frag)
    {
        struct clx_netif_ioctl_rx_fragment __user *user_fragment;

        ptr_skb = rx_frag->ptr_skb;

        /* copy the fragment from user */
        user_fragment = &user_packet->fragments[frag_idx];
        if (copy_from_user(&kfragments[frag_idx], user_fragment,
                           sizeof(struct clx_netif_ioctl_rx_fragment))) {
            dbg_print(DBG_ERR, "copy from user pkt fragment failed. unit=%u, frag_idx=%d\n", unit,
                      frag_idx);
            ret = -EFAULT;
            goto out_free;
        }

        /* copy dma data to the dma addr allocated by user */
        if (copy_to_user((void __user *)(clx_addr_t)kfragments[frag_idx].fragment_dma_addr,
                         ptr_skb->data, ptr_skb->len)) {
            dbg_print(DBG_ERR, "copy to user pkt data failed. unit=%u, frag_idx=%d\n", unit,
                      frag_idx);
            ret = -EFAULT;
            goto out_free;
        }

        /* update fragment_size */
        kfragments[frag_idx].fragment_size = ptr_skb->len;

        if (copy_to_user(user_fragment, &kfragments[frag_idx],
                         sizeof(struct clx_netif_ioctl_rx_fragment))) {
            dbg_print(DBG_ERR, "copy to user pkt fragment failed. unit=%u, frag_idx=%d\n", unit,
                      frag_idx);
            ret = -EFAULT;
            goto out_free;
        }
        print_packet(DBG_RX_PAYLOAD, ptr_skb->data, ptr_skb->len);

        frag_idx++;
    }

    /* update num_fragments to user */
    if (copy_to_user(&user_packet->num_fragments, &rx_packet->list_count, sizeof(uint32_t))) {
        dbg_print(DBG_ERR, "copy to user pkt num_fragments failed. unit=%u\n", unit);
        ret = -EFAULT;
        goto out_free;
    }

    if (rx_packet != clx_dma_rx_packet_queue_dequeue(&clx_dma_drv(unit)->rx_queue)) {
        dbg_print(DBG_ERR, "queue first entry error,rx_packet:%p. unit=%u\n", rx_packet, unit);
    }

    dma_free_rx_packet(unit, rx_packet, true);
    ret = 0;

out_free:
    kfree(kpacket);
    return ret;
}

int
clx_netif_send_from_sdk(uint32_t unit, unsigned long arg)
{
    struct sk_buff *ptr_skb = NULL;
    struct clx_netif_ioctl_rx_packet kpacket;
    struct clx_netif_ioctl_rx_fragment *kfragments;
    struct clx_netif_ioctl_rx_packet __user *user_packet = (void __user *)arg;
    int frag_idx = 0, ret = 0;
    uint32_t copy_len = 0;
    uint32_t retry = 0;

    if (copy_from_user(&kpacket, user_packet, sizeof(struct clx_netif_ioctl_rx_packet))) {
        dbg_print(DBG_ERR, "copy from user failed. unit=%u\n", unit);
        return -EFAULT;
    }

    ptr_skb = dev_alloc_skb(kpacket.packet_len);
    if (NULL == ptr_skb) {
        dbg_print(DBG_ERR, "dev_alloc_skb failed. unit=%u\n", unit);
        knet_fault_event_report(KNET_FAULT_EVENT_KENT_DMA_ALLOC_FAIL);
        return -ENOMEM;
    }
    ptr_skb->len = kpacket.packet_len;

    kfragments =
        kmalloc(kpacket.num_fragments * sizeof(struct clx_netif_ioctl_rx_fragment), GFP_KERNEL);
    if (!kfragments) {
        dbg_print(DBG_ERR, "kmalloc failed. unit=%u\n", unit);
        ret = -ENOMEM;
        goto out_free;
    }

    // copy fragment from the user
    if (copy_from_user(kfragments, user_packet->fragments,
                       kpacket.num_fragments * sizeof(struct clx_netif_ioctl_rx_fragment))) {
        dbg_print(DBG_ERR, "copy from user failed. unit=%u\n", unit);
        ret = -EFAULT;
        goto out_free;
    }

    for (frag_idx = 0; frag_idx < kpacket.num_fragments; frag_idx++) {
        if (copy_from_user(&(((uint8_t *)ptr_skb->data)[copy_len]),
                           (void __user *)(clx_addr_t)kfragments[frag_idx].fragment_dma_addr,
                           kfragments[frag_idx].fragment_size)) {
            dbg_print(DBG_ERR, "copy from user failed. unit=%u\n", unit);
            ret = -EFAULT;
            goto out_free;
        }
        copy_len += kfragments[frag_idx].fragment_size;
    }

    kfree(kfragments);
    print_packet(DBG_TX_PAYLOAD, ptr_skb->data, ptr_skb->len);
    if (clx_misc_dev->test_perf.tx_enable_test == 1) {
        clx_netif_performance_test_tx(unit, ptr_skb);
    }

    do {
        ret = clx_dma_drv(unit)->tx_packet(unit, kpacket.channel, ptr_skb);
        if (ret == 0) {
            break;
        }

        if (ret != -EBUSY) {
            dbg_print(DBG_ERR, "tx_packet failed. unit=%u, ret=%d\n", unit, ret);
            dev_kfree_skb_any(ptr_skb);
        }
        retry++;
    } while((ret == -EBUSY) && (retry < CLX_NETIF_PKT_SEND_RETYR_NUM));

    return ret;
out_free:
    kfree(kfragments);
    dev_kfree_skb_any(ptr_skb);
    return ret;
}

int
clx_netif_set_port_map(uint32_t unit, unsigned long arg)
{
    struct clx_ioctl_port_map_cookie port_map_cookie;
    struct clx_ioctl_port_map_cookie __user *user_port_map = (void __user *)arg;
    int ret = 0;

    if (copy_from_user(&port_map_cookie, user_port_map, sizeof(struct clx_ioctl_port_map_cookie))) {
        return -EFAULT;
    }

    if (port_map_cookie.slice >= clx_netif_drv(unit)->slices_per_unit ||
        port_map_cookie.slice_port >= clx_netif_drv(unit)->ports_per_slice) {
        dbg_print(DBG_ERR, "u=%u, bad param slice=%u slice_port=%u port_di=%u\n", unit,
                  port_map_cookie.slice, port_map_cookie.slice_port, port_map_cookie.port_di);
        return -EFAULT;
    }

    CLX_NETIF_SET_PORT_DI(unit, port_map_cookie.slice, port_map_cookie.slice_port,
                          port_map_cookie.port_di);

    dbg_print(DBG_RX, "u=%u, slice=%u slice_port=%u port_di=%u\n", unit, port_map_cookie.slice,
              port_map_cookie.slice_port, port_map_cookie.port_di);

    return ret;
}

int
clx_netif_set_port_attr(uint32_t unit, unsigned long arg)
{
    struct clx_ioctl_port_attr_cookie port_attr_cookie;
    struct clx_ioctl_port_attr_cookie __user *user_port_attr = (void __user *)arg;
    int ret = 0;

    if (copy_from_user(&port_attr_cookie, user_port_attr, sizeof(struct clx_ioctl_port_attr_cookie))) {
        return -EFAULT;
    }

    if (CLX_NETIF_PORT_DI_MAX_NUM <= port_attr_cookie.port_di) {
        dbg_print(DBG_WARN, "u=%u, port_di=%u bad param!\n", unit, port_attr_cookie.port_di);
        return -EINVAL;
    }

    if (port_attr_cookie.pvid == 0) {
        clx_netif_drv(unit)->port_db[port_attr_cookie.port_di].pvid = CLX_NETIF_DFLT_VLAN;
    } else {
        clx_netif_drv(unit)->port_db[port_attr_cookie.port_di].pvid = port_attr_cookie.pvid;
    }

    dbg_print(DBG_RX, "u=%u, port_di=%u pvid=%u\n", unit, port_attr_cookie.port_di, port_attr_cookie.pvid);

    return ret;
}

int
clx_netif_get_port_attr(uint32_t unit, unsigned long arg)
{
    struct clx_ioctl_port_attr_cookie port_attr_cookie;
    struct clx_ioctl_port_attr_cookie __user *user_port_attr = (void __user *)arg;
    int ret = 0;

    if (copy_from_user(&port_attr_cookie, user_port_attr, sizeof(struct clx_ioctl_port_attr_cookie))) {
        return -EFAULT;
    }

    if (CLX_NETIF_PORT_DI_MAX_NUM <= port_attr_cookie.port_di) {
        dbg_print(DBG_WARN, "u=%u, port_di=%u bad param!\n", unit, port_attr_cookie.port_di);
        return -EINVAL;
    }

    port_attr_cookie.pvid = clx_netif_drv(unit)->port_db[port_attr_cookie.port_di].pvid;

    dbg_print(DBG_RX, "u=%u, port_di=%u pvid=%u\n", unit, port_attr_cookie.port_di, port_attr_cookie.pvid);

    if (copy_to_user(user_port_attr, &port_attr_cookie, sizeof(struct clx_ioctl_port_attr_cookie))) {
        dbg_print(DBG_ERR, "Failed to copy port attr to user space. unit=%u\n", unit);
        return -EFAULT;
    }

    return ret;
}

static void
insert_rule_by_priority(struct list_head *head, struct profile_rule *new_rule)
{
    struct profile_rule *rule;
    struct list_head *pos;

    list_for_each(pos, head)
    {
        rule = list_entry(pos, struct profile_rule, list);
        if (new_rule->priority < rule->priority) {
            list_add_tail(&new_rule->list, pos);
            return;
        }
    }

    list_add_tail(&new_rule->list, head);
}

int
clx_netif_rx_profile_create(uint32_t unit, unsigned long arg)
{
    int rc = 0;
    struct profile_rule *user_rule = (struct profile_rule *)arg;
    struct profile_rule *new_rule = NULL;
    size_t copy_size;
    unsigned long flags = 0;

    new_rule = kmalloc(sizeof(struct profile_rule), GFP_KERNEL);
    if (!new_rule) {
        dbg_print(DBG_ERR, "Failed to allocate memory for new profile rule. unit=%u\n", unit);
        return -ENOMEM;
    }

    copy_size = offsetof(struct profile_rule, list);
    if (copy_from_user(new_rule, user_rule, copy_size)) {
        dbg_print(DBG_ERR, "Failed to copy profile rule from user space. unit=%u\n", unit);
        kfree(new_rule);
        return -EFAULT;
    }

    // Add new rule to the profile list.
    spin_lock_irqsave(&clx_netif_drv(unit)->profile.lock, flags);
    new_rule->id =
        find_first_zero_bit(clx_netif_drv(unit)->profile.profile_id_bitmap, CLX_PROFILE_MAX_NUM);
    if (new_rule->id >= CLX_PROFILE_MAX_NUM) {
        spin_unlock_irqrestore(&clx_netif_drv(unit)->profile.lock, flags);
        dbg_print(DBG_PROFILE, "No available profile ID. unit=%u\n", unit);
        kfree(new_rule);
        return -ENOSPC;
    }
    set_bit(new_rule->id, clx_netif_drv(unit)->profile.profile_id_bitmap);
    insert_rule_by_priority(&clx_netif_drv(unit)->profile.list, new_rule);
    spin_unlock_irqrestore(&clx_netif_drv(unit)->profile.lock, flags);

    print_profile_rule(new_rule);

    if (copy_to_user(&user_rule->id, &new_rule->id, sizeof(new_rule->id))) {
        dbg_print(DBG_ERR, "Failed to copy profile id to user space. unit=%u\n", unit);
        spin_lock_irqsave(&clx_netif_drv(unit)->profile.lock, flags);
        list_del(&new_rule->list);
        spin_unlock_irqrestore(&clx_netif_drv(unit)->profile.lock, flags);
        kfree(new_rule);
        return -EFAULT;
    }
    dbg_print(DBG_INFO, "New profile rule added successfully. unit=%u\n", unit);

    return rc;
}

int
clx_netif_rx_profile_destroy(uint32_t unit, unsigned long arg)
{
    struct profile_rule *rule, *tmp;
    unsigned long flags = 0;
    bool found = false;
    uint32_t profile_id;

    if (copy_from_user(&profile_id, (uint32_t __user *)arg, sizeof(uint32_t))) {
        return -EFAULT;
    }

    spin_lock_irqsave(&clx_netif_drv(unit)->profile.lock, flags);
    list_for_each_entry_safe(rule, tmp, &clx_netif_drv(unit)->profile.list, list)
    {
        if (rule->id == profile_id) {
            list_del(&rule->list);
            kfree(rule);
            found = true;
            dbg_print(DBG_PROFILE, "Profile rule with ID %u destroyed successfully. unit=%u\n",
                      profile_id, unit);
            break;
        }
    }
    clear_bit(profile_id, clx_netif_drv(unit)->profile.profile_id_bitmap);
    spin_unlock_irqrestore(&clx_netif_drv(unit)->profile.lock, flags);

    if (!found) {
        dbg_print(DBG_PROFILE, "Profile rule with ID %u not found. unit=%u\n", profile_id, unit);
        return 0;
    }

    return 0;
}

static void
clx_netif_rx_profile_destroy_all(uint32_t unit)
{
    struct profile_rule *rule, *tmp;
    unsigned long flags = 0;

    spin_lock_irqsave(&clx_netif_drv(unit)->profile.lock, flags);
    list_for_each_entry_safe(rule, tmp, &clx_netif_drv(unit)->profile.list, list)
    {
        dbg_print(DBG_PROFILE, "Profile rule with ID %u destroyed successfully. unit=%u\n",
                  rule->id, unit);
        clear_bit(rule->id, clx_netif_drv(unit)->profile.profile_id_bitmap);
        list_del(&rule->list);
        kfree(rule);
    }
    spin_unlock_irqrestore(&clx_netif_drv(unit)->profile.lock, flags);
}

int
clx_netif_rx_profile_get(uint32_t unit, unsigned long arg)
{
    struct profile_rule *rule, *tmp;
    struct profile_rule *user_rule = (struct profile_rule *)arg;
    struct profile_rule k_rule;
    unsigned long flags = 0;
    bool found = false;
    size_t copy_size;

    copy_size = offsetof(struct profile_rule, list);

    if (copy_from_user(&k_rule, (void __user *)arg, copy_size)) {
        dbg_print(DBG_ERR, "Failed to copy profile rule from user space. unit=%u\n", unit);
        return -EFAULT;
    }

    spin_lock_irqsave(&clx_netif_drv(unit)->profile.lock, flags);
    list_for_each_entry_safe(rule, tmp, &clx_netif_drv(unit)->profile.list, list)
    {
        if (rule->id == k_rule.id) {
            found = true;
            dbg_print(DBG_PROFILE, "Found profile rule with ID %u successfully. unit=%u\n",
                      k_rule.id, unit);
            break;
        }
    }
    spin_unlock_irqrestore(&clx_netif_drv(unit)->profile.lock, flags);

    if (found) {
        print_profile_rule(rule);
        if (copy_to_user(user_rule, rule, copy_size)) {
            dbg_print(DBG_ERR, "Failed to copy profile id to user space. unit=%u\n", unit);
            return -EFAULT;
        }
    } else {
        dbg_print(DBG_PROFILE, "Profile rule with ID %u not found. unit=%u\n", k_rule.id, unit);
        k_rule.rc = CLX_IOCTL_E_ENTRY_NOT_FOUND;
        if (copy_to_user(user_rule, &k_rule, copy_size)) {
            dbg_print(DBG_ERR, "Failed to copy profile id to user space. unit=%u\n", unit);
            return -EFAULT;
        }
        return 0;
    }

    return 0;
}

struct profile_rule *
clx_netif_match_profile(uint32_t unit,
                        uint32_t port_di,
                        uint32_t reason,
                        struct dma_rx_packet *rx_packet)
{
    struct dma_rx_frag_buffer *rx_block;
    struct profile_rule *rule, *matched_rule = NULL;
    uint8_t *packet_payload;
    int i, j;

    rx_block = list_first_entry(&rx_packet->rx_frag, struct dma_rx_frag_buffer, rx_frag);
    packet_payload = rx_block->ptr_skb->data + clx_dma_drv(unit)->dma_hdr_size;

    spin_lock(&clx_netif_drv(unit)->profile.lock);
    list_for_each_entry(rule, &clx_netif_drv(unit)->profile.list, list)
    {
        bool match = true;
        /* match reason */
        if (rule->match_reason.match_type == 1) {
            if (!CLX_REASON_BITMAP_CHK(rule->match_reason.reason_bitmap, reason)) {
                continue;
            }
        }

        /* match port_di */
        if (rule->match_port.match_type == 1) {
            if (rule->match_port.port_di != port_di) {
                continue;
            }
        }

        /* match pattern */
        for (i = 0; i < MAX_PATTERN_NUM; i++) {
            struct match_pattern *mp = &rule->match_pattern[i];
            if (mp->match_type == 1) {
                for (j = 0; j < MAX_PATTERN_LENGTH; j++) {
                    uint8_t data_byte = packet_payload[mp->offset + j];
                    uint8_t masked_data = data_byte & mp->mask[j];
                    uint8_t masked_pattern = mp->pattern[j] & mp->mask[j];
                    if (masked_data != masked_pattern) {
                        match = false;
                        break;
                    }
                }
            }

            if (!match)
                break;
        }
        if (!match)
            continue;

        matched_rule = rule;
        break;
    }
    spin_unlock(&clx_netif_drv(unit)->profile.lock);

    if (matched_rule) {
        dbg_print(DBG_PROFILE, "profile name:%s, rx pakcet action: %s. unit=%u\n",
                  matched_rule->name, action_str[matched_rule->action], unit);
    } else {
        dbg_print(DBG_PROFILE, "rx pakcet action: ACTION_NETDEV. unit=%u\n", unit);
    }
    return matched_rule;
}

int
clx_netif_init(void)
{
    uint32_t unit = 0;
    uint32_t di = 0;

    for (unit = 0; unit < clx_misc_dev->pci_dev_num; unit++) {
        // netif
        memset(clx_netif_drv(unit)->netif_di2id_map, -1,
               sizeof(clx_netif_drv(unit)->netif_di2id_map));
        bitmap_zero(clx_netif_drv(unit)->netif_id_bitmap, CLX_NETIF_MAX_NUM);

        // profile id
        INIT_LIST_HEAD(&clx_netif_drv(unit)->profile.list);
        spin_lock_init(&clx_netif_drv(unit)->profile.lock);
        // To allocate profile id.
        bitmap_zero(clx_netif_drv(unit)->profile.profile_id_bitmap, CLX_PROFILE_MAX_NUM);

        // init port db
        memset(clx_netif_drv(unit)->port_db, 0, sizeof(clx_netif_drv(unit)->port_db));
        for(di = 0; di < CLX_NETIF_PORT_DI_MAX_NUM; di++) {
            clx_netif_drv(unit)->port_db[di].pvid = CLX_NETIF_DFLT_VLAN;
        }
    }
    return 0;
}

void
clx_netif_deinit(void)
{
    uint32_t unit = 0;

    for (unit = 0; unit < clx_misc_dev->pci_dev_num; unit++) {
        clx_netif_rx_profile_destroy_all(unit);
        clx_netif_net_dev_destroy_all(unit);
    }
}
