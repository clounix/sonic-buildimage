/*******************************************************************************
*  Copyright Statement:
*  --------------------
*  This software and the information contained therein are protected by
*  copyright and other intellectual property laws and terms herein is
*  confidential. The software may not be copied and the information
*  contained herein may not be used or disclosed except with the written
*  permission of Hangzhou Clounix Technology Limited. (C) 2020-2023
*
*  BY OPENING THIS FILE, BUYER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
*  THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("CLOUNIX SOFTWARE")
*  RECEIVED FROM CLOUNIX AND/OR ITS REPRESENTATIVES ARE PROVIDED TO BUYER ON
*  AN "AS-IS" BASIS ONLY. CLOUNIX EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES,
*  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
*  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
*  NEITHER DOES CLOUNIX PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
*  SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
*  SUPPLIED WITH THE CLOUNIX SOFTWARE, AND BUYER AGREES TO LOOK ONLY TO SUCH
*  THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. CLOUNIX SHALL ALSO
*  NOT BE RESPONSIBLE FOR ANY CLOUNIX SOFTWARE RELEASES MADE TO BUYER'S
*  SPECIFICATION OR TO CONFORM TO A PARTICULAR STANDARD OR OPEN FORUM.
*
*  BUYER'S SOLE AND EXCLUSIVE REMEDY AND CLOUNIX'S ENTIRE AND CUMULATIVE
*  LIABILITY WITH RESPECT TO THE CLOUNIX SOFTWARE RELEASED HEREUNDER WILL BE,
*  AT CLOUNIX'S OPTION, TO REVISE OR REPLACE THE CLOUNIX SOFTWARE AT ISSUE,
*  OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY BUYER TO
*  CLOUNIX FOR SUCH CLOUNIX SOFTWARE AT ISSUE.
*
*  THE TRANSACTION CONTEMPLATED HEREUNDER SHALL BE CONSTRUED IN ACCORDANCE
*  WITH THE LAWS OF THE PEOPLE'S REPUBLIC OF CHINA, EXCLUDING ITS CONFLICT OF
*  LAWS PRINCIPLES.  ANY DISPUTES, CONTROVERSIES OR CLAIMS ARISING THEREOF AND
*  RELATED THERETO SHALL BE SETTLED BY LAWSUIT IN HANGZHOU,CHINA UNDER.
*
*******************************************************************************/

/* FILE NAME:  hal_mt_namchabarwa_pkt_knl.c
 * PURPOSE:
 *      To provide Linux kernel for PDMA TX/RX control.
 *
 * NOTES:
 *
 */

/*****************************************************************************
 * INCLUDE FILE DECLARATIONS
 *****************************************************************************
 */
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/kthread.h>
#include <linux/semaphore.h>
#include <linux/spinlock.h>
#include <linux/spinlock_types.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/miscdevice.h>
#include <linux/wait.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/pci.h>
#include <linux/module.h>
#include <linux/if.h>
#include <uapi/linux/ethtool.h>
#include <linux/ethtool.h>

/* netif */
#include <netif_osal.h>
#include <netif_perf.h>
#include <netif_nl.h>

#include <mountain/namchabarwa/hal_mt_namchabarwa_pkt_knl.h>
#include <clx_dev_knl.h>

/* clx_sdk */
#include <osal/osal_mdc.h>

/*****************************************************************************
 * CHIP DEPENDENT VARIABLES
 *****************************************************************************
 */
/* Interrupt */
#define HAL_MT_NAMCHABARWA_PKT_ERR_REG(__unit__)                   (_hal_mt_namchabarwa_pkt_err_intr_vec[0].intr_reg)
#define HAL_MT_NAMCHABARWA_PKT_TCH_REG(__unit__, __channel__)      (_hal_mt_namchabarwa_pkt_intr_vec[4 + (__channel__)].intr_reg)
#define HAL_MT_NAMCHABARWA_PKT_RCH_REG(__unit__, __channel__)      (_hal_mt_namchabarwa_pkt_intr_vec[0 + (__channel__)].intr_reg)

#define HAL_MT_NAMCHABARWA_PKT_ERR_EVENT(__unit__)                 (&_hal_mt_namchabarwa_pkt_err_intr_vec[0].intr_event)
#define HAL_MT_NAMCHABARWA_PKT_TCH_EVENT(__unit__, __channel__)    (&_hal_mt_namchabarwa_pkt_intr_vec[4 + (__channel__)].intr_event)
#define HAL_MT_NAMCHABARWA_PKT_RCH_EVENT(__unit__, __channel__)    (&_hal_mt_namchabarwa_pkt_intr_vec[0 + (__channel__)].intr_event)

#define HAL_MT_NAMCHABARWA_PKT_ERR_CNT(__unit__)                   (_hal_mt_namchabarwa_pkt_err_intr_vec[0].intr_cnt)
#define HAL_MT_NAMCHABARWA_PKT_TCH_CNT(__unit__, __channel__)      (_hal_mt_namchabarwa_pkt_intr_vec[4 + (__channel__)].intr_cnt)
#define HAL_MT_NAMCHABARWA_PKT_RCH_CNT(__unit__, __channel__)      (_hal_mt_namchabarwa_pkt_intr_vec[0 + (__channel__)].intr_cnt)

typedef struct
{
    UI32_T                              intr_reg;
    CLX_SEMAPHORE_ID_T                  intr_event;
    UI32_T                              intr_cnt;

} HAL_MT_NAMCHABARWA_PKT_INTR_VEC_T;

typedef struct HAL_MT_NAMCHABARWA_PKT_PROFILE_NODE_S
{
    HAL_MT_NAMCHABARWA_PKT_NETIF_PROFILE_T         *ptr_profile;
    struct HAL_MT_NAMCHABARWA_PKT_PROFILE_NODE_S   *ptr_next_node;

} HAL_MT_NAMCHABARWA_PKT_PROFILE_NODE_T;

typedef struct
{
    HAL_MT_NAMCHABARWA_PKT_NETIF_INTF_T            meta;
    struct net_device                   *ptr_net_dev;
    HAL_MT_NAMCHABARWA_PKT_PROFILE_NODE_T          *ptr_profile_list;  /* the profiles binding to this interface */

} HAL_MT_NAMCHABARWA_PKT_NETIF_PORT_DB_T;


static HAL_MT_NAMCHABARWA_PKT_INTR_VEC_T           _hal_mt_namchabarwa_pkt_intr_vec[] =
{
    { /* 0: RX_CH0   */ 1UL << 0, 0x0, 0 },
    { /* 1: RX_CH1   */ 1UL << 1, 0x0, 0 },
    { /* 2: RX_CH2   */ 1UL << 2, 0x0, 0 },
    { /* 3: RX_CH3   */ 1UL << 3, 0x0, 0 },
    { /* 4: TX_CH0   */ 1UL << 4, 0x0, 0 },
    { /* 5: TX_CH1   */ 1UL << 5, 0x0, 0 },
    { /* 6: TX_CH2   */ 1UL << 6, 0x0, 0 },
    { /* 7: TX_CH3   */ 1UL << 7, 0x0, 0 },
};
static HAL_MT_NAMCHABARWA_PKT_INTR_VEC_T           _hal_mt_namchabarwa_pkt_err_intr_vec[] =
{
    { /* 0: RX_CH0   */ 1UL << 0, 0x0, 0 },
    { /* 1: RX_CH1   */ 1UL << 1, 0x0, 0 },
    { /* 2: RX_CH2   */ 1UL << 2, 0x0, 0 },
    { /* 3: RX_CH3   */ 1UL << 3, 0x0, 0 },
    { /* 4: TX_CH0   */ 1UL << 4, 0x0, 0 },
    { /* 5: TX_CH1   */ 1UL << 5, 0x0, 0 },
    { /* 6: TX_CH2   */ 1UL << 6, 0x0, 0 },
    { /* 7: TX_CH3   */ 1UL << 7, 0x0, 0 },
};

/*****************************************************************************
 * NAMING CONSTANT DECLARATIONS
 *****************************************************************************
 */
/* Sleep Time Definitions */
#define HAL_MT_NAMCHABARWA_PKT_TX_DEQUE_SLEEP()            osal_sleepThread(1000) /* us */
#define HAL_MT_NAMCHABARWA_PKT_RX_DEQUE_SLEEP()            osal_sleepThread(1000) /* us */
#define HAL_MT_NAMCHABARWA_PKT_TX_ENQUE_RETRY_SLEEP()      osal_sleepThread(1000) /* us */
#define HAL_MT_NAMCHABARWA_PKT_RX_ENQUE_RETRY_SLEEP()      osal_sleepThread(1000) /* us */
#define HAL_MT_NAMCHABARWA_PKT_ALLOC_MEM_RETRY_SLEEP()     osal_sleepThread(1000) /* us */

/* Network Device Definitions */
/* In case that the watchdog alarm during warm-boot if intf isn't killed */
#define HAL_MT_NAMCHABARWA_PKT_TX_TIMEOUT                  (30*HZ)
#define HAL_MT_NAMCHABARWA_PKT_MAX_ETH_FRAME_SIZE          (HAL_MT_NAMCHABARWA_PKT_RX_MAX_LEN)
#define HAL_MT_NAMCHABARWA_PKT_MAX_PORT_NUM                (HAL_MT_NAMCHABARWA_PORT_NUM + 1) /* CPU port */

#define HAL_MT_NAMCHABARWA_PKT_NET_PROFILE_NUM_MAX         (256)

static HAL_MT_NAMCHABARWA_PKT_NETIF_PROFILE_T              *_ptr_hal_mt_namchabarwa_pkt_profile_entry[HAL_MT_NAMCHABARWA_PKT_NET_PROFILE_NUM_MAX] = {0};
static HAL_MT_NAMCHABARWA_PKT_NETIF_PORT_DB_T              _hal_mt_namchabarwa_pkt_port_db[HAL_MT_NAMCHABARWA_PKT_MAX_PORT_NUM];

/*****************************************************************************
 * MACRO VLAUE DECLARATIONS
 *****************************************************************************
 */

/*****************************************************************************
 * MACRO FUNCTION DECLARATIONS
 *****************************************************************************
 */
/*---------------------------------------------------------------------------*/
#define HAL_MT_NAMCHABARWA_PKT_GET_DRV_CB_PTR(unit)                (&_hal_mt_namchabarwa_pkt_drv_cb[unit])
/*---------------------------------------------------------------------------*/
#define HAL_MT_NAMCHABARWA_PKT_GET_TX_CB_PTR(unit)                 (&_hal_mt_namchabarwa_pkt_tx_cb[unit])
#define HAL_MT_NAMCHABARWA_PKT_GET_TX_PDMA_PTR(unit, channel)      (&_hal_mt_namchabarwa_pkt_tx_cb[unit].pdma[channel])
#define HAL_MT_NAMCHABARWA_PKT_GET_TX_GPD_PTR(unit, channel, gpd)  (&_hal_mt_namchabarwa_pkt_tx_cb[unit].pdma[channel].ptr_gpd_align_start_addr[gpd])
/*---------------------------------------------------------------------------*/
#define HAL_MT_NAMCHABARWA_PKT_GET_RX_CB_PTR(unit)                 (&_hal_mt_namchabarwa_pkt_rx_cb[unit])
#define HAL_MT_NAMCHABARWA_PKT_GET_RX_PDMA_PTR(unit, channel)      (&_hal_mt_namchabarwa_pkt_rx_cb[unit].pdma[channel])
#define HAL_MT_NAMCHABARWA_PKT_GET_RX_GPD_PTR(unit, channel, gpd)  (&_hal_mt_namchabarwa_pkt_rx_cb[unit].pdma[channel].ptr_gpd_align_start_addr[gpd])
/*---------------------------------------------------------------------------*/
#define HAL_MT_NAMCHABARWA_PKT_GET_PORT_DB(port)                   (&_hal_mt_namchabarwa_pkt_port_db[port])
#define HAL_MT_NAMCHABARWA_PKT_GET_PORT_PROFILE_LIST(port)         (_hal_mt_namchabarwa_pkt_port_db[port].ptr_profile_list)
#define HAL_MT_NAMCHABARWA_PKT_GET_PORT_NETDEV(port)               _hal_mt_namchabarwa_pkt_port_db[port].ptr_net_dev

/*****************************************************************************
 * DATA TYPE DECLARATIONS
 *****************************************************************************
 */
/* ----------------------------------------------------------------------------------- General structure */
typedef struct
{
    UI32_T                          unit;
    UI32_T                          channel;

} HAL_MT_NAMCHABARWA_PKT_ISR_COOKIE_T;

typedef struct
{
    CLX_HUGE_T                      que_id;
    CLX_SEMAPHORE_ID_T              sema;
    UI32_T                          len;      /* Software CPU queue maximum length.        */
    UI32_T                          weight;   /* The weight for thread de-queue algorithm. */

} HAL_MT_NAMCHABARWA_PKT_SW_QUEUE_T;

typedef struct
{
    /* handleErrorTask */
    CLX_THREAD_ID_T                 err_task_id;

    /* INTR dispatcher */
    CLX_ISRLOCK_ID_T                intr_lock;
    UI32_T                          intr_bitmap;

#define HAL_MT_NAMCHABARWA_PKT_INIT_DRV           (1UL << 0)
#define HAL_MT_NAMCHABARWA_PKT_INIT_TASK          (1UL << 1)
#define HAL_MT_NAMCHABARWA_PKT_INIT_INTR          (1UL << 2)
#define HAL_MT_NAMCHABARWA_PKT_INIT_RX_START      (1UL << 3)
    /* a bitmap to record the init status */
    UI32_T                          init_flag;

} HAL_MT_NAMCHABARWA_PKT_DRV_CB_T;

/* ----------------------------------------------------------------------------------- TX structure */
typedef struct
{
    /* CLX_SEMAPHORE_ID_T           sema; */

    /* since the Tx GPD ring may be accessed by multiple process including
     * ndo_start_xmit (SW IRQ), it must be protected with an ISRLOCK
     * instead of the original semaphore
     */
    CLX_ISRLOCK_ID_T                ring_lock;

    UI32_T                          used_idx; /* SW send index = LAMP simulate the Tx HW index */
    UI32_T                          free_idx; /* SW free index */
    UI32_T                          used_gpd_num;
    UI32_T                          free_gpd_num;
    UI32_T                          soft_work_idx;
    UI32_T                          gpd_num;

    HAL_MT_NAMCHABARWA_PKT_TX_GPD_T            *ptr_gpd_start_addr;
    HAL_MT_NAMCHABARWA_PKT_TX_GPD_T            *ptr_gpd_align_start_addr;
    BOOL_T                          err_flag;

    /* ASYNC */
    HAL_MT_NAMCHABARWA_PKT_TX_SW_GPD_T         **pptr_sw_gpd_ring;
    HAL_MT_NAMCHABARWA_PKT_TX_SW_GPD_T         **pptr_sw_gpd_bulk; /* temporary store packets to be enque */

    /* SYNC_INTR */
    CLX_SEMAPHORE_ID_T              sync_intr_sema;

} HAL_MT_NAMCHABARWA_PKT_TX_PDMA_T;

typedef struct
{
    HAL_MT_NAMCHABARWA_PKT_TX_WAIT_T           wait_mode;
    HAL_MT_NAMCHABARWA_PKT_TX_PDMA_T           pdma[HAL_MT_NAMCHABARWA_PKT_TX_CHANNEL_LAST];
    HAL_MT_NAMCHABARWA_PKT_TX_CNT_T            cnt;

    /* handleTxDoneTask */
    CLX_THREAD_ID_T                 isr_task_id[HAL_MT_NAMCHABARWA_PKT_TX_CHANNEL_LAST];
    HAL_MT_NAMCHABARWA_PKT_ISR_COOKIE_T        isr_task_cookie[HAL_MT_NAMCHABARWA_PKT_TX_CHANNEL_LAST];

    /* txTask */
    HAL_MT_NAMCHABARWA_PKT_SW_QUEUE_T          sw_queue;
    CLX_SEMAPHORE_ID_T              sync_sema;
    CLX_THREAD_ID_T                 task_id;
    BOOL_T                          running;     /* TRUE when Init txTask
                                                  * FALSE when Destroy txTask
                                                  */
    /* to block net intf Tx in driver level since netif_tx_disable()
     * cannot always prevent intf from Tx in time
     */
    BOOL_T                          net_tx_allowed;
} HAL_MT_NAMCHABARWA_PKT_TX_CB_T;

/* ----------------------------------------------------------------------------------- RX structure */
typedef struct
{
    CLX_SEMAPHORE_ID_T              sema;
    UI32_T                          pop_idx; /* ring curr index */
    UI32_T                          work_idx; /* ring work index */
    UI32_T                          gpd_num;

    HAL_MT_NAMCHABARWA_PKT_RX_GPD_T            *ptr_gpd_start_addr;
    HAL_MT_NAMCHABARWA_PKT_RX_GPD_T            *ptr_gpd_align_start_addr;
    BOOL_T                          err_flag;
    struct sk_buff                  **pptr_skb_ring;
} HAL_MT_NAMCHABARWA_PKT_RX_PDMA_T;

typedef struct
{
    /* Rx system configuration */
    UI32_T                          buf_len;

    HAL_MT_NAMCHABARWA_PKT_RX_SCHED_T          sched_mode;
    HAL_MT_NAMCHABARWA_PKT_RX_PDMA_T           pdma[HAL_MT_NAMCHABARWA_PKT_RX_CHANNEL_LAST];
    HAL_MT_NAMCHABARWA_PKT_RX_CNT_T            cnt;

    /* handleRxDoneTask */
    CLX_THREAD_ID_T                 isr_task_id[HAL_MT_NAMCHABARWA_PKT_RX_CHANNEL_LAST];
    HAL_MT_NAMCHABARWA_PKT_ISR_COOKIE_T        isr_task_cookie[HAL_MT_NAMCHABARWA_PKT_RX_CHANNEL_LAST];

    /* rxTask */
    HAL_MT_NAMCHABARWA_PKT_SW_QUEUE_T          sw_queue[HAL_MT_NAMCHABARWA_PKT_RX_QUEUE_NUM];
    UI32_T                          deque_idx;
    CLX_SEMAPHORE_ID_T              sync_sema;
    CLX_THREAD_ID_T                 task_id;
    CLX_SEMAPHORE_ID_T              deinit_sema; /* To sync-up the Rx-stop and thread flush queues */
    BOOL_T                          running;     /* TRUE when rxStart
                                                  * FALSE when rxStop
                                                  */

} HAL_MT_NAMCHABARWA_PKT_RX_CB_T;

/* ----------------------------------------------------------------------------------- Network Device */
struct net_device_priv
{
    struct net_device               *ptr_net_dev;
    struct net_device_stats         stats;
    UI32_T                          unit;
    UI32_T                          id;
    UI32_T                          port;
    UI16_T                          vlan;
    UI32_T                          speed;
};

typedef enum
{
    HAL_MT_NAMCHABARWA_PKT_DEST_NETDEV = 0,
    HAL_MT_NAMCHABARWA_PKT_DEST_SDK,
    HAL_MT_NAMCHABARWA_PKT_DEST_NETLINK,
    HAL_MT_NAMCHABARWA_PKT_DEST_DROP,
    HAL_MT_NAMCHABARWA_PKT_DEST_LAST
} HAL_MT_NAMCHABARWA_PKT_DEST_T;

/*****************************************************************************
 * GLOBAL VARIABLE DECLARATIONS
 *****************************************************************************
 */

/*****************************************************************************
 * STATIC VARIABLE DECLARATIONS
 *****************************************************************************
 */
/*---------------------------------------------------------------------------*/
static HAL_MT_NAMCHABARWA_PKT_DRV_CB_T         _hal_mt_namchabarwa_pkt_drv_cb[OSAL_MDC_MAX_CHIPS_PER_SYSTEM];
static HAL_MT_NAMCHABARWA_PKT_TX_CB_T          _hal_mt_namchabarwa_pkt_tx_cb[OSAL_MDC_MAX_CHIPS_PER_SYSTEM];
static HAL_MT_NAMCHABARWA_PKT_RX_CB_T          _hal_mt_namchabarwa_pkt_rx_cb[OSAL_MDC_MAX_CHIPS_PER_SYSTEM];
/*---------------------------------------------------------------------------*/

/*****************************************************************************
 * LOCAL SUBPROGRAM DECLARATIONS
 *****************************************************************************
 */
/* ----------------------------------------------------------------------------------- Interrupt */
/* ----------------------------------------------------------------------------------- Interrupt */
static CLX_ERROR_NO_T
_osal_mdc_dma_intr_callback(
    void                        *ptr_cookie)
{
    UI32_T                      unit = ((AML_DEV_MSI_DATA_T *)ptr_cookie)->unit;
    UI32_T                      irq = ((AML_DEV_MSI_DATA_T *)ptr_cookie)->msi;
    UI32_T                      channel = 0;
    UI32_T                      vec = sizeof(_hal_mt_namchabarwa_pkt_intr_vec) / sizeof(HAL_MT_NAMCHABARWA_PKT_INTR_VEC_T);
    CLX_IRQ_FLAGS_T             irq_flag = 0;
    HAL_MT_NAMCHABARWA_PKT_DRV_CB_T        *ptr_cb = HAL_MT_NAMCHABARWA_PKT_GET_DRV_CB_PTR(unit);
    UI32_T                      pdma_intr_mask = ptr_cb->intr_bitmap, chain_intr_mask = 0xffffffff;
    UI32_T                      intr_status = 0;
    UI32_T                      err_intr_status = 0;
    UI32_T                      channel_mask = 1;
    BOOL_T                      abnormal_intr_flag = FALSE;

    /* Pdma normal interrupt */
    if(irq < HAL_MT_NAMCHABARWA_PKT_RX_CHANNEL_LAST + HAL_MT_NAMCHABARWA_PKT_TX_CHANNEL_LAST)
    {
        channel = irq;
        // mask pdma channel interrupt.
        osal_mdc_writePciReg(unit,
            HAL_MT_NAMCHABARWA_PKT_PDMA_CFG_PDMA2PCIE_INTR_CH0_MASK + channel*0x4,
            &channel_mask, sizeof(UI32_T));

        osal_triggerEvent(&_hal_mt_namchabarwa_pkt_intr_vec[channel].intr_event);
        _hal_mt_namchabarwa_pkt_intr_vec[channel].intr_cnt++;
    }
    /* general dma channel interrupt*/
    else if(irq < HAL_MT_NAMHABARWA_INTR_ALL_MSI_OFFSET)
    {
        channel = irq;
        // mask general channel interrupt.
        osal_mdc_writePciReg(unit,
            HAL_MT_NAMCHABARWA_PKT_PDMA_CFG_PDMA2PCIE_INTR_CH0_MASK + channel*0x4,
            &channel_mask, sizeof(UI32_T));
    }
    /* dma error interrupt */
    else if(irq == HAL_MT_NAMHABARWA_INTR_ALL_MSI_OFFSET)
    {
        // mask chain interrupt
        osal_mdc_writePciReg(unit, HAL_MT_NAMCHABARWA_INTR_TOP_MASK_REG, &chain_intr_mask, sizeof(UI32_T));
        // mask pdma interrupt
        osal_mdc_writePciReg(unit, HAL_MT_NAMCHABARWA_PKT_PDMA_CFG_PDMA2PCIE_INTR_MASK_ALL, &pdma_intr_mask, sizeof(UI32_T));

        osal_mdc_readPciReg(unit, HAL_MT_NAMCHABARWA_PKT_PDMA_STA_PDMA_NORMAL_INTR, &intr_status, sizeof(UI32_T));

        osal_mdc_readPciReg(unit, HAL_MT_NAMCHABARWA_CHAIN28_SLV_INTR_REG, &err_intr_status, sizeof(UI32_T));
        // pdma normal itnr
        if(intr_status)
        {
            while(intr_status != 0)
            {
                if (intr_status & (0x1 << channel))
                {
                    // Mask all channels that generate normal interrupt.
                    osal_mdc_writePciReg(unit,
                            HAL_MT_NAMCHABARWA_PKT_PDMA_CFG_PDMA2PCIE_INTR_CH0_MASK + channel*0x4,
                            &channel_mask, sizeof(UI32_T));
                    // clear pkt dma channel intr. clear others in user-space.
                    if(channel < vec)
                    {
                        osal_triggerEvent(&_hal_mt_namchabarwa_pkt_intr_vec[channel].intr_event);
                        _hal_mt_namchabarwa_pkt_intr_vec[channel].intr_cnt++;
                    }
                    HAL_MT_NAMCHABARWA_PKT_CLR_BITMAP(intr_status, 0x1 << channel);
                }
                channel ++;
            }
        }
        // abnormal interrupt
        if(err_intr_status & HAL_MT_NAMCHABARWA_PDMA_SLV_INTR)
        {
            osal_takeIsrLock(&ptr_cb->intr_lock, &irq_flag);
            for (channel = 0; channel < HAL_MT_NAMCHABARWA_PKT_RX_CHANNEL_LAST + HAL_MT_NAMCHABARWA_PKT_TX_CHANNEL_LAST; channel ++)
            {
                osal_mdc_readPciReg(unit, HAL_MT_NAMCHABARWA_PKT_PDMA_IRQ_PDMA_ABNORMAL_CH0_INTR + channel * 0xC, &intr_status, sizeof(UI32_T));
                if(intr_status == 0)
                {
                    continue;
                }
                // Mask the channel that generate abnormal interrupt.
                osal_mdc_writePciReg(unit, 
                        HAL_MT_NAMCHABARWA_PKT_PDMA_IRQ_PDMA_ABNORMAL_CH0_INTR_MSK + channel * 0xC,
                        &channel_mask, sizeof(UI32_T));
                abnormal_intr_flag = TRUE;
            }

            if(abnormal_intr_flag)
            {
                osal_triggerEvent(HAL_MT_NAMCHABARWA_PKT_ERR_EVENT(unit));
            }
            osal_giveIsrLock(&ptr_cb->intr_lock, &irq_flag);
        }
        pdma_intr_mask = 0;
        osal_takeIsrLock(&ptr_cb->intr_lock, &irq_flag);
        osal_mdc_writePciReg(unit, HAL_MT_NAMCHABARWA_PKT_PDMA_CFG_PDMA2PCIE_INTR_MASK_ALL, &pdma_intr_mask, sizeof(UI32_T));
        osal_giveIsrLock(&ptr_cb->intr_lock, &irq_flag);
    }

    return (CLX_E_OK);
}


static CLX_ERROR_NO_T
_hal_mt_namchabarwa_pkt_maskIntr(
    const UI32_T                unit,
    const UI32_T                channel)
{
    HAL_MT_NAMCHABARWA_PKT_DRV_CB_T        *ptr_cb = HAL_MT_NAMCHABARWA_PKT_GET_DRV_CB_PTR(unit);
    CLX_IRQ_FLAGS_T             irq_flag = 0;
    UI32_T                      intr_mask = 0x1;

    osal_takeIsrLock(&ptr_cb->intr_lock, &irq_flag);
    osal_mdc_writePciReg(unit, 
            HAL_MT_NAMCHABARWA_PKT_PDMA_CFG_PDMA2PCIE_INTR_CH0_MASK + channel*0x4, 
            &intr_mask, sizeof(UI32_T));
    osal_giveIsrLock(&ptr_cb->intr_lock, &irq_flag);

    return (CLX_E_OK);
}

static CLX_ERROR_NO_T
_hal_mt_namchabarwa_pkt_clearIntr(
    const UI32_T                unit,
    const UI32_T                channel)
{
    HAL_MT_NAMCHABARWA_PKT_DRV_CB_T        *ptr_cb = HAL_MT_NAMCHABARWA_PKT_GET_DRV_CB_PTR(unit);
    CLX_IRQ_FLAGS_T             irq_flag = 0;
    UI32_T                      intr_clr = 0x1 << channel;

    osal_takeIsrLock(&ptr_cb->intr_lock, &irq_flag);

    osal_mdc_writePciReg(unit, HAL_MT_NAMCHABARWA_PKT_PDMA_CFW_PDMA2PCIE_INTR_CLR, &intr_clr, sizeof(UI32_T));

    osal_giveIsrLock(&ptr_cb->intr_lock, &irq_flag);

    return (CLX_E_OK);
}

static CLX_ERROR_NO_T
_hal_mt_namchabarwa_pkt_unmaskIntr(
    const UI32_T                unit,
    const UI32_T                channel)
{
    HAL_MT_NAMCHABARWA_PKT_DRV_CB_T        *ptr_cb = HAL_MT_NAMCHABARWA_PKT_GET_DRV_CB_PTR(unit);
    CLX_IRQ_FLAGS_T             irq_flag = 0;
    UI32_T                      intr_unmask = 0x0;

    osal_takeIsrLock(&ptr_cb->intr_lock, &irq_flag);
    osal_mdc_writePciReg(unit, 
            HAL_MT_NAMCHABARWA_PKT_PDMA_CFG_PDMA2PCIE_INTR_CH0_MASK + channel*0x4, 
            &intr_unmask, sizeof(UI32_T));
    osal_giveIsrLock(&ptr_cb->intr_lock, &irq_flag);

    return (CLX_E_OK);
}

/* ----------------------------------------------------------------------------------- RW HW Regs */
/* FUNCTION NAME: _hal_mt_namchabarwa_pkt_setRxWorkIdx
 * PURPOSE:
 *      To set rx channel work index.
 * INPUT:
 *      unit            --  The unit ID
 *      channel         --  The target RX channel
 * OUTPUT:
 *      None.
 * RETURN:
 *      CLX_E_OK        --  Successfully configure the register.
 *      CLX_E_OTHERS    --  Configure the register failed.
 * NOTES:
 *      None
 */
static CLX_ERROR_NO_T
_hal_mt_namchabarwa_pkt_setRxWorkIdx(
    const UI32_T                    unit,
    const HAL_MT_NAMCHABARWA_PKT_RX_CHANNEL_T  channel,
    const UI32_T                    work_idx)
{
    osal_mdc_writePciReg(unit, 
    HAL_MT_NAMCHABARWA_PKT_GET_PDMA_WORD_RCH_REG(HAL_MT_NAMCHABARWA_PKT_GET_MMIO(HAL_MT_NAMCHABARWA_PKT_PDMA_CFG_CH0_DESC_WORK_IDX), channel), 
    &work_idx, sizeof(UI32_T));

    return CLX_E_OK;
}

/* FUNCTION NAME: _hal_mt_namchabarwa_pkt_getRxWorkIdx
 * PURPOSE:
 *      To get rx channel work index.
 * INPUT:
 *      unit            --  The unit ID
 *      channel         --  The target RX channel
 * OUTPUT:
 *      None.
 * RETURN:
 *      CLX_E_OK        --  Successfully configure the register.
 *      CLX_E_OTHERS    --  Configure the register failed.
 * NOTES:
 *      None
 */
static UI32_T
_hal_mt_namchabarwa_pkt_getRxWorkIdx(
    const UI32_T                    unit,
    const HAL_MT_NAMCHABARWA_PKT_RX_CHANNEL_T  channel)
{
    UI32_T                          work_idx;
    osal_mdc_readPciReg(unit, 
    HAL_MT_NAMCHABARWA_PKT_GET_PDMA_WORD_RCH_REG(HAL_MT_NAMCHABARWA_PKT_GET_MMIO(HAL_MT_NAMCHABARWA_PKT_PDMA_CFG_CH0_DESC_WORK_IDX), channel),
    &work_idx, sizeof(UI32_T));
    return work_idx;
}


/* FUNCTION NAME: _hal_mt_namchabarwa_pkt_setTxWorkIdx
 * PURPOSE:
 *      To set tx channel work index.
 * INPUT:
 *      unit            --  The unit ID
 *      channel         --  The target RX channel
 * OUTPUT:
 *      None.
 * RETURN:
 *      CLX_E_OK        --  Successfully configure the register.
 *      CLX_E_OTHERS    --  Configure the register failed.
 * NOTES:
 *      None
 */
static UI32_T
_hal_mt_namchabarwa_pkt_getRxPopIdx(
    const UI32_T                    unit,
    const HAL_MT_NAMCHABARWA_PKT_RX_CHANNEL_T  channel)
{
    UI32_T                          pop_idx;
    osal_mdc_readPciReg(unit, 
    HAL_MT_NAMCHABARWA_PKT_GET_PDMA_WORD_RCH_REG(HAL_MT_NAMCHABARWA_PKT_GET_MMIO(HAL_MT_NAMCHABARWA_PKT_PDMA_CFG_CH0_DESC_POP_IDX), channel),
    &pop_idx, sizeof(UI32_T));
    return pop_idx;
}

/* FUNCTION NAME: _hal_mt_namchabarwa_pkt_setTxWorkIdx
 * PURPOSE:
 *      To set tx channel work index.
 * INPUT:
 *      unit            --  The unit ID
 *      channel         --  The target TX channel
 * OUTPUT:
 *      None.
 * RETURN:
 *      CLX_E_OK        --  Successfully configure the register.
 *      CLX_E_OTHERS    --  Configure the register failed.
 * NOTES:
 *      None
 */
static CLX_ERROR_NO_T
_hal_mt_namchabarwa_pkt_setTxWorkIdx(
    const UI32_T                    unit,
    const HAL_MT_NAMCHABARWA_PKT_TX_CHANNEL_T  channel,
    const UI32_T                    work_idx)
{
    osal_mdc_writePciReg(unit, 
    HAL_MT_NAMCHABARWA_PKT_GET_PDMA_WORD_TCH_REG(HAL_MT_NAMCHABARWA_PKT_GET_MMIO(HAL_MT_NAMCHABARWA_PKT_PDMA_CFG_CH0_DESC_WORK_IDX), channel),
    &work_idx, sizeof(UI32_T));
    return CLX_E_OK;
}

#if 0
/* FUNCTION NAME: _hal_mt_namchabarwa_pkt_getTxWorkIdx
 * PURPOSE:
 *      To get tx channel work index.
 * INPUT:
 *      unit            --  The unit ID
 *      channel         --  The target TX channel
 * OUTPUT:
 *      None.
 * RETURN:
 *      CLX_E_OK        --  Successfully configure the register.
 *      CLX_E_OTHERS    --  Configure the register failed.
 * NOTES:
 *      None
 */
static UI32_T
_hal_mt_namchabarwa_pkt_getTxWorkIdx(
    const UI32_T                    unit,
    const HAL_MT_NAMCHABARWA_PKT_TX_CHANNEL_T  channel)
{
    UI32_T                          work_idx;
    osal_mdc_readPciReg(unit, 
    HAL_MT_NAMCHABARWA_PKT_GET_PDMA_WORD_TCH_REG(HAL_MT_NAMCHABARWA_PKT_GET_MMIO(HAL_MT_NAMCHABARWA_PKT_PDMA_CFG_CH0_DESC_WORK_IDX), channel),
    &work_idx, sizeof(UI32_T));
    return work_idx;
}

/* FUNCTION NAME: _hal_mt_namchabarwa_pkt_getTxPopIdx
 * PURPOSE:
 *      To get tx channel pop index.
 * INPUT:
 *      unit            --  The unit ID
 *      channel         --  The target TX channel
 * OUTPUT:
 *      None.
 * RETURN:
 *      CLX_E_OK        --  Successfully configure the register.
 *      CLX_E_OTHERS    --  Configure the register failed.
 * NOTES:
 *      None
 */
static UI32_T
_hal_mt_namchabarwa_pkt_getTxPopIdx(
    const UI32_T                    unit,
    const HAL_MT_NAMCHABARWA_PKT_TX_CHANNEL_T  channel)
{
    UI32_T                          pop_idx;
    osal_mdc_readPciReg(unit, 
        HAL_MT_NAMCHABARWA_PKT_GET_PDMA_WORD_TCH_REG(HAL_MT_NAMCHABARWA_PKT_GET_MMIO(HAL_MT_NAMCHABARWA_PKT_PDMA_CFG_CH0_DESC_POP_IDX), channel),
        &pop_idx, sizeof(UI32_T));
    return pop_idx;
}
#endif

/* FUNCTION NAME: _hal_mt_namchabarwa_pkt_startTxChannelReg
 * PURPOSE:
 *      To issue "START" command to the target TX channel.
 * INPUT:
 *      unit            --  The unit ID
 *      channel         --  The target TX channel
 * OUTPUT:
 *      None.
 * RETURN:
 *      CLX_E_OK        --  Successfully configure the register.
 *      CLX_E_OTHERS    --  Configure the register failed.
 * NOTES:
 *      None
 */
static CLX_ERROR_NO_T
_hal_mt_namchabarwa_pkt_startTxChannelReg(
    const UI32_T                    unit,
    const HAL_MT_NAMCHABARWA_PKT_TX_CHANNEL_T  channel)
{
    UI32_T                          enable;
    osal_mdc_readPciReg(unit, HAL_MT_NAMCHABARWA_PKT_GET_MMIO(HAL_MT_NAMCHABARWA_PKT_PDMA_CFG_CH_ENABLE), &enable, sizeof(UI32_T));
    HAL_MT_NAMCHABARWA_PKT_SET_BITMAP(enable,(1 << HAL_MT_NAMCHABARWA_PKT_TX_CHANNEL(channel)));
    osal_mdc_writePciReg(unit,HAL_MT_NAMCHABARWA_PKT_GET_MMIO(HAL_MT_NAMCHABARWA_PKT_PDMA_CFG_CH_ENABLE),&enable, sizeof(UI32_T));

    return (CLX_E_OK);
}

/* FUNCTION NAME: _hal_mt_namchabarwa_pkt_startRxChannelReg
 * PURPOSE:
 *      To issue "START" command to the target RX channel.
 * INPUT:
 *      unit            --  The unit ID
 *      channel         --  The target RX channel
 * OUTPUT:
 *      None
 * RETURN:
 *      CLX_E_OK        --  Successfully configure the register.
 *      CLX_E_OTHERS    --  Configure the register failed.
 * NOTES:
 *      None
 */
static CLX_ERROR_NO_T
_hal_mt_namchabarwa_pkt_startRxChannelReg(
    const UI32_T                    unit,
    const HAL_MT_NAMCHABARWA_PKT_RX_CHANNEL_T  channel)
{
    UI32_T                          enable;
    osal_mdc_readPciReg(unit, HAL_MT_NAMCHABARWA_PKT_GET_MMIO(HAL_MT_NAMCHABARWA_PKT_PDMA_CFG_CH_ENABLE), &enable, sizeof(UI32_T));
    HAL_MT_NAMCHABARWA_PKT_SET_BITMAP(enable,(1 << HAL_MT_NAMCHABARWA_PKT_RX_CHANNEL(channel)));
    osal_mdc_writePciReg(unit,HAL_MT_NAMCHABARWA_PKT_GET_MMIO(HAL_MT_NAMCHABARWA_PKT_PDMA_CFG_CH_ENABLE),&enable, sizeof(UI32_T));

    return (CLX_E_OK);
}

