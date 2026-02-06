#ifndef __KG_CHIP_H__
#define __KG_CHIP_H__

#define KG_DMA_SCRATCH               0x00041400
#define KG_CFG_TOP_INTR_MSK          0x000400DC
#define KG_STA_TOP_RAW_INTR          0x000400E0
#define KG_STA_TOP_INTR              0x000400E4
#define KG_CFG_MSI_ADDR              0x000400EC /*64BIT*/
#define KG_CFG_MSI_DATA              0x000400F4 /*64BIT*/
#define KG_CFG_PDMA_CH_ENABLE        0x00041404
#define KG_CFG_PDMA_DESC_LOCATION    0x00041408
#define KG_CFG_PDMA_DESC_ENDIAN      0x0004140C
#define KG_CFG_PDMA_DATA_ENDIAN_SWAP 0x00041410
#define KG_CFG_PDMA_PPH_SWAP         0x00041414
#define KG_CFG_AXI_PROTO_INFO        0x00041418
#define KG_CFG_AXI_FIFO_ALM_FULL     0x0004141C
#define KG_CFG_AXI_TIMEOUT_THR       0x00041420 /*96BIT*/
#define KG_CFG_AXI0_OUTSTD_SIZE      0x0004142C
#define KG_CFG_AXI1_OUTSTD_SIZE      0x00041430
// #define KG_CFG_FIFIO_PATH_SEL             0x00041434
// #define KG_CFG_PDMA_CRC_EN                0x00041438
#define KG_CFG_P2H_RX_FIFO_ALM_FULL         0x00041434
#define KG_CFG_P2H_TX_FIFO_ALM_FULL         0x00041438
#define KG_CFG_P2E_RX_FIFO_ALM_FULL         0x0004143C
#define KG_CFG_P2E_TX_FIFO_ALM_FULL         0x00041440
#define KG_CFG_TX_FIFO_TIMEOUT_THR          0x00041444
#define KG_CFG_SLV_TIMEOUT_THR              0x00041448
#define KG_CFG_PDMA_NORMAL_MASK_ALL         0x000414A0
#define KG_CFG_PDMA_P2H_INTR_MSK            0x000414A4
#define KG_CFG_PDMA_GEN_INTR_MSK            0x000414A8
#define KG_CFG_PDMA_P2E_INTR_MSK            0x000414AC
#define KG_CFG_PDMA_LRN_INTR_MSK            0x000414B0
#define KG_CFG_PDMA_IOAM_INTR_MSK           0x000414B4
#define KG_CFG_PDMA_NFE_INTR_MSK            0x000414B8
#define KG_CFG_PDMA_MSI_INTR_MSK            0x000414BC
#define KG_CFW_PDMA_P2H_INTR_CLR            0x000414C0
#define KG_CFW_PDMA_GEN_INTR_CLR            0x000414C4
#define KG_CFW_PDMA_LRN_INTR_CLR            0x000414C8
#define KG_CFW_PDMA_IOAM_INTR_CLR           0x000414CC
#define KG_CFW_PDMA_NFE_INTR_CLR            0x000414D0
#define KG_CFG_PDMA_MSI_INTR_CLR            0x000414D4
#define KG_CFW_PDMA_P2H_INTR_TEST           0x000414D8
#define KG_CFW_PDMA_GEN_INTR_TEST           0x000414DC
#define KG_CFW_PDMA_P2E_INTR_TEST           0x000414E0
#define KG_CFW_PDMA_LRN_INTR_TEST           0x000414E4
#define KG_CFW_PDMA_IOAM_INTR_TEST          0x000414E8
#define KG_CFW_PDMA_NFE_INTR_TEST           0x000414EC
#define KG_STA_PDMA_NORMAL_INTR             0x000414F0
#define KG_STA_PDMA_MSI_INTR                0x000414F4
#define KG_CFG_PDMA_RING_BASE_0             0x000414F8
#define KG_CFG_PDMA_RING_SIZE_0             0x000415A0
#define KG_CFG_PDMA_DESC_WORK_IDX_0         0x000415F4
#define KG_STA_PDMA_DESC_POP_IDX_0          0x00041648
#define KG_CFG_PDMA_MODE_0                  0x0004169C
#define KG_CFW_PDMA_GLOBAL_RESET            0x000416F0
#define KG_CFW_PDMA_CH_RESET                0x000416F4
#define KG_CFW_PDMA_CH_RESTART              0x000416F8
#define KG_IRQ_PDMA_ABNORMAL_P2H_INTR       0x000416FC
#define KG_IRQ_PDMA_ABNORMAL_P2H_INTR_MSK   0x00041700
#define KG_IRQ_PDMA_ABNORMAL_P2H_INTR_MSK1  0x00041708 /* for ecpu */
#define KG_IRQ_PDMA_ABNORMAL_GEN_INTR       0x0004170C
#define KG_IRQ_PDMA_ABNORMAL_GEN_INTR_MSK   0x00041710
#define KG_IRQ_PDMA_ABNORMAL_GEN_INTR_MSK1  0x00041718 /* for ecpu */
#define KG_IRQ_PDMA_ABNORMAL_LRN_INTR       0x0004172C
#define KG_IRQ_PDMA_ABNORMAL_LRN_INTR_MSK   0x00041730
#define KG_IRQ_PDMA_ABNORMAL_LRN_INTR_MSK1  0x00041738 /* for ecpu */
#define KG_IRQ_PDMA_ABNORMAL_IOAM_INTR      0x0004173C
#define KG_IRQ_PDMA_ABNORMAL_IOAM_INTR_MSK  0x00041740
#define KG_IRQ_PDMA_ABNORMAL_IOAM_INTR_MSK1 0x00041748 /* for ecpu */
#define KG_IRQ_PDMA_ABNORMAL_NFE_INTR       0x0004174C
#define KG_IRQ_PDMA_ABNORMAL_NFE_INTR_MSK   0x00041750
#define KG_STA_PDMA_ERR_TYPE_0              0x000417B0
#define KG_STA_PDMA_OUTSTD_DESC_0           0x00041804
#define KG_STA_PDMA_RD_ERR_DESC_0           0x00041858
#define KG_STA_PDMA_WB_DESC_0               0x00041A50
#define KG_STA_PDMA_SRC_ERR_INFO_0          0x00041C48
#define KG_STA_PDMA_DST_ERR_INFO_0          0x00041F90
#define KG_CFG_PDMA_DESC_NUM_THR            0x00042218
#define KG_IRQ_PDMA_DESC_ALM_EMPTY          0x0004221C
#define KG_IRQ_PDMA_DESC_ALM_EMPTY_MSK      0x00042220
#define KG_IRQ_PDMA_MALFORM_INTR            0x0004222C
#define KG_IRQ_PDMA_MALFORM_INTR_MSK        0x00042230
#define KG_CFG_PDMA_MISC                    0x0004223C
#define KG_CFG_PDMA_MSI_RETRY_TRANSFER_NUM  0x00042240
#define KG_STA_PDMA_MSI_INFO                0x00042244
#define KG_CFG_PDMA_SRC_ECPU_DATA_FLOW      0x00042250
#define KG_CFG_PDMA_DST_ECPU_DATA_FLOW      0x00042254
#define KG_STA_RX_TX_FIFO_EMPTY             0x00042264
#define KG_CFG_DIS_P2H_RX_DESC_AE_EN        0x00042268
#define KG_CFG_P2H_RX_DESC_AE_THR           0x0004226C
#define KG_HIER_IRQ                         0x00042270
#define KG_HIER_IRQ_MSK                     0x00042274

