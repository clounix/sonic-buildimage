#ifndef __CLX_NETIF_H__
#define __CLX_NETIF_H__

#include "knet_types.h"
#include "knet_common.h"
#include "knet_dma.h"
#include <linux/spinlock.h>
#include <linux/netdevice.h>
#include <net/genetlink.h>

#define CLX_NETIF_MAX_NUM         (288)
#define CLX_PROFILE_MAX_NUM       (288)
#define CLX_NETLINK_MAX_NUM       (288)
#define CLX_NETIF_PORT_DI_MAX_NUM (2048)
#define CLX_NETIF_WAIT_RX_TIMEOUT (3000)
#define CLX_NETIF_PKT_SEND_RETYR_NUM (100)
#define CLX_NETIF_DFLT_VLAN       (1)

#define IPPROTO_IFA 0x00FD /* IFA protocol */

#define CLX_NETIF_PORT_MAP_INDEX(unit, slice, slice_port)                                        \
    (unit * clx_netif_drv(unit)->ports_num_unit + slice * clx_netif_drv(unit)->ports_per_slice + \
     slice_port)
#define CLX_NETIF_GET_PORT_DI(unit, slice, slice_port) \
    clx_netif_drv(unit)->ptr_port_map_db[CLX_NETIF_PORT_MAP_INDEX(unit, slice, slice_port)]
#define CLX_NETIF_SET_PORT_DI(unit, slice, slice_port, di) \
    clx_netif_drv(unit)->ptr_port_map_db[CLX_NETIF_PORT_MAP_INDEX(unit, slice, slice_port)] = di

struct ifa_header {
#if defined(__LITTLE_ENDIAN_BITFIELD)
    __u8 gns : 4,    /* Group Number Space */
        version : 4; /* Version (should be 2) */
#elif defined(__BIG_ENDIAN_BITFIELD)
    __u8 version : 4, /* Version (should be 2) */
        gns : 4;      /* Group Number Space */
#endif
    __u8 next_hdr : 8; /* Next Header field */
#if defined(__LITTLE_ENDIAN_BITFIELD)
    __u8 c_flag : 1,   /* C bit - Configuration flag */
        ta_flag : 1,   /* TA bit - Timestamp Available flag */
        i_flag : 1,    /* I bit - Integrity flag */
        ts_flag : 1,   /* TS bit - Time Synchronization flag */
        mf_flag : 1,   /* MF bit - More Fragments flag */
        r1 : 1,        /* Reserved bit 1 */
        r2 : 1,        /* Reserved bit 2 */
        r3 : 1;        /* Reserved bit 3 */
#elif defined(__BIG_ENDIAN_BITFIELD)
    __u8 r3 : 1,     /* Reserved bit 3 */
        r2 : 1,      /* Reserved bit 2 */
        r1 : 1,      /* Reserved bit 1 */
        mf_flag : 1, /* MF bit - More Fragments flag */
        ts_flag : 1, /* TS bit - Time Synchronization flag */
        i_flag : 1,  /* I bit - Integrity flag */
        ta_flag : 1, /* TA bit - Timestamp Available flag */
        c_flag : 1;  /* C bit - Configuration flag */
#endif
    __u8 max_length : 8; /* Maximum length field */
};

struct ifa_metadata {
    union {
        __be32 first_field;
        struct {
#if defined(__BIG_ENDIAN_BITFIELD)
            __be32 lns:4,
                   ip_ttl:8,
                   device_id:20;
#else
            __be32 device_id:20,
                   ip_ttl:8,
                   lns:4;
#endif
        } first_bits;
    };

    union {
        __be32 second_field;
        struct {
#if defined(__BIG_ENDIAN_BITFIELD)
            __be32 egr_port_speed:4,
                   congestion:2,
                   queue_id:6,
                   rx_timestamp_sec:20;
#else
            __be32 rx_timestamp_sec:20,
                   queue_id:6,
                   congestion:2,
                   egr_port_speed:4;
#endif
        } second_bits;
    };
    __be16 egr_sys_port;
    __be16 igr_sys_port;
    __be32 rx_timestamp_nano_sec;
    __be32 residence_time_nano_sec;
    __be32 queue_xmit_byte_count;
    __be16 queue_data;
    __be16 queue_depth;
    __be32 node_id;
};