/* FUNCTION NAME: _hal_mt_namchabarwa_pkt_stopTxChannelReg
 * PURPOSE:
 *      To issue "STOP" command to the target TX channel.
 * INPUT:
 *      unit            --  The unit ID
 *      channel         --  The target TX channel
 * OUTPUT:
 *      None
 * RETURN:
 *      CLX_E_OK        --  Successfully configure the register.
 *      CLX_E_OTHERS    --  Configure the register failed.
 * NOTES:
 *      None
 */
static CLX_ERROR_NO_T
_hal_mt_namchabarwa_pkt_stopTxChannelReg(
    const UI32_T                    unit,
    const HAL_MT_NAMCHABARWA_PKT_TX_CHANNEL_T  channel)
{
    UI32_T                          enable;
    osal_mdc_readPciReg(unit, HAL_MT_NAMCHABARWA_PKT_GET_MMIO(HAL_MT_NAMCHABARWA_PKT_PDMA_CFG_CH_ENABLE), &enable, sizeof(UI32_T));
    HAL_MT_NAMCHABARWA_PKT_CLR_BITMAP(enable,(1 << HAL_MT_NAMCHABARWA_PKT_TX_CHANNEL(channel)));
    osal_mdc_writePciReg(unit,HAL_MT_NAMCHABARWA_PKT_GET_MMIO(HAL_MT_NAMCHABARWA_PKT_PDMA_CFG_CH_ENABLE),&enable, sizeof(UI32_T));

    return (CLX_E_OK);
}

/* FUNCTION NAME: _hal_mt_namchabarwa_pkt_stopRxChannelReg
 * PURPOSE:
 *      To issue "STOP" command to the target RX channel.
 * INPUT:
 *      unit            --  The unit ID
 *      channel         --  The target RX channel
 * OUTPUT:
 *      None
 * RETURN:
 *      CLX_E_OK        --  Successfully configure the register.
 *      CLX_E_OTHERS    --  Configure the register failed.
 * NOTES:
 *      None
 */
static CLX_ERROR_NO_T
_hal_mt_namchabarwa_pkt_stopRxChannelReg(
    const UI32_T                    unit,
    const HAL_MT_NAMCHABARWA_PKT_RX_CHANNEL_T  channel)
{
    UI32_T                          enable;
    osal_mdc_readPciReg(unit, HAL_MT_NAMCHABARWA_PKT_GET_MMIO(HAL_MT_NAMCHABARWA_PKT_PDMA_CFG_CH_ENABLE), &enable, sizeof(UI32_T));
    HAL_MT_NAMCHABARWA_PKT_SET_BITMAP(enable,(1 << HAL_MT_NAMCHABARWA_PKT_RX_CHANNEL(channel)));
    osal_mdc_writePciReg(unit,HAL_MT_NAMCHABARWA_PKT_GET_MMIO(HAL_MT_NAMCHABARWA_PKT_PDMA_CFG_CH_ENABLE),&enable, sizeof(UI32_T));

    return (CLX_E_OK);
}

static CLX_ERROR_NO_T
_hal_mt_namchabarwa_pkt_resetRxChannelReg(
    const UI32_T                    unit,
    const HAL_MT_NAMCHABARWA_PKT_RX_CHANNEL_T  channel)
{
    UI32_T                          reset;
    osal_mdc_readPciReg(unit, HAL_MT_NAMCHABARWA_PKT_GET_MMIO(HAL_MT_NAMCHABARWA_PKT_PDMA_CFW_CHANNEL_RESET), &reset, sizeof(UI32_T));
    HAL_MT_NAMCHABARWA_PKT_CLR_BITMAP(reset,(1 << HAL_MT_NAMCHABARWA_PKT_RX_CHANNEL(channel)));
    osal_mdc_writePciReg(unit,HAL_MT_NAMCHABARWA_PKT_GET_MMIO(HAL_MT_NAMCHABARWA_PKT_PDMA_CFW_CHANNEL_RESET),&reset, sizeof(UI32_T));
    HAL_MT_NAMCHABARWA_PKT_SET_BITMAP(reset,(1 << HAL_MT_NAMCHABARWA_PKT_RX_CHANNEL(channel)));
    osal_mdc_writePciReg(unit,HAL_MT_NAMCHABARWA_PKT_GET_MMIO(HAL_MT_NAMCHABARWA_PKT_PDMA_CFW_CHANNEL_RESET),&reset, sizeof(UI32_T));

    return (CLX_E_OK);
}

static CLX_ERROR_NO_T
_hal_mt_namchabarwa_pkt_resetTxChannelReg(
    const UI32_T                    unit,
    const HAL_MT_NAMCHABARWA_PKT_TX_CHANNEL_T  channel)
{
    UI32_T                          reset;
    osal_mdc_readPciReg(unit, HAL_MT_NAMCHABARWA_PKT_GET_MMIO(HAL_MT_NAMCHABARWA_PKT_PDMA_CFW_CHANNEL_RESET), &reset, sizeof(UI32_T));
    HAL_MT_NAMCHABARWA_PKT_CLR_BITMAP(reset,(1 << HAL_MT_NAMCHABARWA_PKT_TX_CHANNEL(channel)));
    osal_mdc_writePciReg(unit,HAL_MT_NAMCHABARWA_PKT_GET_MMIO(HAL_MT_NAMCHABARWA_PKT_PDMA_CFW_CHANNEL_RESET),&reset, sizeof(UI32_T));
    HAL_MT_NAMCHABARWA_PKT_SET_BITMAP(reset,(1 << HAL_MT_NAMCHABARWA_PKT_TX_CHANNEL(channel)));
    osal_mdc_writePciReg(unit,HAL_MT_NAMCHABARWA_PKT_GET_MMIO(HAL_MT_NAMCHABARWA_PKT_PDMA_CFW_CHANNEL_RESET),&reset, sizeof(UI32_T));

    return (CLX_E_OK);
}

CLX_ERROR_NO_T
_hal_mt_namchabarwa_pkt_globalResetPdmaReg(
    const UI32_T                    unit)
{
    UI32_T                          reset = 0;
    osal_mdc_writePciReg(unit,HAL_MT_NAMCHABARWA_PKT_GET_MMIO(HAL_MT_NAMCHABARWA_PKT_PDMA_CFW_PDMA_CH_RESET),&reset, sizeof(UI32_T));

    reset = 1;
    osal_mdc_writePciReg(unit,HAL_MT_NAMCHABARWA_PKT_GET_MMIO(HAL_MT_NAMCHABARWA_PKT_PDMA_CFW_PDMA_CH_RESET),&reset, sizeof(UI32_T));

    return (CLX_E_OK);
}

static CLX_ERROR_NO_T
_hal_mt_namchabarwa_pkt_restartRxChannelReg(
    const UI32_T                    unit,
    const HAL_MT_NAMCHABARWA_PKT_RX_CHANNEL_T  channel)
{
    UI32_T                          reset;
    osal_mdc_readPciReg(unit, HAL_MT_NAMCHABARWA_PKT_GET_MMIO(HAL_MT_NAMCHABARWA_PKT_PDMA_CFW_CHANNEL_RESTART), &reset, sizeof(UI32_T));
    HAL_MT_NAMCHABARWA_PKT_SET_BITMAP(reset,(1 << HAL_MT_NAMCHABARWA_PKT_RX_CHANNEL(channel)));
    osal_mdc_writePciReg(unit,HAL_MT_NAMCHABARWA_PKT_GET_MMIO(HAL_MT_NAMCHABARWA_PKT_PDMA_CFW_CHANNEL_RESTART),&reset, sizeof(UI32_T));

    return (CLX_E_OK);
}

static CLX_ERROR_NO_T
_hal_mt_namchabarwa_pkt_restartTxChannelReg(
    const UI32_T                    unit,
    const HAL_MT_NAMCHABARWA_PKT_TX_CHANNEL_T  channel)
{
    UI32_T                          reset;
    osal_mdc_readPciReg(unit, HAL_MT_NAMCHABARWA_PKT_GET_MMIO(HAL_MT_NAMCHABARWA_PKT_PDMA_CFW_CHANNEL_RESTART), &reset, sizeof(UI32_T));
    HAL_MT_NAMCHABARWA_PKT_SET_BITMAP(reset,(1 << HAL_MT_NAMCHABARWA_PKT_TX_CHANNEL(channel)));
    osal_mdc_writePciReg(unit,HAL_MT_NAMCHABARWA_PKT_GET_MMIO(HAL_MT_NAMCHABARWA_PKT_PDMA_CFW_CHANNEL_RESTART),&reset, sizeof(UI32_T));

    return (CLX_E_OK);
}

static CLX_ERROR_NO_T
_hal_mt_namchabarwa_pkt_clearPcxErrIntrReg(const UI32_T unit)
{
    UI32_T                          data;

    /*PCX_TOP_SYM_CMST_RESP_ERR_LOG reg Read Clear*/
    osal_mdc_readPciReg(unit, HAL_MT_NAMCHABARWA_PKT_GET_MMIO(HAL_MT_NAMCHABARWA_PCX_TOP_SYM_CMST_RESP_ERR_LOG), &data, sizeof(UI32_T));

    return (CLX_E_OK);
}

static CLX_ERROR_NO_T
_hal_mt_namchabarwa_pkt_clearRxPdmaAbnormalIntrReg(
    const UI32_T                    unit,
    const HAL_MT_NAMCHABARWA_PKT_RX_CHANNEL_T  channel)
{

    UI32_T                          data;

    /*PDMA_IRQ_ABNORMAL_INTR reg Write 1 Clear*/
    // osal_mdc_readPciReg(unit, HAL_MT_NAMCHABARWA_PKT_GET_MMIO(HAL_MT_NAMCHABARWA_PKT_PDMA_IRQ_ABNORMAL_INTR), &data, sizeof(UI32_T));
    // HAL_MT_NAMCHABARWA_PKT_SET_BITMAP(data,(1 << HAL_MT_NAMCHABARWA_PKT_RX_CHANNEL(channel)));
    // osal_mdc_writePciReg(unit,HAL_MT_NAMCHABARWA_PKT_GET_MMIO(HAL_MT_NAMCHABARWA_PKT_PDMA_IRQ_ABNORMAL_INTR),&data, sizeof(UI32_T));
    data = 1;
    osal_mdc_writePciReg(unit,HAL_MT_NAMCHABARWA_PKT_GET_MMIO(HAL_MT_NAMCHABARWA_PKT_PDMA_IRQ_PDMA_ABNORMAL_CH0_INTR + 0xC * channel),&data, sizeof(UI32_T));

    return (CLX_E_OK);
}

static CLX_ERROR_NO_T
_hal_mt_namchabarwa_pkt_clearTxPdmaAbnormalIntrReg(
    const UI32_T                    unit,
    const HAL_MT_NAMCHABARWA_PKT_TX_CHANNEL_T  channel)
{

    UI32_T                          data;

    /*reg Write 1 Clear*/
    osal_mdc_readPciReg(unit, HAL_MT_NAMCHABARWA_PKT_GET_MMIO(HAL_MT_NAMCHABARWA_PKT_PDMA_IRQ_ABNORMAL_INTR), &data, sizeof(UI32_T));
    HAL_MT_NAMCHABARWA_PKT_SET_BITMAP(data,(1 << HAL_MT_NAMCHABARWA_PKT_TX_CHANNEL(channel)));
    osal_mdc_writePciReg(unit,HAL_MT_NAMCHABARWA_PKT_GET_MMIO(HAL_MT_NAMCHABARWA_PKT_PDMA_IRQ_ABNORMAL_INTR),&data, sizeof(UI32_T));

    return (CLX_E_OK);
}

static CLX_ERROR_NO_T
_hal_mt_namchabarwa_pkt_unmaskRxPdmaAbnormalIntrReg(
    const UI32_T                    unit,
    const HAL_MT_NAMCHABARWA_PKT_RX_CHANNEL_T  channel)
{

    UI32_T                          pdma_intr_mask = 0;

    osal_mdc_writePciReg(unit,
        HAL_MT_NAMCHABARWA_PKT_PDMA_IRQ_PDMA_ABNORMAL_CH0_INTR_MSK + channel * 0xC,
        &pdma_intr_mask, sizeof(UI32_T));
    return (CLX_E_OK);
}

CLX_ERROR_NO_T
_hal_mt_namchabarwa_pkt_unmaskTxPdmaAbnormalIntrReg(
    const UI32_T                    unit,
    const HAL_MT_NAMCHABARWA_PKT_TX_CHANNEL_T  channel)
{

    UI32_T                          pdma_intr_mask = 0;

    osal_mdc_writePciReg(unit,
        HAL_MT_NAMCHABARWA_PKT_PDMA_IRQ_PDMA_ABNORMAL_CH0_INTR_MSK + HAL_MT_NAMCHABARWA_PKT_TX_CHANNEL(channel) * 0xC,
        &pdma_intr_mask, sizeof(UI32_T));
    return (CLX_E_OK);
}

static CLX_ERROR_NO_T
_hal_mt_namchabarwa_pkt_swapRxEndianReg(
    const UI32_T                    unit,
    const HAL_MT_NAMCHABARWA_PKT_RX_CHANNEL_T  channel)
{
    UI32_T                          swap;
    osal_mdc_readPciReg(unit, HAL_MT_NAMCHABARWA_PKT_GET_MMIO(HAL_MT_NAMCHABARWA_PKT_PDMA_CFG_DATA_ENDIAN), &swap, sizeof(UI32_T));
    HAL_MT_NAMCHABARWA_PKT_SET_BITMAP(swap,(1 << HAL_MT_NAMCHABARWA_PKT_RX_CHANNEL(channel)));
    osal_mdc_writePciReg(unit,HAL_MT_NAMCHABARWA_PKT_GET_MMIO(HAL_MT_NAMCHABARWA_PKT_PDMA_CFG_DATA_ENDIAN),&swap, sizeof(UI32_T));

    return (CLX_E_OK);
}

static CLX_ERROR_NO_T
_hal_mt_namchabarwa_pkt_swapTxEndianReg(
    const UI32_T                    unit,
    const HAL_MT_NAMCHABARWA_PKT_RX_CHANNEL_T  channel)
{
    UI32_T                          swap;
    osal_mdc_readPciReg(unit, HAL_MT_NAMCHABARWA_PKT_GET_MMIO(HAL_MT_NAMCHABARWA_PKT_PDMA_CFG_DATA_ENDIAN), &swap, sizeof(UI32_T));
    HAL_MT_NAMCHABARWA_PKT_SET_BITMAP(swap,(1 << HAL_MT_NAMCHABARWA_PKT_TX_CHANNEL(channel)));
    osal_mdc_writePciReg(unit,HAL_MT_NAMCHABARWA_PKT_GET_MMIO(HAL_MT_NAMCHABARWA_PKT_PDMA_CFG_DATA_ENDIAN),&swap, sizeof(UI32_T));

    return (CLX_E_OK);
}

/* ----------------------------------------------------------------------------------- Init HW Regs */
/* FUNCTION NAME: _hal_mt_namchabarwa_pkt_setTxGpdStartAddrReg
 * PURPOSE:
 *      To configure the start address and the length of target GPD ring of TX channel.
 * INPUT:
 *      unit            --  The unit ID
 *      channel         --  The target TX channel
 *      gpd_start_addr  --  The start address of the GPD ring
 *      gpd_ring_sz     --  The size of the GPD ring
 * OUTPUT:
 *      None
 * RETURN:
 *      CLX_E_OK        --  Successfully configure the register.
 *      CLX_E_OTHERS    --  Configure the register failed.
 * NOTES:
 *      None
 */
static CLX_ERROR_NO_T
_hal_mt_namchabarwa_pkt_setTxGpdStartAddrReg(
    const UI32_T                        unit,
    const HAL_MT_NAMCHABARWA_PKT_TX_CHANNEL_T      channel,
    const CLX_ADDR_T                    gpd_start_addr,
    const UI32_T                        gpd_ring_sz)
{
    CLX_ERROR_NO_T      rc = CLX_E_OK;

    /* Configure the low 64-bit address. */
    rc = osal_mdc_writePciReg(unit,
            HAL_MT_NAMCHABARWA_PKT_GET_PDMA_DWORD_TCH_REG(HAL_MT_NAMCHABARWA_PKT_GET_MMIO(HAL_MT_NAMCHABARWA_PKT_PDMA_CFG_CH0_RING_BASE), channel),
            (UI32_T *)&gpd_start_addr, sizeof(CLX_ADDR_T));

    /* Configure the GPD ring size. */
    if (CLX_E_OK == rc)
    {
        rc = osal_mdc_writePciReg(unit,
            HAL_MT_NAMCHABARWA_PKT_GET_PDMA_WORD_TCH_REG(HAL_MT_NAMCHABARWA_PKT_GET_MMIO(HAL_MT_NAMCHABARWA_PKT_PDMA_CFG_CH0_RING_SIZE), channel),
            &gpd_ring_sz, sizeof(UI32_T));
    }
    OSAL_PRINT((OSAL_DBG_TX | OSAL_DBG_DEBUG),
                                    "u=%u, txch=%u, ring_base:0x%llx, ring_size:%d \n",
                                    unit, channel, gpd_start_addr, gpd_ring_sz);
    return (rc);
}

/* FUNCTION NAME: _hal_mt_namchabarwa_pkt_setRxGpdStartAddrReg
 * PURPOSE:
 *      To configure the start address and the length of target GPD ring of RX channel.
 * INPUT:
 *      unit            --  The unit ID
 *      channel         --  The target RX channel
 *      gpd_start_addr  --  The start address of the GPD ring
 *      gpd_ring_sz     --  The size of the GPD ring
 * OUTPUT:
 *      None
 * RETURN:
 *      CLX_E_OK        --  Successfully configure the register.
 *      CLX_E_OTHERS    --  Configure the register failed.
 * NOTES:
 *      None
 */
static CLX_ERROR_NO_T
_hal_mt_namchabarwa_pkt_setRxGpdStartAddrReg(
    const UI32_T                        unit,
    const HAL_MT_NAMCHABARWA_PKT_RX_CHANNEL_T      channel,
    const CLX_ADDR_T                    gpd_start_addr,
    const UI32_T                        gpd_ring_sz)
{
    CLX_ERROR_NO_T      rc = CLX_E_OK;

    /* Configure the 64-bit address. */
    rc = osal_mdc_writePciReg(unit,
            HAL_MT_NAMCHABARWA_PKT_GET_PDMA_DWORD_RCH_REG(HAL_MT_NAMCHABARWA_PKT_GET_MMIO(HAL_MT_NAMCHABARWA_PKT_PDMA_CFG_CH0_RING_BASE), channel),
            (UI32_T *)&gpd_start_addr, sizeof(CLX_ADDR_T));

    /* Configure the GPD ring size. */
    if (CLX_E_OK == rc)
    {
        rc = osal_mdc_writePciReg(unit,
            HAL_MT_NAMCHABARWA_PKT_GET_PDMA_WORD_RCH_REG(HAL_MT_NAMCHABARWA_PKT_GET_MMIO(HAL_MT_NAMCHABARWA_PKT_PDMA_CFG_CH0_RING_SIZE), channel),
            &gpd_ring_sz, sizeof(UI32_T));
    }
    OSAL_PRINT((OSAL_DBG_RX | OSAL_DBG_DEBUG),
                                    "u=%u, rxch=%u, ring_base:0x%llx, ring_size:%d \n",
                                    unit, channel, gpd_start_addr, gpd_ring_sz);
    return (rc);
}

/* ----------------------------------------------------------------------------------- ISR HW Regs */

/* FUNCTION NAME: hal_mt_namchabarwa_pkt_getTxIntrCnt
 * PURPOSE:
 *      To get the PDMA TX interrupt counters of the target channel.
 * INPUT:
 *      unit            -- The unit ID
 *      channel         -- The target channel
 * OUTPUT:
 *      None
 * RETURN:
 *      CLX_E_OK        -- Successfully get the counters.
 * NOTES:
 *      None
 */
CLX_ERROR_NO_T
hal_mt_namchabarwa_pkt_getTxIntrCnt(
    const UI32_T            unit,
    const UI32_T            channel,
    UI32_T                  *ptr_intr_cnt)
{
    *ptr_intr_cnt = HAL_MT_NAMCHABARWA_PKT_TCH_CNT(unit, channel);
    return (CLX_E_OK);
}

/* FUNCTION NAME: hal_mt_namchabarwa_pkt_getRxIntrCnt
 * PURPOSE:
 *      To get the PDMA RX interrupt counters of the target channel.
 * INPUT:
 *      unit            -- The unit ID
 *      channel         -- The target channel
 * OUTPUT:
 *      None
 * RETURN:
 *      CLX_E_OK        -- Successfully get the counters.
 * NOTES:
 *      None
 */
CLX_ERROR_NO_T
hal_mt_namchabarwa_pkt_getRxIntrCnt(
    const UI32_T            unit,
    const UI32_T            channel,
    UI32_T                  *ptr_intr_cnt)
{
    *ptr_intr_cnt = HAL_MT_NAMCHABARWA_PKT_RCH_CNT(unit, channel);
    return (CLX_E_OK);
}

/* FUNCTION NAME: hal_mt_namchabarwa_pkt_getTxKnlCnt
 * PURPOSE:
 *      To get the PDMA TX counters of the target channel.
 * INPUT:
 *      unit            -- The unit ID
 *      ptr_cookie      -- Pointer of the TX cookie
 * OUTPUT:
 *      None
 * RETURN:
 *      CLX_E_OK        -- Successfully get the counters.
 * NOTES:
 *      None
 */
CLX_ERROR_NO_T
hal_mt_namchabarwa_pkt_getTxKnlCnt(
    const UI32_T                        unit,
    void                                *ptr_data)
{
    HAL_MT_NAMCHABARWA_PKT_TX_CB_T                  *ptr_tx_cb = HAL_MT_NAMCHABARWA_PKT_GET_TX_CB_PTR(unit);
    HAL_MT_NAMCHABARWA_PKT_IOCTL_CH_CNT_COOKIE_T    *ptr_cookie = ptr_data;

    osal_io_copyToUser(&ptr_cookie->tx_cnt, &ptr_tx_cb->cnt, sizeof(HAL_MT_NAMCHABARWA_PKT_TX_CNT_T));
    return (CLX_E_OK);
}

/* FUNCTION NAME: hal_mt_namchabarwa_pkt_getRxKnlCnt
 * PURPOSE:
 *      To get the PDMA RX counters of the target channel.
 * INPUT:
 *      unit            -- The unit ID
 *      ptr_cookie      -- Pointer of the RX cookie
 * OUTPUT:
 *      None
 * RETURN:
 *      CLX_E_OK        -- Successfully get the counters.
 * NOTES:
 *      None
 */
CLX_ERROR_NO_T
hal_mt_namchabarwa_pkt_getRxKnlCnt(
    const UI32_T                        unit,
    void                                *ptr_data)
{
    HAL_MT_NAMCHABARWA_PKT_RX_CB_T                  *ptr_rx_cb = HAL_MT_NAMCHABARWA_PKT_GET_RX_CB_PTR(unit);
    HAL_MT_NAMCHABARWA_PKT_IOCTL_CH_CNT_COOKIE_T    *ptr_cookie = ptr_data;

    osal_io_copyToUser(&ptr_cookie->rx_cnt, &ptr_rx_cb->cnt, sizeof(HAL_MT_NAMCHABARWA_PKT_RX_CNT_T));
    return (CLX_E_OK);
}

/* FUNCTION NAME: hal_mt_namchabarwa_pkt_clearTxKnlCnt
 * PURPOSE:
 *      To clear the PDMA TX counters of the target channel.
 * INPUT:
 *      unit            -- The unit ID
 *      ptr_cookie      -- Pointer of the TX cookie
 * OUTPUT:
 *      None
 * RETURN:
 *      CLX_E_OK        -- Successfully clear the counters.
 * NOTES:
 *      None
 */
CLX_ERROR_NO_T
hal_mt_namchabarwa_pkt_clearTxKnlCnt(
    const UI32_T                    unit,
    void                            *ptr_data)
{
    HAL_MT_NAMCHABARWA_PKT_TX_CB_T             *ptr_tx_cb = HAL_MT_NAMCHABARWA_PKT_GET_TX_CB_PTR(unit);
    UI32_T                                      channel = 0;
    HAL_MT_NAMCHABARWA_PKT_IOCTL_TX_COOKIE_T    *ptr_cookie = ptr_data;

    osal_io_copyFromUser(&channel, &ptr_cookie->channel, sizeof(UI32_T));
    if (channel >= HAL_MT_NAMCHABARWA_PKT_TX_CHANNEL_LAST)
    {
        OSAL_PRINT(OSAL_DBG_ERR,
                            "u=%u, tx channel=%u is outof range[0-3]\n",
                            unit, channel);
        return (CLX_E_BAD_PARAMETER);
    }

    ptr_tx_cb->cnt.deque_ok   = 0x0;
    ptr_tx_cb->cnt.deque_fail = 0x0;
    ptr_tx_cb->cnt.wait_event = 0x0;
    ptr_tx_cb->cnt.no_memory  = 0x0;

    osal_memset(&ptr_tx_cb->cnt.channel[channel], 0x0,
                sizeof(HAL_MT_NAMCHABARWA_PKT_TX_CHANNEL_CNT_T));
    return (CLX_E_OK);
}

/* FUNCTION NAME: hal_mt_namchabarwa_pkt_clearRxKnlCnt
 * PURPOSE:
 *      To clear the PDMA RX counters of the target channel.
 * INPUT:
 *      unit            -- The unit ID
 *      ptr_cookie      -- Pointer of the RX cookie
 * OUTPUT:
 *      None
 * RETURN:
 *      CLX_E_OK        -- Successfully clear the counters.
 * NOTES:
 *      None
 */
CLX_ERROR_NO_T
hal_mt_namchabarwa_pkt_clearRxKnlCnt(
    const UI32_T                    unit,
    void                            *ptr_data)
{
    HAL_MT_NAMCHABARWA_PKT_RX_CB_T              *ptr_rx_cb = HAL_MT_NAMCHABARWA_PKT_GET_RX_CB_PTR(unit);
    UI32_T                                      channel = 0;
    HAL_MT_NAMCHABARWA_PKT_IOCTL_RX_COOKIE_T    *ptr_cookie = ptr_data;

    osal_io_copyFromUser(&channel, &ptr_cookie->channel, sizeof(UI32_T));
    if (channel >= HAL_MT_NAMCHABARWA_PKT_RX_CHANNEL_LAST)
    {
        OSAL_PRINT(OSAL_DBG_ERR,
                            "u=%u, rx channel=%u is outof range[0-3]\n",
                            unit, channel);
        return (CLX_E_BAD_PARAMETER);
    }

    ptr_rx_cb->cnt.invoke_gpd_callback = 0x0;
    ptr_rx_cb->cnt.wait_event          = 0x0;
    ptr_rx_cb->cnt.no_memory           = 0x0;

    osal_memset(&ptr_rx_cb->cnt.channel[channel], 0x0,
                sizeof(HAL_MT_NAMCHABARWA_PKT_RX_CHANNEL_CNT_T));
    return (CLX_E_OK);
}

/* FUNCTION NAME: hal_mt_namchabarwa_pkt_setPortAttr
 * PURPOSE:
 *      To set the port attributes such as status or speeds.
 * INPUT:
 *      unit            -- The unit ID
 *      ptr_cookie      -- Pointer of the Port cookie
 * OUTPUT:
 *      None
 * RETURN:
 *      CLX_E_OK        -- Successfully set the attributes.
 * NOTES:
 *      None
 */
CLX_ERROR_NO_T
hal_mt_namchabarwa_pkt_setPortAttr(
    const UI32_T                        unit,
    void                                *ptr_data)
{
#define HAL_MT_NAMCHABARWA_PKT_PORT_STATUS_UP          (1)
#define HAL_MT_NAMCHABARWA_PKT_PORT_STATUS_DOWN        (0)
    struct net_device                   *ptr_net_dev;
    struct net_device_priv              *ptr_priv;
    UI32_T                              port;
    UI32_T                              status;
    UI32_T                              speed;
    HAL_MT_NAMCHABARWA_PKT_IOCTL_PORT_COOKIE_T     *ptr_cookie = ptr_data;

    osal_io_copyFromUser(&port,   &ptr_cookie->port, sizeof(UI32_T));
    osal_io_copyFromUser(&status, &ptr_cookie->status, sizeof(UI32_T));
    osal_io_copyFromUser(&speed,  &ptr_cookie->speed, sizeof(UI32_T));

    /* coverity: port used as an index of _hal_mt_namchabarwa_pkt_port_db[129] */
    if (HAL_MT_NAMCHABARWA_PKT_MAX_PORT_NUM > port)
    {
        ptr_net_dev = HAL_MT_NAMCHABARWA_PKT_GET_PORT_NETDEV(port);
    }
    else
    {
        OSAL_PRINT(OSAL_DBG_ERR,
                            "u=%u, port=%u is outof range[0-128]\n",
                            unit, port);
        return (CLX_E_BAD_PARAMETER);
    }

    if ((NULL != ptr_net_dev) && (port<HAL_MT_NAMCHABARWA_PKT_MAX_PORT_NUM))
    {
        if (HAL_MT_NAMCHABARWA_PKT_PORT_STATUS_UP == status)
        {
            netif_carrier_on(ptr_net_dev);
        }
        else
        {
            netif_carrier_off(ptr_net_dev);
        }

        /* Link speed config */
        ptr_priv = netdev_priv(ptr_net_dev);
        ptr_priv->speed = speed;
    }
    return (CLX_E_OK);
}

/* FUNCTION NAME: hal_mt_namchabarwa_pkt_getPortAttr
 * PURPOSE:
 *      To get the port attributes such as status or speeds.
 * INPUT:
 *      unit            -- The unit ID
 *      ptr_cookie      -- Pointer of the Port cookie
 * OUTPUT:
 *      None
 * RETURN:
 *      CLX_E_OK        -- Successfully set the attributes.
 * NOTES:
 *      None
 */
CLX_ERROR_NO_T
hal_mt_namchabarwa_pkt_getPortAttr(
    const UI32_T                              unit,
    void                                      *ptr_data)
{
    HAL_MT_NAMCHABARWA_PKT_IOCTL_PORT_COOKIE_T     *ptr_cookie = ptr_data;
    struct net_device                   *ptr_net_dev = NULL;
    struct net_device_priv              *ptr_priv;
    UI32_T                              port;
    UI32_T                              status;
    UI32_T                              speed;

    port = ptr_cookie->port;
    //TODO_FIXME_PORT
    //ptr_net_dev = HAL_NETIF_MT_NAMCHABARWA_PKT_GET_PORT_NETDEV(unit,port);
    if ((NULL == ptr_net_dev) || (port >= HAL_MT_NAMCHABARWA_PKT_MAX_PORT_NUM))
    {
         OSAL_PRINT( OSAL_DBG_ERR,
            "%s(%d): Failed to get netdev, port %d\n",
                __FUNCTION__, __LINE__, port);
        return -1;
    }
    status = netif_carrier_ok(ptr_net_dev);

    ptr_priv = netdev_priv(ptr_net_dev);
    speed = ptr_priv->speed;
    ptr_cookie->status = status;
    ptr_cookie->speed = speed;
    return (CLX_E_OK);
}

static void
_hal_mt_namchabarwa_pkt_lockRxChannelAll(
    const UI32_T                        unit)
{
    UI32_T                              rch;
    HAL_MT_NAMCHABARWA_PKT_RX_PDMA_T               *ptr_rx_pdma;

    for (rch = 0; rch < HAL_MT_NAMCHABARWA_PKT_RX_CHANNEL_LAST; rch++)
    {
        ptr_rx_pdma = HAL_MT_NAMCHABARWA_PKT_GET_RX_PDMA_PTR(unit, rch);
        osal_takeSemaphore(&ptr_rx_pdma->sema, CLX_SEMAPHORE_WAIT_FOREVER);
    }
}

static void
_hal_mt_namchabarwa_pkt_unlockRxChannelAll(
    const UI32_T                        unit)
{
    UI32_T                              rch;
    HAL_MT_NAMCHABARWA_PKT_RX_PDMA_T               *ptr_rx_pdma;

    for (rch = 0; rch < HAL_MT_NAMCHABARWA_PKT_RX_CHANNEL_LAST; rch++)
    {
        ptr_rx_pdma = HAL_MT_NAMCHABARWA_PKT_GET_RX_PDMA_PTR(unit, rch);
        osal_giveSemaphore(&ptr_rx_pdma->sema);
    }
}


static CLX_ERROR_NO_T
_hal_mt_namchabarwa_pkt_setIntfProperty(
    const UI32_T                        unit,
    void                                *ptr_data)
{
    UI32_T                              intf_id;
    NETIF_NL_INTF_PROPERTY_T            property;
    UI32_T                              param0;
    UI32_T                              param1;
    CLX_ERROR_NO_T                      rc;
    HAL_MT_NAMCHABARWA_PKT_NL_IOCTL_COOKIE_T       *ptr_cookie = ptr_data;

    osal_io_copyFromUser(&intf_id,  &ptr_cookie->intf_id,  sizeof(UI32_T));
    osal_io_copyFromUser(&property, &ptr_cookie->property, sizeof(NETIF_NL_INTF_PROPERTY_T));
    osal_io_copyFromUser(&param0,   &ptr_cookie->param0,   sizeof(UI32_T));
    osal_io_copyFromUser(&param1,   &ptr_cookie->param1,   sizeof(UI32_T));

    _hal_mt_namchabarwa_pkt_lockRxChannelAll(unit);

    /* coverity: intf_id used as an offset of intf_entry[256] */
    if (HAL_MT_NAMCHABARWA_PKT_NET_PROFILE_NUM_MAX > intf_id)
    {
        rc = netif_nl_setIntfProperty(unit, intf_id, property, param0, param1);
    }
    else
    {
        OSAL_PRINT(OSAL_DBG_ERR,
                            "u=%u, intf_id=%u is outof range[0-255]\n",
                            unit, intf_id);
        rc = CLX_E_BAD_PARAMETER;
    }

    _hal_mt_namchabarwa_pkt_unlockRxChannelAll(unit);

    osal_io_copyToUser(&ptr_cookie->rc, &rc, sizeof(CLX_ERROR_NO_T));

    return (rc);
}

static CLX_ERROR_NO_T
_hal_mt_namchabarwa_pkt_getIntfProperty(
    const UI32_T                        unit,
    void                                *ptr_data)
{
    UI32_T                              intf_id;
    NETIF_NL_INTF_PROPERTY_T            property;
    UI32_T                              param0;
    UI32_T                              param1;
    CLX_ERROR_NO_T                      rc;
    HAL_MT_NAMCHABARWA_PKT_NL_IOCTL_COOKIE_T       *ptr_cookie = ptr_data;

    osal_io_copyFromUser(&intf_id,  &ptr_cookie->intf_id,  sizeof(UI32_T));
    osal_io_copyFromUser(&property, &ptr_cookie->property, sizeof(NETIF_NL_INTF_PROPERTY_T));
    osal_io_copyFromUser(&param0,   &ptr_cookie->param0,   sizeof(UI32_T));
    osal_io_copyFromUser(&param1,   &ptr_cookie->param1,   sizeof(UI32_T));

    /* coverity: intf_id used as an offset of intf_entry[256] */
    if (HAL_MT_NAMCHABARWA_PKT_NET_PROFILE_NUM_MAX > intf_id)
    {
        rc = netif_nl_getIntfProperty(unit, intf_id, property, &param0, &param1);
    }
    else
    {
        OSAL_PRINT(OSAL_DBG_ERR,
                            "u=%u, intf_id=%u is outof range[0-255]\n",
                            unit, intf_id);
        rc = CLX_E_BAD_PARAMETER;
    }

    osal_io_copyToUser(&ptr_cookie->param0, &param0, sizeof(UI32_T));
    osal_io_copyToUser(&ptr_cookie->param1, &param1, sizeof(UI32_T));
    osal_io_copyToUser(&ptr_cookie->rc, &rc, sizeof(CLX_ERROR_NO_T));

    return (rc);
}

static CLX_ERROR_NO_T
_hal_mt_namchabarwa_pkt_createNetlink(
    const UI32_T                        unit,
    void                                *ptr_data)
{
    NETIF_NL_NETLINK_T                  netlink;
    UI32_T                              netlink_id;
    CLX_ERROR_NO_T                      rc;
    HAL_MT_NAMCHABARWA_PKT_NL_IOCTL_COOKIE_T       *ptr_cookie = ptr_data;

    osal_io_copyFromUser(&netlink, &ptr_cookie->netlink, sizeof(NETIF_NL_NETLINK_T));

    _hal_mt_namchabarwa_pkt_lockRxChannelAll(unit);

    if (NETIF_NL_NETLINK_MC_GROUP_NUM > netlink.mc_group_num)
    {
        rc = netif_nl_createNetlink(unit, &netlink, &netlink_id);
    }
    else
    {
        OSAL_PRINT(OSAL_DBG_ERR,
                                    "u=%u, netlink.mc_group_num=%d is outof range[0-(8k-1)]\n",
                                    unit, netlink.mc_group_num);
        return (CLX_E_BAD_PARAMETER);
    }

    _hal_mt_namchabarwa_pkt_unlockRxChannelAll(unit);

    osal_io_copyToUser(&ptr_cookie->netlink.id, &netlink_id, sizeof(UI32_T));
    osal_io_copyToUser(&ptr_cookie->rc, &rc, sizeof(CLX_ERROR_NO_T));

    return (rc);
}

static CLX_ERROR_NO_T
_hal_mt_namchabarwa_pkt_destroyNetlink(
    const UI32_T                        unit,
    void                                *ptr_data)
{
    UI32_T                              netlink_id;
    CLX_ERROR_NO_T                      rc;
    HAL_MT_NAMCHABARWA_PKT_NL_IOCTL_COOKIE_T       *ptr_cookie = ptr_data;

    osal_io_copyFromUser(&netlink_id, &ptr_cookie->netlink.id, sizeof(UI32_T));

    _hal_mt_namchabarwa_pkt_lockRxChannelAll(unit);

    /* coverity: intf_id used as an offset of _netif_nl_cb.fam_entry[256] */
    if (HAL_MT_NAMCHABARWA_PKT_NET_PROFILE_NUM_MAX > netlink_id)
    {
        rc = netif_nl_destroyNetlink(unit, netlink_id);
    }
    else
    {
        OSAL_PRINT(OSAL_DBG_ERR,
                            "u=%u, netlink_id=%u is outof range[0-255]\n",
                            unit, netlink_id);
        rc = CLX_E_BAD_PARAMETER;
    }

    _hal_mt_namchabarwa_pkt_unlockRxChannelAll(unit);

    osal_io_copyToUser(&ptr_cookie->rc, &rc, sizeof(CLX_ERROR_NO_T));

    return (rc);
}