/* ERROR LOG */
#define KG_IRQ_CMST_RESP_ERR_INTR     0x00040018
#define KG_IRQ_CMST_RESP_ERR_INTR_MSK 0x0004001C
#define KG_SYM_CMST_RESP_ERR_LOG      0x00040028

// PCX
#define KG_CTL_EFUSE_VECTOR_SEL 0x00040154
#define KG_STA_EFUSE_VECTOR     0x00040158

// slve chain0 for p2e
#define KG_CFG_CHAIN0_INTR_REG (0x7FFFFC)
#define KG_PDMA_ERROR_IRQ_BIT  (0x1 << 1)

#define KG_PDMA_CHANNEL_RD_DESC_ERROR (1 << 0)
#define KG_PDMA_CHANNEL_WR_DESC_ERROR (1 << 1)
#define KG_PDMA_CHANNEL_RD_DATA_ERROR (1 << 2)
#define KG_PDMA_CHANNEL_WR_DATA_ERROR (1 << 3)
#define KG_REG_RD_ERR_DES_SIZE        (0x18)
#define KG_REG_WR_ERR_DES_SIZE        (0x18)
#define KG_REG_RD_DATA_ERR_SIZE       (0x28)
#define KG_REG_WR_DATA_ERR_SIZE       (0x28)

typedef enum kg_dma_channel_e {
    /* Packet DMA channels */
    KG_DMA_CH_RX0 = 0,
    KG_DMA_CH_PACKET_START = KG_DMA_CH_RX0,
    KG_DMA_CH_RX1,
    KG_DMA_CH_RX2,
    KG_DMA_CH_RX3,
    KG_DMA_CH_TX0,
    KG_DMA_CH_TX1,
    KG_DMA_CH_TX2,
    KG_DMA_CH_TX3,
    KG_DMA_CH_PACKET_END = KG_DMA_CH_TX3,

    /* General DMA channels */
    KG_DMA_CH_GEN0,
    KG_DMA_CH_GEN_START = KG_DMA_CH_GEN0,
    KG_DMA_CH_GEN1,
    KG_DMA_CH_GEN2,
    KG_DMA_CH_GEN3,
    KG_DMA_CH_GEN4,
    KG_DMA_CH_GEN5,
    KG_DMA_CH_GEN6,
    KG_DMA_CH_GEN7,
    KG_DMA_CH_GEN_END = KG_DMA_CH_GEN7,

    /* ECPU DMA channels */
    KG_DMA_CH_ECPU_RX,
    KG_DMA_CH_ECPU_TX,

    /* FIFO DMA channels */
    KG_DMA_CH_LRN_FIFO,
    KG_DMA_CH_FIFO_START = KG_DMA_CH_LRN_FIFO,
    KG_DMA_CH_IOAM_FIFO,
    KG_DMA_CH_NFE_FIFO,
    KG_DMA_CH_FIFO_END = KG_DMA_CH_NFE_FIFO,

    KG_DMA_CH_LAST
} kg_dma_channel_t;