#define IFA_GET_QUEUE_ID(second_field) \
    ((ntohl(second_field) >> 20) & 0x3F)

typedef struct clx_netif_ifa_cfg_s {
    uint32_t ip_prot; /* ifa protocol type in ipv4/v6 header */
    uint32_t node_id; /* node_id in metadata */
} clx_netif_ifa_cfg_t;

struct net_device_priv {
    struct net_device *ptr_net_dev;
    struct net_device_stats stats;
    uint32_t unit;
    uint32_t id;
    uint32_t port_di;
    uint16_t vlan;
    uint16_t duplex;
    uint32_t speed;
    uint32_t tx_channel;
    uint32_t max_mtu;
    uint8_t vlan_tag_type;
    uint8_t skip_port_state_event;
    /* psample */
    uint32_t igr_sample_rate;
    uint32_t egr_sample_rate;
    uint32_t tc;
};

struct clx_netif_cnt {
    uint32_t packets;
    uint32_t bytes;
    uint32_t enqueue_ok;
    uint32_t enqueue_fail;
    uint32_t dequeue_ok;
    uint32_t dequeue_fail;
    uint32_t interrupts;
};

struct netif_port {
    struct clx_netif_ioctl_intf intf;
    struct net_device *ptr_net_dev;
};

typedef struct {
    uint16_t pvid;  // [kg] igress pph igr_vlan is invalid;
} port_db_t;

struct profile_list {
    struct list_head list;
    spinlock_t lock;
    DECLARE_BITMAP(profile_id_bitmap, CLX_PROFILE_MAX_NUM);
};

/* Netlink structure */
typedef enum {
    NETIF_NL_PKT_PSAMPLE_OTHER = 0,
    NETIF_NL_PKT_PSAMPLE_INGRESS,
    NETIF_NL_PKT_PSAMPLE_EGRESS,
    NETIF_NL_PKT_PSAMPLE_MAX,
} netlink_psample_dir_e;

typedef enum {
    NETLINK_RX_TYPE_OTHER,
    NETLINK_RX_TYPE_SFLOW,
    NETLINK_RX_TYPE_MOD,
} netlink_rx_type_e;

struct netlink_rx_pkt_extra {
    uint16_t iifindex;
    uint16_t eifindex;
    netlink_psample_dir_e psample_dir;
    uint32_t sample_rate;
    uint32_t igr_port_si;
};

struct netlink_rx_cookie {
    struct clx_netif_rx_dst_netlink *nl;
    struct netlink_rx_pkt_extra pkt;
    netlink_rx_type_e netlink_type;
};

struct netlink_list {
    struct list_head list;
    spinlock_t lock;
    DECLARE_BITMAP(netlink_id_bitmap, CLX_NETLINK_MAX_NUM);
};

/* NETIF DRV */
typedef int (*clx_pkt_get_dst)(uint32_t unit,
                               struct dma_rx_packet *rx_packet,
                               uint32_t *rx_port,
                               uint32_t *cpu_reason);
typedef int (*clx_pkt_parse_pkt_info)(uint32_t unit,
                                      struct dma_rx_packet *rx_packet,
                                      void *ptr_cookie);
typedef struct {
    uint32_t mtu;
    uint32_t cpu_port;
    clx_pkt_get_dst get_pkt_dst;
    clx_pkt_parse_pkt_info parse_netlink_info;

    uint32_t netif_di2id_map[CLX_NETIF_PORT_DI_MAX_NUM];
    DECLARE_BITMAP(netif_id_bitmap, CLX_NETIF_MAX_NUM);
    struct netif_port netif_db[CLX_NETIF_MAX_NUM];
    struct profile_list profile;
    struct netlink_list netlink;

    /* cnt */
    struct clx_netif_cnt cnt;

    /* for mod dmac modify */
    bool enable_mod_dmac;
    clx_mac_t mod_dmac;

    /* port_di map */
    uint32_t ports_num_unit;
    uint32_t slices_per_unit;
    uint32_t unit_num;
    uint32_t dies_per_unit;
    uint32_t slices_per_die;
    uint32_t ports_per_slice;
    uint32_t *ptr_port_map_db; /* [unit_num][dies_per_unit][slices_per_die * ports_per_slice] */
    port_db_t port_db[CLX_NETIF_PORT_DI_MAX_NUM];
} clx_netif_drv_cb_t;