static CLX_ERROR_NO_T
_hal_mt_namchabarwa_pkt_getNetlink(
    const UI32_T                        unit,
    void                                *ptr_data)
{
    UI32_T                              id;
    NETIF_NL_NETLINK_T                  netlink;
    CLX_ERROR_NO_T                      rc;
    HAL_MT_NAMCHABARWA_PKT_NL_IOCTL_COOKIE_T       *ptr_cookie = ptr_data;

    osal_io_copyFromUser(&id, &ptr_cookie->netlink.id, sizeof(UI32_T));

    /* coverity: intf_id used as an offset of _netif_nl_cb.fam_entry[256] */
    if (HAL_MT_NAMCHABARWA_PKT_NET_PROFILE_NUM_MAX > id)
    {
        rc = netif_nl_getNetlink(unit, id, &netlink);
    }
    else
    {
        OSAL_PRINT(OSAL_DBG_ERR,
                            "u=%u, id=%u is outof range[0-255]\n",
                            unit, id);
        rc = CLX_E_BAD_PARAMETER;
    }

    if (CLX_E_OK == rc)
    {
        osal_io_copyToUser(&ptr_cookie->netlink, &netlink, sizeof(NETIF_NL_NETLINK_T));
    }
    else
    {
        rc = CLX_E_ENTRY_NOT_FOUND;
    }

    osal_io_copyToUser(&ptr_cookie->rc, &rc, sizeof(CLX_ERROR_NO_T));

    return (CLX_E_OK);
}


/* ----------------------------------------------------------------------------------- independent func */
/* FUNCTION NAME: _hal_mt_namchabarwa_pkt_enQueue
 * PURPOSE:
 *      To enqueue the target data.
 * INPUT:
 *      ptr_que     -- Pointer for the target queue
 *      ptr_data    -- Pointer for the data to be enqueued
 * OUTPUT:
 *      None
 * RETURN:
 *      CLX_E_OK    -- Successfully enqueue the data.
 * NOTES:
 *      None
 */
static CLX_ERROR_NO_T
_hal_mt_namchabarwa_pkt_enQueue(
    HAL_MT_NAMCHABARWA_PKT_SW_QUEUE_T  *ptr_que,
    void                    *ptr_data)
{
    CLX_ERROR_NO_T          rc = CLX_E_OK;

    osal_takeSemaphore(&ptr_que->sema, CLX_SEMAPHORE_WAIT_FOREVER);
    rc = osal_que_enque(&ptr_que->que_id, ptr_data);
    osal_giveSemaphore(&ptr_que->sema);

    return (rc);
}

/* FUNCTION NAME: _hal_mt_namchabarwa_pkt_deQueue
 * PURPOSE:
 *      To dequeue the target data.
 * INPUT:
 *      ptr_que     -- Pointer for the target queue
 *      pptr_data   -- Pointer for the data pointer to be dequeued
 * OUTPUT:
 *      None
 * RETURN:
 *      CLX_E_OK    -- Successfully dequeue the data.
 * NOTES:
 *      None
 */
static CLX_ERROR_NO_T
_hal_mt_namchabarwa_pkt_deQueue(
    HAL_MT_NAMCHABARWA_PKT_SW_QUEUE_T  *ptr_que,
    void                    **pptr_data)
{
    CLX_ERROR_NO_T          rc = CLX_E_OK;

    osal_takeSemaphore(&ptr_que->sema, CLX_SEMAPHORE_WAIT_FOREVER);
    rc = osal_que_deque(&ptr_que->que_id, pptr_data);
    osal_giveSemaphore(&ptr_que->sema);

    return (rc);
}

/* FUNCTION NAME: _hal_mt_namchabarwa_pkt_getQueueCount
 * PURPOSE:
 *      To obtain the current GPD number in the target RX queue.
 * INPUT:
 *      ptr_que             -- Pointer for the target queue
 *      ptr_count           -- Pointer for the data count
 * OUTPUT:
 *      None
 * RETURN:
 *      CLX_E_OK            -- Successfully obtain the GPD count.
 *      CLX_E_BAD_PARAMETER -- Parameter pointer is null.
 * NOTES:
 *      None
 */
static CLX_ERROR_NO_T
_hal_mt_namchabarwa_pkt_getQueueCount(
    HAL_MT_NAMCHABARWA_PKT_SW_QUEUE_T  *ptr_que,
    UI32_T                  *ptr_count)
{
    CLX_ERROR_NO_T          rc = CLX_E_OK;

    osal_takeSemaphore(&ptr_que->sema, CLX_SEMAPHORE_WAIT_FOREVER);
    osal_que_getCount(&ptr_que->que_id, ptr_count);
    osal_giveSemaphore(&ptr_que->sema);

    return (rc);
}

/* FUNCTION NAME: _hal_mt_namchabarwa_pkt_allocRxPayloadBuf
 * PURPOSE:
 *      To allocate the RX packet payload buffer for the GPD.
 * INPUT:
 *      unit            --  The unit ID
 *      channel         --  The target RX channel
 *      gpd_idx         --  The current GPD index
 * OUTPUT:
 *      None
 * RETURN:
 *      CLX_E_OK        --  Successfully allocate the buffer.
 *      CLX_E_NO_MEMORY --  Allocate the buffer failed.
 * NOTES:
 *      None
 */
static CLX_ERROR_NO_T
_hal_mt_namchabarwa_pkt_allocRxPayloadBuf(
    const UI32_T                    unit,
    const UI32_T                    channel,
    const UI32_T                    gpd_idx)
{
    CLX_ERROR_NO_T                  rc = CLX_E_OK;
    HAL_MT_NAMCHABARWA_PKT_RX_CB_T             *ptr_rx_cb = HAL_MT_NAMCHABARWA_PKT_GET_RX_CB_PTR(unit);
    volatile HAL_MT_NAMCHABARWA_PKT_RX_GPD_T   *ptr_rx_gpd = HAL_MT_NAMCHABARWA_PKT_GET_RX_GPD_PTR(unit, channel, gpd_idx);
    CLX_ADDR_T                      phy_addr = 0;

    HAL_MT_NAMCHABARWA_PKT_RX_PDMA_T           *ptr_rx_pdma = HAL_MT_NAMCHABARWA_PKT_GET_RX_PDMA_PTR(unit, channel);
    struct sk_buff                  *ptr_skb = NULL;

    ptr_skb = osal_skb_alloc(ptr_rx_cb->buf_len);
    if (NULL == ptr_skb)
    {
        return (CLX_E_NO_MEMORY); 
    }

    /* map skb to dma */
    phy_addr = osal_skb_mapDma(ptr_skb, DMA_FROM_DEVICE);
    if (0x0 == phy_addr)
    {
        OSAL_PRINT(OSAL_DBG_ERR,
                        "u=%u, rxch=%u, skb dma map err, size=%u\n",
                        unit, channel, ptr_skb->len);
        osal_skb_free(ptr_skb);
        return (CLX_E_NO_MEMORY);
    }

    ptr_rx_pdma->pptr_skb_ring[gpd_idx] = ptr_skb;
    ptr_rx_gpd->d_addr_hi               = CLX_ADDR_64_HI(phy_addr);
    ptr_rx_gpd->d_addr_lo               = CLX_ADDR_64_LOW(phy_addr);
    ptr_rx_gpd->size                    = ptr_rx_cb->buf_len;
    ptr_rx_gpd->interrupt               = 0;

    osal_dma_flushCache((void *)ptr_rx_gpd, sizeof(HAL_MT_NAMCHABARWA_PKT_RX_GPD_T));
    return (rc);
}

/* FUNCTION NAME: _hal_mt_namchabarwa_pkt_freeRxPayloadBuf
 * PURPOSE:
 *      To free the RX packet payload buffer for the GPD.
 * INPUT:
 *      unit            --  The unit ID
 *      channel         --  The target RX channel
 *      gpd_idx         --  The current GPD index
 * OUTPUT:
 *      None
 * RETURN:
 *      CLX_E_OK        --  Successfully free the buffer.
 * NOTES:
 *      None
 */
static CLX_ERROR_NO_T
_hal_mt_namchabarwa_pkt_freeRxPayloadBuf(
    const UI32_T                    unit,
    const UI32_T                    channel,
    const UI32_T                    gpd_idx)
{
    CLX_ERROR_NO_T                  rc = CLX_E_OK;
    volatile HAL_MT_NAMCHABARWA_PKT_RX_GPD_T   *ptr_rx_gpd = HAL_MT_NAMCHABARWA_PKT_GET_RX_GPD_PTR(unit, channel, gpd_idx);
    CLX_ADDR_T                      phy_addr = 0;

    HAL_MT_NAMCHABARWA_PKT_RX_PDMA_T           *ptr_rx_pdma = HAL_MT_NAMCHABARWA_PKT_GET_RX_PDMA_PTR(unit, channel);
    struct sk_buff                  *ptr_skb = NULL;

    phy_addr = CLX_ADDR_32_TO_64(ptr_rx_gpd->d_addr_hi, ptr_rx_gpd->d_addr_lo);
    if(0x0 == phy_addr)
    {
        return (CLX_E_OTHERS);
    }

    /* unmap dma */
    ptr_skb = ptr_rx_pdma->pptr_skb_ring[gpd_idx];
    osal_skb_unmapDma(phy_addr, ptr_skb->len, DMA_FROM_DEVICE);
    osal_skb_free(ptr_skb);
    osal_memset((void *)ptr_rx_gpd,0,sizeof(HAL_MT_NAMCHABARWA_PKT_RX_GPD_T));

    return (rc);
}

/* FUNCTION NAME: _hal_mt_namchabarwa_pkt_freeRxPayloadBufGpd
 * PURPOSE:
 *      To free the RX packet payload buffer for the GPD.
 * INPUT:
 *      unit            --  The unit ID
 *      ptr_sw_gpd      --  The pointer of RX SW GPD
 * OUTPUT:
 *      None
 * RETURN:
 *      CLX_E_OK        --  Successfully free the buffer.
 * NOTES:
 *      None
 */
static CLX_ERROR_NO_T
_hal_mt_namchabarwa_pkt_freeRxPayloadBufGpd(
    const UI32_T                    unit,
    HAL_MT_NAMCHABARWA_PKT_RX_SW_GPD_T         *ptr_sw_gpd)
{
    CLX_ERROR_NO_T                  rc = CLX_E_OK;
    CLX_ADDR_T                      phy_addr = 0;

    struct sk_buff                  *ptr_skb = NULL;

    phy_addr = CLX_ADDR_32_TO_64(ptr_sw_gpd->rx_gpd.d_addr_hi, ptr_sw_gpd->rx_gpd.d_addr_lo);
    if(0x0 == phy_addr)
    {
        return (CLX_E_OTHERS);
    }

    ptr_skb = ptr_sw_gpd->ptr_cookie;
    osal_skb_free(ptr_skb);

    return (rc);
}

/* FUNCTION NAME: _hal_mt_namchabarwa_initTxPdmaRing
 * PURPOSE:
 *      To initialize the GPD ring of target TX channel.
 *
 * INPUT:
 *      unit            --  The unit ID
 *      channel         --  The target TX channel
 * OUTPUT:
 *      None
 * RETURN:
 *      CLX_E_OK        -- Successfully initialize the GPD ring.
 * NOTES:
 *      None
 */
static CLX_ERROR_NO_T
_hal_mt_namchabarwa_pkt_initTxPdmaRing(
    const UI32_T                    unit,
    const HAL_MT_NAMCHABARWA_PKT_TX_CHANNEL_T  channel)
{
    CLX_ERROR_NO_T                  rc = CLX_E_OK;
    HAL_MT_NAMCHABARWA_PKT_TX_PDMA_T           *ptr_tx_pdma = HAL_MT_NAMCHABARWA_PKT_GET_TX_PDMA_PTR(unit, channel);
    volatile HAL_MT_NAMCHABARWA_PKT_TX_GPD_T   *ptr_tx_gpd = NULL;
    CLX_ADDR_T                      phy_addr = 0;
    UI32_T                          gpd_idx = 0;

    for (gpd_idx = 0; gpd_idx < ptr_tx_pdma->gpd_num; gpd_idx++)
    {
        ptr_tx_gpd = HAL_MT_NAMCHABARWA_PKT_GET_TX_GPD_PTR(unit, channel, gpd_idx);
        osal_memset((void *)ptr_tx_gpd, 0x0, sizeof(HAL_MT_NAMCHABARWA_PKT_TX_GPD_T));
        osal_dma_flushCache((void *)ptr_tx_gpd, sizeof(HAL_MT_NAMCHABARWA_PKT_TX_GPD_T));
    }

    phy_addr = osal_dma_convertVirtToPhy(ptr_tx_pdma->ptr_gpd_align_start_addr);
    rc = _hal_mt_namchabarwa_pkt_setTxGpdStartAddrReg(unit, channel, phy_addr, ptr_tx_pdma->gpd_num);

    return (rc);
}

/* FUNCTION NAME: _hal_mt_namchabarwa_pkt_initRxPdmaRing
 * PURPOSE:
 *      To initialize the RX GPD ring.
 * INPUT:
 *      unit            --  The target unit
 *      channel         --  The target RX channel
 * OUTPUT:
 *      None
 * RETURN:
 *      CLX_E_OK        --  Successfully initialize the RX GPD ring.
 * NOTES:
 *      None
 */
static CLX_ERROR_NO_T
_hal_mt_namchabarwa_pkt_initRxPdmaRing(
    const UI32_T                    unit,
    const HAL_MT_NAMCHABARWA_PKT_RX_CHANNEL_T  channel)
{
    CLX_ERROR_NO_T                  rc = CLX_E_OK;
    HAL_MT_NAMCHABARWA_PKT_RX_PDMA_T           *ptr_rx_pdma = HAL_MT_NAMCHABARWA_PKT_GET_RX_PDMA_PTR(unit, channel);
    volatile HAL_MT_NAMCHABARWA_PKT_RX_GPD_T   *ptr_rx_gpd = NULL;
    CLX_ADDR_T                      phy_addr = 0;
    UI32_T                          gpd_idx = 0;

    for (gpd_idx = 0; gpd_idx < ptr_rx_pdma->gpd_num; gpd_idx++)
    {
        ptr_rx_gpd = HAL_MT_NAMCHABARWA_PKT_GET_RX_GPD_PTR(unit, channel, gpd_idx);
        osal_memset((void *)ptr_rx_gpd, 0x0, sizeof(HAL_MT_NAMCHABARWA_PKT_RX_GPD_T));
        ptr_rx_gpd->dinc = 1;
        osal_dma_flushCache((void *)ptr_rx_gpd, sizeof(HAL_MT_NAMCHABARWA_PKT_RX_GPD_T));
    }

    phy_addr = osal_dma_convertVirtToPhy(ptr_rx_pdma->ptr_gpd_align_start_addr);
    rc = _hal_mt_namchabarwa_pkt_setRxGpdStartAddrReg(unit, channel, phy_addr, ptr_rx_pdma->gpd_num);

    return (rc);
}

/* FUNCTION NAME: _hal_mt_namchabarwa_pkt_initRxPdmaRingBuf
 * PURPOSE:
 *      To de-init the Rx PDMA ring configuration.
 * INPUT:
 *      unit            --  The unit ID
 *      channel         --  The target RX channel
 * OUTPUT:
 *      None
 * RETURN:
 *      CLX_E_OK        --  Successfully de-init the Rx PDMA ring.
 * NOTES:
 *      None
 */
static CLX_ERROR_NO_T
_hal_mt_namchabarwa_pkt_initRxPdmaRingBuf(
    const UI32_T                    unit,
    const HAL_MT_NAMCHABARWA_PKT_RX_CHANNEL_T  channel)
{
    CLX_ERROR_NO_T                  rc = CLX_E_OK;
    HAL_MT_NAMCHABARWA_PKT_RX_CB_T             *ptr_rx_cb = HAL_MT_NAMCHABARWA_PKT_GET_RX_CB_PTR(unit);
    HAL_MT_NAMCHABARWA_PKT_RX_PDMA_T           *ptr_rx_pdma = HAL_MT_NAMCHABARWA_PKT_GET_RX_PDMA_PTR(unit, channel);
    volatile HAL_MT_NAMCHABARWA_PKT_RX_GPD_T   *ptr_rx_gpd = NULL;
    UI32_T                          gpd_idx = 0;

    if (0 == ptr_rx_cb->buf_len)
    {
        return (CLX_E_BAD_PARAMETER);
    }

    for (gpd_idx = 0; gpd_idx < ptr_rx_pdma->gpd_num; gpd_idx++)
    {
        ptr_rx_gpd = HAL_MT_NAMCHABARWA_PKT_GET_RX_GPD_PTR(unit, channel, gpd_idx);
        osal_dma_invalidateCache((void *)ptr_rx_gpd, sizeof(HAL_MT_NAMCHABARWA_PKT_RX_GPD_T));

        rc = _hal_mt_namchabarwa_pkt_allocRxPayloadBuf(unit, channel, gpd_idx);
        if (CLX_E_OK == rc)
        {
            osal_dma_flushCache((void *)ptr_rx_gpd, sizeof(HAL_MT_NAMCHABARWA_PKT_RX_GPD_T));
        }
        else
        {
            ptr_rx_cb->cnt.no_memory++;
            break;
        }
    }

    return (rc);
}

/* FUNCTION NAME: _hal_mt_namchabarwa_pkt_deinitRxPdmaRingBuf
 * PURPOSE:
 *      To de-init the Rx PDMA ring configuration.
 * INPUT:
 *      unit            --  The unit ID
 *      channel         --  The target RX channel
 * OUTPUT:
 *      None
 * RETURN:
 *      CLX_E_OK        --  Successfully de-init the Rx PDMA ring.
 * NOTES:
 *      None
 */
static CLX_ERROR_NO_T
_hal_mt_namchabarwa_pkt_deinitRxPdmaRingBuf(
    const UI32_T                    unit,
    const HAL_MT_NAMCHABARWA_PKT_RX_CHANNEL_T  channel)
{
    CLX_ERROR_NO_T                  rc = CLX_E_OK;
    HAL_MT_NAMCHABARWA_PKT_RX_PDMA_T           *ptr_rx_pdma = HAL_MT_NAMCHABARWA_PKT_GET_RX_PDMA_PTR(unit, channel);
    UI32_T                          gpd_idx = 0;

    for (gpd_idx = 0; ((gpd_idx < ptr_rx_pdma->gpd_num) && (CLX_E_OK == rc)); gpd_idx++)
    {
        rc = _hal_mt_namchabarwa_pkt_freeRxPayloadBuf(unit, channel, gpd_idx);
    }
    return (rc);
}

/* FUNCTION NAME: _hal_mt_namchabarwa_pkt_checkTxPdmaRecoverStatus
 * PURPOSE:
 *      To check status if the PDMA is ready to recover.
 * INPUT:
 *      unit            --  The unit ID
 *      channel         --  The target TX channel
 * OUTPUT:
 *      None
 * RETURN:
 *      CLX_E_OK        --  Successfully ready to recover PDMA.
 * NOTES:
 *
 */
static CLX_ERROR_NO_T
_hal_mt_namchabarwa_pkt_checkTxPdmaRecoverStatus(
    const UI32_T                    unit,
    const HAL_MT_NAMCHABARWA_PKT_TX_CHANNEL_T  channel)
{
    UI32_T data = 0;

    /* read outstd_desc reg means unfinished descripotr data req cmd cnts.
    outstd_desc should be 0 before restart/reset channel*/
    osal_mdc_readPciReg(unit,
            HAL_MT_NAMCHABARWA_PKT_GET_PDMA_WORD_TCH_REG(HAL_MT_NAMCHABARWA_PKT_PDMA_STA_PDMA_CH0_OUTSTD_DESC,channel),
            &data, sizeof(UI32_T));

    return ((data == 0) ? CLX_E_OK : CLX_E_OTHERS);
}

/* FUNCTION NAME: _hal_mt_namchabarwa_pkt_checkRxPdmaRecoverStatus
 * PURPOSE:
 *      To check status if the PDMA is ready to recover.
 * INPUT:
 *      unit            --  The unit ID
 *      channel         --  The target RX channel
 * OUTPUT:
 *      None
 * RETURN:
 *      CLX_E_OK        --  Successfully ready to recover PDMA.
 * NOTES:
 *
 */
static CLX_ERROR_NO_T
_hal_mt_namchabarwa_pkt_checkRxPdmaRecoverStatus(
    const UI32_T                    unit,
    const HAL_MT_NAMCHABARWA_PKT_RX_CHANNEL_T  channel)
{
    UI32_T data = 0;

    /* read outstd_desc reg means unfinished descripotr data req cmd cnts.
    outstd_desc should be 0 before restart/reset channel*/
    osal_mdc_readPciReg(unit,
        HAL_MT_NAMCHABARWA_PKT_GET_PDMA_WORD_RCH_REG(HAL_MT_NAMCHABARWA_PKT_PDMA_STA_PDMA_CH0_OUTSTD_DESC,channel),
        &data, sizeof(UI32_T));

    return ((data == 0) ? CLX_E_OK : CLX_E_OTHERS);
}

/* FUNCTION NAME: _hal_mt_namchabarwa_pkt_recoverTxPdma
 * PURPOSE:
 *      To recover the PDMA status to the initial state.
 * INPUT:
 *      unit            --  The unit ID
 *      channel         --  The target TX channel
 * OUTPUT:
 *      None
 * RETURN:
 *      CLX_E_OK        --  Successfully recover PDMA.
 * NOTES:
 *
 */
static CLX_ERROR_NO_T
_hal_mt_namchabarwa_pkt_recoverTxPdma(
    const UI32_T                    unit,
    const HAL_MT_NAMCHABARWA_PKT_TX_CHANNEL_T  channel)
{
    CLX_ERROR_NO_T                  rc = CLX_E_OK;
    UI32_T                          loop_cnt = 10000; // 10 *1000 * 1000 us

    _hal_mt_namchabarwa_pkt_stopTxChannelReg(unit, channel);

    _hal_mt_namchabarwa_pkt_clearPcxErrIntrReg(unit);

    _hal_mt_namchabarwa_pkt_clearTxPdmaAbnormalIntrReg(unit, channel);

    rc = _hal_mt_namchabarwa_pkt_checkTxPdmaRecoverStatus(unit, channel);
    while(rc != CLX_E_OK)
    {
        if (0 == loop_cnt)
        {
            OSAL_PRINT((OSAL_DBG_TX | OSAL_DBG_ERR),"u=%u ch=%u not ready to recover tx pdma!!!\n",unit,channel);
            return (rc);
        }
        osal_sleepThread(1000);
        rc = _hal_mt_namchabarwa_pkt_checkTxPdmaRecoverStatus(unit, channel);
        loop_cnt--;
    }

    _hal_mt_namchabarwa_pkt_restartTxChannelReg(unit, channel);
    //rc = _hal_mt_namchabarwa_pkt_initTxPdmaRing(unit, channel);
    _hal_mt_namchabarwa_pkt_startTxChannelReg(unit, channel);

    return (rc);
}

/* FUNCTION NAME: _hal_mt_namchabarwa_pkt_recoverRxPdma
 * PURPOSE:
 *      To recover the RX PDMA from the error state.
 * INPUT:
 *      unit            --  The unit ID
 *      channel         --  The target RX channel
 * OUTPUT:
 *      None
 * RETURN:
 *      CLX_E_OK        --  Successfully recovery the PDMA.
 * NOTES:
 *      None
 */
static CLX_ERROR_NO_T
_hal_mt_namchabarwa_pkt_recoverRxPdma(
    const UI32_T                    unit,
    const HAL_MT_NAMCHABARWA_PKT_RX_CHANNEL_T  channel)
{
    CLX_ERROR_NO_T                  rc = CLX_E_OK;
    UI32_T                          loop_cnt = 10000; // 10 *1000 * 1000 us
    // HAL_MT_NAMCHABARWA_PKT_RX_PDMA_T           *ptr_rx_pdma = HAL_MT_NAMCHABARWA_PKT_GET_RX_PDMA_PTR(unit, channel);

    /* Release the software GPD ring and configure it again. */
    _hal_mt_namchabarwa_pkt_stopRxChannelReg(unit, channel);
    // rc = _hal_mt_namchabarwa_pkt_deinitRxPdmaRingBuf(unit, channel);
    // if (CLX_E_OK != rc)
    // {
    //     return (rc);
    // }

    _hal_mt_namchabarwa_pkt_clearPcxErrIntrReg(unit);

    _hal_mt_namchabarwa_pkt_clearRxPdmaAbnormalIntrReg(unit, channel);

    rc = _hal_mt_namchabarwa_pkt_checkRxPdmaRecoverStatus(unit, channel);
    while(rc != CLX_E_OK)
    {
        if (0 == loop_cnt)
        {
            OSAL_PRINT((OSAL_DBG_TX | OSAL_DBG_ERR),"u=%u ch=%u not ready to recover rx pdma!!!\n",unit,channel);
            return (rc);
        }
        osal_sleepThread(1000);
        rc = _hal_mt_namchabarwa_pkt_checkRxPdmaRecoverStatus(unit, channel);
        loop_cnt--;
    }

    _hal_mt_namchabarwa_pkt_restartRxChannelReg(unit, channel);
    // rc = _hal_mt_namchabarwa_pkt_initRxPdmaRing(unit, channel);
    // if (CLX_E_OK != rc)
    // {
    //     return (rc);
    // }
    // rc = _hal_mt_namchabarwa_pkt_initRxPdmaRingBuf(unit, channel);
    // if (CLX_E_OK != rc)
    // {
    //     return (rc);
    // }
    _hal_mt_namchabarwa_pkt_startRxChannelReg(unit, channel);

    return (rc);
}

/* FUNCTION NAME: _hal_mt_namchabarwa_pkt_freeTxGpdList
 * PURPOSE:
 *      To free the TX SW GPD link list.
 * INPUT:
 *      unit            --  The unit ID
 *      ptr_sw_gpd      --  The pointer of TX SW GPD
 * OUTPUT:
 *      None
 * RETURN:
 *      CLX_E_OK        --  Successfully free the GPD list.
 * NOTES:
 *      None
 */
static void
_hal_mt_namchabarwa_pkt_freeTxGpdList(
    UI32_T                          unit,
    HAL_MT_NAMCHABARWA_PKT_TX_SW_GPD_T         *ptr_sw_gpd)
{
    HAL_MT_NAMCHABARWA_PKT_TX_SW_GPD_T         *ptr_sw_gpd_cur = NULL;

    while (NULL != ptr_sw_gpd)
    {
        ptr_sw_gpd_cur = ptr_sw_gpd;
        ptr_sw_gpd = ptr_sw_gpd->ptr_next;
        osal_free(ptr_sw_gpd_cur);
    }
}

/* FUNCTION NAME: _hal_mt_namchabarwa_pkt_freeRxGpdList
 * PURPOSE:
 *      To free the RX SW GPD link list.
 * INPUT:
 *      unit            --  The unit ID
 *      ptr_sw_gpd      --  The pointer of RX SW GPD
 *      free_payload    --  TRUE: To free the buf in SDK, FALSE: in user process.
 * OUTPUT:
 *      None
 * RETURN:
 *      CLX_E_OK        --  Successfully recovery the PDMA.
 * NOTES:
 *      None
 */
static CLX_ERROR_NO_T
_hal_mt_namchabarwa_pkt_freeRxGpdList(
    UI32_T                          unit,
    HAL_MT_NAMCHABARWA_PKT_RX_SW_GPD_T         *ptr_sw_gpd,
    BOOL_T                          free_payload)
{
    HAL_MT_NAMCHABARWA_PKT_RX_SW_GPD_T         *ptr_sw_gpd_cur = NULL;

    while (NULL != ptr_sw_gpd)
    {
        ptr_sw_gpd_cur = ptr_sw_gpd;
        ptr_sw_gpd = ptr_sw_gpd->ptr_next;
        if (TRUE == free_payload)
        {
            _hal_mt_namchabarwa_pkt_freeRxPayloadBufGpd(unit, ptr_sw_gpd_cur);
        }
        osal_free(ptr_sw_gpd_cur);
    }

    return (CLX_E_OK);
}

/* ----------------------------------------------------------------------------------- pkt_drv */
/* FUNCTION NAME: _hal_mt_namchabarwa_pkt_txEnQueueBulk
 * PURPOSE:
 *      To enqueue numbers of packet in the bulk buffer
 * INPUT:
 *      unit            --  The unit ID
 *      channel         --  The target channel
 *      number          --  The number of packet to be enqueue
 * OUTPUT:
 *      None
 * RETURN:
 *      None
 * NOTES:
 *      None
 */
static void
_hal_mt_namchabarwa_pkt_txEnQueueBulk(
    const UI32_T                    unit,
    const UI32_T                    channel,
    const UI32_T                    number)
{
    HAL_MT_NAMCHABARWA_PKT_TX_PDMA_T           *ptr_tx_pdma = HAL_MT_NAMCHABARWA_PKT_GET_TX_PDMA_PTR(unit, channel);
    HAL_MT_NAMCHABARWA_PKT_TX_SW_GPD_T         *ptr_sw_gpd = NULL;
    UI32_T                          idx;

    for (idx = 0; idx < number; idx++)
    {
        ptr_sw_gpd = ptr_tx_pdma->pptr_sw_gpd_bulk[idx];
        ptr_tx_pdma->pptr_sw_gpd_bulk[idx] = NULL;
        if (NULL != ptr_sw_gpd->callback)
        {
            ptr_sw_gpd->callback(unit, ptr_sw_gpd, ptr_sw_gpd->ptr_cookie);
        }
    }
}


/* FUNCTION NAME: _hal_mt_namchabarwa_pkt_strictTxDeQueue
 * PURPOSE:
 *      To dequeue the packets based on the strict algorithm.
 * INPUT:
 *      unit            --  The unit ID
 *      ptr_cookie      --  Pointer of the TX cookie
 * OUTPUT:
 *      None
 * RETURN:
 *      CLX_E_OK        --  Successfully dequeue the packets.
 * NOTES:
 *      None
 */
static CLX_ERROR_NO_T
_hal_mt_namchabarwa_pkt_strictTxDeQueue(
    const UI32_T                    unit,
    void                            *ptr_data)
{
    CLX_ERROR_NO_T                  rc = CLX_E_OK;
    HAL_MT_NAMCHABARWA_PKT_TX_CB_T             *ptr_tx_cb = HAL_MT_NAMCHABARWA_PKT_GET_TX_CB_PTR(unit);
    HAL_MT_NAMCHABARWA_PKT_TX_SW_GPD_T         *ptr_sw_gpd = NULL;
    CLX_ADDR_T                      sw_gpd_addr;
    UI32_T                          que_cnt = 0;
    HAL_MT_NAMCHABARWA_PKT_IOCTL_TX_COOKIE_T   *ptr_cookie = ptr_data;

    /* get queue count */
    _hal_mt_namchabarwa_pkt_getQueueCount(&ptr_tx_cb->sw_queue, &que_cnt);

    /* wait txTask event */
    if (0 == que_cnt)
    {
        osal_waitEvent(&ptr_tx_cb->sync_sema);
        if (FALSE == ptr_tx_cb->running)
        {
            return (CLX_E_OTHERS); /* deinit */
        }

        ptr_tx_cb->cnt.wait_event++;

        /* re-get queue count */
        _hal_mt_namchabarwa_pkt_getQueueCount(&ptr_tx_cb->sw_queue, &que_cnt);
    }

    /* deque */
    if (que_cnt > 0)
    {
        rc = _hal_mt_namchabarwa_pkt_deQueue(&ptr_tx_cb->sw_queue, (void **)&ptr_sw_gpd);
        if (CLX_E_OK == rc)
        {
            ptr_tx_cb->cnt.deque_ok++;

            sw_gpd_addr = (CLX_ADDR_T)ptr_sw_gpd->ptr_cookie;

            /* Give the address of pre-saved SW GPD back to userspace */
            osal_io_copyToUser(&ptr_cookie->done_sw_gpd_addr,
                               &sw_gpd_addr,
                               sizeof(CLX_ADDR_T));

            /* free kernel sw_gpd */
            _hal_mt_namchabarwa_pkt_freeTxGpdList(unit, ptr_sw_gpd);
        }
        else
        {
            ptr_tx_cb->cnt.deque_fail++;
        }
    }
    else
    {
        /* It may happen at last gpd, return error and do not invoke callback. */
        rc = CLX_E_OTHERS;
    }

    return (rc);
}

/* FUNCTION NAME: _hal_mt_namchabarwa_pkt_rxCheckReason
 * PURPOSE:
 *      To check the packets to linux kernel/user.
 * INPUT:
 *      ptr_rx_gpd      -- Pointer of the RX GPD
 *      ptr_hit_prof    -- Pointer of the hit flag
 * OUTPUT:
 *      None
 * RETURN:
 *      CLX_E_OK        -- Successfully dispatch the packets.
 * NOTES:
 *      Reference to pkt_srv.
 */
static void
_hal_mt_namchabarwa_pkt_rxCheckReason(
    HAL_MT_NAMCHABARWA_PKT_RX_SW_GPD_T *ptr_sw_gpd,
    HAL_MT_NAMCHABARWA_PKT_NETIF_PROFILE_T     *ptr_profile,
    BOOL_T                          *ptr_hit_prof)
{
    HAL_PKT_RX_REASON_BITMAP_T      *ptr_reason_bitmap = &ptr_profile->reason_bitmap;
    UI32_T                          bitval = 0;
    UI32_T                          bitmap = 0x0;


    if (0 == (ptr_profile->flags & HAL_MT_NAMCHABARWA_PKT_NETIF_PROFILE_FLAGS_REASON))
    {
        /* It means that reason doesn't metters */
        *ptr_hit_prof = TRUE;
        return;
    }

#define HAL_MT_NAMCHABARWA_PKT_DI_CPU_MIN       (32)
#define HAL_MT_NAMCHABARWA_PKT_DI_CPU_MAX       (35)

    switch (ptr_sw_gpd->ptr_pph_l2->fwd_op)
    {
        case HAL_MT_NAMCHABARWA_PKT_PPH_TYPE_L2:
        case HAL_MT_NAMCHABARWA_PKT_PPH_TYPE_L3UC:
        case HAL_MT_NAMCHABARWA_PKT_PPH_TYPE_L25:
            bitval = ptr_sw_gpd->ptr_pph_l2->cpu_reason;
            bitmap = 1UL << (bitval % 32);
            if (0 != (ptr_reason_bitmap->pp_bitmap[bitval / 32] & bitmap))
            {
                *ptr_hit_prof = TRUE;
            }
            break;
        default:
            *ptr_hit_prof = FALSE;
            break;
    }
}

static BOOL_T
_hal_mt_namchabarwa_pkt_comparePatternWithPayload(
    volatile HAL_MT_NAMCHABARWA_PKT_RX_GPD_T   *ptr_rx_gpd,
    const UI8_T                     *ptr_pattern,
    const UI8_T                     *ptr_mask,
    const UI32_T                    offset)
{
    CLX_ADDR_T                      phy_addr = 0;
    UI8_T                           *ptr_virt_addr = NULL;
    UI32_T                          idx;

    /* Get the packet payload */
    phy_addr = CLX_ADDR_32_TO_64(ptr_rx_gpd->d_addr_hi, ptr_rx_gpd->d_addr_lo);
    ptr_virt_addr = (C8_T *) osal_dma_convertPhyToVirt(phy_addr);

    /* skip private header */
    ptr_virt_addr += HAL_MT_NAMCHABARWA_PKT_EMAC_SZ + HAL_MT_NAMCHABARWA_PKT_PPH_HDR_SZ;

    for (idx=0; idx<CLX_NETIF_PROFILE_PATTERN_LEN; idx++)
    {
        /* per-byte comparison  */
        if ((ptr_virt_addr[offset+idx] & ptr_mask[idx]) != (ptr_pattern[idx] & ptr_mask[idx]))
        {
            OSAL_PRINT(OSAL_DBG_PROFILE,
                            "prof match failed, byte idx=%d, pattern=0x%02X != 0x%02X, mask=0x%02X\n",
                            offset+idx, ptr_pattern[idx], ptr_virt_addr[offset+idx], ptr_mask[idx]);
            return (FALSE);
        }
    }

    return (TRUE);
}


static void
_hal_mt_namchabarwa_pkt_rxCheckPattern(
    volatile HAL_MT_NAMCHABARWA_PKT_RX_GPD_T   *ptr_rx_gpd,
    HAL_MT_NAMCHABARWA_PKT_NETIF_PROFILE_T     *ptr_profile,
    BOOL_T                          *ptr_hit_prof)
{
    UI32_T      idx;
    BOOL_T      match;

    /* Check if need to compare pattern */
    if ((ptr_profile->flags & (HAL_MT_NAMCHABARWA_PKT_NETIF_PROFILE_FLAGS_PATTERN_0 |
                               HAL_MT_NAMCHABARWA_PKT_NETIF_PROFILE_FLAGS_PATTERN_1 |
                               HAL_MT_NAMCHABARWA_PKT_NETIF_PROFILE_FLAGS_PATTERN_2 |
                               HAL_MT_NAMCHABARWA_PKT_NETIF_PROFILE_FLAGS_PATTERN_3)) != 0)
    {
        /* Need to compare the payload with at least one of the four patterns */
        /* Pre-assume that the result is positive */
        *ptr_hit_prof = TRUE;

        /* If any of the following comparison fails, the result will be changed to negtive */
    }
    else
    {
        return;
    }

    for (idx=0; idx<CLX_NETIF_PROFILE_PATTERN_NUM; idx++)
    {
        OSAL_PRINT(OSAL_DBG_PROFILE,
                        "compare pattern id=%d\n", idx);
        if (0 != (ptr_profile->flags & (HAL_MT_NAMCHABARWA_PKT_NETIF_PROFILE_FLAGS_PATTERN_0 << idx)))
        {
            match = _hal_mt_namchabarwa_pkt_comparePatternWithPayload(ptr_rx_gpd,
                                                           ptr_profile->pattern[idx],
                                                           ptr_profile->mask[idx],
                                                           ptr_profile->offset[idx]);
            if (TRUE == match)
            {
                /* Do nothing */
            }
            else
            {
                /* Change the result to negtive */
                *ptr_hit_prof = FALSE;
                break;
            }
        }
    }
}