#define KG_MAX_UNIT_NUM             (2)  /* max kg unit num*/
#define KG_MAX_ASIC_NUM_PER_UNIT    (2)  /* D2D 2 dies, 1 slice on 1 die*/
#define KG_MAX_DIE_NUM_PER_UNIT     (2)  /* D2D 2 dies*/
#define KG_MAX_SLICE_NUM_PER_UNIT   (1)  /* 1 slice on 1 die*/
#define KG_PORTS_PER_SLICE          (88) /*0-83 physical ports, 84-85 cpu ports, 86-87 rec ports per die*/
#define KG_PKT_CPU_PORT             (84)
#define KG_PKT_SRC_PORT(slice, port) (slice * KG_PORTS_PER_SLICE + port)


#define KG_CFG_PDMA_CHx_RING_BASE(__channel__) (KG_CFG_PDMA_RING_BASE_0 + 0x8 * (__channel__))
#define KG_CFG_PDMA_CHx_RING_SIZE(__channel__) (KG_CFG_PDMA_RING_SIZE_0 + 0x4 * (__channel__))
#define KG_CFG_PDMA_CHx_DESC_WORK_IDX(__channel__) \
    (KG_CFG_PDMA_DESC_WORK_IDX_0 + 0x4 * (__channel__))
#define KG_STA_PDMA_CHx_DESC_POP_IDX(__channel__) (KG_STA_PDMA_DESC_POP_IDX_0 + 0x4 * (__channel__))
#define KG_CFG_PDMA_CHx_MODE(__channel__)         (KG_CFG_PDMA_MODE_0 + 0x4 * (__channel__))
#define KG_CFG_PDMA_CHx_ERR_TYPE(__channel__)     (KG_STA_PDMA_ERR_TYPE_0 + 0x4 * (__channel__))
#define KG_CFG_PDMA_CHx_OUTSTD_DESC(__channel__)  (KG_STA_PDMA_OUTSTD_DESC_0 + 0x4 * (__channel__))
#define KG_CFG_PDMA_CHx_RD_ERR_DESC(__channel__) \
    (KG_STA_PDMA_RD_ERR_DESC_0 + KG_REG_RD_ERR_DES_SIZE * (__channel__))
#define KG_CFG_PDMA_CHx_WB_DESC(__channel__) \
    (KG_STA_PDMA_WB_DESC_0 + KG_REG_WR_ERR_DES_SIZE * (__channel__))
#define KG_CFG_PDMA_CHx_SRC_ERR_INFO(__channel__) \
    (KG_STA_PDMA_SRC_ERR_INFO_0 + KG_REG_RD_DATA_ERR_SIZE * (__channel__))
#define KG_CFG_PDMA_CHx_DST_ERR_INFO(__channel__) \
    (KG_STA_PDMA_DST_ERR_INFO_0 + KG_REG_WR_DATA_ERR_SIZE * (__channel__))

#define KG_PKT_PPH_HDR_SZ            (40)
#define KG_PKT_PDMA_HDR_SZ           (KG_PKT_PPH_HDR_SZ)
#define KG_PKT_TX_MAX_LEN            (KG_MAX_PKT_SIZE)
#define KG_PKT_RX_MAX_LEN            (KG_MAX_PKT_SIZE + KG_PKT_PDMA_HDR_SZ) /* EPP tunnel header */
#define KG_PKT_MIN_LEN               (64) /* Ethernet definition */
#define KG_MAX_PKT_SIZE              (10200)

#define KG_SET_BITMAP(bitmap, mask_bitmap) (bitmap = ((bitmap) | (mask_bitmap)))
#define KG_CLR_BITMAP(bitmap, mask_bitmap) (bitmap = ((bitmap) & (~(mask_bitmap))))
#define KG_GET_BITMAP(flags, bit)          ((((flags) & (bit)) > 0) ? 1 : 0)

/* cpu reason */
#define KG_PKT_RX_MOD_REASON        (511)
#define KG_PKT_RX_EGR_SFLOW_SAMPLER (448)
#define KG_PKT_RX_IGR_SFLOW_SAMPLER (352)

typedef struct {
    uint32_t s_addr_lo : 32;
    uint32_t s_addr_hi : 16;
    uint32_t size : 16;
    uint32_t d_addr_lo : 32;
    uint32_t d_addr_hi : 16;

    /*status[127:112]*/
    uint32_t interrupt : 1;
    uint32_t err : 1;
    uint32_t eop : 1;
    uint32_t sop : 1;
    uint32_t sinc : 1;
    uint32_t dinc : 1;
    uint32_t xfer_size : 5;
    uint32_t limit_xfer_en : 1;
    uint32_t mst_slv_die : 1; // pp fifo master/slave die
    uint32_t reserve : 3;

} kg_descriptor_t;

#if defined(CLX_EN_HOST_64_BIT_BIG_ENDIAN) || defined(CLX_EN_HOST_64_BIT_LITTLE_ENDIAN)
#define KG_DESCRIPTOR_ALIGN_ADDR(pdma_addr, align_sz) \
    (((pdma_addr) + (align_sz)) & 0xFFFFFFFFFFFFFFF0)