/* netdevice operation */
int
clx_netif_net_dev_create(uint32_t unit, unsigned long arg);
int
clx_netif_net_dev_destroy(uint32_t unit, unsigned long arg);
int
clx_netif_net_dev_set(uint32_t unit, unsigned long arg);
int
clx_netif_get_netdev(uint32_t unit, unsigned long arg);
int
clx_netif_set_intf_attr(uint32_t unit, unsigned long arg);
int
clx_netif_get_intf_attr(uint32_t unit, unsigned long arg);
int
clx_netif_get_netdev_cnt(uint32_t unit, unsigned long arg);
int
clx_netif_clear_netdev_cnt(uint32_t unit, unsigned long arg);

/* receive/send packet from/to sdk */
int
clx_netif_receive_to_sdk(uint32_t unit, unsigned long arg);
int
clx_netif_send_from_sdk(uint32_t unit, unsigned long arg);

/* packet to kernel */
struct sk_buff *
netif_construct_skb_from_rx_packet(uint32_t unit,
                                   uint32_t port_di,
                                   struct dma_rx_packet *rx_packet);
struct sk_buff *
netif_construct_fast_skb_from_rx_packet(uint32_t unit, struct dma_rx_packet *rx_packet);

int
clx_netif_netdev_receive_skb(uint32_t unit, struct dma_rx_packet *rx_packet, uint32_t port_di);

/* profile operation */
int
clx_netif_rx_profile_create(uint32_t unit, unsigned long arg);
int
clx_netif_rx_profile_destroy(uint32_t unit, unsigned long arg);
int
clx_netif_rx_profile_get(uint32_t unit, unsigned long arg);
struct profile_rule *
clx_netif_match_profile(uint32_t unit,
                        uint32_t port_di,
                        uint32_t reason,
                        struct dma_rx_packet *rx_packet);

/* Netlink operation */
int
clx_netif_create_netlink(uint32_t unit, unsigned long arg);
int
clx_netif_destroy_netlink(uint32_t unit, unsigned long arg);
int
clx_netif_get_netlink(uint32_t unit, unsigned long arg);
int
clx_netif_set_pkt_mod(uint32_t unit, unsigned long arg);
int
clx_netif_get_pkt_mod(uint32_t unit, unsigned long arg);
int
netif_netlink_reveive_skb(uint32_t unit,
                          struct dma_rx_packet *rx_packet,
                          uint32_t port_di,
                          void *ptr_cookie);
int
clx_netlink_init(void);
void
clx_netlink_deinit(void);

int
clx_netif_set_port_map(uint32_t unit, unsigned long arg);

int
clx_netif_set_port_attr(uint32_t unit, unsigned long arg);

int
clx_netif_get_port_attr(uint32_t unit, unsigned long arg);

int
clx_netif_init(void);
void
clx_netif_deinit(void);

void
print_packet(uint32_t loglvl, const unsigned char *data, size_t len);

uint32_t
clx_netif_di2id_lookup(uint32_t unit, uint32_t di);

void
clx_netif_performance_test_rx(uint32_t unit, struct dma_rx_packet *rx_packet);
int
clx_netif_netdev_receive_send_ifa(uint32_t unit, struct dma_rx_packet *rx_packet, uint32_t port_di);

int
clx_netif_set_ifa_cfg(uint32_t unit, unsigned long arg);

int
clx_netif_get_ifa_cfg(uint32_t unit, unsigned long arg);

netdev_tx_t
clx_netif_fast_tx(uint32_t unit, struct sk_buff *ptr_skb, uint16_t di, uint8_t tc);

void
clx_netif_performance_test_tx(uint32_t unit, struct sk_buff *ptr_skb);

void
netif_process_skb_from_rx_packet(uint32_t unit,
                                 uint32_t port_di,
                                 struct dma_rx_packet *rx_packet,
                                 struct sk_buff *ptr_merge_skb);

struct net_device *
clx_netif_get_netdev_from_sys_port(uint32_t unit, uint16_t sys_port);
#endif // __CLX_NETIF_H__