static void
_hal_mt_namchabarwa_pkt_matchUserProfile(
    HAL_MT_NAMCHABARWA_PKT_RX_SW_GPD_T *ptr_sw_gpd,
    HAL_MT_NAMCHABARWA_PKT_PROFILE_NODE_T      *ptr_profile_list,
    HAL_MT_NAMCHABARWA_PKT_NETIF_PROFILE_T     **pptr_profile_hit)
{
    HAL_MT_NAMCHABARWA_PKT_PROFILE_NODE_T      *ptr_curr_node = ptr_profile_list;
    BOOL_T                          hit;

    *pptr_profile_hit = NULL;

    while (NULL != ptr_curr_node)
    {
        /* 1st match reason */
        _hal_mt_namchabarwa_pkt_rxCheckReason(ptr_sw_gpd, ptr_curr_node->ptr_profile, &hit);
        if (TRUE == hit)
        {
            OSAL_PRINT(OSAL_DBG_PROFILE,
                            "rx prof matched by reason\n");

            /* Then, check pattern */
            _hal_mt_namchabarwa_pkt_rxCheckPattern(&ptr_sw_gpd->rx_gpd, ptr_curr_node->ptr_profile, &hit);
            if (TRUE == hit)
            {
                OSAL_PRINT(OSAL_DBG_PROFILE,
                                "rx prof matched by pattern\n");

                *pptr_profile_hit = ptr_curr_node->ptr_profile;
                break;
            }
        }

        /* Seach the next profile (priority lower) */
        ptr_curr_node = ptr_curr_node->ptr_next_node;
    }
}

static void
_hal_mt_namchabarwa_pkt_getPacketDest(
    HAL_MT_NAMCHABARWA_PKT_RX_SW_GPD_T *ptr_sw_gpd,
    HAL_MT_NAMCHABARWA_PKT_DEST_T              *ptr_dest,
    void                            **pptr_cookie)
{
    UI32_T                          port;
    HAL_MT_NAMCHABARWA_PKT_PROFILE_NODE_T      *ptr_profile_list;
    HAL_MT_NAMCHABARWA_PKT_NETIF_PROFILE_T     *ptr_profile_hit;
    HAL_MT_NAMCHABARWA_PKT_RX_GPD_T   *ptr_rx_gpd = NULL;

    ptr_rx_gpd = &ptr_sw_gpd->rx_gpd;
    port = ptr_sw_gpd->ptr_pph_l2->src_idx;
    ptr_profile_list = HAL_MT_NAMCHABARWA_PKT_GET_PORT_PROFILE_LIST(port);

    _hal_mt_namchabarwa_pkt_matchUserProfile(ptr_sw_gpd,
                                  ptr_profile_list,
                                  &ptr_profile_hit);
    if (NULL != ptr_profile_hit)
    {
        if (HAL_MT_NAMCHABARWA_PKT_NETIF_RX_DST_NETLINK == ptr_profile_hit->dst_type)
        {
            *ptr_dest = HAL_MT_NAMCHABARWA_PKT_DEST_NETLINK;
            *pptr_cookie = (void *)&ptr_profile_hit->netlink;
        }
        else
        {
            *ptr_dest = HAL_MT_NAMCHABARWA_PKT_DEST_SDK;
        }
    }
    else
    {
        *ptr_dest = HAL_MT_NAMCHABARWA_PKT_DEST_NETDEV;
    }
}

/* FUNCTION NAME: _hal_mt_namchabarwa_pkt_print_payload
 * PURPOSE:
 *      To dump the values of fields for the specified RX GPD.
 * INPUT:
 *      ptr_virt_addr  --  Pointer for the RX PKT buf address
 *      buf_len        --  pkt buf_len
 * OUTPUT:
 *      None
 * RETURN:
 *      None
 * NOTES:
 *      None
 */
static void
_hal_mt_namchabarwa_pkt_print_payload(UI8_T * ptr_virt_addr, UI32_T buf_len)
{
    UI32_T i = 0;

    osal_printf("==========================  PKT BUF %p %d ====================== \n",ptr_virt_addr,buf_len);
    while(i < buf_len)
    {
        osal_printf("%02x ",*((UI8_T *)ptr_virt_addr + i));
        i++;
        if(i % 8 == 0)
            osal_printf("\n");
    }

}

/* FUNCTION NAME: hal_mt_namchabarwa_pkt_showRxPdmaGpd
 * PURPOSE:
 *      To dump the values of fields for the specified RX GPD.
 * INPUT:
 *      unit        --  The unit ID
 *      ptr_rx_gpd  --  Pointer for the RX GPD
 * OUTPUT:
 *      None
 * RETURN:
 *      CLX_E_OK    --  Successfully show the RX GPD content.
 * NOTES:
 *      None
 */
static CLX_ERROR_NO_T
hal_mt_namchabarwa_pkt_showPdmaGpd(
    const UI32_T                        unit,
    const volatile HAL_MT_NAMCHABARWA_PKT_GPD_T *ptr_gpd)
{
    UI8_T                          *ptr_virt_addr = NULL;
    CLX_ADDR_T                      phy_addr = 0x0;
    UI32_T                          buf_len = 0;

    osal_printf("d_addr_hi          : 0x%08x\n",  ptr_gpd->d_addr_hi);
    osal_printf("d_addr_lo          : 0x%08x\n",  ptr_gpd->d_addr_lo);
    osal_printf("s_addr_hi          : 0x%08x\n",  ptr_gpd->s_addr_hi);
    osal_printf("s_addr_lo          : 0x%08x\n",  ptr_gpd->s_addr_lo);
    osal_printf("size               : 0x%-10x\n", ptr_gpd->size);
    osal_printf("interrupt          : 0x%-10x\n", ptr_gpd->interrupt);
    osal_printf("err                : 0x%-10x\n", ptr_gpd->err);
    osal_printf("sop                : 0x%-10x\n", ptr_gpd->sop);
    osal_printf("eop                : 0x%-10x\n", ptr_gpd->eop);
    osal_printf("dinc               : 0x%-10x\n", ptr_gpd->dinc);
    osal_printf("sinc               : 0x%-10x\n", ptr_gpd->sinc);
    osal_printf("limit_xfer_en      : 0x%-10x\n", ptr_gpd->limit_xfer_en);
    osal_printf("xfer_size          : 0x%-10x\n", ptr_gpd->xfer_size);
    

    /* cpu header */
    // _hal_mt_namchabarwa_pkt_showPdmaPph(&ptr_gpd->pph_l2);

    phy_addr = CLX_ADDR_32_TO_64(ptr_gpd->d_addr_hi, ptr_gpd->d_addr_lo);
    ptr_virt_addr = (UI8_T *) osal_dma_convertPhyToVirt(phy_addr);
        buf_len = ptr_gpd->size;
    if (0 != buf_len)
    {
        osal_printf("d_addr_hi:0x%x,d_addr_lo:0x%x, phy_addr:0x%llx, len=%d\n", ptr_gpd->d_addr_hi, ptr_gpd->d_addr_lo, phy_addr, buf_len);
        _hal_mt_namchabarwa_pkt_print_payload((UI8_T *)ptr_virt_addr, buf_len);
    }
    return (CLX_E_OK);
}

static void _hal_mt_namchabarwa_pkt_print_pph(HAL_MT_NAMCHABARWA_PKT_PPH_L2_T *ptr_pph_l2)
{
    HAL_MT_NAMCHABARWA_PKT_PPH_L3UC_T *ptr_pph_l3 = NULL;
    HAL_MT_NAMCHABARWA_PKT_PPH_L25_T *ptr_pph_l25 = NULL;

    /*print common field*/
    osal_printf("\n ==========================  PPH %u====================== \n",ptr_pph_l2->fwd_op);
    osal_printf("fwd_op                      :%u\n", ptr_pph_l2->fwd_op);
    osal_printf("tc                          :%u\n", ptr_pph_l2->tc);
    osal_printf("color                       :%u\n", ptr_pph_l2->color);
    osal_printf("hash_val                    :0x%x\n", ptr_pph_l2->hash_val);
    osal_printf("dst_idx                     :0x%x\n", HAL_MT_NAMCHABARWA_PKT_PPH_GET_DST_IDX(ptr_pph_l2));
    osal_printf("src_idx                     :0x%x\n", ptr_pph_l2->src_idx);
    osal_printf("skip_epp                    :%u\n", ptr_pph_l2->skip_epp);
    osal_printf("igr_acl_label               :0x%x\n", HAL_MT_NAMCHABARWA_PKT_PPH_GET_IGR_ACL_LABLE(ptr_pph_l2));
    osal_printf("qos_dnt_modify              :%u\n", ptr_pph_l2->qos_dnt_modify);
    osal_printf("qos_tnl_uniform             :%u\n", ptr_pph_l2->qos_tnl_uniform);
    osal_printf("qos_pcp_dei_val             :%u\n", ptr_pph_l2->qos_pcp_dei_val);
    osal_printf("pkt_journal                 :%u\n", ptr_pph_l2->pkt_journal);
    osal_printf("port_num                    :0x%x\n", ptr_pph_l2->port_num);
    osal_printf("die_id                      :%u\n", ptr_pph_l2->die_id);
    osal_printf("slice_id                    :%u\n", ptr_pph_l2->slice_id);
    osal_printf("skip_ipp                    :%u\n", ptr_pph_l2->skip_ipp);
    osal_printf("mirror_bmap                 :%u\n", ptr_pph_l2->mirror_bmap);
    osal_printf("cpu_reason                  :%u\n", ptr_pph_l2->cpu_reason);
    osal_printf("src_bdi                     :0x%x\n", ptr_pph_l2->src_bdi);
    osal_printf("decap_act                   :%u\n", ptr_pph_l2->decap_act);
    osal_printf("igr_is_fab                  :%u\n", ptr_pph_l2->igr_is_fab);

    switch(ptr_pph_l2->fwd_op)
    {
        case HAL_MT_NAMCHABARWA_PKT_PPH_TYPE_L2:
            osal_printf("evpn_esi                    :0x%x\n", HAL_MT_NAMCHABARWA_PKT_PPH_GET_EVPN_ESI(ptr_pph_l2));
            osal_printf("mpls_pwcw_vld               :%u\n", ptr_pph_l2->mpls_pwcw_vld);
            osal_printf("tnl_idx                     :0x%x\n", ptr_pph_l2->tnl_idx);
            osal_printf("tnl_bd                      :0x%x\n", HAL_MT_NAMCHABARWA_PKT_PPH_GET_TNL_BD(ptr_pph_l2));
            osal_printf("mpls_ctl                    :%u\n", ptr_pph_l2->mpls_ctl);
            osal_printf("ecn_enable                  :%u\n", ptr_pph_l2->ecn_enable);
            osal_printf("ecn                         :%u\n", ptr_pph_l2->ecn);
            osal_printf("igr_vid_pop_num             :%u\n", ptr_pph_l2->igr_vid_pop_num);
            osal_printf("pvlan_port_type             :%u\n", ptr_pph_l2->pvlan_port_type);
            osal_printf("src_vlan                    :0x%x\n", ptr_pph_l2->src_vlan);
            osal_printf("tapping_push_o              :%u\n", ptr_pph_l2->tapping_push_o);
            osal_printf("tapping_push_t              :%u\n", ptr_pph_l2->tapping_push_t);
            osal_printf("mac_learn_en                :%u\n", ptr_pph_l2->mac_learn_en);
            osal_printf("ptp_info                    :%u\n", HAL_MT_NAMCHABARWA_PKT_PPH_GET_PTP_INFO(ptr_pph_l2));
            osal_printf("int_mm_mode                 :%u\n", ptr_pph_l2->int_mm_mode);
            osal_printf("int_profile                 :%u\n", ptr_pph_l2->int_profile);
            osal_printf("int_role                    :%u\n", ptr_pph_l2->int_role);
            osal_printf("timestamp                   :0x%x\n", ptr_pph_l2->timestamp);
            break;
        case HAL_MT_NAMCHABARWA_PKT_PPH_TYPE_L3UC:
            ptr_pph_l3 = (HAL_MT_NAMCHABARWA_PKT_PPH_L3UC_T *)ptr_pph_l2;
            osal_printf("evpn_esi                    :%u\n", HAL_MT_NAMCHABARWA_PKT_PPH_GET_EVPN_ESI(ptr_pph_l3));
            osal_printf("mpls_pwcw_vld               :%u\n", ptr_pph_l3->mpls_pwcw_vld);
            osal_printf("tnl_idx                     :%u\n", ptr_pph_l3->tnl_idx);
            osal_printf("tnl_bd                      :%u\n", HAL_MT_NAMCHABARWA_PKT_PPH_GET_TNL_BD(ptr_pph_l3));
            osal_printf("mpls_ctl                    :%u\n", ptr_pph_l3->mpls_ctl);
            osal_printf("ecn_enable                  :%u\n", ptr_pph_l3->ecn_enable);
            osal_printf("ecn                         :%u\n", ptr_pph_l3->ecn);
            osal_printf("decr_ttl                    :%u\n", ptr_pph_l3->decr_ttl);
            osal_printf("decap_prop_ttl              :%u\n", ptr_pph_l3->decap_prop_ttl);
            osal_printf("mpls_inner_l2               :%u\n", ptr_pph_l3->mpls_inner_l2);
            osal_printf("dst_bdi                     :%u\n", ptr_pph_l3->dst_bdi);
            osal_printf("mac_da                      :%llu", HAL_MT_NAMCHABARWA_PKT_PPH_GET_MAC_DA(ptr_pph_l3));
            osal_printf("nxt_sid_opcode              :%u\n", ptr_pph_l3->nxt_sid_opcode);
            osal_printf("decr_sl                     :%u\n", ptr_pph_l3->decr_sl);
            osal_printf("usid_func_en                :%u\n", ptr_pph_l3->usid_func_en);
            osal_printf("usid_arg_en                 :%u\n", ptr_pph_l3->usid_arg_en);
            osal_printf("srv6_encaps_red             :%u\n", ptr_pph_l3->srv6_encaps_red);
            osal_printf("srv6_insert_red             :%u\n", ptr_pph_l3->srv6_insert_red);
            osal_printf("srv6_func_hit               :%u\n", ptr_pph_l3->srv6_func_hit);
            osal_printf("mac_learn_en                :%u\n", ptr_pph_l3->mac_learn_en);
            osal_printf("srv6_encap_end              :%u\n", ptr_pph_l3->srv6_encap_end);
            osal_printf("ptp_info                    :%u\n", HAL_MT_NAMCHABARWA_PKT_PPH_GET_PTP_INFO(ptr_pph_l3));
            osal_printf("int_mm_mode                 :%u\n", ptr_pph_l3->int_mm_mode);
            osal_printf("int_profile                 :%u\n", ptr_pph_l3->int_profile);
            osal_printf("int_role                    :%u\n", ptr_pph_l3->int_role);
            osal_printf("timestamp                   :0x%x\n", ptr_pph_l3->timestamp);
            break;
        case HAL_MT_NAMCHABARWA_PKT_PPH_TYPE_L25:
            ptr_pph_l25 = (HAL_MT_NAMCHABARWA_PKT_PPH_L25_T *)ptr_pph_l2;
            osal_printf("mpls_lbl                    :%u\n", HAL_MT_NAMCHABARWA_PKT_PPH_GET_MPLS_LBL(ptr_pph_l25));
            osal_printf("is_swap                     :%u\n", ptr_pph_l25->is_swap);
            osal_printf("tnl_idx                     :%u\n", ptr_pph_l25->tnl_idx);
            osal_printf("tnl_bd                      :%u\n", HAL_MT_NAMCHABARWA_PKT_PPH_GET_TNL_BD(ptr_pph_l25));
            osal_printf("decr_ttl                    :%u\n", ptr_pph_l25->decr_ttl);
            osal_printf("decap_prop_ttl              :%u\n", ptr_pph_l25->decap_prop_ttl);
            osal_printf("mac_learn_en                :%u\n", ptr_pph_l25->mac_learn_en);
            osal_printf("timestamp                   :0x%x\n", ptr_pph_l25->timestamp);
            break;
        default:
            osal_printf("pph type is not valid!!!");
    }
}

/* FUNCTION NAME: _hal_mt_namchabarwa_pkt_rxEnQueue
 * PURPOSE:
 *      To enqueue the packets to multiple queues.
 * INPUT:
 *      unit            -- The unit ID
 *      channel         -- The target channel
 *      ptr_sw_gpd      -- Pointer for the SW Rx GPD link list
 * OUTPUT:
 *      None
 * RETURN:
 *      CLX_E_OK        -- Successfully enqueue the packets.
 * NOTES:
 *      None
 */
static void
_hal_mt_namchabarwa_pkt_rxEnQueue(
    const UI32_T                    unit,
    const UI32_T                    channel,
    HAL_MT_NAMCHABARWA_PKT_RX_SW_GPD_T         *ptr_sw_gpd)
{
    HAL_MT_NAMCHABARWA_PKT_RX_CB_T             *ptr_rx_cb = HAL_MT_NAMCHABARWA_PKT_GET_RX_CB_PTR(unit);
    HAL_MT_NAMCHABARWA_PKT_RX_SW_GPD_T         *ptr_sw_first_gpd = ptr_sw_gpd;
    void                            *ptr_virt_addr = NULL;
    CLX_ADDR_T                      phy_addr = 0;
    HAL_MT_NAMCHABARWA_PKT_DEST_T              dest_type;

    /* skb meta */
    UI32_T                          port = 0, len = 0, total_len = 0;
    struct net_device               *ptr_net_dev = NULL;
    struct net_device_priv          *ptr_priv = NULL;
    struct sk_buff                  *ptr_skb = NULL, *ptr_merge_skb = NULL;
    UI32_T                          copy_offset;
    void                            *ptr_dest;
    UI32_T                              i = 0;

    /* To verify kernel Rx performance */
    if (CLX_E_OK == perf_rxTest())
    {
        while (NULL != ptr_sw_gpd)
        {
            len = ptr_sw_gpd->rx_gpd.size;

            total_len += len;

            /* unmap dma */
            phy_addr = CLX_ADDR_32_TO_64(ptr_sw_gpd->rx_gpd.d_addr_hi, ptr_sw_gpd->rx_gpd.d_addr_lo);
            osal_skb_unmapDma(phy_addr, len, DMA_FROM_DEVICE);
            /* next */
            ptr_sw_gpd = ptr_sw_gpd->ptr_next;
        }
        perf_rxCallback(total_len);
        _hal_mt_namchabarwa_pkt_freeRxGpdList(unit, ptr_sw_first_gpd, TRUE);
        return ;
    }

    /* unmap dma to make sure data transfer completely */
    phy_addr = CLX_ADDR_32_TO_64(ptr_sw_gpd->rx_gpd.d_addr_hi, ptr_sw_gpd->rx_gpd.d_addr_lo);
    ptr_virt_addr = ptr_sw_gpd->ptr_cookie;
    ptr_skb = (struct sk_buff *)ptr_virt_addr;
    //osal_skb_unmapDma(phy_addr, ptr_skb->len, DMA_FROM_DEVICE);

    /* get pph in skb*/
    ptr_sw_gpd->ptr_pph_l2 = (HAL_MT_NAMCHABARWA_PKT_PPH_L2_T *)((UI8_T *)ptr_skb->data + HAL_MT_NAMCHABARWA_PKT_EMAC_SZ);

    if (0 != (((OSAL_DBG_RX | OSAL_DBG_INFO)) & (verbosity)))
    {
        _hal_mt_namchabarwa_pkt_print_payload((UI8_T *)ptr_sw_gpd->ptr_pph_l2, HAL_MT_NAMCHABARWA_PKT_PPH_HDR_SZ);
    }

    i = 0;
    while(i < HAL_MT_NAMCHABARWA_PKT_PPH_HDR_SZ/4)
    {
        *((UI32_T *)ptr_sw_gpd->ptr_pph_l2+i) = HAL_MT_NAMCHABARWA_PKT_BE_TO_HOST32(*((UI32_T *)ptr_sw_gpd->ptr_pph_l2+i));
        i++;
    }

    if (0 != (((OSAL_DBG_RX | OSAL_DBG_INFO)) & (verbosity)))
    {
        osal_printf("after HAL_MT_NAMCHABARWA_PKT_BE_TO_HOST32 pph:\n");
        _hal_mt_namchabarwa_pkt_print_payload((UI8_T *)ptr_sw_gpd->ptr_pph_l2, HAL_MT_NAMCHABARWA_PKT_PPH_HDR_SZ);
        _hal_mt_namchabarwa_pkt_print_pph(ptr_sw_gpd->ptr_pph_l2);
    }

    _hal_mt_namchabarwa_pkt_getPacketDest(ptr_sw_gpd, &dest_type, &ptr_dest);

    if ((HAL_MT_NAMCHABARWA_PKT_DEST_NETDEV  == dest_type) ||
        (HAL_MT_NAMCHABARWA_PKT_DEST_NETLINK == dest_type))
    {
        /* need to encap the packet as skb */
        ptr_sw_gpd = ptr_sw_first_gpd;
        while (NULL != ptr_sw_gpd)
        {
            len = ptr_sw_gpd->rx_gpd.size;

            total_len += len;

            /* unmap dma */
            phy_addr = CLX_ADDR_32_TO_64(ptr_sw_gpd->rx_gpd.d_addr_hi, ptr_sw_gpd->rx_gpd.d_addr_lo);
            ptr_virt_addr = ptr_sw_gpd->ptr_cookie;

            ptr_skb = (struct sk_buff *)ptr_virt_addr;

            /* note here ptr_skb->len is the total buffer size not means the actual Rx packet len
             * it should be updated later
             */
            //osal_skb_unmapDma(phy_addr, ptr_skb->len, DMA_FROM_DEVICE);

            /* reset ptr_skb->len with real packet len instead of total buffer size */
            if (NULL == ptr_sw_gpd->ptr_next)
            {
                /* strip CRC padded by asic for the last gpd segment */
                ptr_skb->len = len - ETH_FCS_LEN;
            }
            else
            {
                ptr_skb->len = len;
            }

            skb_set_tail_pointer(ptr_skb, ptr_skb->len);

            /* next */
            ptr_sw_gpd = ptr_sw_gpd->ptr_next;
        }

        port = ptr_sw_first_gpd->ptr_pph_l2->src_idx;
        ptr_net_dev = HAL_MT_NAMCHABARWA_PKT_GET_PORT_NETDEV(port);

        /* if the packet is composed of multiple gpd (skb), need to merge it into a single skb */
        if (NULL != ptr_sw_first_gpd->ptr_next)
        {
            OSAL_PRINT(OSAL_DBG_RX,
                            "u=%u, rxch=%u, rcv pkt size=%u > gpd buf size=%u\n",
                            unit, channel, total_len, ptr_rx_cb->buf_len);
            ptr_merge_skb = osal_skb_alloc(total_len - ETH_FCS_LEN);
            if (NULL != ptr_merge_skb)
            {
                copy_offset = 0;
                ptr_sw_gpd = ptr_sw_first_gpd;
                while (NULL != ptr_sw_gpd)
                {
                    ptr_skb = (struct sk_buff *)ptr_sw_gpd->ptr_cookie;
                    OSAL_PRINT(OSAL_DBG_RX,
                                    "u=%u, rxch=%u, copy size=%u to buf offset=%u\n",
                                    unit, channel, ptr_skb->len, copy_offset);

                    memcpy(&(((UI8_T *)ptr_merge_skb->data)[copy_offset]),
                           ptr_skb->data, ptr_skb->len);
                    copy_offset += ptr_skb->len;
                    ptr_sw_gpd = ptr_sw_gpd->ptr_next;
                }
                /* put the merged skb to ptr_skb for the following process */
                ptr_skb = ptr_merge_skb;
            }
            else
            {
                OSAL_PRINT((OSAL_DBG_ERR | OSAL_DBG_RX),
                                "u=%u, rxch=%u, alloc skb failed, size=%u\n",
                                unit, channel, (total_len - ETH_FCS_LEN));
            }

            /* free both sw_gpd and the skb attached on it */
            _hal_mt_namchabarwa_pkt_freeRxGpdList(unit, ptr_sw_first_gpd, TRUE);
        }
        else
        {
            /* free only sw_gpd */
            _hal_mt_namchabarwa_pkt_freeRxGpdList(unit, ptr_sw_first_gpd, FALSE);
        }

        /* if NULL netdev, drop the skb */
        if (NULL == ptr_net_dev)
        {
            ptr_rx_cb->cnt.channel[channel].netdev_miss++;
            osal_skb_free(ptr_skb);
            OSAL_PRINT((OSAL_DBG_ERR | OSAL_DBG_RX),
                            "u=%u, rxch=%u, find netdev failed\n",
                            unit, channel);
            return;
        }

        /* skb handling */
        ptr_skb->dev = ptr_net_dev;
        ptr_skb->pkt_type = PACKET_HOST; /* this packet is for me */
        ptr_skb->ip_summed = CHECKSUM_UNNECESSARY; /* skip checksum */

        /* send to linux */
        if (dest_type == HAL_MT_NAMCHABARWA_PKT_DEST_NETDEV)
        {
            /* skip ethernet header only for Linux net interface*/
            ptr_skb->protocol = eth_type_trans(ptr_skb, ptr_net_dev);

            //
            skb_pull(ptr_skb, HAL_MT_NAMCHABARWA_PKT_EMAC_SZ + HAL_MT_NAMCHABARWA_PKT_PPH_HDR_SZ);
            osal_skb_recv(ptr_skb);
#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 11, 0)
            ptr_net_dev->last_rx = jiffies;
#endif
            ptr_priv = netdev_priv(ptr_net_dev);
            ptr_priv->stats.rx_packets++;
            ptr_priv->stats.rx_bytes += total_len;
        }
        else
        {
            OSAL_PRINT(OSAL_DBG_PROFILE,
                            "hit profile dest=netlink, name=%s, mcgrp=%s\n",
                            ((NETIF_NL_RX_DST_NETLINK_T *)ptr_dest)->name,
                            ((NETIF_NL_RX_DST_NETLINK_T *)ptr_dest)->mc_group_name);
            netif_nl_rxSkb(unit, ptr_skb, ptr_dest);
        }
    }
    else if (HAL_MT_NAMCHABARWA_PKT_DEST_SDK == dest_type)
    {
        while (0 != _hal_mt_namchabarwa_pkt_enQueue(&ptr_rx_cb->sw_queue[channel], ptr_sw_gpd))
        {
            ptr_rx_cb->cnt.channel[channel].enque_retry++;
            HAL_MT_NAMCHABARWA_PKT_RX_ENQUE_RETRY_SLEEP();
        }
        ptr_rx_cb->cnt.channel[channel].enque_ok++;

        osal_triggerEvent(&ptr_rx_cb->sync_sema);
        ptr_rx_cb->cnt.channel[channel].trig_event++;
    }
    else if (HAL_MT_NAMCHABARWA_PKT_DEST_DROP == dest_type)
    {
        _hal_mt_namchabarwa_pkt_freeRxGpdList(unit, ptr_sw_first_gpd, TRUE);
    }
    else
    {
        OSAL_PRINT((OSAL_DBG_ERR | OSAL_DBG_RX),
                        "u=%u, rxch=%u, invalid pkt dest=%d\n",
                        unit, channel, dest_type);
    }
}

static CLX_ERROR_NO_T
_hal_mt_namchabarwa_pkt_flushRxQueue(
    const UI32_T                unit,
    HAL_MT_NAMCHABARWA_PKT_SW_QUEUE_T      *ptr_que)
{
    HAL_MT_NAMCHABARWA_PKT_RX_SW_GPD_T     *ptr_sw_gpd_knl = NULL;
    CLX_ERROR_NO_T              rc;

    while (1)
    {
        rc = _hal_mt_namchabarwa_pkt_deQueue(ptr_que, (void **)&ptr_sw_gpd_knl);
        if (CLX_E_OK == rc)
        {
            _hal_mt_namchabarwa_pkt_freeRxGpdList(unit, ptr_sw_gpd_knl, TRUE);
        }
        else
        {
            break;
        }
    }

    return (CLX_E_OK);
}

/* FUNCTION NAME: _hal_mt_namchabarwa_pkt_schedRxDeQueue
 * PURPOSE:
 *      To dequeue the packets based on the configured algorithm.
 * INPUT:
 *      unit            -- The unit ID
 *      ptr_cookie      -- Pointer of the RX cookie
 * OUTPUT:
 *      None
 * RETURN:
 *      CLX_E_OK        -- Successfully dequeue the packets.
 * NOTES:
 *      None
 */
static CLX_ERROR_NO_T
_hal_mt_namchabarwa_pkt_schedRxDeQueue(
    const UI32_T                    unit,
    void                            *ptr_data)
{
    HAL_MT_NAMCHABARWA_PKT_IOCTL_RX_COOKIE_T   ioctl_data;
    HAL_MT_NAMCHABARWA_PKT_IOCTL_RX_GPD_T      ioctl_gpd;
    HAL_MT_NAMCHABARWA_PKT_RX_CB_T             *ptr_rx_cb = HAL_MT_NAMCHABARWA_PKT_GET_RX_CB_PTR(unit);
    HAL_MT_NAMCHABARWA_PKT_RX_SW_GPD_T         *ptr_sw_gpd_knl = NULL;
    HAL_MT_NAMCHABARWA_PKT_RX_SW_GPD_T         *ptr_sw_first_gpd_knl = NULL;
    UI32_T                          que_cnt = 0;
    UI32_T                          queue   = 0;
    UI32_T                          idx     = 0;
    UI32_T                          gpd_idx = 0;
    /* copy Rx sw_gpd */
    volatile HAL_MT_NAMCHABARWA_PKT_RX_GPD_T   *ptr_rx_gpd = NULL;
    void                            *ptr_virt_addr = NULL;
    CLX_ADDR_T                      phy_addr = 0;
    UI32_T                          buf_len = 0;
    CLX_ERROR_NO_T                  rc = CLX_E_OK;
    HAL_MT_NAMCHABARWA_PKT_IOCTL_RX_COOKIE_T   *ptr_cookie = ptr_data;

    if (FALSE == ptr_rx_cb->running)
    {
        /* not running, return no pkts */
        rc = CLX_E_OTHERS;
        osal_io_copyToUser(&ptr_cookie->rc, &rc, sizeof(CLX_ERROR_NO_T));
        return (CLX_E_OK);
    }

    /* get queue and count */
    for (idx = 0; idx < HAL_MT_NAMCHABARWA_PKT_RX_QUEUE_NUM; idx++)
    {
        /* to gurantee the opportunity where each queue can be handler */
        queue = ((ptr_rx_cb->deque_idx + idx) % HAL_MT_NAMCHABARWA_PKT_RX_QUEUE_NUM);
        _hal_mt_namchabarwa_pkt_getQueueCount(&ptr_rx_cb->sw_queue[queue], &que_cnt);
        if (que_cnt > 0)
        {
            ptr_rx_cb->deque_idx = ((queue + 1) % HAL_MT_NAMCHABARWA_PKT_RX_QUEUE_NUM);
            break;
        }
    }

    /* If all of the queues are empty, wait rxTask event */
    if (0 == que_cnt)
    {
        osal_waitEvent(&ptr_rx_cb->sync_sema);

        ptr_rx_cb->cnt.wait_event++;

        /* re-get queue and count */
        for (queue = 0; queue < HAL_MT_NAMCHABARWA_PKT_RX_QUEUE_NUM; queue++)
        {
            _hal_mt_namchabarwa_pkt_getQueueCount(&ptr_rx_cb->sw_queue[queue], &que_cnt);
            if (que_cnt > 0)
            {
                ptr_rx_cb->deque_idx = ((queue + 1) % HAL_MT_NAMCHABARWA_PKT_RX_QUEUE_NUM);
                break;
            }
        }
    }

    /* deque */
    if ((que_cnt > 0) && (queue < HAL_MT_NAMCHABARWA_PKT_RX_QUEUE_NUM))
    {
        rc = _hal_mt_namchabarwa_pkt_deQueue(&ptr_rx_cb->sw_queue[queue], (void **)&ptr_sw_gpd_knl);
        if (CLX_E_OK == rc)
        {
            ptr_rx_cb->cnt.channel[queue].deque_ok++;
            ptr_sw_first_gpd_knl = ptr_sw_gpd_knl;

            osal_io_copyFromUser(&ioctl_data, ptr_cookie, sizeof(HAL_MT_NAMCHABARWA_PKT_IOCTL_RX_COOKIE_T));

            while (NULL != ptr_sw_gpd_knl)
            {
                /* get the IOCTL GPD from user */
                osal_io_copyFromUser(&ioctl_gpd,
                                        ((void *)((CLX_HUGE_T)ioctl_data.ioctl_gpd_addr))
                                            + gpd_idx*sizeof(HAL_MT_NAMCHABARWA_PKT_IOCTL_RX_GPD_T),
                                        sizeof(HAL_MT_NAMCHABARWA_PKT_IOCTL_RX_GPD_T));

                /* get knl buf addr */
                ptr_rx_gpd = &ptr_sw_gpd_knl->rx_gpd;
                phy_addr = CLX_ADDR_32_TO_64(ptr_rx_gpd->d_addr_hi, ptr_rx_gpd->d_addr_lo);

                ptr_virt_addr = ptr_sw_gpd_knl->ptr_cookie;
                osal_skb_unmapDma(phy_addr, ((struct sk_buff *)ptr_virt_addr)->len, DMA_FROM_DEVICE);

                buf_len = ptr_rx_gpd->size;

                /* overwrite whole rx_gpd to user
                    * the user should re-assign the correct value to data_buf_addr_hi, data_buf_addr_low
                    * after this IOCTL returns
                    */
                osal_io_copyToUser((void *)((CLX_HUGE_T)ioctl_gpd.hw_gpd_addr),
                                    &ptr_sw_gpd_knl->rx_gpd,
                                    sizeof(HAL_MT_NAMCHABARWA_PKT_RX_GPD_T));
                /* copy buf */
                /* DMA buf address allocated by the user is store in ptr_ioctl_data->gpd[idx].cookie */
                osal_io_copyToUser((void *)((CLX_HUGE_T)ioctl_gpd.dma_buf_addr),
                                    ((struct sk_buff *)ptr_virt_addr)->data, buf_len);
                ptr_sw_gpd_knl->ptr_cookie = ptr_virt_addr;

                /* next */
                ptr_sw_gpd_knl = ptr_sw_gpd_knl->ptr_next;
                gpd_idx++;
            }

            /* Must free kernel sw_gpd */
            _hal_mt_namchabarwa_pkt_freeRxGpdList(unit, ptr_sw_first_gpd_knl, TRUE);
        }
        else
        {
            ptr_rx_cb->cnt.channel[queue].deque_fail++;
            rc = CLX_E_OTHERS;
            osal_io_copyToUser(&ptr_cookie->rc, &rc, sizeof(CLX_ERROR_NO_T));
            return (CLX_E_OK);
        }
    }
    else
    {
        /* it means that all queue's are flush -> rx stop flow */
        rc = CLX_E_OTHERS;
        osal_io_copyToUser(&ptr_cookie->rc, &rc, sizeof(CLX_ERROR_NO_T));
        return (CLX_E_OK);
    }

    return (rc);
}

/* FUNCTION NAME: _hal_mt_namchabarwa_pkt_waitTxDone
 * PURPOSE:
 *      To determine the next action after transfer the packet to HW.
 * INPUT:
 *      unit            --  The unit ID
 *      channel         --  The target TX channel
 *      ptr_sw_gpd      --  Pointer for the SW Tx GPD link list
 * OUTPUT:
 *      None
 * RETURN:
 *      CLX_E_OK        -- Successfully perform the target action.
 * NOTES:
 *      None
 */
static CLX_ERROR_NO_T
_hal_mt_namchabarwa_pkt_waitTxDone(
    const UI32_T                    unit,
    const HAL_MT_NAMCHABARWA_PKT_TX_CHANNEL_T  channel,
          HAL_MT_NAMCHABARWA_PKT_TX_SW_GPD_T   *ptr_sw_gpd)
{
    CLX_ERROR_NO_T                  rc = CLX_E_OK;
    HAL_MT_NAMCHABARWA_PKT_TX_CB_T             *ptr_tx_cb = HAL_MT_NAMCHABARWA_PKT_GET_TX_CB_PTR(unit);
    HAL_MT_NAMCHABARWA_PKT_TX_PDMA_T           *ptr_tx_pdma = HAL_MT_NAMCHABARWA_PKT_GET_TX_PDMA_PTR(unit, channel);
    volatile HAL_MT_NAMCHABARWA_PKT_TX_GPD_T   *ptr_tx_gpd = NULL;
    UI32_T                          last_gpd_idx = 0;
    UI32_T                          loop_cnt = 0;

    if (HAL_MT_NAMCHABARWA_PKT_TX_WAIT_ASYNC == ptr_tx_cb->wait_mode)
    {
        ;
    }
    else if (HAL_MT_NAMCHABARWA_PKT_TX_WAIT_SYNC_INTR == ptr_tx_cb->wait_mode)
    {
        osal_takeSemaphore(&ptr_tx_pdma->sync_intr_sema, HAL_MT_NAMCHABARWA_PKT_PDMA_TX_INTR_TIMEOUT);
        /* rc = _hal_mt_namchabarwa_pkt_invokeTxGpdCallback(unit, ptr_sw_gpd); */
    }
    else if (HAL_MT_NAMCHABARWA_PKT_TX_WAIT_SYNC_POLL == ptr_tx_cb->wait_mode)
    {
        last_gpd_idx  = ptr_tx_pdma->free_idx + ptr_tx_pdma->used_gpd_num;
        last_gpd_idx %= ptr_tx_pdma->gpd_num;
        ptr_tx_gpd    = HAL_MT_NAMCHABARWA_PKT_GET_TX_GPD_PTR(unit, channel, last_gpd_idx);

        while (1 == ptr_tx_gpd->interrupt)
        {
            osal_dma_invalidateCache((void *)ptr_tx_gpd, sizeof(HAL_MT_NAMCHABARWA_PKT_TX_GPD_T));
            loop_cnt++;
            if (0 == loop_cnt % HAL_MT_NAMCHABARWA_PKT_PDMA_TX_POLL_MAX_LOOP)
            {
                ptr_tx_cb->cnt.channel[channel].poll_timeout++;
                rc = CLX_E_OTHERS;
                break;
            }
        }

        if (CLX_E_OK == rc)
        {
            ptr_tx_pdma->free_gpd_num += ptr_tx_pdma->used_gpd_num;
            ptr_tx_pdma->used_gpd_num  = 0;
            ptr_tx_pdma->free_idx      = ptr_tx_pdma->used_idx;
            /* rc = _hal_mt_namchabarwa_pkt_invokeTxGpdCallback(unit, ptr_sw_gpd); */
        }
    }

    return (rc);
}