#else
#define KG_DESCRIPTOR_ALIGN_ADDR(pdma_addr, align_sz) (((pdma_addr) + (align_sz)) & 0xFFFFFFF0)
#endif

#define KG_PKT_SWAP16(__data__) (((__data__) >> 8) | ((__data__) << 8))
#define KG_PKT_SWAP32(__data__)                                                           \
    (((__data__) >> 24) | (((__data__) >> 8) & 0xFF00) | (((__data__) << 8) & 0xFF0000) | \
     ((__data__) << 24))

typedef enum {
    KG_PKT_PPH_TYPE_L2 = 0,
    KG_PKT_PPH_TYPE_L25,
    KG_PKT_PPH_TYPE_L3UC = 2,
    KG_PKT_PPH_TYPE_L3MC = 2,
    KG_PKT_PPH_TYPE_LAST

} KG_PKT_PPH_TYPE_T;

#pragma pack(push, 1)
#if defined(CLX_EN_HOST_32_BIT_LITTLE_ENDIAN) || defined(CLX_EN_HOST_64_BIT_LITTLE_ENDIAN)
typedef struct {
    uint32_t ptp_info : 32; /* #{seqID_8, ptp_off_7, csmoff_14, flg_3} */
    uint32_t spare3 : 32;
    uint32_t ecn : 2;
    uint32_t cpu_cud : 4;         /* egr direct to cpu see cud_type */
    uint32_t ver : 1;             /*  */
    uint32_t sampler : 7;         /* {vld, idx} only for cpu read */
    uint32_t spare2 : 19;
    uint32_t cpu_reason : 9;      /* reason code */
    uint32_t mirror_bmap : 8;     /*mirror copy bit map*/
    uint32_t asic_id : 7;         /* die id.*/
    uint32_t port_id : 7;         /* port num.*/
    uint32_t igr_vlan : 12;       /* ingress vlan*/
    uint32_t spare1 : 3;
    uint32_t skip_ipp : 1;        /*skip IPP (relevant if ingress from CPU port)*/
    uint32_t spare0 : 8;
    uint32_t mpls_pwcw_vld : 1;   /*MPLS Decap stack has PWCW from ingress LSP*/
    uint32_t mpls_vpn_order : 2;  /* which regular label is VPN or not exist(3) */
    uint32_t qos_tnl_uniform : 1; /*while decap, use outer qos to inner qos*/
    uint32_t decap_act : 3;       /*"decap action
                                    0: no decap
                                    1: MPLS pop or transit
                                    2: IP tunnel decap
                                    3: erspan termination
                                    4: SRH remove, next header field of preceding header may need update
                                    5: IPv6 hdr with all its extention hdr
                                    6: frc decap ip/mpls"*/
    uint32_t igr_is_fab : 1;      /*indicate if igr port_type is FAB*/
    uint32_t timestamp : 32;      /*time stamp 2-bit sec; 30-bit ns*/
    uint32_t evpn_esi : 20;       /*# src_tnl_idx or mpls_esi_label (0 is not valid)*/
    uint32_t spare_b : 7;
    uint32_t stacking : 5;        /* stacking */
    uint32_t src_supp_tag : 6;    /* src tag*/
    uint32_t pcp_dei_vlan : 1;    /* pkt push a prio-vlan for pcp-dei; pop it anyway */
    uint32_t src_bdi : 14;        /*ingress Bdi*/
    uint32_t pkt_journal : 1;     /*instruct downstream pipeline stages to capture states*/
    uint32_t spare_a : 2;
    uint32_t qos_dnt_modify : 1;  /*indicating that egress side not rewrite pkg qos*/
    uint32_t mac_learn_en : 1;
    uint32_t igr_acl_label : 16;  /*Reload as IOAM 10b flow id.*/
    uint32_t skip_epp : 1;        /*skip EPP if destination is to the CPU port. Keep original packet
                                     format*/
    uint32_t src_idx : 14;        /*"0 - 4K: system wide physical port/lag
                                    4K - 20K: system wide single IP/LSP tunnel index to reach remote
                                   peer"*/
    uint32_t dst_idx : 16;        /*"UC_PORT: 0~2k-1
                                    UC_LAG: 3.5k~4k-1
                                    TNL: 4k~12k-1
                                    MC_L2BD: 12k~28k-1
                                    MC_L3MEL: 28k~36k-1"*/
    uint32_t hash_val : 12;       /*for egress mpls/tunnel entropy*/
    uint32_t igr_vid_pop_num : 3; /*ingress vid num to pop*/
    uint32_t ecn_enable : 1;
    uint32_t color : 2;           /*internal drop precedence*/
    uint32_t tc : 3;              /*internal traffic class*/
    uint32_t fwd_op : 2;          /*tenant packet forwarding operation
                                    0: L2
                                    1: L2.5 MPLS
                                    2: L3 UC/L3 MC*/
} kg_pkt_pph_l2_t;

typedef struct {
    uint32_t ptp_info : 32;    /* #{seqID_8, ptp_off_7, csmoff_14, flg_3} */
    uint32_t mac_da_low : 32;  /* low bits of dst mac */
    uint32_t ecn : 2;
    uint32_t cpu_cud : 4;      /* egr direct to cpu see cud_type */
    uint32_t ver : 1;          /*  */
    uint32_t sampler : 7;      /* {vld, idx} only for cpu read */
    uint32_t spare2 : 3;
    uint32_t mac_da_high : 16; /* high bits of  dst mac */
    uint32_t cpu_reason : 9;   /* reason code */
    uint32_t mirror_bmap : 8;  /*mirror copy bit map*/
    uint32_t asic_id : 7;      /* die id.*/
    uint32_t port_id : 7;      /* port num.*/
    uint32_t src_bdi : 14;     /* src di for learning*/
    uint32_t spare1 : 1;
    uint32_t skip_ipp : 1;     /*skip IPP (relevant if ingress from CPU port)*/
    uint32_t spare0 : 1;
    uint32_t usid_arg_en : 1;
    uint32_t usid_func_en : 1;
    uint32_t decr_sl : 1;
    uint32_t nxt_sid_opcode : 2;
    uint32_t decap_prop_ttl : 1;
    uint32_t mpls_inner_l2 : 1;
    uint32_t mpls_pwcw_vld : 1;   /*MPLS Decap stack has PWCW from ingress LSP*/
    uint32_t mpls_vpn_order : 2;  /* which regular label is VPN or not exist(3) */
    uint32_t qos_tnl_uniform : 1; /*while decap, use outer qos to inner qos*/
    uint32_t decap_act : 3;       /*"decap action
                                    0: no decap
                                    1: MPLS pop or transit
                                    2: IP tunnel decap
                                    3: erspan termination
                                    4: SRH remove, next header field of preceding header may need update
                                    5: IPv6 hdr with all its extention hdr
                                    6: frc decap ip/mpls"*/
    uint32_t igr_is_fab : 1;      /*indicate if igr port_type is FAB*/
    uint32_t timestamp : 32;      /*time stamp 2-bit sec; 30-bit ns*/
    uint32_t evpn_esi : 20;       /*# src_tnl_idx or mpls_esi_label (0 is not valid)*/
    uint32_t srv6_encap_end : 1;
    uint32_t srv6_encaps_red : 1;
    uint32_t srv6_insert_red : 1;
    uint32_t spare_d : 4;
    uint32_t stacking : 5;       /* stacking */
    uint32_t src_supp_tag : 6;   /* src tag*/
    uint32_t spare_c : 1;
    uint32_t dst_bdi : 14;       /* dst di; src bdi for L3MC*/
    uint32_t pkt_journal : 1;    /*instruct downstream pipeline stages to capture states*/
    uint32_t spare_b : 2;
    uint32_t qos_dnt_modify : 1; /*indicating that egress side not rewrite pkg qos*/
    uint32_t mac_learn_en : 1;
    uint32_t igr_acl_label : 16; /*Reload as IOAM 10b flow id.*/
    uint32_t skip_epp : 1;       /*skip EPP if destination is to the CPU port. Keep original packet
                                    format*/
    uint32_t src_idx : 14;       /*"0 - 4K: system wide physical port/lag
                                   4K - 20K: system wide single IP/LSP tunnel index to reach remote
                                  peer"*/
    uint32_t dst_idx : 16;       /*"UC_PORT: 0~2k-1
                                   UC_LAG: 3.5k~4k-1
                                   TNL: 4k~12k-1
                                   MC_L2BD: 12k~28k-1
                                   MC_L3MEL: 28k~36k-1"*/
    uint32_t hash_val : 12;      /*for egress mpls/tunnel entropy*/
    uint32_t spare_a : 2;        /* spare */
    uint32_t decr_ttl : 1;       /*decrement TTL*/
    uint32_t ecn_enable : 1;
    uint32_t color : 2;          /*internal drop precedence*/
    uint32_t tc : 3;             /*internal traffic class*/
    uint32_t fwd_op : 2;         /*tenant packet forwarding operation
                                   0: L2
                                   1: L2.5 MPLS
                                   2: L3 UC/L3 MC*/
} kg_pkt_pph_l3uc_t;

typedef kg_pkt_pph_l3uc_t kg_pkt_pph_l3mc_t;