static CLX_ERROR_NO_T
_hal_mt_namchabarwa_pkt_resumeAllIntf(
    const UI32_T                        unit)
{
    struct net_device                   *ptr_net_dev = NULL;
    UI32_T                              port;

    /* Unregister net devices by id */
    for (port = 0; port < HAL_MT_NAMCHABARWA_PKT_MAX_PORT_NUM; port++)
    {
        ptr_net_dev = HAL_MT_NAMCHABARWA_PKT_GET_PORT_NETDEV(port);
        if (NULL != ptr_net_dev)
        {
            if (netif_queue_stopped(ptr_net_dev))
            {
                netif_wake_queue(ptr_net_dev);
            }
        }
    }

    return (CLX_E_OK);
}

static CLX_ERROR_NO_T
_hal_mt_namchabarwa_pkt_suspendAllIntf(
    const UI32_T                        unit)
{
    struct net_device                   *ptr_net_dev = NULL;
    UI32_T                              port;

    /* Unregister net devices by id */
    for (port = 0; port < HAL_MT_NAMCHABARWA_PKT_MAX_PORT_NUM; port++)
    {
        ptr_net_dev = HAL_MT_NAMCHABARWA_PKT_GET_PORT_NETDEV(port);
        if (NULL != ptr_net_dev)
        {
            netif_stop_queue(ptr_net_dev);
        }
    }

    return (CLX_E_OK);
}

static CLX_ERROR_NO_T
_hal_mt_namchabarwa_pkt_stopAllIntf(
    const UI32_T                        unit)
{
    struct net_device                   *ptr_net_dev = NULL;
    UI32_T                              port;

    /* Unregister net devices by id */
    for (port = 0; port < HAL_MT_NAMCHABARWA_PKT_MAX_PORT_NUM; port++)
    {
        ptr_net_dev = HAL_MT_NAMCHABARWA_PKT_GET_PORT_NETDEV(port);
        if (NULL != ptr_net_dev)
        {
            netif_tx_disable(ptr_net_dev);
        }
    }

    return (CLX_E_OK);
}

/* FUNCTION NAME: hal_mt_namchabarwa_pkt_sendGpd
 * PURPOSE:
 *      To perform the packet transmission form CPU to the switch.
 * INPUT:
 *      unit            --  The unit ID
 *      channel         --  The target TX channel
 *      ptr_sw_gpd      --  Pointer for the SW Tx GPD link list
 * OUTPUT:
 *      None
 * RETURN:
 *      CLX_E_OK        --  Successfully perform the transferring.
 * NOTES:
 *      None
 */
CLX_ERROR_NO_T
hal_mt_namchabarwa_pkt_sendGpd(
    const UI32_T                    unit,
    const HAL_MT_NAMCHABARWA_PKT_TX_CHANNEL_T  channel,
          HAL_MT_NAMCHABARWA_PKT_TX_SW_GPD_T   *ptr_sw_gpd)
{
    CLX_ERROR_NO_T                  rc = CLX_E_OK;
    HAL_MT_NAMCHABARWA_PKT_TX_CB_T             *ptr_tx_cb = HAL_MT_NAMCHABARWA_PKT_GET_TX_CB_PTR(unit);
    HAL_MT_NAMCHABARWA_PKT_TX_PDMA_T           *ptr_tx_pdma = HAL_MT_NAMCHABARWA_PKT_GET_TX_PDMA_PTR(unit, channel);
    volatile HAL_MT_NAMCHABARWA_PKT_TX_GPD_T   *ptr_tx_gpd = NULL;
    HAL_MT_NAMCHABARWA_PKT_TX_SW_GPD_T         *ptr_sw_first_gpd = ptr_sw_gpd;
    UI32_T                          used_idx = 0;
    UI32_T                          used_gpd_num = ptr_sw_gpd->gpd_num;
    CLX_IRQ_FLAGS_T                 irq_flags;
    HAL_MT_NAMCHABARWA_PKT_DRV_CB_T            *ptr_cb = HAL_MT_NAMCHABARWA_PKT_GET_DRV_CB_PTR(unit);

    if (0 != (ptr_cb->init_flag & HAL_MT_NAMCHABARWA_PKT_INIT_TASK))
    {
        osal_takeIsrLock(&ptr_tx_pdma->ring_lock, &irq_flags);

        /* If not PDMA error */
        if (FALSE == ptr_tx_pdma->err_flag)
        {
            /* Make Sure GPD is enough */
            if (ptr_tx_pdma->free_gpd_num >= used_gpd_num)
            {
                used_idx = ptr_tx_pdma->used_idx;
                while (NULL != ptr_sw_gpd)
                {
                    ptr_tx_gpd = HAL_MT_NAMCHABARWA_PKT_GET_TX_GPD_PTR(unit, channel, used_idx);
                    osal_dma_invalidateCache((void *)ptr_tx_gpd, sizeof(HAL_MT_NAMCHABARWA_PKT_TX_GPD_T));

                    /* Fill in HW-GPD Ring */
                    osal_memcpy((void *)ptr_tx_gpd, &ptr_sw_gpd->tx_gpd, sizeof(HAL_MT_NAMCHABARWA_PKT_TX_GPD_T));
                    osal_dma_flushCache((void *)ptr_tx_gpd, sizeof(HAL_MT_NAMCHABARWA_PKT_TX_GPD_T));

                    /* next */
                    used_idx++;
                    used_idx %= ptr_tx_pdma->gpd_num;
                    ptr_sw_gpd = ptr_sw_gpd->ptr_next;
                }
                ptr_tx_pdma->soft_work_idx += used_gpd_num;
                _hal_mt_namchabarwa_pkt_setTxWorkIdx(unit, channel, ptr_tx_pdma->soft_work_idx);

                if (HAL_MT_NAMCHABARWA_PKT_TX_WAIT_ASYNC == ptr_tx_cb->wait_mode)
                {
                    /* Fill 1st GPD in SW-GPD Ring */
                    ptr_tx_pdma->pptr_sw_gpd_ring[ptr_tx_pdma->used_idx] = ptr_sw_first_gpd;
                }

                /* update Tx PDMA */
                ptr_tx_pdma->used_idx      = used_idx;
                ptr_tx_pdma->used_gpd_num += used_gpd_num;
                ptr_tx_pdma->free_gpd_num -= used_gpd_num;

                ptr_tx_cb->cnt.channel[channel].send_ok++;

                _hal_mt_namchabarwa_pkt_waitTxDone(unit, channel, ptr_sw_first_gpd);

                /* reserve 1 packet buffer for each port in case that the suspension is too late */
#define HAL_MT_NAMCHABARWA_PKT_KNL_TX_RING_AVBL_GPD_LOW      (HAL_MT_NAMCHABARWA_PORT_NUM)
                if (ptr_tx_pdma->free_gpd_num < HAL_MT_NAMCHABARWA_PKT_KNL_TX_RING_AVBL_GPD_LOW)
                {
                    OSAL_PRINT(OSAL_DBG_TX,
                                    "u=%u, txch=%u, tx avbl gpd < %d, suspend all netdev\n",
                                    unit, channel, HAL_MT_NAMCHABARWA_PKT_KNL_TX_RING_AVBL_GPD_LOW);
                    _hal_mt_namchabarwa_pkt_suspendAllIntf(unit);
                }
            }
            else
            {
                rc = CLX_E_TABLE_FULL;
            }
        }
        else
        {
            OSAL_PRINT((OSAL_DBG_ERR | OSAL_DBG_TX),
                            "u=%u, txch=%u, pdma hw err\n",
                            unit, channel);
            rc = CLX_E_OTHERS;
        }

        osal_giveIsrLock(&ptr_tx_pdma->ring_lock, &irq_flags);
    }
    else
    {
        OSAL_PRINT(OSAL_DBG_ERR,
                        "Tx failed, task already deinit\n");
        rc = CLX_E_OTHERS;
    }
    return (rc);
}

/* ----------------------------------------------------------------------------------- pkt_srv */
/* ----------------------------------------------------------------------------------- Rx Init */
static CLX_ERROR_NO_T
_hal_mt_namchabarwa_pkt_rxStop(
    const UI32_T                unit)
{
    CLX_ERROR_NO_T              rc = CLX_E_OK;
    HAL_MT_NAMCHABARWA_PKT_RX_CHANNEL_T    channel = 0;
    UI32_T                      idx;
    HAL_MT_NAMCHABARWA_PKT_RX_CB_T         *ptr_rx_cb = HAL_MT_NAMCHABARWA_PKT_GET_RX_CB_PTR(unit);
    HAL_MT_NAMCHABARWA_PKT_DRV_CB_T        *ptr_cb = HAL_MT_NAMCHABARWA_PKT_GET_DRV_CB_PTR(unit);
    HAL_MT_NAMCHABARWA_PKT_RX_PDMA_T       *ptr_rx_pdma = NULL;

    /* Check if Rx is already stopped*/
    if (0 == (ptr_cb->init_flag & HAL_MT_NAMCHABARWA_PKT_INIT_RX_START))
    {
        OSAL_PRINT((OSAL_DBG_RX | OSAL_DBG_ERR),
                        "u=%u, rx stop failed, not started\n", unit);
        return (CLX_E_OK);
    }

    /* Check if PKT Drv/Task were de-init before stopping Rx */
    /* Currently, we help to stop Rx when deinit Drv/Task, so it shouldn't enter below logic */
    if ((0 == (ptr_cb->init_flag & HAL_MT_NAMCHABARWA_PKT_INIT_TASK)) ||
        (0 == (ptr_cb->init_flag & HAL_MT_NAMCHABARWA_PKT_INIT_DRV)))
    {
        OSAL_PRINT((OSAL_DBG_RX | OSAL_DBG_ERR),
                        "u=%u, rx stop failed, pkt task & pkt drv not init\n", unit);
        return (CLX_E_OK);
    }

    /* Deinit Rx PDMA and free buf for Rx GPD */
    for (channel = 0; channel < HAL_MT_NAMCHABARWA_PKT_RX_CHANNEL_LAST; channel++)
    {
        ptr_rx_pdma = HAL_MT_NAMCHABARWA_PKT_GET_RX_PDMA_PTR(unit, channel);

        osal_takeSemaphore(&ptr_rx_pdma->sema, CLX_SEMAPHORE_WAIT_FOREVER);
        _hal_mt_namchabarwa_pkt_stopRxChannelReg(unit, channel);
        rc = _hal_mt_namchabarwa_pkt_deinitRxPdmaRingBuf(unit, channel);
        osal_giveSemaphore(&ptr_rx_pdma->sema);
    }


    /* flush packets in all queues since Rx task may be blocked in user space
     * in this case it won't do ioctl to kernel to handle remaining packets
     */
    for (idx = 0; idx < HAL_MT_NAMCHABARWA_PKT_RX_QUEUE_NUM; idx++)
    {
        _hal_mt_namchabarwa_pkt_flushRxQueue(unit, &ptr_rx_cb->sw_queue[idx]);
    }

    /* Return user thread */
    ptr_rx_cb->running = FALSE;
    ptr_cb->init_flag &= (~HAL_MT_NAMCHABARWA_PKT_INIT_RX_START);

    OSAL_PRINT(OSAL_DBG_RX,
                    "u=%u, rx stop done, init flag=0x%x\n", unit, ptr_cb->init_flag);

    osal_triggerEvent(&ptr_rx_cb->sync_sema);

    return (rc);
}

static CLX_ERROR_NO_T
_hal_mt_namchabarwa_pkt_rxStart(
    const UI32_T                unit)
{
    CLX_ERROR_NO_T              rc = CLX_E_OK;
    HAL_MT_NAMCHABARWA_PKT_RX_CHANNEL_T    channel = 0;
    HAL_MT_NAMCHABARWA_PKT_RX_CB_T         *ptr_rx_cb = HAL_MT_NAMCHABARWA_PKT_GET_RX_CB_PTR(unit);
    HAL_MT_NAMCHABARWA_PKT_DRV_CB_T        *ptr_cb = HAL_MT_NAMCHABARWA_PKT_GET_DRV_CB_PTR(unit);
    HAL_MT_NAMCHABARWA_PKT_RX_PDMA_T       *ptr_rx_pdma = NULL;

    if (0 != (ptr_cb->init_flag & HAL_MT_NAMCHABARWA_PKT_INIT_RX_START))
    {
        OSAL_PRINT((OSAL_DBG_RX | OSAL_DBG_ERR),
                        "u=%u, rx start failed, already started\n", unit);
        return (CLX_E_OK);
    }
    

    /* init Rx PDMA and alloc buf for Rx GPD */
    for (channel = 0; channel < HAL_MT_NAMCHABARWA_PKT_RX_CHANNEL_LAST; channel++)
    {
        ptr_rx_pdma = HAL_MT_NAMCHABARWA_PKT_GET_RX_PDMA_PTR(unit, channel);

        osal_takeSemaphore(&ptr_rx_pdma->sema, CLX_SEMAPHORE_WAIT_FOREVER);
        _hal_mt_namchabarwa_pkt_stopRxChannelReg(unit, channel);
        rc = _hal_mt_namchabarwa_pkt_initRxPdmaRingBuf(unit, channel);
        if (CLX_E_OK == rc)
        {
            ptr_rx_pdma->work_idx = _hal_mt_namchabarwa_pkt_getRxWorkIdx(unit, channel);
            ptr_rx_pdma->pop_idx = _hal_mt_namchabarwa_pkt_getRxPopIdx(unit, channel);
            if (ptr_rx_pdma->pop_idx == ptr_rx_pdma->work_idx)
            {
                ptr_rx_pdma->work_idx = (ptr_rx_pdma->pop_idx + ptr_rx_pdma->gpd_num - 1) % ptr_rx_pdma->gpd_num;
                _hal_mt_namchabarwa_pkt_setRxWorkIdx(unit,channel, ptr_rx_pdma->work_idx);
            }
        }
        _hal_mt_namchabarwa_pkt_startRxChannelReg(unit, channel);
        osal_giveSemaphore(&ptr_rx_pdma->sema);
    }

    /* enable to dequeue rx packets */
    ptr_rx_cb->running = TRUE;

    /* set the flag to record init state */
    ptr_cb->init_flag |= HAL_MT_NAMCHABARWA_PKT_INIT_RX_START;

    OSAL_PRINT(OSAL_DBG_RX,
                    "u=%u, rx start done, init flag=0x%x\n", unit, ptr_cb->init_flag);
    return (rc);
}

/* FUNCTION NAME: hal_mt_namchabarwa_pkt_setRxKnlConfig
 * PURPOSE:
 *      1. To stop the Rx channel and deinit the Rx subsystem.
 *      2. To init the Rx subsystem and start the Rx channel.
 *      3. To restart the Rx subsystem
 * INPUT:
 *      unit            -- The unit ID
 *      ptr_cookie      -- Pointer of the RX cookie
 * OUTPUT:
 *      None
 * RETURN:
 *      CLX_E_OK        -- Successfully configure the RX parameters.
 *      CLX_E_OTHERS    -- Configure the parameter failed.
 * NOTES:
 *
 */
CLX_ERROR_NO_T
hal_mt_namchabarwa_pkt_setRxKnlConfig(
    const UI32_T                    unit,
    void                            *ptr_data)
{
    CLX_ERROR_NO_T                  rc = CLX_E_OK;
    HAL_MT_NAMCHABARWA_PKT_RX_CB_T             *ptr_rx_cb = HAL_MT_NAMCHABARWA_PKT_GET_RX_CB_PTR(unit);
    HAL_MT_NAMCHABARWA_PKT_DRV_CB_T            *ptr_cb = HAL_MT_NAMCHABARWA_PKT_GET_DRV_CB_PTR(unit);
    HAL_MT_NAMCHABARWA_PKT_IOCTL_RX_TYPE_T     rx_type = HAL_MT_NAMCHABARWA_PKT_IOCTL_RX_TYPE_LAST;
    HAL_MT_NAMCHABARWA_PKT_IOCTL_RX_COOKIE_T   *ptr_cookie = ptr_data;

    osal_io_copyFromUser(&rx_type, &ptr_cookie->rx_type, sizeof(HAL_MT_NAMCHABARWA_PKT_IOCTL_RX_TYPE_T));

    if (HAL_MT_NAMCHABARWA_PKT_IOCTL_RX_TYPE_DEINIT == rx_type)
    {
        _hal_mt_namchabarwa_pkt_rxStop(unit);
    }
    if (HAL_MT_NAMCHABARWA_PKT_IOCTL_RX_TYPE_INIT == rx_type)
    {
        /* To prevent buffer size from being on-the-fly changed */
        if (0 != (ptr_cb->init_flag & HAL_MT_NAMCHABARWA_PKT_INIT_RX_START))
        {
            OSAL_PRINT((OSAL_DBG_RX | OSAL_DBG_ERR),
                             "u=%u, rx stop failed, not started\n", unit);
            return (CLX_E_OK);
        }

        osal_io_copyFromUser(&ptr_rx_cb->buf_len, &ptr_cookie->buf_len, sizeof(UI32_T));
        ptr_rx_cb->buf_len += HAL_MT_NAMCHABARWA_PKT_PDMA_HDR_SZ;
        _hal_mt_namchabarwa_pkt_rxStart(unit);
    }

    return (rc);
}

/* FUNCTION NAME: hal_mt_namchabarwa_pkt_getRxKnlConfig
 * PURPOSE:
 *      To get the Rx subsystem configuration.
 * INPUT:
 *      unit            -- The unit ID
 *      ptr_cookie      -- Pointer of the RX cookie
 * OUTPUT:
 *      None
 * RETURN:
 *      CLX_E_OK        -- Successfully configure the RX parameters.
 *      CLX_E_OTHERS    -- Configure the parameter failed.
 * NOTES:
 *
 */
CLX_ERROR_NO_T
hal_mt_namchabarwa_pkt_getRxKnlConfig(
    const UI32_T                    unit,
    void                            *ptr_data)
{
    HAL_MT_NAMCHABARWA_PKT_RX_CB_T             *ptr_rx_cb = HAL_MT_NAMCHABARWA_PKT_GET_RX_CB_PTR(unit);
    HAL_MT_NAMCHABARWA_PKT_IOCTL_RX_COOKIE_T   *ptr_cookie = ptr_data;

    osal_io_copyToUser(&ptr_cookie->buf_len, &ptr_rx_cb->buf_len, sizeof(UI32_T));

    return (CLX_E_OK);
}

/* ----------------------------------------------------------------------------------- Deinit */
/* FUNCTION NAME: hal_mt_namchabarwa_pkt_deinitTask
 * PURPOSE:
 *      To de-initialize the Task for packet module.
 * INPUT:
 *      unit            --  The unit ID
 * OUTPUT:
 *      None
 * RETURN:
 *      CLX_E_OK        --  Successfully dinitialize the control block.
 *      CLX_E_OTHERS    --  Initialize the control block failed.
 * NOTES:
 *      None
 */
CLX_ERROR_NO_T
hal_mt_namchabarwa_pkt_deinitTask(
    const UI32_T            unit,
    void                    *ptr_data)
{
    HAL_MT_NAMCHABARWA_PKT_DRV_CB_T    *ptr_cb    = HAL_MT_NAMCHABARWA_PKT_GET_DRV_CB_PTR(unit);
    HAL_MT_NAMCHABARWA_PKT_TX_CB_T     *ptr_tx_cb = HAL_MT_NAMCHABARWA_PKT_GET_TX_CB_PTR(unit);
    HAL_MT_NAMCHABARWA_PKT_RX_CB_T     *ptr_rx_cb = HAL_MT_NAMCHABARWA_PKT_GET_RX_CB_PTR(unit);
    UI32_T                  channel = 0;

    /* to prevent net intf from Tx packet */
    ptr_tx_cb->net_tx_allowed = FALSE;

    /* In case that some undestroyed net intf keep Tx after task deinit */
    _hal_mt_namchabarwa_pkt_stopAllIntf(unit);

    if (0 == (ptr_cb->init_flag & HAL_MT_NAMCHABARWA_PKT_INIT_TASK))
    {
        OSAL_PRINT((OSAL_DBG_RX | OSAL_DBG_ERR),
                        "u=%u, rx stop failed, not started\n", unit);
        return (CLX_E_OK);
    }

    /* Need to stop Rx before de-init Task */
    if (0 != (ptr_cb->init_flag & HAL_MT_NAMCHABARWA_PKT_INIT_RX_START))
    {
        OSAL_PRINT((OSAL_DBG_RX | OSAL_DBG_ERR),
                        "u=%u, pkt task deinit failed, rx not stop\n", unit);

        _hal_mt_namchabarwa_pkt_rxStop(unit);
    }

    /* Make the Rx IOCTL from userspace return back*/
    osal_triggerEvent(&ptr_rx_cb->sync_sema);

    /* Destroy txTask */
    if (HAL_MT_NAMCHABARWA_PKT_TX_WAIT_ASYNC == ptr_tx_cb->wait_mode)
    {
        ptr_tx_cb->running = FALSE;
        osal_triggerEvent(&ptr_tx_cb->sync_sema);
    }

    /* Destroy handleRxDoneTask */
    for (channel = 0; channel < HAL_MT_NAMCHABARWA_PKT_RX_CHANNEL_LAST; channel++)
    {
        osal_stopThread(&ptr_rx_cb->isr_task_id[channel]);
        osal_triggerEvent(HAL_MT_NAMCHABARWA_PKT_RCH_EVENT(unit, channel));
        osal_destroyThread(&ptr_rx_cb->isr_task_id[channel]);
    }

    /* Destroy handleTxDoneTask */
    for (channel = 0; channel < HAL_MT_NAMCHABARWA_PKT_TX_CHANNEL_LAST; channel++)
    {
        osal_stopThread(&ptr_tx_cb->isr_task_id[channel]);
        osal_triggerEvent(HAL_MT_NAMCHABARWA_PKT_TCH_EVENT(unit, channel));
        osal_destroyThread(&ptr_tx_cb->isr_task_id[channel]);
    }

    /* Destroy handleErrorTask */
    osal_stopThread(&ptr_cb->err_task_id);
    osal_triggerEvent(HAL_MT_NAMCHABARWA_PKT_ERR_EVENT(unit));
    osal_destroyThread(&ptr_cb->err_task_id);

    /* Set the flag to record init state */
    ptr_cb->init_flag &= (~HAL_MT_NAMCHABARWA_PKT_INIT_TASK);

    OSAL_PRINT(OSAL_DBG_RX,
                    "u=%u, pkt task deinit done, init flag=0x%x\n",
                    unit, ptr_cb->init_flag);

    return (CLX_E_OK);
}

/* FUNCTION NAME: _hal_mt_namchabarwa_pkt_deinitTxPdma
 * PURPOSE:
 *      To de-initialize the Tx PDMA configuration of the specified channel.
 * INPUT:
 *      unit            --  The unit ID
 *      channel         --  The target Tx channel
 * OUTPUT:
 *      None
 * RETURN:
 *      CLX_E_OK        --  Successfully de-init the Tx PDMA.
 *      CLX_E_OTHERS    --  De-init the Tx PDMA failed.
 * NOTES:
 *      None
 */
static CLX_ERROR_NO_T
_hal_mt_namchabarwa_pkt_deinitTxPdma(
    const UI32_T                    unit,
    const HAL_MT_NAMCHABARWA_PKT_TX_CHANNEL_T  channel)
{
    HAL_MT_NAMCHABARWA_PKT_TX_CB_T             *ptr_tx_cb   = HAL_MT_NAMCHABARWA_PKT_GET_TX_CB_PTR(unit);
    HAL_MT_NAMCHABARWA_PKT_TX_PDMA_T           *ptr_tx_pdma = HAL_MT_NAMCHABARWA_PKT_GET_TX_PDMA_PTR(unit, channel);

    _hal_mt_namchabarwa_pkt_stopTxChannelReg(unit, channel);

    /* Free DMA and flush queue */
    osal_dma_free(ptr_tx_pdma->ptr_gpd_start_addr);

    if (HAL_MT_NAMCHABARWA_PKT_TX_WAIT_ASYNC == ptr_tx_cb->wait_mode)
    {
        osal_free(ptr_tx_pdma->pptr_sw_gpd_ring);
        osal_free(ptr_tx_pdma->pptr_sw_gpd_bulk);
    }
    else if (HAL_MT_NAMCHABARWA_PKT_TX_WAIT_SYNC_INTR == ptr_tx_cb->wait_mode)
    {
        osal_destroySemaphore(&ptr_tx_pdma->sync_intr_sema);
    }

    osal_destroyIsrLock(&ptr_tx_pdma->ring_lock);

    return (CLX_E_OK);
}

/* FUNCTION NAME: _hal_mt_namchabarwa_pkt_deinitRxPdma
 * PURPOSE:
 *      To de-initialize the Rx PDMA configuration of the specified channel.
 * INPUT:
 *      unit            --  The unit ID
 *      channel         --  The target Rx channel
 * OUTPUT:
 *      None
 * RETURN:
 *      CLX_E_OK        --  Successfully de-init the Rx PDMA.
 * NOTES:
 *      None
 */
static CLX_ERROR_NO_T
_hal_mt_namchabarwa_pkt_deinitRxPdma(
    const UI32_T                    unit,
    const HAL_MT_NAMCHABARWA_PKT_RX_CHANNEL_T  channel)
{
    HAL_MT_NAMCHABARWA_PKT_RX_PDMA_T           *ptr_rx_pdma = HAL_MT_NAMCHABARWA_PKT_GET_RX_PDMA_PTR(unit, channel);

    /* Free DMA */
    osal_dma_free(ptr_rx_pdma->ptr_gpd_start_addr);

    osal_giveSemaphore(&ptr_rx_pdma->sema);
    osal_destroySemaphore(&ptr_rx_pdma->sema);

    return (CLX_E_OK);
}

/* FUNCTION NAME: _hal_mt_namchabarwa_pkt_deinitPktCb
 * PURPOSE:
 *      To de-init the control block of Drv.
 * INPUT:
 *      unit            --  The unit ID
 * OUTPUT:
 *      None
 * RETURN:
 *      CLX_E_OK        --  Successfully de-init the control block.
 * NOTES:
 *      None
 */
static CLX_ERROR_NO_T
_hal_mt_namchabarwa_pkt_deinitPktCb(
    const UI32_T                unit)
{
    HAL_MT_NAMCHABARWA_PKT_DRV_CB_T        *ptr_cb = HAL_MT_NAMCHABARWA_PKT_GET_DRV_CB_PTR(unit);
    UI32_T                      idx = 0, vec = sizeof(_hal_mt_namchabarwa_pkt_intr_vec) / sizeof(HAL_MT_NAMCHABARWA_PKT_INTR_VEC_T);

    for (idx = 0; idx < vec; idx++)
    {
        osal_destroyEvent(&_hal_mt_namchabarwa_pkt_intr_vec[idx].intr_event);
        osal_destroyEvent(&_hal_mt_namchabarwa_pkt_err_intr_vec[idx].intr_event);
        ptr_cb->intr_bitmap &= ~(_hal_mt_namchabarwa_pkt_intr_vec[idx].intr_reg);
    }

    /* Unregister PKT interrupt functions */
    osal_mdc_registerIsr(unit, NULL, NULL);
    osal_mdc_synchronize_irq(unit);
    osal_destroyIsrLock(&ptr_cb->intr_lock);

    return (CLX_E_OK);
}

/* FUNCTION NAME: _hal_mt_namchabarwa_pkt_deinitPktTxCb
 * PURPOSE:
 *      To de-init the control block of Tx PDMA.
 * INPUT:
 *      unit            --  The unit ID
 * OUTPUT:
 *      None
 * RETURN:
 *      CLX_E_OK        --  Successfully de-init the control block.
 * NOTES:
 *      None
 */
static CLX_ERROR_NO_T
_hal_mt_namchabarwa_pkt_deinitPktTxCb(
    const UI32_T                unit)
{
    CLX_ERROR_NO_T              rc = CLX_E_OK;
    HAL_MT_NAMCHABARWA_PKT_TX_CB_T         *ptr_tx_cb = HAL_MT_NAMCHABARWA_PKT_GET_TX_CB_PTR(unit);
    HAL_MT_NAMCHABARWA_PKT_TX_CHANNEL_T    channel = 0;

    /* Deinitialize TX PDMA sub-system.*/
    for (channel = 0; channel < HAL_MT_NAMCHABARWA_PKT_TX_CHANNEL_LAST; channel++)
    {
        _hal_mt_namchabarwa_pkt_deinitTxPdma(unit, channel);
    }

    if (HAL_MT_NAMCHABARWA_PKT_TX_WAIT_ASYNC == ptr_tx_cb->wait_mode)
    {
        /* Destroy the sync semaphore of txTask */
        osal_destroyEvent(&ptr_tx_cb->sync_sema);

        /* Deinitialize Tx GPD-queue (of first SW-GPD) from handleTxDoneTask to txTask */
        osal_destroySemaphore(&ptr_tx_cb->sw_queue.sema);
        osal_que_destroy(&ptr_tx_cb->sw_queue.que_id);
    }

    return (rc);
}

/* FUNCTION NAME: _hal_mt_namchabarwa_pkt_deinitPktRxCb
 * PURPOSE:
 *      To de-init the control block of Rx PDMA.
 * INPUT:
 *      unit            --  The unit ID
 * OUTPUT:
 *      None
 * RETURN:
 *      CLX_E_OK        --  Successfully de-init the control block.
 * NOTES:
 *      None
 */
static CLX_ERROR_NO_T
_hal_mt_namchabarwa_pkt_deinitPktRxCb(
    const UI32_T                unit)
{
    CLX_ERROR_NO_T              rc = CLX_E_OK;
    HAL_MT_NAMCHABARWA_PKT_RX_CB_T         *ptr_rx_cb = HAL_MT_NAMCHABARWA_PKT_GET_RX_CB_PTR(unit);
    HAL_MT_NAMCHABARWA_PKT_RX_CHANNEL_T    channel = 0;
    UI32_T                      queue = 0;

    /* Deinitialize RX PDMA sub-system */
    for (channel = 0; channel < HAL_MT_NAMCHABARWA_PKT_RX_CHANNEL_LAST; channel++)
    {
        _hal_mt_namchabarwa_pkt_deinitRxPdma(unit, channel);
    }

    /* Destroy the sync semaphore of rxTask */
    osal_destroyEvent(&ptr_rx_cb->sync_sema);

    /* Deinitialize Rx GPD-queue (of first SW-GPD) from handleRxDoneTask to rxTask */
    for (queue = 0; queue < HAL_MT_NAMCHABARWA_PKT_RX_QUEUE_NUM; queue++)
    {
        osal_destroySemaphore(&ptr_rx_cb->sw_queue[queue].sema);
        osal_que_destroy(&ptr_rx_cb->sw_queue[queue].que_id);
    }

    return (rc);
}

/* FUNCTION NAME: _hal_mt_namchabarwa_pkt_deinitL1Isr
 * PURPOSE:
 *      To de-initialize the PDMA L1 ISR configuration.
 * INPUT:
 *      unit            -- The unit ID
 * OUTPUT:
 *      None
 * RETURN:
 *      CLX_E_OK        -- Successfully de-initialize for the L1 ISR.
 * NOTES:
 *      None
 */
static CLX_ERROR_NO_T
_hal_mt_namchabarwa_pkt_deinitL1Isr(
    const UI32_T            unit)
{
    UI32_T                  idx = 0, vec = sizeof(_hal_mt_namchabarwa_pkt_intr_vec) / sizeof(HAL_MT_NAMCHABARWA_PKT_INTR_VEC_T);

    for (idx = 0; idx < vec; idx++)
    {
        _hal_mt_namchabarwa_pkt_maskIntr(unit, idx);
    }

    return (CLX_E_OK);
}

/* FUNCTION NAME: hal_mt_namchabarwa_pkt_deinitPktDrv
 * PURPOSE:
 *      To invoke the functions to de-initialize the control block for each
 *      PDMA subsystem.
 * INPUT:
 *      unit            --  The unit ID
 * OUTPUT:
 *      None
 * RETURN:
 *      CLX_E_OK        --  Successfully de-initialize the control blocks.
 *      CLX_E_OTHERS    --  De-initialize the control blocks failed.
 * NOTES:
 *      None
 */
CLX_ERROR_NO_T
hal_mt_namchabarwa_pkt_deinitPktDrv(
    const UI32_T            unit,
    void                    *ptr_data)
{
    HAL_MT_NAMCHABARWA_PKT_DRV_CB_T    *ptr_cb = HAL_MT_NAMCHABARWA_PKT_GET_DRV_CB_PTR(unit);
    CLX_ERROR_NO_T          rc = CLX_E_OK;

    if (0 == (ptr_cb->init_flag & HAL_MT_NAMCHABARWA_PKT_INIT_DRV))
    {
        OSAL_PRINT((OSAL_DBG_RX | OSAL_DBG_ERR),
                        "u=%u, pkt drv deinit failed, not inited\n", unit);
        return (CLX_E_OK);
    }

    if (CLX_E_OK == rc)
    {
        rc = _hal_mt_namchabarwa_pkt_deinitL1Isr(unit);
    }
    if (CLX_E_OK == rc)
    {
        rc = _hal_mt_namchabarwa_pkt_deinitPktRxCb(unit);
    }
    if (CLX_E_OK == rc)
    {
        rc = _hal_mt_namchabarwa_pkt_deinitPktTxCb(unit);
    }
    if (CLX_E_OK == rc)
    {
        rc = _hal_mt_namchabarwa_pkt_deinitPktCb(unit);
    }

    ptr_cb->init_flag &= (~HAL_MT_NAMCHABARWA_PKT_INIT_DRV);

    OSAL_PRINT(OSAL_DBG_COMMON,
                    "u=%u, pkt drv deinit done, init flag=0x%x\n",
                    unit, ptr_cb->init_flag);
    return (rc);
}

/* ----------------------------------------------------------------------------------- Init */
/* FUNCTION NAME: _hal_mt_namchabarwa_pkt_handleTxErrStat
 * PURPOSE:
 *      To handle the error which occurs in TX channels.
 * INPUT:
 *      unit            --  The unit ID
 *      channel         --  The channel where the error occurs
 * OUTPUT:
 *      None
 * RETURN:
 *      CLX_E_OK        --  Successfully handle the error situation.
 * NOTES:
 *
 */
static CLX_ERROR_NO_T
_hal_mt_namchabarwa_pkt_handleTxErrStat(
    const UI32_T                    unit,
    const HAL_MT_NAMCHABARWA_PKT_TX_CHANNEL_T  channel)
{
    HAL_MT_NAMCHABARWA_PKT_TX_PDMA_T           *ptr_tx_pdma = HAL_MT_NAMCHABARWA_PKT_GET_TX_PDMA_PTR(unit, channel);
    HAL_MT_NAMCHABARWA_PKT_TX_CB_T             *ptr_tx_cb = HAL_MT_NAMCHABARWA_PKT_GET_TX_CB_PTR(unit);
    UI32_T err_intr_status = 0;
    UI32_T err_type = 0;

    osal_mdc_readPciReg(unit,
        HAL_MT_NAMCHABARWA_PKT_PDMA_IRQ_PDMA_ABNORMAL_CH0_INTR + 0xC *  HAL_MT_NAMCHABARWA_PKT_TX_CHANNEL(channel),
        &err_intr_status, sizeof(UI32_T));
    if(0 == err_intr_status)
    {
        return (CLX_E_OK);
    }

    /* Set the error flag. */
    osal_takeSemaphore(&ptr_tx_pdma->ring_lock, CLX_SEMAPHORE_WAIT_FOREVER);
    ptr_tx_pdma->err_flag = TRUE;
    osal_giveSemaphore(&ptr_tx_pdma->ring_lock);

    //read err_type
    osal_mdc_readPciReg(unit,
        HAL_MT_NAMCHABARWA_PKT_GET_PDMA_WORD_TCH_REG(HAL_MT_NAMCHABARWA_PKT_PDMA_STA_PDMA_CH0_ERR_TYPE,channel),
        &err_type, sizeof(UI32_T));
    if(0 != (err_type & HAL_MT_NAMCHABARWA_PKT_PDMA_CHANNEL_RD_DESC_ERROR))
    {
        ptr_tx_cb->cnt.channel[channel].rd_desc_err++;
    }

    if(0 != (err_type & HAL_MT_NAMCHABARWA_PKT_PDMA_CHANNEL_WR_DESC_ERROR))
    {
        ptr_tx_cb->cnt.channel[channel].wr_desc_err++;
    }

    if(0 != (err_type & HAL_MT_NAMCHABARWA_PKT_PDMA_CHANNEL_RD_DATA_ERROR))
    {
        ptr_tx_cb->cnt.channel[channel].rd_data_err++;
    }

    if(0 != (err_type & HAL_MT_NAMCHABARWA_PKT_PDMA_CHANNEL_WR_DATA_ERROR))
    {
        ptr_tx_cb->cnt.channel[channel].wr_data_err++;
    }

    osal_triggerEvent(HAL_MT_NAMCHABARWA_PKT_TCH_EVENT(unit, channel));

    return (CLX_E_OK);
}

/* FUNCTION NAME: _hal_mt_namchabarwa_pkt_handleRxErrStat
 * PURPOSE:
 *      To handle the error which occurs in RX channels.
 * INPUT:
 *      unit            --  The unit ID
 *      channel         --  The channel where the error occurs
 * OUTPUT:
 *      None
 * RETURN:
 *      CLX_E_OK        --  Successfully handle the error situation.
 * NOTES:
 *
 */