typedef struct {
    uint32_t spare6 : 32;
    uint32_t spare5 : 32;
    uint32_t spare4 : 2;
    uint32_t cpu_cud : 4;         /* egr direct to cpu see cud_type */
    uint32_t ver : 1;             /*  */
    uint32_t sampler : 7;         /* {vld, idx} only for cpu read */
    uint32_t spare3 : 19;
    uint32_t cpu_reason : 9;      /* reason code */
    uint32_t mirror_bmap : 8;     /*mirror copy bit map*/
    uint32_t asic_id : 7;         /* die id.*/
    uint32_t port_id : 7;         /* port num.*/
    uint32_t spare2 : 15;
    uint32_t skip_ipp : 1;        /*skip IPP (relevant if ingress from CPU port)*/
    uint32_t spare1 : 6;
    uint32_t decap_prop_ttl : 1;  /*propagate TTL*/
    uint32_t decr_ttl : 1;        /*decrement TTL*/
    uint32_t spare0 : 1;
    uint32_t mpls_vpn_order : 2;  /* which regular label is VPN or not exist(3) */
    uint32_t qos_tnl_uniform : 1; /*while decap, use outer qos to inner qos*/
    uint32_t decap_act : 3;       /*"decap action
                                    0: no decap
                                    1: MPLS pop or transit
                                    2: IP tunnel decap
                                    3: erspan termination
                                    4: SRH remove, next header field of preceding header may need update
                                    5: IPv6 hdr with all its extention hdr
                                    6: frc decap ip/mpls"*/
    uint32_t igr_is_fab : 1;      /*indicate if igr port_type is FAB*/
    uint32_t timestamp : 32;      /*time stamp 2-bit sec; 30-bit ns*/
    uint32_t mpls_lbl : 20;       /* mpls label */
    uint32_t spare_d : 7;
    uint32_t stacking : 5;        /* stacking */
    uint32_t src_supp_tag : 6;    /* src tag*/
    uint32_t spare_c : 2;
    uint32_t tnl_idx : 13;        /* tunnel idx */
    uint32_t pkt_journal : 1;     /*instruct downstream pipeline stages to capture states*/
    uint32_t spare_b : 2;
    uint32_t qos_dnt_modify : 1;  /*indicating that egress side not rewrite pkg qos*/
    uint32_t mac_learn_en : 1;
    uint32_t igr_acl_label : 16;  /*Reload as IOAM 10b flow id.*/
    uint32_t skip_epp : 1;        /*skip EPP if destination is to the CPU port. Keep original packet
                                     format*/
    uint32_t src_idx : 14;        /*"0 - 4K: system wide physical port/lag
                                    4K - 20K: system wide single IP/LSP tunnel index to reach remote
                                   peer"*/
    uint32_t dst_idx : 16;        /*"UC_PORT: 0~2k-1
                                    UC_LAG: 3.5k~4k-1
                                    TNL: 4k~12k-1
                                    MC_L2BD: 12k~28k-1
                                    MC_L3MEL: 28k~36k-1"*/
    uint32_t hash_val : 12;       /*for egress mpls/tunnel entropy*/
    uint32_t spare_a : 4;
    uint32_t color : 2;           /*internal drop precedence*/
    uint32_t tc : 3;              /*internal traffic class*/
    uint32_t fwd_op : 2;          /*tenant packet forwarding operation
                                    0: L2
                                    1: L2.5 MPLS
                                    2: L3 UC/L3 MC*/
} kg_pkt_pph_l25_t;

#define KG_PKT_HOST_TO_BE16(__data__) KG_PKT_SWAP16(__data__)
#define KG_PKT_HOST_TO_BE32(__data__) KG_PKT_SWAP32(__data__)
#define KG_PKT_HOST_TO_LE16(__data__) (__data__)
#define KG_PKT_HOST_TO_LE32(__data__) (__data__)
#define KG_PKT_BE_TO_HOST16(__data__) KG_PKT_SWAP16(__data__)
#define KG_PKT_BE_TO_HOST32(__data__) KG_PKT_SWAP32(__data__)

#elif defined(CLX_EN_HOST_32_BIT_BIG_ENDIAN) || defined(CLX_EN_HOST_64_BIT_BIG_ENDIAN)
typedef struct {
    uint32_t fwd_op : 2;          /*tenant packet forwarding operation
                                    0: L2
                                    1: L2.5 MPLS
                                    2: L3 UC/L3 MC*/
    uint32_t tc : 3;              /*internal traffic class*/
    uint32_t color : 2;           /*internal drop precedence*/
    uint32_t ecn_enable : 1;
    uint32_t igr_vid_pop_num : 3; /*ingress vid num to pop*/
    uint32_t hash_val : 12;       /*for egress mpls/tunnel entropy*/
    uint32_t dst_idx : 16;        /*"UC_PORT: 0~2k-1
                                    UC_LAG: 3.5k~4k-1
                                    TNL: 4k~12k-1
                                    MC_L2BD: 12k~28k-1
                                    MC_L3MEL: 28k~36k-1"*/
    uint32_t src_idx : 14;        /*"0 - 4K: system wide physical port/lag
                                    4K - 20K: system wide single IP/LSP tunnel index to reach remote
                                   peer"*/
    uint32_t skip_epp : 1;        /*skip EPP if destination is to the CPU port. Keep original packet
                                     format*/
    uint32_t igr_acl_label : 16;  /*Reload as IOAM 10b flow id.*/
    uint32_t mac_learn_en : 1;
    uint32_t qos_dnt_modify : 1;  /*indicating that egress side not rewrite pkg qos*/
    uint32_t spare_a : 2;
    uint32_t pkt_journal : 1;     /*instruct downstream pipeline stages to capture states*/
    uint32_t src_bdi : 14;        /*ingress Bdi*/
    uint32_t pcp_dei_vlan : 1;    /* pkt push a prio-vlan for pcp-dei; pop it anyway */
    uint32_t src_supp_tag : 6;    /* src tag*/
    uint32_t stacking : 5;        /* stacking */
    uint32_t spare_b : 7;
    uint32_t evpn_esi : 20;       /*# src_tnl_idx or mpls_esi_label (0 is not valid)*/
    uint32_t timestamp : 32;      /*time stamp 2-bit sec; 30-bit ns*/
    uint32_t igr_is_fab : 1;      /*indicate if igr port_type is FAB*/
    uint32_t decap_act : 3;       /*"decap action
                                    0: no decap
                                    1: MPLS pop or transit
                                    2: IP tunnel decap
                                    3: erspan termination
                                    4: SRH remove, next header field of preceding header may need update
                                    5: IPv6 hdr with all its extention hdr
                                    6: frc decap ip/mpls"*/
    uint32_t qos_tnl_uniform : 1; /*while decap, use outer qos to inner qos*/
    uint32_t mpls_vpn_order : 2;  /* which regular label is VPN or not exist(3) */
    uint32_t mpls_pwcw_vld : 1;   /*MPLS Decap stack has PWCW from ingress LSP*/
    uint32_t spare0 : 8;
    uint32_t skip_ipp : 1;        /*skip IPP (relevant if ingress from CPU port)*/
    uint32_t spare1 : 3;
    uint32_t igr_vlan : 12;       /* ingress vlan*/
    uint32_t port_id : 7;         /* .*/
    uint32_t asic_id : 7;         /* .*/
    uint32_t mirror_bmap : 8;     /*mirror copy bit map*/
    uint32_t cpu_reason : 9;      /* reason code */
    uint32_t spare2 : 19;
    uint32_t sampler : 7;         /* {vld, idx} only for cpu read */
    uint32_t ver : 1;             /*  */
    uint32_t cpu_cud : 4;         /* egr direct to cpu see cud_type */
    uint32_t ecn : 2;
    uint32_t spare3 : 32;
    uint32_t ptp_info : 32; /* #{seqID_8, ptp_off_7, csmoff_14, flg_3} */
} kg_pkt_pph_l2_t;