static CLX_ERROR_NO_T
_hal_mt_namchabarwa_pkt_handleRxErrStat(
    const UI32_T                    unit,
    const HAL_MT_NAMCHABARWA_PKT_RX_CHANNEL_T  channel)
{
    HAL_MT_NAMCHABARWA_PKT_RX_PDMA_T           *ptr_rx_pdma = HAL_MT_NAMCHABARWA_PKT_GET_RX_PDMA_PTR(unit, channel);
    HAL_MT_NAMCHABARWA_PKT_RX_CB_T             *ptr_rx_cb = HAL_MT_NAMCHABARWA_PKT_GET_RX_CB_PTR(unit);
    UI32_T err_intr_status = 0;
    UI32_T data = 0;
    UI32_T err_type = 0;

    osal_mdc_readPciReg(unit,
        HAL_MT_NAMCHABARWA_PKT_PDMA_IRQ_PDMA_ABNORMAL_CH0_INTR + 0xC *  HAL_MT_NAMCHABARWA_PKT_RX_CHANNEL(channel),
        &err_intr_status, sizeof(UI32_T));
    if(0 == err_intr_status)
    {
        return (CLX_E_OK);
    }

    /* Set the error flag. */
    osal_takeSemaphore(&ptr_rx_pdma->sema, CLX_SEMAPHORE_WAIT_FOREVER);
    ptr_rx_pdma->err_flag = TRUE;
    osal_giveSemaphore(&ptr_rx_pdma->sema);

    //read err_type
    osal_mdc_readPciReg(unit, HAL_MT_NAMCHABARWA_PKT_GET_PDMA_WORD_RCH_REG(HAL_MT_NAMCHABARWA_PKT_PDMA_STA_PDMA_CH0_ERR_TYPE,channel), &err_type, sizeof(UI32_T));
    if(0 != (err_type & HAL_MT_NAMCHABARWA_PKT_PDMA_CHANNEL_RD_DESC_ERROR))
    {
        ptr_rx_cb->cnt.channel[channel].rd_desc_err++;
    }

    if(0 != (err_type & HAL_MT_NAMCHABARWA_PKT_PDMA_CHANNEL_WR_DESC_ERROR))
    {
        ptr_rx_cb->cnt.channel[channel].wr_desc_err++;
    }

    if(0 != (err_type & HAL_MT_NAMCHABARWA_PKT_PDMA_CHANNEL_RD_DATA_ERROR))
    {
        ptr_rx_cb->cnt.channel[channel].rd_data_err++;
    }

    if(0 != (err_type & HAL_MT_NAMCHABARWA_PKT_PDMA_CHANNEL_WR_DATA_ERROR))
    {
        ptr_rx_cb->cnt.channel[channel].wr_data_err++;
    }

    //read malform interrupt
    osal_mdc_readPciReg(unit, HAL_MT_NAMCHABARWA_PKT_PDMA_IRQ_PDMA_MALFORM_INTR, &data, sizeof(UI32_T));
    if(((0 != (data & HAL_MT_NAMCHABARWA_PKT_PDMA_P2H_RX_FIFO0_OVF)) && (channel == 0)) ||
        ((0 != (data & HAL_MT_NAMCHABARWA_PKT_PDMA_P2H_RX_FIFO1_OVF)) && (channel == 1)) ||
        ((0 != (data & HAL_MT_NAMCHABARWA_PKT_PDMA_P2H_RX_FIFO2_OVF)) && (channel == 2)) ||
        ((0 != (data & HAL_MT_NAMCHABARWA_PKT_PDMA_P2H_RX_FIFO3_OVF)) && (channel == 3)))
    {
        ptr_rx_cb->cnt.channel[channel].p2h_rx_fifo_ovf++;
    }

    osal_triggerEvent(HAL_MT_NAMCHABARWA_PKT_RCH_EVENT(unit, channel));

    return (CLX_E_OK);
}

/* FUNCTION NAME: _hal_mt_namchabarwa_pkt_handleErrorTask
 * PURPOSE:
 *      To invoke the corresponding handler for the L2 interrupts.
 * INPUT:
 *      ptr_argv        --  The unit ID
 * OUTPUT:
 *      None
 * RETURN:
 *      None
 * NOTES:
 *      None
 */
static void
_hal_mt_namchabarwa_pkt_handleErrorTask(
    void                    *ptr_argv)
{
    UI32_T                  unit = (UI32_T)((CLX_HUGE_T)ptr_argv);
    UI32_T                  channel = 0;

    osal_initRunThread();
    do
    {
        /* receive Error-ISR */
        osal_waitEvent(HAL_MT_NAMCHABARWA_PKT_ERR_EVENT(unit));
        if (CLX_E_OK != osal_isRunThread())
        {
            OSAL_PRINT(OSAL_DBG_COMMON,
                            "u=%u, err task destroyed\n", unit);
            break; /* deinit-thread */
        }

        for (channel = 0; channel < HAL_MT_NAMCHABARWA_PKT_RX_CHANNEL_LAST; channel++)
        {
            _hal_mt_namchabarwa_pkt_handleRxErrStat(unit, channel);
        }

        for (channel = 0; channel < HAL_MT_NAMCHABARWA_PKT_TX_CHANNEL_LAST; channel++)
        {
            _hal_mt_namchabarwa_pkt_handleTxErrStat(unit, channel);
        }
    } while (CLX_E_OK == osal_isRunThread());
    osal_exitRunThread();
}

/* FUNCTION NAME: _hal_mt_namchabarwa_pkt_handleTxDoneTask
 * PURPOSE:
 *      To handle the TX done interrupt for the specified TX channel.
 * INPUT:
 *      ptr_argv        --  The unit ID and channel ID
 * OUTPUT:
 *      None
 * RETURN:
 *      None
 * NOTES:
 *      None
 */
static void
_hal_mt_namchabarwa_pkt_handleTxDoneTask(
    void                    *ptr_argv)
{
    /* cookie or index */
    UI32_T                          unit    = ((HAL_MT_NAMCHABARWA_PKT_ISR_COOKIE_T *)ptr_argv)->unit;
    HAL_MT_NAMCHABARWA_PKT_TX_CHANNEL_T        channel = (HAL_MT_NAMCHABARWA_PKT_TX_CHANNEL_T)
                                              ((HAL_MT_NAMCHABARWA_PKT_ISR_COOKIE_T *)ptr_argv)->channel;
    /* control block */
    HAL_MT_NAMCHABARWA_PKT_TX_CB_T             *ptr_tx_cb = HAL_MT_NAMCHABARWA_PKT_GET_TX_CB_PTR(unit);
    HAL_MT_NAMCHABARWA_PKT_TX_PDMA_T           *ptr_tx_pdma = HAL_MT_NAMCHABARWA_PKT_GET_TX_PDMA_PTR(unit, channel);
    volatile HAL_MT_NAMCHABARWA_PKT_TX_GPD_T   *ptr_tx_gpd = NULL;
    UI32_T                          first_gpd_idx = 0; /* To record the first GPD */
    UI32_T                          loop_cnt = 0;
    CLX_IRQ_FLAGS_T                 irg_flags;
    unsigned long                   timeout  = 0;
    UI32_T                          bulk_pkt_cnt = 0;

    osal_initRunThread();
    do
    {
        /* receive Tx-Done-ISR */
        osal_waitEvent(HAL_MT_NAMCHABARWA_PKT_TCH_EVENT(unit, channel));
        if (CLX_E_OK != osal_isRunThread())
        {
            OSAL_PRINT(OSAL_DBG_TX,
                            "u=%u, txch=%u, tx done task destroyed\n", unit, channel);
            break; /* deinit-thread */
        }

        /* protect Tx PDMA
         * for sync-intr, the sema is locked by sendGpd
         */
        if (HAL_MT_NAMCHABARWA_PKT_TX_WAIT_SYNC_INTR != ptr_tx_cb->wait_mode)
        {
            osal_takeIsrLock(&ptr_tx_pdma->ring_lock, &irg_flags);
        }

        loop_cnt = ptr_tx_pdma->used_gpd_num;
        while (loop_cnt > 0)
        {
            ptr_tx_gpd = HAL_MT_NAMCHABARWA_PKT_GET_TX_GPD_PTR(unit, channel, ptr_tx_pdma->free_idx);
            osal_dma_invalidateCache((void *)ptr_tx_gpd, sizeof(HAL_MT_NAMCHABARWA_PKT_TX_GPD_T));

            /* If hwo=HW, it might be:
             * 1. err_flag=TRUE  -> HW breakdown -> enque and recover -> break
             * 2. err_flag=FALSE -> HW busy -> break
             */
            if (TRUE == ptr_tx_pdma->err_flag)
            {
                /* do error recover */
                first_gpd_idx = 0;
                if (CLX_E_OK == _hal_mt_namchabarwa_pkt_recoverTxPdma(unit, channel))
                {
                    ptr_tx_pdma->err_flag = FALSE;
                    ptr_tx_cb->cnt.channel[channel].err_recover++;
                }
                else
                {
                    OSAL_PRINT((OSAL_DBG_TX | OSAL_DBG_ERR),
                                    "u=%u, txch=%u, err recover failed\n",
                                    unit, channel);
                }

                /*unmask abnormal intr*/
                _hal_mt_namchabarwa_pkt_unmaskTxPdmaAbnormalIntrReg(unit, channel);
                break;
            }

            if (HAL_MT_NAMCHABARWA_PKT_TX_WAIT_ASYNC == ptr_tx_cb->wait_mode)
            {
                /* If hwo=SW and ch=0, record the head of sw gpd in bulk buf */
                if (1 == ptr_tx_gpd->interrupt)
                {
                    ptr_tx_gpd->interrupt = 0;
                    ptr_tx_pdma->pptr_sw_gpd_bulk[bulk_pkt_cnt]
                        = ptr_tx_pdma->pptr_sw_gpd_ring[first_gpd_idx];

                    bulk_pkt_cnt++;
                    ptr_tx_pdma->pptr_sw_gpd_ring[first_gpd_idx] = NULL;

                    /* next SW-GPD must be the head of another PKT->SW-GPD */
                    first_gpd_idx = ptr_tx_pdma->free_idx + 1;
                    first_gpd_idx %= ptr_tx_pdma->gpd_num;
                }
            }

            /* update Tx PDMA */
            ptr_tx_pdma->free_idx++;
            ptr_tx_pdma->free_idx %= ptr_tx_pdma->gpd_num;
            ptr_tx_pdma->used_gpd_num--;
            ptr_tx_pdma->free_gpd_num++;
            loop_cnt--;
        }

        /* let the netdev resume Tx */
        _hal_mt_namchabarwa_pkt_resumeAllIntf(unit);

        /* update ISR and counter */
        ptr_tx_cb->cnt.channel[channel].tx_done++;
        
        /*unmask normal intr*/
        _hal_mt_namchabarwa_pkt_clearIntr(unit, HAL_MT_NAMCHABARWA_PKT_TX_CHANNEL(channel));
        _hal_mt_namchabarwa_pkt_unmaskIntr(unit, HAL_MT_NAMCHABARWA_PKT_TX_CHANNEL(channel));

        if (HAL_MT_NAMCHABARWA_PKT_TX_WAIT_SYNC_INTR != ptr_tx_cb->wait_mode)
        {
            osal_giveIsrLock(&ptr_tx_pdma->ring_lock, &irg_flags);
        }
        else
        {
            osal_giveSemaphore(&ptr_tx_pdma->sync_intr_sema);
        }

        /* enque packet after releasing the spinlock */
        _hal_mt_namchabarwa_pkt_txEnQueueBulk(unit, channel, bulk_pkt_cnt);
        bulk_pkt_cnt = 0;

        /* prevent this task from executing too long */
        if (!(time_before(jiffies, timeout)))
        {
            schedule();
            timeout = jiffies + 1; /* continuously free tx descriptor for 1 tick */
        }

    } while (CLX_E_OK == osal_isRunThread());
    osal_exitRunThread();
}

/* FUNCTION NAME: _hal_mt_namchabarwa_pkt_handleRxDoneTask
 * PURPOSE:
 *      To handle the RX done interrupt for the specified RX channel.
 * INPUT:
 *      ptr_argv        --  The unit ID and channel ID
 * OUTPUT:
 *      None
 * RETURN:
 *      None
 * NOTES:
 *      None
 */
static void
_hal_mt_namchabarwa_pkt_handleRxDoneTask(
    void                    *ptr_argv)
{
    /* cookie or index */
    UI32_T                          unit    = ((HAL_MT_NAMCHABARWA_PKT_ISR_COOKIE_T *)ptr_argv)->unit;
    HAL_MT_NAMCHABARWA_PKT_RX_CHANNEL_T        channel = (HAL_MT_NAMCHABARWA_PKT_RX_CHANNEL_T)
                                              ((HAL_MT_NAMCHABARWA_PKT_ISR_COOKIE_T *)ptr_argv)->channel;

    /* control block */
    HAL_MT_NAMCHABARWA_PKT_RX_CB_T             *ptr_rx_cb = HAL_MT_NAMCHABARWA_PKT_GET_RX_CB_PTR(unit);
    HAL_MT_NAMCHABARWA_PKT_RX_PDMA_T           *ptr_rx_pdma = HAL_MT_NAMCHABARWA_PKT_GET_RX_PDMA_PTR(unit, channel);
    volatile HAL_MT_NAMCHABARWA_PKT_RX_GPD_T   *ptr_rx_gpd = NULL;

    HAL_MT_NAMCHABARWA_PKT_RX_SW_GPD_T         *ptr_sw_gpd = NULL;
    HAL_MT_NAMCHABARWA_PKT_RX_SW_GPD_T         *ptr_sw_first_gpd = NULL;
    UI32_T                          loop_cnt = 0;
    unsigned long                   timeout  = 0;
    void                            *ptr_virt_addr = NULL;
    CLX_ADDR_T                      phy_addr = 0;
    struct sk_buff                  *ptr_skb = NULL;

    osal_initRunThread();
    ptr_rx_pdma->pop_idx = _hal_mt_namchabarwa_pkt_getRxPopIdx(unit, channel);
    ptr_rx_pdma->work_idx = _hal_mt_namchabarwa_pkt_getRxWorkIdx(unit,channel);
    
    do
    {

        /* receive Rx-Done-ISR */
        osal_waitEvent(HAL_MT_NAMCHABARWA_PKT_RCH_EVENT(unit, channel));

        if (CLX_E_OK != osal_isRunThread())
        {
            OSAL_PRINT(OSAL_DBG_RX,
                            "u=%u, rxch=%u, rx done task destroyed\n", unit, channel);
            break; /* deinit-thread */
        }

        /* check if Rx-system is inited */
        if (0 == ptr_rx_cb->buf_len)
        {
            OSAL_PRINT((OSAL_DBG_RX | OSAL_DBG_ERR),
                            "u=%u, rxch=%u, rx gpd buf len=0\n",
                            unit, channel);
            continue;
        }
        

        /* protect Rx PDMA */
        osal_takeSemaphore(&ptr_rx_pdma->sema, CLX_SEMAPHORE_WAIT_FOREVER);

        for (loop_cnt = 0; loop_cnt < ptr_rx_pdma->gpd_num; loop_cnt++)
        {
            /* If hwo=HW, it might be:
             * 1. err_flag=TRUE  -> HW breakdown -> enque and recover -> break
             * 2. err_flag=FALSE -> HW busy -> break
             */
            if (TRUE == ptr_rx_pdma->err_flag)
            {
                OSAL_PRINT((OSAL_DBG_RX | OSAL_DBG_ERR),"unit:%d rxch:%d, abnormal_irq happen!!!!\n", unit, channel);

                // /* free the last incomplete Rx packet */
                // if ((NULL != ptr_sw_first_gpd) &&
                //     (NULL != ptr_sw_gpd))
                // {
                //     ptr_sw_gpd->ptr_next = NULL;
                //     ptr_sw_first_gpd->rx_complete = FALSE;
                //     _hal_mt_namchabarwa_pkt_rxEnQueue(unit, channel, ptr_sw_first_gpd);
                //     ptr_sw_first_gpd = NULL;
                // }

                /* do error recover */
                if (CLX_E_OK == _hal_mt_namchabarwa_pkt_recoverRxPdma(unit, channel))
                {
                    ptr_rx_pdma->err_flag = FALSE;
                    ptr_rx_cb->cnt.channel[channel].err_recover++;
                    OSAL_PRINT((OSAL_DBG_RX | OSAL_DBG_ERR),
                                    "u=%u, rxch=%u, err recover pdma success!\n",
                                    unit, channel);
                }
                else
                {
                    OSAL_PRINT((OSAL_DBG_RX | OSAL_DBG_ERR),
                                    "u=%u, rxch=%u, err recover failed\n",
                                    unit, channel);
                }

                /*unmask abnormal intr*/
                _hal_mt_namchabarwa_pkt_unmaskRxPdmaAbnormalIntrReg(unit, channel);
                break;
            }

            ptr_rx_gpd = HAL_MT_NAMCHABARWA_PKT_GET_RX_GPD_PTR(unit, channel, ptr_rx_pdma->pop_idx);

            if (ptr_rx_gpd->interrupt == 0)
            {
                /*unmask normal intr*/
                _hal_mt_namchabarwa_pkt_clearIntr(unit, channel);
                _hal_mt_namchabarwa_pkt_unmaskIntr(unit, channel);
                break;
            }

            ptr_virt_addr = ptr_rx_pdma->pptr_skb_ring[ptr_rx_pdma->pop_idx];
            ptr_skb = (struct sk_buff *)ptr_virt_addr;
            phy_addr = CLX_ADDR_32_TO_64(ptr_rx_gpd->d_addr_hi, ptr_rx_gpd->d_addr_lo);
            /* Move HW-GPD to SW-GPD and append to a link-list */
            if (1 == ptr_rx_gpd->sop)
            {
                ptr_sw_first_gpd = (HAL_MT_NAMCHABARWA_PKT_RX_SW_GPD_T *)osal_alloc(sizeof(HAL_MT_NAMCHABARWA_PKT_RX_SW_GPD_T));
                ptr_sw_gpd = ptr_sw_first_gpd;
                if (NULL != ptr_sw_gpd)
                {
                    memcpy(&ptr_sw_gpd->rx_gpd, (void *)ptr_rx_gpd, sizeof(HAL_MT_NAMCHABARWA_PKT_RX_GPD_T));
                }
                else
                {
                    ptr_rx_cb->cnt.no_memory++;
                    OSAL_PRINT((OSAL_DBG_RX | OSAL_DBG_ERR),
                                    "u=%u, rxch=%u, alloc 1st sw gpd failed, size=%zu\n",
                                    unit, channel, sizeof(HAL_MT_NAMCHABARWA_PKT_RX_SW_GPD_T));
                    break;
                }
                osal_skb_unmapDma(phy_addr, ptr_skb->len, DMA_FROM_DEVICE);
                /* get pph in skb*/
                ptr_sw_gpd->ptr_pph_l2 = (HAL_MT_NAMCHABARWA_PKT_PPH_L2_T *)((UI8_T *)ptr_skb->data + HAL_MT_NAMCHABARWA_PKT_EMAC_SZ);
            }
            else
            {
                ptr_sw_gpd->ptr_next = (HAL_MT_NAMCHABARWA_PKT_RX_SW_GPD_T *)osal_alloc(sizeof(HAL_MT_NAMCHABARWA_PKT_RX_SW_GPD_T));
                ptr_sw_gpd = ptr_sw_gpd->ptr_next;
                if (NULL != ptr_sw_gpd)
                {
                    memcpy(&ptr_sw_gpd->rx_gpd, (void *)ptr_rx_gpd, sizeof(HAL_MT_NAMCHABARWA_PKT_RX_GPD_T));
                }
                else
                {
                    ptr_rx_cb->cnt.no_memory++;
                    OSAL_PRINT((OSAL_DBG_RX | OSAL_DBG_ERR),
                                    "u=%u, rxch=%u, alloc mid sw gpd failed, size=%zu\n",
                                    unit, channel, sizeof(HAL_MT_NAMCHABARWA_PKT_RX_SW_GPD_T));
                    break;
                }
                
                osal_skb_unmapDma(phy_addr, ptr_skb->len, DMA_FROM_DEVICE);
            }

            ptr_sw_gpd->ptr_cookie = ptr_rx_pdma->pptr_skb_ring[ptr_rx_pdma->pop_idx];

            /* If hwo=SW and ch=*, re-alloc-buf and resume */
            while (CLX_E_OK != _hal_mt_namchabarwa_pkt_allocRxPayloadBuf(unit, channel, ptr_rx_pdma->pop_idx))
            {
                ptr_rx_cb->cnt.no_memory++;
                HAL_MT_NAMCHABARWA_PKT_ALLOC_MEM_RETRY_SLEEP();
            }

            /* Enque the SW-GPD to rxTask */
            if (HAL_MT_NAMCHABARWA_PKT_PDMA_CH_PKT_EOP == ptr_rx_gpd->eop)
            {
                ptr_sw_gpd->ptr_next = NULL;
                ptr_sw_first_gpd->rx_complete = TRUE;

                if (0 != (((OSAL_DBG_RX | OSAL_DBG_INFO)) & (verbosity)))
                {
                    ptr_sw_gpd = ptr_sw_first_gpd;
                    while(ptr_sw_gpd != NULL)
                    {
                        hal_mt_namchabarwa_pkt_showPdmaGpd(unit, (HAL_MT_NAMCHABARWA_PKT_GPD_T *)(&ptr_sw_gpd->rx_gpd));
                        ptr_sw_gpd = ptr_sw_gpd->ptr_next;
                    }
                }
                _hal_mt_namchabarwa_pkt_rxEnQueue(unit, channel, ptr_sw_first_gpd);
            }

            // ptr_rx_gpd->size = ptr_rx_cb->buf_len;
            // osal_dma_flushCache((void *)ptr_rx_gpd, sizeof(HAL_MT_NAMCHABARWA_PKT_RX_GPD_T));

            /* update Rx PDMA */
            ptr_rx_pdma->pop_idx++;
            ptr_rx_pdma->pop_idx %= ptr_rx_pdma->gpd_num;
        }

        if (loop_cnt != 0)
        {
            //update work_index
            ptr_rx_pdma->work_idx = (ptr_rx_pdma->work_idx + loop_cnt) % ptr_rx_pdma->gpd_num;
            _hal_mt_namchabarwa_pkt_setRxWorkIdx(unit,channel,ptr_rx_pdma->work_idx);
            //osal_printf("loop_cnt:%u,work_idx:%u,pop_idx:%u,true pop_idx:%u,gpd_num:%d \n",loop_cnt,ptr_rx_pdma->work_idx,ptr_rx_pdma->pop_idx,_hal_mt_namchabarwa_pkt_getRxPopIdx(unit, channel),ptr_rx_pdma->gpd_num);
        }

        osal_giveSemaphore(&ptr_rx_pdma->sema);

        /* update ISR and counter */
        ptr_rx_cb->cnt.channel[channel].rx_done++;

        /* prevent this task from executing too long */
        if (!(time_before(jiffies, timeout)))
        {
            schedule();
            timeout = jiffies + 1; /* continuously rx for 1 tick */
        }

    } while (CLX_E_OK == osal_isRunThread());
    osal_exitRunThread();
}

static void
_hal_mt_namchabarwa_pkt_net_dev_tx_callback(
    const UI32_T                unit,
    HAL_MT_NAMCHABARWA_PKT_TX_SW_GPD_T     *ptr_sw_gpd,
    struct sk_buff              *ptr_skb)
{
    CLX_ADDR_T                  phy_addr = 0;

    /* unmap dma */
    phy_addr = CLX_ADDR_32_TO_64(ptr_sw_gpd->tx_gpd.s_addr_hi, ptr_sw_gpd->tx_gpd.s_addr_lo);
    osal_skb_unmapDma(phy_addr, ptr_skb->len, DMA_TO_DEVICE);

    /* free skb */
    osal_skb_free(ptr_skb);

    /* free gpd */
    osal_free(ptr_sw_gpd);
}

/* FUNCTION NAME: hal_mt_namchabarwa_pkt_initTask
 * PURPOSE:
 *      To initialize the Task for packet module.
 * INPUT:
 *      unit            --  The unit ID
 * OUTPUT:
 *      None
 * RETURN:
 *      CLX_E_OK        --  Successfully dinitialize the control block.
 *      CLX_E_OTHERS    --  Initialize the control block failed.
 * NOTES:
 *      None
 */
CLX_ERROR_NO_T
hal_mt_namchabarwa_pkt_initTask(
    const UI32_T            unit,
    void                    *ptr_data)    
{
    CLX_ERROR_NO_T          rc = CLX_E_OK;
    HAL_MT_NAMCHABARWA_PKT_DRV_CB_T    *ptr_cb = HAL_MT_NAMCHABARWA_PKT_GET_DRV_CB_PTR(unit);
    HAL_MT_NAMCHABARWA_PKT_TX_CB_T     *ptr_tx_cb = HAL_MT_NAMCHABARWA_PKT_GET_TX_CB_PTR(unit);
    HAL_MT_NAMCHABARWA_PKT_RX_CB_T     *ptr_rx_cb = HAL_MT_NAMCHABARWA_PKT_GET_RX_CB_PTR(unit);
    UI32_T                  channel = 0;

    if (0 != (ptr_cb->init_flag & HAL_MT_NAMCHABARWA_PKT_INIT_TASK))
    {
        OSAL_PRINT(OSAL_DBG_ERR,
                        "u=%u, pkt task init failed, not inited\n", unit);
        return (rc);
    }

    /* Init handleErrorTask */
    rc = osal_createThread("ERROR", NETIF_PKT_PKT_ERROR_ISR_THREAD_STACK,
                           NETIF_PKT_PKT_ERROR_ISR_THREAD_PRI, _hal_mt_namchabarwa_pkt_handleErrorTask,
                           (void *)((CLX_HUGE_T)unit), &ptr_cb->err_task_id);

    /* Init handleTxDoneTask */
    for (channel = 0; ((channel < HAL_MT_NAMCHABARWA_PKT_TX_CHANNEL_LAST) && (CLX_E_OK == rc)); channel++)
    {
        ptr_tx_cb->isr_task_cookie[channel].unit    = unit;
        ptr_tx_cb->isr_task_cookie[channel].channel = channel;

        rc = osal_createThread("TX_ISR", NETIF_PKT_PKT_TX_ISR_THREAD_STACK,
                               NETIF_PKT_PKT_TX_ISR_THREAD_PRI, _hal_mt_namchabarwa_pkt_handleTxDoneTask,
                               (void *)&ptr_tx_cb->isr_task_cookie[channel],
                               &ptr_tx_cb->isr_task_id[channel]);
    }

    /* Init handleRxDoneTask */
    for (channel = 0; ((channel < HAL_MT_NAMCHABARWA_PKT_RX_CHANNEL_LAST) && (CLX_E_OK == rc)); channel++)
    {
        ptr_rx_cb->isr_task_cookie[channel].unit    = unit;
        ptr_rx_cb->isr_task_cookie[channel].channel = channel;

        rc = osal_createThread("RX_ISR", NETIF_PKT_PKT_RX_ISR_THREAD_STACK,
                               NETIF_PKT_PKT_RX_ISR_THREAD_PRI, _hal_mt_namchabarwa_pkt_handleRxDoneTask,
                               (void *)&ptr_rx_cb->isr_task_cookie[channel],
                               &ptr_rx_cb->isr_task_id[channel]);
    }

    /* Init txTask */
    if (HAL_MT_NAMCHABARWA_PKT_TX_WAIT_ASYNC == ptr_tx_cb->wait_mode)
    {
        ptr_tx_cb->running = TRUE;
    }

    ptr_cb->init_flag |= HAL_MT_NAMCHABARWA_PKT_INIT_TASK;

    OSAL_PRINT(OSAL_DBG_COMMON,
                    "u=%u, pkt task init done, init flag=0x%x\n", unit, ptr_cb->init_flag);

    /* For some specail case in warmboot, the netifs are not destroyed during sdk deinit
     * but stopped, here we need to resume them with the original carrier status
     */
    _hal_mt_namchabarwa_pkt_resumeAllIntf(unit);

    ptr_tx_cb->net_tx_allowed = TRUE;

    return (rc);
}

/* FUNCTION NAME: _hal_mt_namchabarwa_pkt_initTxPdma
 * PURPOSE:
 *      To initialize the TX PDMA.
 * INPUT:
 *      unit        -- The unit ID
 *      channel     -- The target Tx channel
 * OUTPUT:
 *      None
 * RETURN:
 *      CLX_E_OK    -- Successfully initialize the TX PDMA.
 * NOTES:
 *      None
 */
static CLX_ERROR_NO_T
_hal_mt_namchabarwa_pkt_initTxPdma(
    const UI32_T                    unit,
    const HAL_MT_NAMCHABARWA_PKT_TX_CHANNEL_T  channel)
{
    CLX_ERROR_NO_T                  rc = CLX_E_OK;
    HAL_MT_NAMCHABARWA_PKT_TX_CB_T             *ptr_tx_cb = HAL_MT_NAMCHABARWA_PKT_GET_TX_CB_PTR(unit);
    HAL_MT_NAMCHABARWA_PKT_TX_PDMA_T           *ptr_tx_pdma = HAL_MT_NAMCHABARWA_PKT_GET_TX_PDMA_PTR(unit, channel);
    CLX_IRQ_FLAGS_T                 irg_flags;

    /* Isr lock to protect Tx PDMA */
    osal_createIsrLock("TCH_LCK", &ptr_tx_pdma->ring_lock);

    if (HAL_MT_NAMCHABARWA_PKT_TX_WAIT_SYNC_INTR == ptr_tx_cb->wait_mode)
    {
        /* Sync semaphore to signal sendTxPacket */
        osal_createSemaphore("TCH_SYN", CLX_SEMAPHORE_SYNC, &ptr_tx_pdma->sync_intr_sema);
    }

    /* Reset Tx PDMA */
    osal_takeIsrLock(&ptr_tx_pdma->ring_lock, &irg_flags);

    ptr_tx_pdma->used_idx     = 0;
    ptr_tx_pdma->free_idx     = 0;
    ptr_tx_pdma->used_gpd_num = 0;
    ptr_tx_pdma->free_gpd_num = NETIF_PKT_PKT_TX_GPD_NUM;
    ptr_tx_pdma->gpd_num      = NETIF_PKT_PKT_TX_GPD_NUM;

    /* Prepare the HW-GPD ring */
    ptr_tx_pdma->ptr_gpd_start_addr = (HAL_MT_NAMCHABARWA_PKT_TX_GPD_T *)osal_dma_alloc(
        (ptr_tx_pdma->gpd_num + 1) * sizeof(HAL_MT_NAMCHABARWA_PKT_TX_GPD_T));

    if (NULL != ptr_tx_pdma->ptr_gpd_start_addr)
    {
        osal_memset(ptr_tx_pdma->ptr_gpd_start_addr, 0x0,
            (ptr_tx_pdma->gpd_num + 1) * sizeof(HAL_MT_NAMCHABARWA_PKT_TX_GPD_T));

        ptr_tx_pdma->ptr_gpd_align_start_addr = (HAL_MT_NAMCHABARWA_PKT_TX_GPD_T *)HAL_MT_NAMCHABARWA_PKT_PDMA_ALIGN_ADDR(
            (CLX_HUGE_T)ptr_tx_pdma->ptr_gpd_start_addr, sizeof(HAL_MT_NAMCHABARWA_PKT_TX_GPD_T));

        rc = _hal_mt_namchabarwa_pkt_initTxPdmaRing(unit, channel);
        if (CLX_E_OK == rc)
        {
            _hal_mt_namchabarwa_pkt_startTxChannelReg(unit, channel);
        }
    }
    else
    {
        ptr_tx_cb->cnt.no_memory++;
        rc = CLX_E_NO_MEMORY;
    }

    if (HAL_MT_NAMCHABARWA_PKT_TX_WAIT_ASYNC == ptr_tx_cb->wait_mode)
    {
        if (CLX_E_OK == rc)
        {
            /* Prepare the SW-GPD ring */
            ptr_tx_pdma->pptr_sw_gpd_ring = (HAL_MT_NAMCHABARWA_PKT_TX_SW_GPD_T **)osal_alloc(
                ptr_tx_pdma->gpd_num * sizeof(HAL_MT_NAMCHABARWA_PKT_TX_SW_GPD_T *));

            if (NULL != ptr_tx_pdma->pptr_sw_gpd_ring)
            {
                osal_memset(ptr_tx_pdma->pptr_sw_gpd_ring, 0x0,
                    ptr_tx_pdma->gpd_num * sizeof(HAL_MT_NAMCHABARWA_PKT_TX_SW_GPD_T *));
            }
            else
            {
                ptr_tx_cb->cnt.no_memory++;
                rc = CLX_E_NO_MEMORY;
            }

            /* a temp buffer to store the 1st sw gpd for each packet to be enque
             * we cannot enque packet before release a spinlock
             */
            ptr_tx_pdma->pptr_sw_gpd_bulk = (HAL_MT_NAMCHABARWA_PKT_TX_SW_GPD_T **)osal_alloc(
                ptr_tx_pdma->gpd_num * sizeof(HAL_MT_NAMCHABARWA_PKT_TX_SW_GPD_T *));

            if (NULL != ptr_tx_pdma->pptr_sw_gpd_bulk)
            {
                osal_memset(ptr_tx_pdma->pptr_sw_gpd_bulk, 0x0,
                    ptr_tx_pdma->gpd_num * sizeof(HAL_MT_NAMCHABARWA_PKT_TX_SW_GPD_T *));
            }
            else
            {
                ptr_tx_cb->cnt.no_memory++;
                rc = CLX_E_NO_MEMORY;
            }
        }
    }

    osal_giveIsrLock(&ptr_tx_pdma->ring_lock, &irg_flags);

    return (rc);
}

/* FUNCTION NAME: _hal_mt_namchabarwa_pkt_initRxPdma
 * PURPOSE:
 *      To initialize the RX PDMA.
 * INPUT:
 *      unit        -- The unit ID
 *      channel     -- The target Rx channel
 * OUTPUT:
 *      None
 * RETURN:
 *      CLX_E_OK    -- Successfully initialize the RX PDMA.
 * NOTES:
 *      None
 */
static CLX_ERROR_NO_T
_hal_mt_namchabarwa_pkt_initRxPdma(
    const UI32_T                    unit,
    const HAL_MT_NAMCHABARWA_PKT_RX_CHANNEL_T  channel)
{
    CLX_ERROR_NO_T                  rc = CLX_E_OK;
    HAL_MT_NAMCHABARWA_PKT_RX_CB_T             *ptr_rx_cb = HAL_MT_NAMCHABARWA_PKT_GET_RX_CB_PTR(unit);
    HAL_MT_NAMCHABARWA_PKT_RX_PDMA_T           *ptr_rx_pdma = HAL_MT_NAMCHABARWA_PKT_GET_RX_PDMA_PTR(unit, channel);
    CLX_ADDR_T                      phy_addr1 = 0;
    CLX_ADDR_T                      phy_addr2 = 0;

    /* Binary semaphore to protect Rx PDMA */
    osal_createSemaphore("RCH_LCK", CLX_SEMAPHORE_BINARY, &ptr_rx_pdma->sema);

    /* Reset Rx PDMA */
    osal_takeSemaphore(&ptr_rx_pdma->sema, CLX_SEMAPHORE_WAIT_FOREVER);
    ptr_rx_pdma->pop_idx = 0;
    ptr_rx_pdma->gpd_num = HAL_MT_NAMCHABARWA_PKT_RX_GPD_NUM;

    /* Prepare the HW-GPD ring */
    ptr_rx_pdma->ptr_gpd_start_addr = (HAL_MT_NAMCHABARWA_PKT_RX_GPD_T *)osal_dma_alloc(
        (ptr_rx_pdma->gpd_num + 1) * sizeof(HAL_MT_NAMCHABARWA_PKT_RX_GPD_T));

    if (NULL != ptr_rx_pdma->ptr_gpd_start_addr)
    {
        osal_memset(ptr_rx_pdma->ptr_gpd_start_addr, 0,
            (ptr_rx_pdma->gpd_num + 1) * sizeof(HAL_MT_NAMCHABARWA_PKT_RX_GPD_T));

        ptr_rx_pdma->ptr_gpd_align_start_addr = (HAL_MT_NAMCHABARWA_PKT_RX_GPD_T *)HAL_MT_NAMCHABARWA_PKT_PDMA_ALIGN_ADDR(
            (CLX_HUGE_T)ptr_rx_pdma->ptr_gpd_start_addr, sizeof(HAL_MT_NAMCHABARWA_PKT_RX_GPD_T));
        phy_addr1 = osal_dma_convertVirtToPhy(ptr_rx_pdma->ptr_gpd_start_addr);
        phy_addr2 = osal_dma_convertVirtToPhy(ptr_rx_pdma->ptr_gpd_align_start_addr);

        /* will initRxPdmaRingBuf and start RCH after setRxConfig */
        rc = _hal_mt_namchabarwa_pkt_initRxPdmaRing(unit, channel);
    }
    else
    {
        ptr_rx_cb->cnt.no_memory++;
        rc = CLX_E_NO_MEMORY;
    }

    if (CLX_E_OK == rc)
    {
        /* Prepare the SKB ring */
        ptr_rx_pdma->pptr_skb_ring = (struct sk_buff **)osal_alloc(
            ptr_rx_pdma->gpd_num * sizeof(struct sk_buff *));

        if (NULL != ptr_rx_pdma->pptr_skb_ring)
        {
            osal_memset(ptr_rx_pdma->pptr_skb_ring, 0x0,
                ptr_rx_pdma->gpd_num * sizeof(struct sk_buff *));
        }
        else
        {
            ptr_rx_cb->cnt.no_memory++;
            rc = CLX_E_NO_MEMORY;
        }
    }

    osal_giveSemaphore(&ptr_rx_pdma->sema);

    return (rc);
}

/* FUNCTION NAME: _hal_mt_namchabarwa_pkt_initPktCb
 * PURPOSE:
 *      To initialize the control block of Drv.
 * INPUT:
 *      unit            -- The unit ID
 * OUTPUT:
 *      None
 * RETURN:
 *      CLX_E_OK        -- Successfully initialize the control block.
 * NOTES:
 *      None
 */
static CLX_ERROR_NO_T
_hal_mt_namchabarwa_pkt_initPktCb(
    const UI32_T                unit)
{
    HAL_MT_NAMCHABARWA_PKT_DRV_CB_T        *ptr_cb = HAL_MT_NAMCHABARWA_PKT_GET_DRV_CB_PTR(unit);
    UI32_T                      idx = 0, vec = sizeof(_hal_mt_namchabarwa_pkt_intr_vec) / sizeof(HAL_MT_NAMCHABARWA_PKT_INTR_VEC_T);

    osal_memset(ptr_cb, 0x0, sizeof(HAL_MT_NAMCHABARWA_PKT_DRV_CB_T));

    /* Register PKT interrupt functions */
    osal_createIsrLock("ISR_LOCK", &ptr_cb->intr_lock);
    osal_mdc_registerIsr(unit, _osal_mdc_dma_intr_callback, (void *)((CLX_HUGE_T)unit));

    for (idx = 0; idx < vec; idx++)
    {
        osal_createEvent("ISR_EVENT", &_hal_mt_namchabarwa_pkt_intr_vec[idx].intr_event);
        osal_createEvent("ISR_ERR_EVENT", &_hal_mt_namchabarwa_pkt_err_intr_vec[idx].intr_event);
        ptr_cb->intr_bitmap |= (_hal_mt_namchabarwa_pkt_intr_vec[idx].intr_reg);
    }

    return (CLX_E_OK);
}