typedef struct {
    uint32_t fwd_op : 2;         /*tenant packet forwarding operation
                                   0: L2
                                   1: L2.5 MPLS
                                   2: L3 UC/L3 MC*/
    uint32_t tc : 3;             /*internal traffic class*/
    uint32_t color : 2;          /*internal drop precedence*/
    uint32_t ecn_enable : 1;
    uint32_t decr_ttl : 1;       /*decrement TTL*/
    uint32_t spare_a : 2;        /* spare */
    uint32_t hash_val : 12;      /*for egress mpls/tunnel entropy*/
    uint32_t dst_idx : 16;       /*"UC_PORT: 0~2k-1
                                   UC_LAG: 3.5k~4k-1
                                   TNL: 4k~12k-1
                                   MC_L2BD: 12k~28k-1
                                   MC_L3MEL: 28k~36k-1"*/
    uint32_t src_idx : 14;       /*"0 - 4K: system wide physical port/lag
                                   4K - 20K: system wide single IP/LSP tunnel index to reach remote
                                  peer"*/
    uint32_t skip_epp : 1;       /*skip EPP if destination is to the CPU port. Keep original packet
                                    format*/
    uint32_t igr_acl_label : 16; /*Reload as IOAM 10b flow id.*/
    uint32_t mac_learn_en : 1;
    uint32_t qos_dnt_modify : 1; /*indicating that egress side not rewrite pkg qos*/
    uint32_t spare_b : 2;
    uint32_t pkt_journal : 1;    /*instruct downstream pipeline stages to capture states*/
    uint32_t dst_bdi : 14;       /* dst di; src bdi for L3MC*/
    uint32_t spare_c : 1;
    uint32_t src_supp_tag : 6;   /* src tag*/
    uint32_t stacking : 5;       /* stacking */
    uint32_t spare_d : 4;
    uint32_t srv6_insert_red : 1;
    uint32_t srv6_encaps_red : 1;
    uint32_t srv6_encap_end : 1;
    uint32_t evpn_esi : 20;       /*# src_tnl_idx or mpls_esi_label (0 is not valid)*/
    uint32_t timestamp : 32;      /*time stamp 2-bit sec; 30-bit ns*/
    uint32_t igr_is_fab : 1;      /*indicate if igr port_type is FAB*/
    uint32_t decap_act : 3;       /*"decap action
                                    0: no decap
                                    1: MPLS pop or transit
                                    2: IP tunnel decap
                                    3: erspan termination
                                    4: SRH remove, next header field of preceding header may need update
                                    5: IPv6 hdr with all its extention hdr
                                    6: frc decap ip/mpls"*/
    uint32_t qos_tnl_uniform : 1; /*while decap, use outer qos to inner qos*/
    uint32_t mpls_vpn_order : 2;  /* which regular label is VPN or not exist(3) */
    uint32_t mpls_pwcw_vld : 1;   /*MPLS Decap stack has PWCW from ingress LSP*/
    uint32_t mpls_inner_l2 : 1;
    uint32_t decap_prop_ttl : 1;
    uint32_t nxt_sid_opcode : 2;
    uint32_t decr_sl : 1;
    uint32_t usid_func_en : 1;
    uint32_t usid_arg_en : 1;
    uint32_t spare0 : 1;
    uint32_t skip_ipp : 1;     /*skip IPP (relevant if ingress from CPU port)*/
    uint32_t spare1 : 1;
    uint32_t src_bdi : 14;     /* src di for learning*/
    uint32_t port_id : 7;      /* port num.*/
    uint32_t asic_id : 7;      /* die id.*/
    uint32_t mirror_bmap : 8;  /*mirror copy bit map*/
    uint32_t cpu_reason : 9;   /* reason code */
    uint32_t mac_da_high : 16; /* high bits of  dst mac */
    uint32_t spare2 : 3;
    uint32_t sampler : 7;      /* {vld, idx} only for cpu read */
    uint32_t ver : 1;          /*  */
    uint32_t cpu_cud : 4;      /* egr direct to cpu see cud_type */
    uint32_t ecn : 2;
    uint32_t mac_da_low : 32;  /* low bits of dst mac */
    uint32_t ptp_info : 32;    /* #{seqID_8, ptp_off_7, csmoff_14, flg_3} */
} kg_pkt_pph_l3uc_t;