/* FUNCTION NAME: _hal_mt_namchabarwa_pkt_initPktTxCb
 * PURPOSE:
 *      To initialize the control block of Rx PDMA.
 * INPUT:
 *      unit            -- The unit ID
 * OUTPUT:
 *      None
 * RETURN:
 *      CLX_E_OK        -- Successfully initialize the control block.
 *      CLX_E_OTHERS    -- Configure failed.
 * NOTES:
 *      None
 */
static CLX_ERROR_NO_T
_hal_mt_namchabarwa_pkt_initPktTxCb(
    const UI32_T                unit)
{
    CLX_ERROR_NO_T              rc = CLX_E_OK;
    HAL_MT_NAMCHABARWA_PKT_TX_CB_T         *ptr_tx_cb = HAL_MT_NAMCHABARWA_PKT_GET_TX_CB_PTR(unit);
    HAL_MT_NAMCHABARWA_PKT_TX_CHANNEL_T    channel = 0;

    osal_memset(ptr_tx_cb, 0x0, sizeof(HAL_MT_NAMCHABARWA_PKT_TX_CB_T));

    ptr_tx_cb->wait_mode = HAL_MT_NAMCHABARWA_PKT_TX_WAIT_MODE;

    if (HAL_MT_NAMCHABARWA_PKT_TX_WAIT_ASYNC == ptr_tx_cb->wait_mode)
    {
        /* Sync semaphore to signal txTask */
        osal_createEvent("TX_SYNC", &ptr_tx_cb->sync_sema);

        /* Initialize Tx GPD-queue (of first SW-GPD) from handleTxDoneTask to txTask */
        ptr_tx_cb->sw_queue.len    = NETIF_PKT_PKT_TX_QUEUE_LEN;
        ptr_tx_cb->sw_queue.weight = 0;

        osal_createSemaphore("TX_QUE", CLX_SEMAPHORE_BINARY, &ptr_tx_cb->sw_queue.sema);
        osal_que_create(&ptr_tx_cb->sw_queue.que_id, ptr_tx_cb->sw_queue.len);
    }
    else if (HAL_MT_NAMCHABARWA_PKT_TX_WAIT_SYNC_POLL == ptr_tx_cb->wait_mode)
    {
        /* Disable TX done ISR. */
        for (channel = 0; channel < HAL_MT_NAMCHABARWA_PKT_TX_CHANNEL_LAST; channel++)
        {
            _hal_mt_namchabarwa_pkt_maskIntr(unit, HAL_MT_NAMCHABARWA_PKT_TX_CHANNEL(channel));
        }
    }

    /* Init Tx PDMA */
    for (channel = 0; ((channel < HAL_MT_NAMCHABARWA_PKT_TX_CHANNEL_LAST) && (CLX_E_OK == rc)); channel++)
    {
        _hal_mt_namchabarwa_pkt_stopTxChannelReg(unit, channel);
        _hal_mt_namchabarwa_pkt_resetTxChannelReg(unit, channel);
        rc = _hal_mt_namchabarwa_pkt_initTxPdma(unit, channel);
        _hal_mt_namchabarwa_pkt_swapTxEndianReg(unit, channel);
        _hal_mt_namchabarwa_pkt_startTxChannelReg(unit, channel);
    }

    return (rc);
}

/* FUNCTION NAME: _hal_mt_namchabarwa_pkt_initPktRxCb
 * PURPOSE:
 *      To initialize the control block of Rx PDMA.
 * INPUT:
 *      unit            -- The unit ID
 * OUTPUT:
 *      None
 * RETURN:
 *      CLX_E_OK        -- Successfully initialize the control block.
 *      CLX_E_OTHERS    -- Configure failed.
 * NOTES:
 *
 */
static CLX_ERROR_NO_T
_hal_mt_namchabarwa_pkt_initPktRxCb(
    const UI32_T                unit)
{
    CLX_ERROR_NO_T              rc = CLX_E_OK;
    HAL_MT_NAMCHABARWA_PKT_RX_CB_T         *ptr_rx_cb = HAL_MT_NAMCHABARWA_PKT_GET_RX_CB_PTR(unit);
    HAL_MT_NAMCHABARWA_PKT_RX_CHANNEL_T    channel = 0;
    UI32_T                      queue = 0;

    osal_memset(ptr_rx_cb, 0x0, sizeof(HAL_MT_NAMCHABARWA_PKT_RX_CB_T));

    ptr_rx_cb->sched_mode = NETIF_PKT_PKT_RX_SCHED_MODE;
    ptr_rx_cb->buf_len = HAL_MT_NAMCHABARWA_PKT_RX_MAX_LEN;

    /* Sync semaphore to signal rxTask */
    osal_createEvent("RX_SYNC", &ptr_rx_cb->sync_sema);

    /* Initialize Rx GPD-queue (of first SW-GPD) from handleRxDoneTask to rxTask */
    for (queue = 0; ((queue < HAL_MT_NAMCHABARWA_PKT_RX_QUEUE_NUM) && (CLX_E_OK == rc)); queue++)
    {
        ptr_rx_cb->sw_queue[queue].len    = NETIF_PKT_PKT_RX_QUEUE_LEN;
        ptr_rx_cb->sw_queue[queue].weight = NETIF_PKT_PKT_RX_QUEUE_WEIGHT;

        osal_createSemaphore("RX_QUE", CLX_SEMAPHORE_BINARY, &ptr_rx_cb->sw_queue[queue].sema);
        osal_que_create(&ptr_rx_cb->sw_queue[queue].que_id, ptr_rx_cb->sw_queue[queue].len);
    }

    /* Init Rx PDMA */
    for (channel = 0; ((channel < HAL_MT_NAMCHABARWA_PKT_RX_CHANNEL_LAST) && (CLX_E_OK == rc)); channel++)
    {
        _hal_mt_namchabarwa_pkt_stopRxChannelReg(unit, channel);
        _hal_mt_namchabarwa_pkt_resetRxChannelReg(unit, channel);
        rc = _hal_mt_namchabarwa_pkt_initRxPdma(unit, channel);
        _hal_mt_namchabarwa_pkt_swapRxEndianReg(unit, channel);
        //_hal_mt_namchabarwa_pkt_startRxChannelReg(unit, channel);
    }

    return (rc);
}

/* FUNCTION NAME: _hal_mt_namchabarwa_pkt_initL1Isr
 * PURPOSE:
 *      To initialize the PDMA L1 ISR configuration.
 * INPUT:
 *      unit            -- The unit ID
 * OUTPUT:
 *      None
 * RETURN:
 *      CLX_E_OK        -- Successfully initialize the L1 ISR.
 *      CLX_E_OTHERS    -- Configure failed.
 * NOTES:
 *      None
 */
static CLX_ERROR_NO_T
_hal_mt_namchabarwa_pkt_initL1Isr(
    const UI32_T            unit)
{
    UI32_T                          idx = 0, vec = sizeof(_hal_mt_namchabarwa_pkt_intr_vec) / sizeof(HAL_MT_NAMCHABARWA_PKT_INTR_VEC_T);
    CLX_IRQ_FLAGS_T                 irq_flag = 0;
    UI32_T                          intr_unmask = 0x0;
    HAL_MT_NAMCHABARWA_PKT_DRV_CB_T *ptr_cb = HAL_MT_NAMCHABARWA_PKT_GET_DRV_CB_PTR(unit);

    for (idx = 0; idx < vec; idx++)
    {
        _hal_mt_namchabarwa_pkt_unmaskIntr(unit, idx);
    }

    osal_takeIsrLock(&ptr_cb->intr_lock, &irq_flag);
    osal_mdc_writePciReg(unit, 
            HAL_MT_NAMCHABARWA_PKT_PDMA_CFG_PDMA2PCIE_INTR_MASK_ALL, 
            &intr_unmask, sizeof(UI32_T));
    osal_giveIsrLock(&ptr_cb->intr_lock, &irq_flag);

    return (CLX_E_OK);
}

static CLX_ERROR_NO_T
_hal_mt_namchabarwa_pkt_addProfToList(
    HAL_MT_NAMCHABARWA_PKT_NETIF_PROFILE_T         *ptr_new_profile,
    HAL_MT_NAMCHABARWA_PKT_PROFILE_NODE_T          **pptr_profile_list)
{
    HAL_MT_NAMCHABARWA_PKT_PROFILE_NODE_T      *ptr_new_prof_node;
    HAL_MT_NAMCHABARWA_PKT_PROFILE_NODE_T      *ptr_curr_node, *ptr_prev_node;

    ptr_new_prof_node = osal_alloc(sizeof(HAL_MT_NAMCHABARWA_PKT_PROFILE_NODE_T));
    ptr_new_prof_node->ptr_profile = ptr_new_profile;

    /* Create the 1st node in the interface profile list */
    if (NULL == *pptr_profile_list)
    {
        OSAL_PRINT(OSAL_DBG_PROFILE,
                        "prof list empty\n");
        *pptr_profile_list = ptr_new_prof_node;
        ptr_new_prof_node->ptr_next_node = NULL;
    }
    else
    {
        ptr_prev_node = *pptr_profile_list;
        ptr_curr_node = *pptr_profile_list;

        while (ptr_curr_node != NULL)
        {
            if (ptr_curr_node->ptr_profile->priority <= ptr_new_profile->priority)
            {
                OSAL_PRINT(OSAL_DBG_PROFILE,
                                "find prof id=%d (%s) higher priority=%d, search next\n",
                                ptr_curr_node->ptr_profile->id,
                                ptr_curr_node->ptr_profile->name,
                                ptr_curr_node->ptr_profile->priority);
                /* Search the next node */
                ptr_prev_node = ptr_curr_node;
                ptr_curr_node = ptr_curr_node->ptr_next_node;
            }
            else
            {
                /* Insert intermediate node */
                ptr_new_prof_node->ptr_next_node = ptr_curr_node;
                OSAL_PRINT(OSAL_DBG_PROFILE,
                                "insert prof id=%d (%s) before prof id=%d (%s) (priority=%d >= %d)\n",
                                ptr_new_prof_node->ptr_profile->id,
                                ptr_new_prof_node->ptr_profile->name,
                                ptr_curr_node->ptr_profile->id,
                                ptr_curr_node->ptr_profile->name,
                                ptr_new_prof_node->ptr_profile->priority,
                                ptr_curr_node->ptr_profile->priority);

                if (ptr_prev_node == ptr_curr_node)
                {
                    /* There is no previous node: change the root */
                    *pptr_profile_list = ptr_new_prof_node;
                    OSAL_PRINT(OSAL_DBG_PROFILE,
                                    "insert prof id=%d (%s) to head (priority=%d)\n",
                                    ptr_new_prof_node->ptr_profile->id,
                                    ptr_new_prof_node->ptr_profile->name,
                                    ptr_new_prof_node->ptr_profile->priority);
                }
                else
                {
                    ptr_prev_node->ptr_next_node = ptr_new_prof_node;
                    OSAL_PRINT(OSAL_DBG_PROFILE,
                                    "insert prof id=%d (%s) after prof id=%d (%s) (priority=%d <= %d)\n",
                                    ptr_new_prof_node->ptr_profile->id,
                                    ptr_new_prof_node->ptr_profile->name,
                                    ptr_prev_node->ptr_profile->id,
                                    ptr_prev_node->ptr_profile->name,
                                    ptr_new_prof_node->ptr_profile->priority,
                                    ptr_prev_node->ptr_profile->priority);
                }

                return (CLX_E_OK);
            }
        }

        /* Insert node to the tail of list */
        ptr_prev_node->ptr_next_node = ptr_new_prof_node;
        ptr_new_prof_node->ptr_next_node = NULL;
        OSAL_PRINT(OSAL_DBG_PROFILE,
                        "insert prof id=%d (%s) to tail, after prof id=%d (%s) (priority=%d <= %d)\n",
                        ptr_new_prof_node->ptr_profile->id,
                        ptr_new_prof_node->ptr_profile->name,
                        ptr_prev_node->ptr_profile->id,
                        ptr_prev_node->ptr_profile->name,
                        ptr_new_prof_node->ptr_profile->priority,
                        ptr_prev_node->ptr_profile->priority);
    }

    return (CLX_E_OK);
}

static CLX_ERROR_NO_T
_hal_mt_namchabarwa_pkt_addProfToAllIntf(
    HAL_MT_NAMCHABARWA_PKT_NETIF_PROFILE_T         *ptr_new_profile)
{
    UI32_T                              port;
    HAL_MT_NAMCHABARWA_PKT_NETIF_PORT_DB_T         *ptr_port_db;

    for (port = 0; port < HAL_MT_NAMCHABARWA_PKT_MAX_PORT_NUM; port++)
    {
        ptr_port_db = HAL_MT_NAMCHABARWA_PKT_GET_PORT_DB(port);
        /* Shall we check if the interface is ever created on the port?? */
        /* if (NULL != ptr_port_db->ptr_net_dev) */
        if (1)
        {
            _hal_mt_namchabarwa_pkt_addProfToList(ptr_new_profile, &ptr_port_db->ptr_profile_list);
        }
    }

    return (CLX_E_OK);
}

static HAL_MT_NAMCHABARWA_PKT_NETIF_PROFILE_T *
_hal_mt_namchabarwa_pkt_delProfFromListById(
    const UI32_T                            id,
    HAL_MT_NAMCHABARWA_PKT_PROFILE_NODE_T              **pptr_profile_list)
{
    HAL_MT_NAMCHABARWA_PKT_PROFILE_NODE_T      *ptr_temp_node;
    HAL_MT_NAMCHABARWA_PKT_PROFILE_NODE_T      *ptr_curr_node, *ptr_prev_node;
    HAL_MT_NAMCHABARWA_PKT_NETIF_PROFILE_T     *ptr_profile = NULL;;

    if (NULL != *pptr_profile_list)
    {
        /* Check the 1st node */
        if (id == (*pptr_profile_list)->ptr_profile->id)
        {
            ptr_profile = (*pptr_profile_list)->ptr_profile;
            ptr_temp_node = (*pptr_profile_list);
            (*pptr_profile_list) = ptr_temp_node->ptr_next_node;

            if (NULL != ptr_temp_node->ptr_next_node)
            {
                OSAL_PRINT(OSAL_DBG_PROFILE,
                                "choose prof id=%d (%s) as new head\n",
                                ptr_temp_node->ptr_next_node->ptr_profile->id,
                                ptr_temp_node->ptr_next_node->ptr_profile->name);
            }
            else
            {
                OSAL_PRINT(OSAL_DBG_PROFILE,
                                "prof list is empty\n");
            }


            osal_free(ptr_temp_node);
        }
        else
        {
            ptr_prev_node = *pptr_profile_list;
            ptr_curr_node = ptr_prev_node->ptr_next_node;

            while (NULL != ptr_curr_node)
            {
                if (id != ptr_curr_node->ptr_profile->id)
                {
                    ptr_prev_node = ptr_curr_node;
                    ptr_curr_node = ptr_curr_node->ptr_next_node;
                }
                else
                {
                    OSAL_PRINT(OSAL_DBG_PROFILE,
                                    "find prof id=%d, free done\n", id);

                    ptr_profile = ptr_curr_node->ptr_profile;
                    ptr_prev_node->ptr_next_node = ptr_curr_node->ptr_next_node;
                    osal_free(ptr_curr_node);
                    break;
                }
            }
        }
    }

    if (NULL == ptr_profile)
    {
        OSAL_PRINT((OSAL_DBG_PROFILE | OSAL_DBG_ERR),
                        "find prof failed, id=%d\n", id);
    }

    return (ptr_profile);
}


static CLX_ERROR_NO_T
_hal_mt_namchabarwa_pkt_delProfFromAllIntfById(
    const UI32_T                        id)
{
    UI32_T                              port;
    HAL_MT_NAMCHABARWA_PKT_NETIF_PORT_DB_T         *ptr_port_db;

    for (port = 0; port < HAL_MT_NAMCHABARWA_PKT_MAX_PORT_NUM; port++)
    {
        ptr_port_db = HAL_MT_NAMCHABARWA_PKT_GET_PORT_DB(port);
        /* Shall we check if the interface is ever created on the port?? */
        /* if (NULL != ptr_port_db->ptr_net_dev) */
        if (1)
        {
            _hal_mt_namchabarwa_pkt_delProfFromListById(id, &ptr_port_db->ptr_profile_list);
        }
    }
    return (CLX_E_OK);
}

static CLX_ERROR_NO_T
_hal_mt_namchabarwa_pkt_allocProfEntry(
    HAL_MT_NAMCHABARWA_PKT_NETIF_PROFILE_T         *ptr_profile)
{
    UI32_T          idx;

    for (idx=0; idx<HAL_MT_NAMCHABARWA_PKT_NET_PROFILE_NUM_MAX; idx++)
    {
        if (NULL == _ptr_hal_mt_namchabarwa_pkt_profile_entry[idx])
        {
            OSAL_PRINT(OSAL_DBG_PROFILE,
                            "alloc prof entry failed, id=%d\n", idx);
            _ptr_hal_mt_namchabarwa_pkt_profile_entry[idx] = ptr_profile;
            ptr_profile->id = idx;
            return (CLX_E_OK);
        }
    }
    return (CLX_E_TABLE_FULL);
}

static HAL_MT_NAMCHABARWA_PKT_NETIF_PROFILE_T  *
_hal_mt_namchabarwa_pkt_freeProfEntry(
    const UI32_T                 id)
{
    HAL_MT_NAMCHABARWA_PKT_NETIF_PROFILE_T         *ptr_profile = NULL;

    if (id < HAL_MT_NAMCHABARWA_PKT_NET_PROFILE_NUM_MAX)
    {
        ptr_profile = _ptr_hal_mt_namchabarwa_pkt_profile_entry[id];
        _ptr_hal_mt_namchabarwa_pkt_profile_entry[id] = NULL;
    }

    return (ptr_profile);
}

static CLX_ERROR_NO_T
_hal_mt_namchabarwa_pkt_destroyAllIntf(
    const UI32_T                        unit)
{
    HAL_MT_NAMCHABARWA_PKT_NETIF_PORT_DB_T         *ptr_port_db;
    UI32_T                              port = 0;

    /* Unregister net devices by id, although the "id" is now relavent to "port" we still perform a search */
    for (port = 0; port < HAL_MT_NAMCHABARWA_PKT_MAX_PORT_NUM; port++)
    {
        ptr_port_db = HAL_MT_NAMCHABARWA_PKT_GET_PORT_DB(port);
        if (NULL != ptr_port_db->ptr_net_dev)       /* valid intf */
        {
            OSAL_PRINT(OSAL_DBG_INTF,
                            "u=%u, find intf %s (id=%d) on phy port=%d, destroy done\n",
                            unit,
                            ptr_port_db->meta.name,
                            ptr_port_db->meta.port,
                            ptr_port_db->meta.port);

            netif_tx_disable(ptr_port_db->ptr_net_dev);
            unregister_netdev(ptr_port_db->ptr_net_dev);
            free_netdev(ptr_port_db->ptr_net_dev);

            /* Don't need to remove profiles on this port.
             * In fact, the profile is binding to "port" not "intf".
             */
            /* _hal_mt_namchabarwa_pkt_destroyProfList(ptr_port_db->ptr_profile_list); */

            osal_memset(ptr_port_db, 0x0, sizeof(HAL_MT_NAMCHABARWA_PKT_NETIF_PORT_DB_T));
        }
    }

    return (CLX_E_OK);
}

static CLX_ERROR_NO_T
_hal_mt_namchabarwa_pkt_delProfListOnAllIntf(
    const UI32_T                        unit)
{
    HAL_MT_NAMCHABARWA_PKT_NETIF_PORT_DB_T         *ptr_port_db;
    UI32_T                              port = 0;
    HAL_MT_NAMCHABARWA_PKT_PROFILE_NODE_T          *ptr_curr_node, *ptr_next_node;

    /* Unregister net devices by id, although the "id" is now relavent to "port" we still perform a search */
    for (port = 0; port < HAL_MT_NAMCHABARWA_PKT_MAX_PORT_NUM; port++)
    {
        ptr_port_db = HAL_MT_NAMCHABARWA_PKT_GET_PORT_DB(port);
        if (NULL != ptr_port_db->ptr_profile_list)       /* valid intf */
        {
            ptr_curr_node = ptr_port_db->ptr_profile_list;
            while (NULL != ptr_curr_node)
            {
                OSAL_PRINT(OSAL_DBG_PROFILE,
                                "u=%u, del prof id=%d on phy port=%d\n",
                                unit, ptr_curr_node->ptr_profile->id, port);

                ptr_next_node = ptr_curr_node->ptr_next_node;
                osal_free(ptr_curr_node);
                ptr_curr_node = ptr_next_node;
            }
        }
    }

    return (CLX_E_OK);
}

static CLX_ERROR_NO_T
_hal_mt_namchabarwa_pkt_destroyAllProfile(
    const UI32_T                        unit)
{
    HAL_MT_NAMCHABARWA_PKT_NETIF_PROFILE_T         *ptr_profile;
    UI32_T                              prof_id;

    _hal_mt_namchabarwa_pkt_delProfListOnAllIntf(unit);

    for (prof_id=0; prof_id<CLX_NETIF_PROFILE_NUM_MAX; prof_id++)
    {
        ptr_profile = _hal_mt_namchabarwa_pkt_freeProfEntry(prof_id);
        if (NULL != ptr_profile)
        {
            OSAL_PRINT(OSAL_DBG_PROFILE,
                            "u=%u, destroy prof id=%d, name=%s, priority=%d, flag=0x%x\n",
                            unit,
                            ptr_profile->id,
                            ptr_profile->name,
                            ptr_profile->priority,
                            ptr_profile->flags);
            osal_free(ptr_profile);
        }
    }

    return (CLX_E_OK);
}

/* FUNCTION NAME: hal_mt_namchabarwa_pkt_initPktDrv
 * PURPOSE:
 *      To invoke the functions to initialize the control block for each
 *      PDMA subsystem.
 * INPUT:
 *      unit            --  The unit ID
 * OUTPUT:
 *      None
 * RETURN:
 *      CLX_E_OK        --  Successfully initialize the control blocks.
 *      CLX_E_OTHERS    --  Initialize the control blocks failed.
 * NOTES:
 *      None
 */
CLX_ERROR_NO_T
hal_mt_namchabarwa_pkt_initPktDrv(
    const UI32_T            unit,
    void                    *ptr_data)
{
    CLX_ERROR_NO_T          rc = CLX_E_OK;
    UI32_T                  channel = 0;
    UI32_T                  intr_mask_mode = 0x1;
    UI32_T                  mask_intr = 0xffffffff;
    UI32_T                  clear_intr = 0xffffffff;
    HAL_MT_NAMCHABARWA_PKT_DRV_CB_T    *ptr_cb = HAL_MT_NAMCHABARWA_PKT_GET_DRV_CB_PTR(unit);

    /* Since the users may kill SDK application without a de-init flow,
     * we help to detect if NETIF is ever init before, and perform deinit.
     * (Because the users cannot perform Task init bypassing Drv init, this
     *  check is required only in here)
     */
    if (0 != (ptr_cb->init_flag & HAL_MT_NAMCHABARWA_PKT_INIT_DRV))
    {
        OSAL_PRINT(OSAL_DBG_ERR,
                        "u=%u, init pkt drv failed, inited\n", unit);

        OSAL_PRINT(OSAL_DBG_ERR,
                        "u=%u, stop rx pkt\n", unit);
        _hal_mt_namchabarwa_pkt_rxStop(unit);

        OSAL_PRINT(OSAL_DBG_ERR,
                        "u=%u, stop all intf\n", unit);
        _hal_mt_namchabarwa_pkt_stopAllIntf(unit);

        OSAL_PRINT(OSAL_DBG_ERR,
                        "u=%u, deinit pkt task\n", unit);

        hal_mt_namchabarwa_pkt_deinitTask(unit, NULL);

        OSAL_PRINT(OSAL_DBG_ERR,
                        "u=%u, deinit pkt drv\n", unit);
        hal_mt_namchabarwa_pkt_deinitPktDrv(unit, NULL);

        OSAL_PRINT(OSAL_DBG_ERR,
                        "u=%u, destroy all prof\n", unit);
        _hal_mt_namchabarwa_pkt_destroyAllProfile(unit);

        OSAL_PRINT(OSAL_DBG_ERR,
                        "u=%u, destroy all netlink\n", unit);
        netif_nl_destroyAllNetlink(unit);

        OSAL_PRINT(OSAL_DBG_ERR,
                        "u=%u, destroy all intf\n", unit);
        _hal_mt_namchabarwa_pkt_destroyAllIntf(unit);
    }

    /* [cold-boot] 1. stop DMA channel
     *             2. disable/mask/clear the interrupt status.
     */
    osal_mdc_writePciReg(unit,
        HAL_MT_NAMCHABARWA_PKT_GET_MMIO(HAL_MT_NAMCHABARWA_PKT_PDMA_CFG_PDMA2PCIE_INTR_MASK),
        &mask_intr, sizeof(UI32_T));

    osal_mdc_writePciReg(unit,
        HAL_MT_NAMCHABARWA_PKT_GET_MMIO(HAL_MT_NAMCHABARWA_PKT_PDMA_CFW_PDMA2PCIE_INTR_CLR),
        &clear_intr, sizeof(UI32_T));

    osal_mdc_writePciReg(unit,
        HAL_MT_NAMCHABARWA_PKT_GET_MMIO(HAL_MT_NAMCHABARWA_PKT_PDMA_CFG_PDMA2PCIE_INTR_MASK_MODE),
        &intr_mask_mode, sizeof(UI32_T));

    for (channel = 0; channel < HAL_MT_NAMCHABARWA_PKT_TX_CHANNEL_LAST; channel++)
    {
        _hal_mt_namchabarwa_pkt_stopTxChannelReg(unit, channel);
    }

    for (channel = 0; channel < HAL_MT_NAMCHABARWA_PKT_RX_CHANNEL_LAST; channel++)
    {
        _hal_mt_namchabarwa_pkt_stopRxChannelReg(unit, channel);
    }

    rc = _hal_mt_namchabarwa_pkt_initPktCb(unit);
    if (CLX_E_OK == rc)
    {
        rc = _hal_mt_namchabarwa_pkt_initPktTxCb(unit);
    }
    if (CLX_E_OK == rc)
    {
        rc = _hal_mt_namchabarwa_pkt_initPktRxCb(unit);
    }
    if (CLX_E_OK == rc)
    {
        rc = _hal_mt_namchabarwa_pkt_initL1Isr(unit);
    }

    if (CLX_E_OK == rc)
    {
        /* Set the flag to record init state */
        ptr_cb->init_flag |= HAL_MT_NAMCHABARWA_PKT_INIT_DRV;

        OSAL_PRINT(OSAL_DBG_RX,
                        "u=%u, pkt drv init done, init flag=0x%x\n", unit, ptr_cb->init_flag);
    }
    else
    {
        OSAL_PRINT(OSAL_DBG_RX,
                        "u=%u, pkt drv init faild, rc=%d\n", unit, rc);
    }

    return (rc);
}

/* ----------------------------------------------------------------------------------- Init: I/O */
CLX_ERROR_NO_T
hal_mt_namchabarwa_pkt_getNetDev(
    const UI32_T                unit,
    const UI32_T                port,
    struct net_device           **pptr_net_dev)
{
    *pptr_net_dev = HAL_MT_NAMCHABARWA_PKT_GET_PORT_NETDEV(port);

    return (CLX_E_OK);
}

CLX_ERROR_NO_T
hal_mt_namchabarwa_pkt_prepareGpd(
    const UI32_T                unit,
    const CLX_ADDR_T            phy_addr,
    const UI32_T                len,
    const UI32_T                port,
    HAL_MT_NAMCHABARWA_PKT_TX_SW_GPD_T  *ptr_sw_gpd,
    HAL_MT_NAMCHABARWA_PKT_PPH_L2_T    *ptr_pph)
{
    /* fill up tx_gpd */
    ptr_sw_gpd->tx_gpd.s_addr_hi              = CLX_ADDR_64_HI(phy_addr);
    ptr_sw_gpd->tx_gpd.s_addr_lo              = CLX_ADDR_64_LOW(phy_addr);
    ptr_sw_gpd->tx_gpd.size                   = len;
    ptr_sw_gpd->tx_gpd.interrupt                     = 0;
    ptr_sw_gpd->tx_gpd.sop                           = 1; 
    ptr_sw_gpd->tx_gpd.eop                           = 1;
    ptr_sw_gpd->tx_gpd.sinc                          = 1;

    /* fill up pp header */
    ptr_pph->skip_ipp             = 1;
    ptr_pph->skip_epp             = 1;
    ptr_pph->color                = 0;   /* Green */
    ptr_pph->tc                   = 7;   /* Max tc */
    ptr_pph->src_idx              = 0;

    HAL_MT_NAMCHABARWA_PKT_PPH_SET_DST_IDX(ptr_pph, port);

    // TODO: fill up pph other fields

    return (CLX_E_OK);
}

/* ----------------------------------------------------------------------------------- Init: net_dev_ops */
static int
_hal_mt_namchabarwa_pkt_net_dev_init(
    struct net_device           *ptr_net_dev)
{
    return 0;
}

static int
_hal_mt_namchabarwa_pkt_net_dev_open(
    struct net_device           *ptr_net_dev)
{
    netif_start_queue(ptr_net_dev);

    return 0;
}

static int
_hal_mt_namchabarwa_pkt_net_dev_stop(
    struct net_device           *ptr_net_dev)
{
    netif_stop_queue(ptr_net_dev);
    return 0;
}

static int
_hal_mt_namchabarwa_pkt_net_dev_ioctl(
    struct net_device           *ptr_net_dev,
    struct ifreq                *ptr_ifreq,
    int                         cmd)
{
    return 0;
}

static netdev_tx_t
_hal_mt_namchabarwa_pkt_net_dev_tx(
    struct sk_buff              *ptr_skb,
    struct net_device           *ptr_net_dev)
{
    struct net_device_priv      *ptr_priv = netdev_priv(ptr_net_dev);
    HAL_MT_NAMCHABARWA_PKT_TX_CB_T         *ptr_tx_cb;
    /* chip meta */
    unsigned int                unit;
    unsigned int                channel        = 0;
    HAL_MT_NAMCHABARWA_PKT_TX_SW_GPD_T     *ptr_sw_gpd    = NULL;
    void                        *ptr_virt_addr = NULL;
    CLX_ADDR_T                  phy_addr       = 0x0;
    struct sk_buff              *ptr_new_skb = NULL;

    if (NULL == ptr_priv)
    {
        /* in case that the netdev has been freed/reset somewhere */
        OSAL_PRINT(OSAL_DBG_ERR, "get netdev_priv failed\n");
        return -EFAULT;
    }

    /* check skb */
    if (NULL == ptr_skb)
    {
        ptr_priv->stats.tx_errors++;
        return -EFAULT;
    }

    unit = ptr_priv->unit;

    ptr_tx_cb = HAL_MT_NAMCHABARWA_PKT_GET_TX_CB_PTR(unit);

    /* for warm de-init procedure, if any net intf not destroyed, it is possible
     * that kernel still has packets to send causing segmentation fault
     */
    if (FALSE == ptr_tx_cb->net_tx_allowed) {
        OSAL_PRINT(OSAL_DBG_ERR, "net tx during sdk de-init\n");
        ptr_priv->stats.tx_dropped++;
        osal_skb_free(ptr_skb);
        return NETDEV_TX_OK;
    }

    /* alloc new skb*/
    ptr_new_skb = osal_skb_alloc(HAL_MT_NAMCHABARWA_PKT_PDMA_HDR_SZ + ptr_skb->len + ETH_FCS_LEN);
    if (NULL == ptr_new_skb)
    {
        return (CLX_E_NO_MEMORY);
    }

    /* pad to 60-bytes if skb_len < 60, see: eth_skb_pad(skb) */
    if (ptr_skb->len < ETH_ZLEN)
    {
        skb_pad(ptr_skb, ETH_ZLEN - ptr_skb->len);
        skb_set_tail_pointer(ptr_skb, ETH_ZLEN);
        ptr_skb->len = ETH_ZLEN;
    }

    /* pad 4-bytes for chip-crc */
    skb_pad(ptr_skb, ETH_FCS_LEN);
    skb_set_tail_pointer(ptr_skb, ETH_FCS_LEN);
    ptr_skb->len += ETH_FCS_LEN;

    memcpy(&(((UI8_T *)ptr_new_skb->data)[HAL_MT_NAMCHABARWA_PKT_PDMA_HDR_SZ]),
                           ptr_skb->data, ptr_skb->len);
    osal_skb_free(ptr_skb);
    ptr_skb = ptr_new_skb;
    ptr_virt_addr = ptr_skb->data;
    if (0 != (((OSAL_DBG_TX | OSAL_DBG_INFO)) & (verbosity)))
    {
        _hal_mt_namchabarwa_pkt_print_payload((UI8_T *)ptr_skb->data, ptr_skb->len);
    }

    /* alloc gpd */
    ptr_sw_gpd = osal_alloc(sizeof(HAL_MT_NAMCHABARWA_PKT_TX_SW_GPD_T));
    if (NULL == ptr_sw_gpd)
    {
        ptr_priv->stats.tx_errors++;
        osal_skb_free(ptr_skb);
    }
    else
    {
        /* map skb to dma */
        phy_addr = osal_skb_mapDma(ptr_skb, DMA_TO_DEVICE);
        if (0x0 == phy_addr)
        {
            OSAL_PRINT(OSAL_DBG_ERR, "u=%u, txch=%u, skb dma map err\n",
                            unit, channel);
            ptr_priv->stats.tx_errors++;
            osal_skb_free(ptr_skb);
            osal_free(ptr_sw_gpd);
        }
        else
        {
            /* trans skb to gpd */
            memset(ptr_sw_gpd, 0x0, sizeof(HAL_MT_NAMCHABARWA_PKT_TX_SW_GPD_T));
            ptr_sw_gpd->callback   = (void *)_hal_mt_namchabarwa_pkt_net_dev_tx_callback;
            ptr_sw_gpd->ptr_cookie = (void *)ptr_skb;
            ptr_sw_gpd->gpd_num    = 1;
            ptr_sw_gpd->ptr_next   = NULL;
            ptr_sw_gpd->channel    = channel;

            /* prepare gpd */
            hal_mt_namchabarwa_pkt_prepareGpd(unit, phy_addr, ptr_skb->len, ptr_priv->port, ptr_sw_gpd, 
                (HAL_MT_NAMCHABARWA_PKT_PPH_L2_T *)((void *)ptr_virt_addr + HAL_MT_NAMCHABARWA_PKT_EMAC_SZ));

#if LINUX_VERSION_CODE <= KERNEL_VERSION(4,6,7)
            ptr_net_dev->trans_start = jiffies;
#else
            netdev_get_tx_queue(ptr_net_dev, 0)->trans_start = jiffies;
#endif
            /* send gpd */
            if (CLX_E_OK == hal_mt_namchabarwa_pkt_sendGpd(unit, channel, ptr_sw_gpd))
            {
                ptr_priv->stats.tx_packets++;
                ptr_priv->stats.tx_bytes += ptr_skb->len - HAL_MT_NAMCHABARWA_PKT_PDMA_HDR_SZ;
            }
            else
            {
                ptr_priv->stats.tx_fifo_errors++;   /* to record the extreme cases where packets are dropped */
                ptr_priv->stats.tx_dropped++;

                osal_skb_unmapDma(phy_addr, ptr_skb->len, DMA_TO_DEVICE);
                osal_skb_free(ptr_skb);
                osal_free(ptr_sw_gpd);
            }
        }
    }

    return NETDEV_TX_OK;
}
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,10,0)
static void
_hal_mt_namchabarwa_pkt_net_dev_tx_timeout(
    struct net_device           *ptr_net_dev,
    unsigned int txqueue)
#else
static void
_hal_mt_namchabarwa_pkt_net_dev_tx_timeout(
    struct net_device           *ptr_net_dev)
#endif
{
    netif_stop_queue(ptr_net_dev);
    osal_sleepThread(1000);
    netif_wake_queue(ptr_net_dev);
}

static struct net_device_stats *
_hal_mt_namchabarwa_pkt_net_dev_get_stats(
    struct net_device           *ptr_net_dev)
{
    struct net_device_priv      *ptr_priv = netdev_priv(ptr_net_dev);

    return (&ptr_priv->stats);
}

static int
_hal_mt_namchabarwa_pkt_net_dev_set_mtu(
    struct net_device           *ptr_net_dev,
    int                         new_mtu)
{
    if (new_mtu < 64 || new_mtu > 9216)
    {
        return -EINVAL;
    }
    ptr_net_dev->mtu = new_mtu; /* This mtu need to be synced to chip's */
    return 0;
}

static int
_hal_mt_namchabarwa_pkt_net_dev_set_mac(
    struct net_device           *ptr_net_dev,
    void                        *ptr_mac_addr)
{
    struct sockaddr             *ptr_addr = ptr_mac_addr;

    memcpy(ptr_net_dev->dev_addr, ptr_addr->sa_data, ptr_net_dev->addr_len);
    return 0;
}

static void
_hal_mt_namchabarwa_pkt_net_dev_set_rx_mode(
    struct net_device           *ptr_dev)
{
    if (ptr_dev->flags & IFF_PROMISC)
    {
    }
    else
    {
        if (ptr_dev->flags & IFF_ALLMULTI)
        {
        }
        else
        {
            if (netdev_mc_empty(ptr_dev))
            {
                return;
            }
        }
    }
}

static struct net_device_ops    _hal_mt_namchabarwa_pkt_net_dev_ops =
{
    .ndo_init            = _hal_mt_namchabarwa_pkt_net_dev_init,
    .ndo_open            = _hal_mt_namchabarwa_pkt_net_dev_open,
    .ndo_stop            = _hal_mt_namchabarwa_pkt_net_dev_stop,
    .ndo_do_ioctl        = _hal_mt_namchabarwa_pkt_net_dev_ioctl,
    .ndo_start_xmit      = _hal_mt_namchabarwa_pkt_net_dev_tx,
    .ndo_tx_timeout      = _hal_mt_namchabarwa_pkt_net_dev_tx_timeout,
    .ndo_get_stats       = _hal_mt_namchabarwa_pkt_net_dev_get_stats,
    .ndo_change_mtu      = _hal_mt_namchabarwa_pkt_net_dev_set_mtu,
    .ndo_set_mac_address = _hal_mt_namchabarwa_pkt_net_dev_set_mac,
    .ndo_set_rx_mode     = _hal_mt_namchabarwa_pkt_net_dev_set_rx_mode,
};

#if LINUX_VERSION_CODE < KERNEL_VERSION(5,10,0)    
static int
_hal_mt_namchabarwa_pkt_net_dev_ethtool_get(
    struct net_device           *ptr_dev,
    struct ethtool_cmd          *ptr_cmd)
{
    struct net_device_priv          *ptr_priv;

    ptr_cmd->supported   = SUPPORTED_1000baseT_Full | SUPPORTED_FIBRE;
    ptr_cmd->port        = PORT_FIBRE;
    ptr_cmd->duplex      = DUPLEX_FULL;

    ptr_priv = netdev_priv(ptr_dev);
    ethtool_cmd_speed_set(ptr_cmd, ptr_priv->speed);

    return 0;
}
#endif

static struct ethtool_ops       _hal_mt_namchabarwa_pkt_net_dev_ethtool_ops =
{
#if LINUX_VERSION_CODE < KERNEL_VERSION(5,10,0)    
    .get_settings        = _hal_mt_namchabarwa_pkt_net_dev_ethtool_get,
#endif
    .get_link            = ethtool_op_get_link,
};

static void
_hal_mt_namchabarwa_pkt_setup(
    struct net_device       *ptr_net_dev)
{
    struct net_device_priv  *ptr_priv = netdev_priv(ptr_net_dev);

    /* setup net device */
    ether_setup(ptr_net_dev);
    ptr_net_dev->netdev_ops     = &_hal_mt_namchabarwa_pkt_net_dev_ops;
    ptr_net_dev->ethtool_ops    = &_hal_mt_namchabarwa_pkt_net_dev_ethtool_ops;
    ptr_net_dev->watchdog_timeo = HAL_MT_NAMCHABARWA_PKT_TX_TIMEOUT;
    ptr_net_dev->mtu            = HAL_MT_NAMCHABARWA_PKT_MAX_ETH_FRAME_SIZE; /* This mtu need to be synced to chip's */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,19,0)
	ptr_net_dev->min_mtu        = 64;
	ptr_net_dev->max_mtu        = 65535;    
#endif
    random_ether_addr(ptr_net_dev->dev_addr); /* Please use the mac-addr of interface. */

    /* setup private data */
    ptr_priv->ptr_net_dev       = ptr_net_dev;
    memset(&ptr_priv->stats, 0, sizeof(struct net_device_stats));
}

static CLX_ERROR_NO_T
_hal_mt_namchabarwa_pkt_createIntf(
    const UI32_T                        unit,
    void                                *ptr_data)
{
    HAL_MT_NAMCHABARWA_PKT_NETIF_INTF_T            net_intf = {0};
    HAL_MT_NAMCHABARWA_PKT_NETIF_PORT_DB_T         *ptr_port_db;
    struct net_device                   *ptr_net_dev = NULL;
    struct net_device_priv              *ptr_priv = NULL;
    CLX_ERROR_NO_T                      rc = CLX_E_OK;

    HAL_MT_NAMCHABARWA_PKT_IOCTL_NETIF_COOKIE_T    *ptr_cookie = ptr_data;
    /* Lock all Rx tasks to avoid any access to the intf during packet processing */
    /* Only Rx tasks are locked since Tx action is performed under a spinlock protection */
    _hal_mt_namchabarwa_pkt_lockRxChannelAll(unit);

    osal_io_copyFromUser(&net_intf, &ptr_cookie->net_intf, sizeof(HAL_MT_NAMCHABARWA_PKT_NETIF_INTF_T));

    OSAL_PRINT((OSAL_DBG_INTF | OSAL_DBG_INFO), "u=%u, create intf name=%s, phy port=%d\n",
                    unit, net_intf.name, net_intf.port);

    /* To check if the interface with the same name exists in kernel */
    ptr_net_dev = dev_get_by_name(&init_net, net_intf.name);
    if (NULL != ptr_net_dev)
    {
        OSAL_PRINT((OSAL_DBG_ERR | OSAL_DBG_INTF),
                        "u=%u, create intf failed, exist same name=%s\n",
                        unit, net_intf.name);

        dev_put(ptr_net_dev);

#if defined(HAL_MT_NAMCHABARWA_PKT_FORCR_REMOVE_DUPLICATE_NETDEV)
        ptr_net_dev->operstate = IF_OPER_DOWN;
        netif_carrier_off(ptr_net_dev);
        netif_tx_disable(ptr_net_dev);
        unregister_netdev(ptr_net_dev);
        free_netdev(ptr_net_dev);
#endif
        _hal_mt_namchabarwa_pkt_unlockRxChannelAll(unit);
        return (CLX_E_ENTRY_EXISTS);
    }

    /* Bind the net dev and intf meta data to internel port-based array */
    /* coverity:net_intf.port used as an index into _hal_mt_namchabarwa_pkt_port_db[129] */
    if (HAL_MT_NAMCHABARWA_PKT_MAX_PORT_NUM > net_intf.port)
    {
        ptr_port_db = HAL_MT_NAMCHABARWA_PKT_GET_PORT_DB(net_intf.port);
    }
    else
    {
        OSAL_PRINT(OSAL_DBG_ERR,
                                    "u=%u, port=%d is outof range[0-128]\n",
                                    unit, net_intf.port);
        return (CLX_E_BAD_PARAMETER);
    }
    
    if (ptr_port_db->ptr_net_dev == NULL)
    {

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 17, 0)
        ptr_net_dev = alloc_netdev(sizeof(struct net_device_priv),
                                   net_intf.name, NET_NAME_UNKNOWN, _hal_mt_namchabarwa_pkt_setup);
#else
        ptr_net_dev = alloc_netdev(sizeof(struct net_device_priv),
                                   net_intf.name, _hal_mt_namchabarwa_pkt_setup);
#endif
        memcpy(ptr_net_dev->dev_addr, net_intf.mac, ptr_net_dev->addr_len);

        ptr_priv = netdev_priv(ptr_net_dev);

        /* Port info will be used when packet sent from this netdev */
        ptr_priv->port = net_intf.port;
        ptr_priv->id   = net_intf.port;
        ptr_priv->unit = unit;

        register_netdev(ptr_net_dev);

        netif_carrier_off(ptr_net_dev);

        net_intf.id = net_intf.port;    /* Currently, id is 1-to-1 mapped to port */
        osal_memcpy(&ptr_port_db->meta, &net_intf, sizeof(HAL_MT_NAMCHABARWA_PKT_NETIF_INTF_T));

        ptr_port_db->ptr_net_dev = ptr_net_dev;

        /* Copy the intf-id to user space */
        osal_io_copyToUser(&ptr_cookie->net_intf, &net_intf, sizeof(HAL_MT_NAMCHABARWA_PKT_NETIF_INTF_T));
    }
    else
    {
        OSAL_PRINT((OSAL_DBG_INTF | OSAL_DBG_ERR),
                        "u=%u, create intf %s failed, exist on phy port=%d\n",
                        unit, net_intf.name, net_intf.port);
        /* The user needs to delete the existing intf binding to the same port */
        rc = CLX_E_ENTRY_EXISTS;
    }

    osal_io_copyToUser(&ptr_cookie->rc, &rc, sizeof(CLX_ERROR_NO_T));

    _hal_mt_namchabarwa_pkt_unlockRxChannelAll(unit);

    return (CLX_E_OK);
}

static CLX_ERROR_NO_T
_hal_mt_namchabarwa_pkt_destroyIntf(
    const UI32_T                        unit,
    void                                *ptr_data)
{
    HAL_MT_NAMCHABARWA_PKT_NETIF_INTF_T            net_intf = {0};
    HAL_MT_NAMCHABARWA_PKT_NETIF_PORT_DB_T         *ptr_port_db;
    UI32_T                              port = 0;
    CLX_ERROR_NO_T                      rc = CLX_E_ENTRY_NOT_FOUND;

    HAL_MT_NAMCHABARWA_PKT_IOCTL_NETIF_COOKIE_T    *ptr_cookie = ptr_data;
    /* Lock all Rx tasks to avoid any access to the intf during packet processing */
    /* Only Rx tasks are locked since Tx action is performed under a spinlock protection */
    _hal_mt_namchabarwa_pkt_lockRxChannelAll(unit);

    osal_io_copyFromUser(&net_intf, &ptr_cookie->net_intf, sizeof(HAL_MT_NAMCHABARWA_PKT_NETIF_INTF_T));

    /* Unregister net devices by id, although the "id" is now relavent to "port" we still perform a search */
    for (port = 0; port < HAL_MT_NAMCHABARWA_PKT_MAX_PORT_NUM; port++)
    {
        ptr_port_db = HAL_MT_NAMCHABARWA_PKT_GET_PORT_DB(port);
        if (NULL != ptr_port_db->ptr_net_dev)       /* valid intf */
        {
            if (ptr_port_db->meta.id == net_intf.id)
            {
                OSAL_PRINT(OSAL_DBG_INTF,
                                "u=%u, find intf %s (id=%d) on phy port=%d, destroy done\n",
                                unit,
                                ptr_port_db->meta.name,
                                ptr_port_db->meta.id,
                                ptr_port_db->meta.port);

                netif_carrier_off(ptr_port_db->ptr_net_dev);
                netif_tx_disable(ptr_port_db->ptr_net_dev);
                unregister_netdev(ptr_port_db->ptr_net_dev);
                free_netdev(ptr_port_db->ptr_net_dev);

                /* Don't need to remove profiles on this port.
                 * In fact, the profile is binding to "port" not "intf".
                 */
                /* _hal_mt_namchabarwa_pkt_destroyProfList(ptr_port_db->ptr_profile_list); */

                osal_memset(ptr_port_db, 0x0, sizeof(HAL_MT_NAMCHABARWA_PKT_NETIF_PORT_DB_T));
                rc = CLX_E_OK;
                break;
            }
        }
    }

    osal_io_copyToUser(&ptr_cookie->rc, &rc, sizeof(CLX_ERROR_NO_T));

    _hal_mt_namchabarwa_pkt_unlockRxChannelAll(unit);

    return (CLX_E_OK);
}

static CLX_ERROR_NO_T
_hal_mt_namchabarwa_pkt_traverseProfList(
    UI32_T                          intf_id,
    HAL_MT_NAMCHABARWA_PKT_PROFILE_NODE_T      *ptr_prof_list)
{
    HAL_MT_NAMCHABARWA_PKT_PROFILE_NODE_T      *ptr_curr_node;

    ptr_curr_node = ptr_prof_list;

    OSAL_PRINT(OSAL_DBG_INTF, "intf id=%d, prof list=", intf_id);
    while(NULL != ptr_curr_node)
    {
        OSAL_PRINT(OSAL_DBG_INTF, "%s (%d) => ",
                        ptr_curr_node->ptr_profile->name,
                        ptr_curr_node->ptr_profile->priority);
        ptr_curr_node = ptr_curr_node->ptr_next_node;
    }
    OSAL_PRINT(OSAL_DBG_INTF, "null\n");
    return (CLX_E_OK);
}


static CLX_ERROR_NO_T
_hal_mt_namchabarwa_pkt_getIntf(
    const UI32_T                        unit,
    void                                *ptr_data)
{
    HAL_MT_NAMCHABARWA_PKT_NETIF_INTF_T            net_intf = {0};
    HAL_MT_NAMCHABARWA_PKT_NETIF_PORT_DB_T         *ptr_port_db;
    UI32_T                              port = 0;
    CLX_ERROR_NO_T                      rc = CLX_E_ENTRY_NOT_FOUND;

    HAL_MT_NAMCHABARWA_PKT_IOCTL_NETIF_COOKIE_T    *ptr_cookie = ptr_data;
    osal_io_copyFromUser(&net_intf, &ptr_cookie->net_intf, sizeof(HAL_MT_NAMCHABARWA_PKT_NETIF_INTF_T));

    for (port = 0; port < HAL_MT_NAMCHABARWA_PKT_MAX_PORT_NUM; port++)
    {
        ptr_port_db = HAL_MT_NAMCHABARWA_PKT_GET_PORT_DB(port);
        if (NULL != ptr_port_db->ptr_net_dev)       /* valid intf */
        {
            if (ptr_port_db->meta.id == net_intf.id)
            {
                OSAL_PRINT(OSAL_DBG_INTF, "u=%u, find intf id=%d\n", unit, net_intf.id);
                _hal_mt_namchabarwa_pkt_traverseProfList(net_intf.id, ptr_port_db->ptr_profile_list);
                osal_io_copyToUser(&ptr_cookie->net_intf, &ptr_port_db->meta, sizeof(HAL_MT_NAMCHABARWA_PKT_NETIF_INTF_T));
                rc = CLX_E_OK;
                break;
            }
        }
    }

    osal_io_copyToUser(&ptr_cookie->rc, &rc, sizeof(CLX_ERROR_NO_T));

    return (CLX_E_OK);
}

static HAL_MT_NAMCHABARWA_PKT_NETIF_PROFILE_T  *
_hal_mt_namchabarwa_pkt_getProfEntry(
    const UI32_T                 id)
{
    HAL_MT_NAMCHABARWA_PKT_NETIF_PROFILE_T         *ptr_profile = NULL;

    if (id < HAL_MT_NAMCHABARWA_PKT_NET_PROFILE_NUM_MAX)
    {
        if (NULL != _ptr_hal_mt_namchabarwa_pkt_profile_entry[id])
        {
            ptr_profile = _ptr_hal_mt_namchabarwa_pkt_profile_entry[id];
        }
    }

    return (ptr_profile);
}

static CLX_ERROR_NO_T
_hal_mt_namchabarwa_pkt_createProfile(
    const UI32_T                        unit,
    void                                *ptr_data)
{
    HAL_MT_NAMCHABARWA_PKT_NETIF_PROFILE_T         *ptr_profile;
    HAL_MT_NAMCHABARWA_PKT_NETIF_PORT_DB_T         *ptr_port_db;
    CLX_ERROR_NO_T                      rc;

    HAL_MT_NAMCHABARWA_PKT_IOCTL_NETIF_COOKIE_T    *ptr_cookie = ptr_data;
    /* Lock all Rx tasks to avoid profiles being refered during packet processing */
    /* Need to lock all Rx tasks since packets from all Rx channels do profile lookup */
    _hal_mt_namchabarwa_pkt_lockRxChannelAll(unit);

    ptr_profile = osal_alloc(sizeof(HAL_MT_NAMCHABARWA_PKT_NETIF_PROFILE_T));
    osal_io_copyFromUser(ptr_profile, &ptr_cookie->net_profile,
                         sizeof(HAL_MT_NAMCHABARWA_PKT_NETIF_PROFILE_T));

    OSAL_PRINT(OSAL_DBG_PROFILE,
                    "u=%u, create prof name=%s, priority=%d, flag=0x%x\n",
                    unit,
                    ptr_profile->name,
                    ptr_profile->priority,
                    ptr_profile->flags);

    /* Save the profile to the profile array and assign the index to ptr_profile->id */
    rc = _hal_mt_namchabarwa_pkt_allocProfEntry(ptr_profile);
    if (CLX_E_OK == rc)
    {
        /* Insert the profile to the corresponding (port) interface */
        if ((ptr_profile->flags & HAL_MT_NAMCHABARWA_PKT_NETIF_PROFILE_FLAGS_PORT) != 0)
        {
            OSAL_PRINT(OSAL_DBG_PROFILE,
                            "u=%u, bind prof to phy port=%d\n", unit, ptr_profile->port);
            ptr_port_db = HAL_MT_NAMCHABARWA_PKT_GET_PORT_DB(ptr_profile->port);
            _hal_mt_namchabarwa_pkt_addProfToList(ptr_profile, &ptr_port_db->ptr_profile_list);
        }
        else
        {
            OSAL_PRINT(OSAL_DBG_PROFILE,
                            "u=%u, bind prof to all intf\n", unit);
            _hal_mt_namchabarwa_pkt_addProfToAllIntf(ptr_profile);
        }

        /* Copy the ptr_profile->id to user space */
        osal_io_copyToUser(&ptr_cookie->net_profile, ptr_profile, sizeof(HAL_MT_NAMCHABARWA_PKT_NETIF_PROFILE_T));
    }
    else
    {
        OSAL_PRINT((OSAL_DBG_PROFILE | OSAL_DBG_ERR),
                        "u=%u, alloc prof entry failed, tbl full\n", unit);
        osal_free(ptr_profile);
    }

    osal_io_copyToUser(&ptr_cookie->rc, &rc, sizeof(CLX_ERROR_NO_T));

    _hal_mt_namchabarwa_pkt_unlockRxChannelAll(unit);

    return (CLX_E_OK);
}

static CLX_ERROR_NO_T
_hal_mt_namchabarwa_pkt_destroyProfile(
    const UI32_T                        unit,
    void                                *ptr_data)
{
    HAL_MT_NAMCHABARWA_PKT_NETIF_PROFILE_T         profile = {0};
    HAL_MT_NAMCHABARWA_PKT_NETIF_PROFILE_T         *ptr_profile;
    CLX_ERROR_NO_T                      rc = CLX_E_OK;

    HAL_MT_NAMCHABARWA_PKT_IOCTL_NETIF_COOKIE_T    *ptr_cookie = ptr_data;
    /* Lock all Rx tasks to avoid profiles being refered during packet processing */
    /* Need to lock all Rx tasks since packets from all Rx channels do profile lookup */
    _hal_mt_namchabarwa_pkt_lockRxChannelAll(unit);

    osal_io_copyFromUser(&profile, &ptr_cookie->net_profile,
                         sizeof(HAL_MT_NAMCHABARWA_PKT_NETIF_PROFILE_T));

    /* Remove the profile from corresponding interface (port) */
    _hal_mt_namchabarwa_pkt_delProfFromAllIntfById(profile.id);

    ptr_profile = _hal_mt_namchabarwa_pkt_freeProfEntry(profile.id);
    if (NULL != ptr_profile)
    {
        OSAL_PRINT(OSAL_DBG_PROFILE,
                        "u=%u, destroy prof id=%d, name=%s, priority=%d, flag=0x%x\n",
                        unit,
                        ptr_profile->id,
                        ptr_profile->name,
                        ptr_profile->priority,
                        ptr_profile->flags);
        osal_free(ptr_profile);
    }

    osal_io_copyToUser(&ptr_cookie->rc, &rc, sizeof(CLX_ERROR_NO_T));

    _hal_mt_namchabarwa_pkt_unlockRxChannelAll(unit);

    return (CLX_E_OK);
}

static CLX_ERROR_NO_T
_hal_mt_namchabarwa_pkt_getProfile(
    const UI32_T                        unit,
    void                                *ptr_data)
{
    HAL_MT_NAMCHABARWA_PKT_NETIF_PROFILE_T         profile = {0};
    HAL_MT_NAMCHABARWA_PKT_NETIF_PROFILE_T         *ptr_profile;
    CLX_ERROR_NO_T                      rc = CLX_E_OK;

    HAL_MT_NAMCHABARWA_PKT_IOCTL_NETIF_COOKIE_T    *ptr_cookie = ptr_data;
    osal_io_copyFromUser(&profile, &ptr_cookie->net_profile, sizeof(HAL_MT_NAMCHABARWA_PKT_NETIF_PROFILE_T));

    ptr_profile = _hal_mt_namchabarwa_pkt_getProfEntry(profile.id);
    if (NULL != ptr_profile)
    {
        osal_io_copyToUser(&ptr_cookie->net_profile, ptr_profile, sizeof(HAL_MT_NAMCHABARWA_PKT_NETIF_PROFILE_T));
    }
    else
    {
        rc = CLX_E_ENTRY_NOT_FOUND;
    }

    osal_io_copyToUser(&ptr_cookie->rc, &rc, sizeof(CLX_ERROR_NO_T));

    return (CLX_E_OK);
}

static CLX_ERROR_NO_T
_hal_mt_namchabarwa_pkt_getIntfCnt(
    const UI32_T                        unit,
    void                                *ptr_data)
{
    HAL_MT_NAMCHABARWA_PKT_NETIF_INTF_T            net_intf = {0};
    HAL_MT_NAMCHABARWA_PKT_NETIF_INTF_CNT_T        intf_cnt = {0};
    HAL_MT_NAMCHABARWA_PKT_NETIF_PORT_DB_T         *ptr_port_db;
    struct net_device_priv              *ptr_priv;
    UI32_T                              port = 0;
    CLX_ERROR_NO_T                      rc = CLX_E_ENTRY_NOT_FOUND;

    HAL_MT_NAMCHABARWA_PKT_IOCTL_NETIF_COOKIE_T    *ptr_cookie = ptr_data;
    osal_io_copyFromUser(&net_intf, &ptr_cookie->net_intf, sizeof(HAL_MT_NAMCHABARWA_PKT_NETIF_INTF_T));

    for (port = 0; port < HAL_MT_NAMCHABARWA_PKT_MAX_PORT_NUM; port++)
    {
        ptr_port_db = HAL_MT_NAMCHABARWA_PKT_GET_PORT_DB(port);
        if (NULL != ptr_port_db->ptr_net_dev)       /* valid intf */
        {
            if (ptr_port_db->meta.id == net_intf.id)
            {
                ptr_priv = netdev_priv(ptr_port_db->ptr_net_dev);
                intf_cnt.rx_pkt   = ptr_priv->stats.rx_packets;
                intf_cnt.tx_pkt   = ptr_priv->stats.tx_packets;
                intf_cnt.tx_error = ptr_priv->stats.tx_errors;
                intf_cnt.tx_queue_full = ptr_priv->stats.tx_fifo_errors;

                rc = CLX_E_OK;
                break;
            }
        }
    }

    osal_io_copyToUser(&ptr_cookie->cnt, &intf_cnt, sizeof(HAL_MT_NAMCHABARWA_PKT_NETIF_INTF_CNT_T));
    osal_io_copyToUser(&ptr_cookie->rc, &rc, sizeof(CLX_ERROR_NO_T));

    return (CLX_E_OK);
}

static CLX_ERROR_NO_T
_hal_mt_namchabarwa_pkt_clearIntfCnt(
    const UI32_T                        unit,
    void                                *ptr_data)
{
    HAL_MT_NAMCHABARWA_PKT_NETIF_INTF_T            net_intf = {0};
    HAL_MT_NAMCHABARWA_PKT_NETIF_PORT_DB_T         *ptr_port_db;
    struct net_device_priv              *ptr_priv;
    UI32_T                              port = 0;
    CLX_ERROR_NO_T                      rc = CLX_E_ENTRY_NOT_FOUND;

    HAL_MT_NAMCHABARWA_PKT_IOCTL_NETIF_COOKIE_T    *ptr_cookie = ptr_data;
    osal_io_copyFromUser(&net_intf, &ptr_cookie->net_intf, sizeof(HAL_MT_NAMCHABARWA_PKT_NETIF_INTF_T));

    for (port = 0; port < HAL_MT_NAMCHABARWA_PKT_MAX_PORT_NUM; port++)
    {
        ptr_port_db = HAL_MT_NAMCHABARWA_PKT_GET_PORT_DB(port);
        if (NULL != ptr_port_db->ptr_net_dev)       /* valid intf */
        {
            if (ptr_port_db->meta.id == net_intf.id)
            {
                ptr_priv = netdev_priv(ptr_port_db->ptr_net_dev);
                ptr_priv->stats.rx_packets = 0;
                ptr_priv->stats.tx_packets = 0;
                ptr_priv->stats.tx_errors  = 0;
                ptr_priv->stats.tx_fifo_errors = 0;

                rc = CLX_E_OK;
                break;
            }
        }
    }

    osal_io_copyToUser(&ptr_cookie->rc, &rc, sizeof(CLX_ERROR_NO_T));

    return (CLX_E_OK);
}

/* ----------------------------------------------------------------------------------- Init: dev_ops */
static void
_hal_mt_namchabarwa_pkt_dev_tx_callback(
    const UI32_T                    unit,
          HAL_MT_NAMCHABARWA_PKT_TX_SW_GPD_T   *ptr_sw_gpd,
          HAL_MT_NAMCHABARWA_PKT_TX_SW_GPD_T   *ptr_sw_gpd_usr)
{
    UI32_T                          channel = ptr_sw_gpd->channel;
    HAL_MT_NAMCHABARWA_PKT_TX_CB_T             *ptr_tx_cb = HAL_MT_NAMCHABARWA_PKT_GET_TX_CB_PTR(unit);

    while (0 != _hal_mt_namchabarwa_pkt_enQueue(&ptr_tx_cb->sw_queue, ptr_sw_gpd))
    {
        ptr_tx_cb->cnt.channel[channel].enque_retry++;
        HAL_MT_NAMCHABARWA_PKT_TX_ENQUE_RETRY_SLEEP();
    }
    ptr_tx_cb->cnt.channel[channel].enque_ok++;

    osal_triggerEvent(&ptr_tx_cb->sync_sema);
    ptr_tx_cb->cnt.channel[channel].trig_event++;
}

CLX_ERROR_NO_T
hal_mt_namchabarwa_pkt_dev_tx(
    const UI32_T                      unit,
    void                              *ptr_data)
{
    int                             ret = 0;
    int                             idx = 0;
    unsigned int                    channel = 0;
    HAL_MT_NAMCHABARWA_PKT_IOCTL_TX_COOKIE_T   tx_cookie;
    HAL_MT_NAMCHABARWA_PKT_IOCTL_TX_GPD_T      ioctl_gpd;
    HAL_MT_NAMCHABARWA_PKT_TX_SW_GPD_T         *ptr_sw_gpd_knl = NULL;
    HAL_MT_NAMCHABARWA_PKT_TX_SW_GPD_T         *ptr_first_sw_gpd_knl = NULL;

    /* copy the tx-cookie */
    osal_io_copyFromUser(&tx_cookie, ptr_data, sizeof(HAL_MT_NAMCHABARWA_PKT_IOCTL_TX_COOKIE_T));

    channel = tx_cookie.channel;

    ptr_sw_gpd_knl = osal_alloc(sizeof(HAL_MT_NAMCHABARWA_PKT_TX_SW_GPD_T));
    ptr_first_sw_gpd_knl = ptr_sw_gpd_knl;

    /* create SW GPD based on the content of each IOCTL GPD */
    while (1)
    {
        osal_io_copyFromUser(&ioctl_gpd,
                             ((void *)((CLX_HUGE_T)tx_cookie.ioctl_gpd_addr))
                                 +idx*sizeof(HAL_MT_NAMCHABARWA_PKT_IOCTL_TX_GPD_T),
                             sizeof(HAL_MT_NAMCHABARWA_PKT_IOCTL_TX_GPD_T));

        ptr_sw_gpd_knl->channel = ioctl_gpd.channel;
        ptr_sw_gpd_knl->gpd_num = ioctl_gpd.gpd_num;
        ptr_sw_gpd_knl->ptr_cookie = (void *)ioctl_gpd.cookie;

        /* directly copy user's HW GPD */
        osal_io_copyFromUser(&ptr_sw_gpd_knl->tx_gpd,
                             (void *)((CLX_HUGE_T)ioctl_gpd.hw_gpd_addr),
                             sizeof(HAL_MT_NAMCHABARWA_PKT_TX_GPD_T));

        /* replace the callback */
        ptr_sw_gpd_knl->callback = (void *)_hal_mt_namchabarwa_pkt_dev_tx_callback;

        /* save the first SW GPD address from userspace since
         * we have replaced the original callback
         */
        ptr_sw_gpd_knl->ptr_cookie = (void *)ioctl_gpd.sw_gpd_addr;

        if (HAL_MT_NAMCHABARWA_PKT_PDMA_CH_PKT_EOP == ptr_sw_gpd_knl->tx_gpd.eop)
        {
            ptr_sw_gpd_knl->ptr_next = NULL;
            break;
        }
        else
        {
            ptr_sw_gpd_knl->ptr_next = (HAL_MT_NAMCHABARWA_PKT_TX_SW_GPD_T *)osal_alloc(
                                            sizeof(HAL_MT_NAMCHABARWA_PKT_TX_SW_GPD_T));
            ptr_sw_gpd_knl = ptr_sw_gpd_knl->ptr_next;
            idx++;
        }
    }

    /* coverity: channel used as an offset of HAL_MT_NAMCHABARWA_PKT_TX_PDMA_T pdma[4] */
    if (HAL_MT_NAMCHABARWA_PKT_TX_CHANNEL_LAST > channel)
    {    
        OSAL_PRINT(OSAL_DBG_TX,
                        "u=%u, channel=%u, s_addr:0x%llx, size:%d\n",
                        unit, channel, CLX_ADDR_32_TO_64(ptr_sw_gpd_knl->tx_gpd.s_addr_hi, ptr_sw_gpd_knl->tx_gpd.s_addr_lo), ptr_sw_gpd_knl->tx_gpd.size);
        ret = hal_mt_namchabarwa_pkt_sendGpd(unit, channel, ptr_first_sw_gpd_knl);
    }
    else
    {
        OSAL_PRINT(OSAL_DBG_ERR,
                            "u=%u, channel=%u is outof range[0-3]\n",
                            unit, channel);
        ret = CLX_E_BAD_PARAMETER;
    }
    
    if (CLX_E_OK != ret)
    {
        _hal_mt_namchabarwa_pkt_freeTxGpdList(unit, ptr_first_sw_gpd_knl);
    }

    /* return 0 if success */
    return (ret);
}

long
hal_mt_namchabarwa_pkt_dev_ioctl(
    const UI32_T                    unit)
{
    CLX_ERROR_NO_T      rc = CLX_E_OK;
    /* network interface */
    _osal_mdc_registerIoctlCallback(unit, OSAL_MDC_IOCTL_TYPE_NETIF_CREATE_INTF,
        _hal_mt_namchabarwa_pkt_createIntf);
    _osal_mdc_registerIoctlCallback(unit, OSAL_MDC_IOCTL_TYPE_NETIF_DESTROY_INTF,
        _hal_mt_namchabarwa_pkt_destroyIntf);
    _osal_mdc_registerIoctlCallback(unit, OSAL_MDC_IOCTL_TYPE_NETIF_GET_INTF,
        _hal_mt_namchabarwa_pkt_getIntf);
    _osal_mdc_registerIoctlCallback(unit, OSAL_MDC_IOCTL_TYPE_NETIF_CREATE_PROFILE,
        _hal_mt_namchabarwa_pkt_createProfile);
    _osal_mdc_registerIoctlCallback(unit, OSAL_MDC_IOCTL_TYPE_NETIF_DESTROY_PROFILE,
        _hal_mt_namchabarwa_pkt_destroyProfile);
    _osal_mdc_registerIoctlCallback(unit, OSAL_MDC_IOCTL_TYPE_NETIF_GET_PROFILE,
        _hal_mt_namchabarwa_pkt_getProfile);
    _osal_mdc_registerIoctlCallback(unit, OSAL_MDC_IOCTL_TYPE_NETIF_GET_INTF_CNT,
        _hal_mt_namchabarwa_pkt_getIntfCnt);
    _osal_mdc_registerIoctlCallback(unit, OSAL_MDC_IOCTL_TYPE_NETIF_CLEAR_INTF_CNT,
        _hal_mt_namchabarwa_pkt_clearIntfCnt);
    //TODO_FIXME_PORT
    _osal_mdc_registerIoctlCallback(unit, OSAL_MDC_IOCTL_TYPE_NETIF_WAIT_RX_FREE,
        _hal_mt_namchabarwa_pkt_schedRxDeQueue);
    //TODO_FIXME_PORT
    _osal_mdc_registerIoctlCallback(unit, OSAL_MDC_IOCTL_TYPE_NETIF_WAIT_TX_FREE,
        _hal_mt_namchabarwa_pkt_strictTxDeQueue);

    _osal_mdc_registerIoctlCallback(unit, OSAL_MDC_IOCTL_TYPE_NETIF_SET_RX_CFG,
        hal_mt_namchabarwa_pkt_setRxKnlConfig);
    _osal_mdc_registerIoctlCallback(unit, OSAL_MDC_IOCTL_TYPE_NETIF_GET_RX_CFG,
        hal_mt_namchabarwa_pkt_getRxKnlConfig);
    _osal_mdc_registerIoctlCallback(unit, OSAL_MDC_IOCTL_TYPE_NETIF_DEINIT_TASK,
        hal_mt_namchabarwa_pkt_deinitTask);
    _osal_mdc_registerIoctlCallback(unit, OSAL_MDC_IOCTL_TYPE_NETIF_DEINIT_DRV,
        hal_mt_namchabarwa_pkt_deinitPktDrv);
    _osal_mdc_registerIoctlCallback(unit, OSAL_MDC_IOCTL_TYPE_NETIF_INIT_TASK,
        hal_mt_namchabarwa_pkt_initTask);
    _osal_mdc_registerIoctlCallback(unit, OSAL_MDC_IOCTL_TYPE_NETIF_INIT_DRV,
        hal_mt_namchabarwa_pkt_initPktDrv);

    /* counter */
    _osal_mdc_registerIoctlCallback(unit, OSAL_MDC_IOCTL_TYPE_NETIF_GET_TX_CNT,
                                    hal_mt_namchabarwa_pkt_getTxKnlCnt);
    _osal_mdc_registerIoctlCallback(unit, OSAL_MDC_IOCTL_TYPE_NETIF_GET_RX_CNT,
                                    hal_mt_namchabarwa_pkt_getRxKnlCnt);
    _osal_mdc_registerIoctlCallback(unit, OSAL_MDC_IOCTL_TYPE_NETIF_CLEAR_TX_CNT,
                                    hal_mt_namchabarwa_pkt_clearTxKnlCnt);
    _osal_mdc_registerIoctlCallback(unit, OSAL_MDC_IOCTL_TYPE_NETIF_CLEAR_RX_CNT,
                                    hal_mt_namchabarwa_pkt_clearRxKnlCnt);

    _osal_mdc_registerIoctlCallback(unit, OSAL_MDC_IOCTL_TYPE_NETIF_SET_PORT_ATTR,
        hal_mt_namchabarwa_pkt_setPortAttr);
    _osal_mdc_registerIoctlCallback(unit, OSAL_MDC_IOCTL_TYPE_NETIF_GET_PORT_ATTR,
        hal_mt_namchabarwa_pkt_getPortAttr);
    _osal_mdc_registerIoctlCallback(unit, OSAL_MDC_IOCTL_TYPE_NETIF_NL_SET_INTF_PROPERTY,
        _hal_mt_namchabarwa_pkt_setIntfProperty);
    _osal_mdc_registerIoctlCallback(unit, OSAL_MDC_IOCTL_TYPE_NETIF_NL_GET_INTF_PROPERTY,
        _hal_mt_namchabarwa_pkt_getIntfProperty);
    _osal_mdc_registerIoctlCallback(unit, OSAL_MDC_IOCTL_TYPE_NETIF_NL_CREATE_NETLINK,
        _hal_mt_namchabarwa_pkt_createNetlink);
    _osal_mdc_registerIoctlCallback(unit, OSAL_MDC_IOCTL_TYPE_NETIF_NL_DESTROY_NETLINK,
        _hal_mt_namchabarwa_pkt_destroyNetlink);
    _osal_mdc_registerIoctlCallback(unit, OSAL_MDC_IOCTL_TYPE_NETIF_NL_GET_NETLINK,
        _hal_mt_namchabarwa_pkt_getNetlink);

    _osal_mdc_registerIoctlCallback(unit, OSAL_MDC_IOCTL_TYPE_NETIF_DEV_TX,
        hal_mt_namchabarwa_pkt_dev_tx);
    return rc;
}

/* ----------------------------------------------------------------------------------- Init/Deinit */
static int netif_init_done = 0;
CLX_ERROR_NO_T
hal_mt_namchabarwa_pkt_init(
    const UI32_T            unit)
{
    /* Since the users may kill SDK application without a de-init flow,
     * we help to detect if NETIF is ever init before, and perform deinit.
     */
    if(netif_init_done && _hal_mt_namchabarwa_pkt_drv_cb[unit].init_flag) {
        OSAL_PRINT(OSAL_DBG_ERR,
           "BUG!!! u=%u, looks the users may kill SDK app without a de-init flow\n", unit);
        return(0);
    }

    /* Init Thread */
    osal_init();

    /* Reset all database*/
    osal_memset(_hal_mt_namchabarwa_pkt_port_db, 0x0,
                (HAL_MT_NAMCHABARWA_PKT_MAX_PORT_NUM * sizeof(HAL_MT_NAMCHABARWA_PKT_NETIF_PORT_DB_T)));
    osal_memset(_hal_mt_namchabarwa_pkt_rx_cb, 0x0,
                OSAL_MDC_MAX_CHIPS_PER_SYSTEM*sizeof(HAL_MT_NAMCHABARWA_PKT_RX_CB_T));
    osal_memset(_hal_mt_namchabarwa_pkt_tx_cb, 0x0,
                OSAL_MDC_MAX_CHIPS_PER_SYSTEM*sizeof(HAL_MT_NAMCHABARWA_PKT_TX_CB_T));
    osal_memset(_hal_mt_namchabarwa_pkt_drv_cb, 0x0,
            OSAL_MDC_MAX_CHIPS_PER_SYSTEM*sizeof(HAL_MT_NAMCHABARWA_PKT_DRV_CB_T));

    netif_nl_init();
    netif_init_done = 1;
    return (0);
}

CLX_ERROR_NO_T
hal_mt_namchabarwa_pkt_exit(
    const UI32_T            unit)
{
    /* 1st. Stop all netdev (if any) to prevent kernel from Tx new packets */
    _hal_mt_namchabarwa_pkt_stopAllIntf(unit);

    /* 2nd. Stop Rx HW DMA and free all the DMA buffer hooked on the ring */
     _hal_mt_namchabarwa_pkt_rxStop(unit);

    /* 3rd. Need to wait Rx done task process all the availavle packets on GPD ring */

    /* 4th. Stop all the internal tasks (if any) */
    hal_mt_namchabarwa_pkt_deinitTask(unit, NULL);

    /* 5th. Deinit pkt driver for common database/interrupt source (if required) */
    hal_mt_namchabarwa_pkt_deinitPktDrv(unit, NULL);

    /* 6th destroy all netlink */
    netif_nl_destroyAllNetlink(unit);

    /* 7th. Clean up those intf/profiles not been destroyed */
    _hal_mt_namchabarwa_pkt_destroyAllProfile(unit);
    _hal_mt_namchabarwa_pkt_destroyAllIntf(unit);

    osal_deinit();

    netif_init_done = 0;
    return (0);
}