typedef kg_pkt_pph_l3uc_t kg_pkt_pph_l3mc_t;

typedef struct {
    uint32_t fwd_op : 2;          /*tenant packet forwarding operation
                                    0: L2
                                    1: L2.5 MPLS
                                    2: L3 UC/L3 MC*/
    uint32_t tc : 3;              /*internal traffic class*/
    uint32_t color : 2;           /*internal drop precedence*/
    uint32_t spare_a : 4;
    uint32_t hash_val : 12;       /*for egress mpls/tunnel entropy*/
    uint32_t dst_idx : 16;        /*"UC_PORT: 0~2k-1
                                    UC_LAG: 3.5k~4k-1
                                    TNL: 4k~12k-1
                                    MC_L2BD: 12k~28k-1
                                    MC_L3MEL: 28k~36k-1"*/
    uint32_t src_idx : 14;        /*"0 - 4K: system wide physical port/lag
                                    4K - 20K: system wide single IP/LSP tunnel index to reach remote
                                   peer"*/
    uint32_t skip_epp : 1;        /*skip EPP if destination is to the CPU port. Keep original packet
                                     format*/
    uint32_t igr_acl_label : 16;  /*Reload as IOAM 10b flow id.*/
    uint32_t mac_learn_en : 1;
    uint32_t qos_dnt_modify : 1;  /*indicating that egress side not rewrite pkg qos*/
    uint32_t spare_b : 2;
    uint32_t pkt_journal : 1;     /*instruct downstream pipeline stages to capture states*/
    uint32_t tnl_idx : 13;        /* tunnel idx */
    uint32_t spare_c : 2;
    uint32_t src_supp_tag : 6;    /* src tag*/
    uint32_t stacking : 5;        /* stacking */
    uint32_t spare_d : 7;
    uint32_t mpls_lbl : 20;       /* mpls label */
    uint32_t timestamp : 32;      /*time stamp 2-bit sec; 30-bit ns*/
    uint32_t igr_is_fab : 1;      /*indicate if igr port_type is FAB*/
    uint32_t decap_act : 3;       /*"decap action
                                    0: no decap
                                    1: MPLS pop or transit
                                    2: IP tunnel decap
                                    3: erspan termination
                                    4: SRH remove, next header field of preceding header may need update
                                    5: IPv6 hdr with all its extention hdr
                                    6: frc decap ip/mpls"*/
    uint32_t qos_tnl_uniform : 1; /*while decap, use outer qos to inner qos*/
    uint32_t mpls_vpn_order : 2;  /* which regular label is VPN or not exist(3) */
    uint32_t spare0 : 1;
    uint32_t decr_ttl : 1;        /*decrement TTL*/
    uint32_t decap_prop_ttl : 1;  /*propagate TTL*/
    uint32_t spare1 : 6;
    uint32_t skip_ipp : 1;        /*skip IPP (relevant if ingress from CPU port)*/
    uint32_t spare2 : 15;
    uint32_t port_id : 7;         /* .*/
    uint32_t asic_id : 7;         /* .*/
    uint32_t mirror_bmap : 8;     /*mirror copy bit map*/
    uint32_t cpu_reason : 9;      /* reason code */
    uint32_t spare3 : 19;
    uint32_t sampler : 7;         /* {vld, idx} only for cpu read */
    uint32_t ver : 1;             /*  */
    uint32_t cpu_cud : 4;         /* egr direct to cpu see cud_type */
    uint32_t spare4 : 2;
    uint32_t spare5 : 32;
    uint32_t spare6 : 32;
} kg_pkt_pph_l25_t;

#define KG_PKT_HOST_TO_LE16(__data__) KG_PKT_SWAP16(__data__)
#define KG_PKT_HOST_TO_LE32(__data__) KG_PKT_SWAP32(__data__)
#define KG_PKT_HOST_TO_BE16(__data__) (__data__)
#define KG_PKT_HOST_TO_BE32(__data__) (__data__)
#define KG_PKT_BE_TO_HOST16(__data__) (__data__)
#define KG_PKT_BE_TO_HOST32(__data__) (__data__)

#endif
#pragma pack(pop)

int
kg_init_pkt_driver(uint32_t unit);

void
kg_deinit_pkt_driver(uint32_t unit);

#endif // __KG_CHIP_H__