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

/* FILE NAME:   clx_netif.h
 * PURPOSE:
 *      Define the NET interface APIs in CLX SDK.
 * NOTES:
 */
#ifndef CLX_NETIF_H
#define CLX_NETIF_H

#include <clx_error.h>
#include <clx_types.h>
#include <clx_pkt.h>

#define CLX_NETIF_NAME_LEN              (32)
#define CLX_NETIF_PROFILE_NUM_MAX       (256)
#define CLX_NETIF_PROFILE_PATTERN_NUM   (4)
#define CLX_NETIF_PROFILE_PATTERN_LEN   (8)

/* ----------------------------------------------------------------------------------- struct */
typedef struct
{
    UI32_T          tx_pkt;
    UI32_T          tx_queue_full;
    UI32_T          tx_error;
    UI32_T          rx_pkt;

} CLX_NETIF_INTF_CNT_T;

typedef struct
{
    /* unique key */
    UI32_T                      id;
    C8_T                        name[CLX_NETIF_NAME_LEN];
    CLX_PORT_T                  port;    /* only support unit port and local port */

    /* metadata */
    CLX_MAC_T                   mac;

#define CLX_NETIF_INTF_FLAGS_MAC        (1UL << 0)
    UI32_T                      flags;

} CLX_NETIF_INTF_T;


typedef struct
{
    C8_T                        name[CLX_NETIF_NAME_LEN];
    C8_T                        mc_group_name[CLX_NETIF_NAME_LEN];
} CLX_NETIF_RX_DST_NETLINK_T;

typedef enum
{
    CLX_NETIF_RX_DST_SDK = 0,
    CLX_NETIF_RX_DST_NETLINK,
    CLX_NETIF_RX_DST_LAST
} CLX_NETIF_RX_DST_TYPE_T;

typedef struct
{
    /* unique key */
    UI32_T                      id;
    C8_T                        name[CLX_NETIF_NAME_LEN];
    UI32_T                      priority;

    /* match fields */
    CLX_PORT_T                  port;     /* only support unit port and local port */
    CLX_PKT_RX_REASON_BITMAP_T  reason_bitmap;
    UI8_T                       pattern[CLX_NETIF_PROFILE_PATTERN_NUM][CLX_NETIF_PROFILE_PATTERN_LEN];
    UI8_T                       mask[CLX_NETIF_PROFILE_PATTERN_NUM][CLX_NETIF_PROFILE_PATTERN_LEN];
    UI32_T                      offset[CLX_NETIF_PROFILE_PATTERN_NUM];

    /* for each flag 1:must hit, 0:don't care */
#define CLX_NETIF_PROFILE_FLAGS_PORT      (1UL << 0)
#define CLX_NETIF_PROFILE_FLAGS_REASON    (1UL << 1)
#define CLX_NETIF_PROFILE_FLAGS_PATTERN_0 (1UL << 2)
#define CLX_NETIF_PROFILE_FLAGS_PATTERN_1 (1UL << 3)
#define CLX_NETIF_PROFILE_FLAGS_PATTERN_2 (1UL << 4)
#define CLX_NETIF_PROFILE_FLAGS_PATTERN_3 (1UL << 5)
    UI32_T                      flags;

    CLX_NETIF_RX_DST_TYPE_T     dst_type;
    CLX_NETIF_RX_DST_NETLINK_T  netlink;

} CLX_NETIF_PROFILE_T;

/* ----------------------------------------------------------------------------------- APIs */
/* FUNCTION NAME: clx_netif_createIntf
 * PURPOSE:
 *      This API is used to create the Network Interface for Linux TCP/IP stack.
 * INPUT:
 *      unit            -- The unit ID
 *      ptr_net_intf    -- Pointer of the Network Interface
 *
 * OUTPUT:
 *      ptr_intf_id     -- Pointer of the Network Interface ID
 *
 * RETURN:
 *      CLX_E_OK        -- Operation is successful.
 *      CLX_E_OTHERS    -- Fail
 * NOTES:
 *      None
 *
 */
CLX_ERROR_NO_T
clx_netif_createIntf(
    const UI32_T            unit,
    CLX_NETIF_INTF_T        *ptr_net_intf,
    UI32_T                  *ptr_intf_id);


/* FUNCTION NAME: clx_netif_destroyIntf
 * PURPOSE:
 *      This API is used to destroy the Network Interface for Linux TCP/IP stack.
 * INPUT:
 *      unit            -- The unit ID
 *      id              -- The Network Interface ID
 *
 * OUTPUT:
 *      None
 * RETURN:
 *      CLX_E_OK        -- Operation is successful.
 *      CLX_E_OTHERS    -- Fail
 * NOTES:
 *      None
 *
 */
CLX_ERROR_NO_T
clx_netif_destroyIntf(
    const UI32_T            unit,
    const UI32_T            intf_id);

/* FUNCTION NAME: clx_netif_getIntf
 * PURPOSE:
 *      This API get the Network Interface for Linux TCP/IP stack.
 * INPUT:
 *      unit            -- The unit ID
 *      intf_id         -- The Network Interface ID
 *
 * OUTPUT:
 *      ptr_net_intf    -- Pointer of the Network Interface
 * RETURN:
 *      CLX_E_OK        -- Operation is successful.
 *      CLX_E_OTHERS    -- Fail
 * NOTES:
 *      None
 *
 */
CLX_ERROR_NO_T
clx_netif_getIntf(
    const UI32_T            unit,
    const UI32_T            intf_id,
    CLX_NETIF_INTF_T        *ptr_net_intf);

/* FUNCTION NAME: clx_netif_getIntfCnt
 * PURPOSE:
 *      This API get the Network Profile counter for Linux TCP/IP stack.
 * INPUT:
 *      unit            -- The unit ID
 *      intf_id         -- The Network Interface ID
 *
 * OUTPUT:
 *      ptr_netif_cnt   -- Pointer of the Network Interface counter
 * RETURN:
 *      CLX_E_OK        -- Operation is successful.
 *      CLX_E_OTHERS    -- Fail
 * NOTES:
 *      None
 *
 */
CLX_ERROR_NO_T
clx_netif_getIntfCnt(
    const UI32_T            unit,
    const UI32_T            intf_id,
    CLX_NETIF_INTF_CNT_T    *ptr_netif_cnt);

/* FUNCTION NAME: clx_netif_clearIntfCnt
 * PURPOSE:
 *      This API clear the Network Profile counter for Linux TCP/IP stack.
 * INPUT:
 *      unit            -- The unit ID
 *      intf_id         -- The Network Interface ID
 *
 * OUTPUT:
 *      None
 * RETURN:
 *      CLX_E_OK        -- Operation is successful.
 *      CLX_E_OTHERS    -- Fail
 * NOTES:
 *      None
 *
 */
CLX_ERROR_NO_T
clx_netif_clearIntfCnt(
    const UI32_T            unit,
    const UI32_T            intf_id);

/* FUNCTION NAME: clx_netif_createProfile
 * PURPOSE:
 *      This API is used to create the Network Profile for Rx packets to User Process.
 * INPUT:
 *      unit            -- The unit ID
 *      ptr_net_profile -- Pointer of the Network Profile
 *
 * OUTPUT:
 *      ptr_profile_id  -- Pointer of the Network Profile ID
 *
 * RETURN:
 *      CLX_E_OK        -- Operation is successful.
 *      CLX_E_OTHERS    -- Fail
 * NOTES:
 *      None
 *
 */
CLX_ERROR_NO_T
clx_netif_createProfile(
    const UI32_T            unit,
    CLX_NETIF_PROFILE_T     *ptr_net_profile,
    UI32_T                  *ptr_profile_id);

/* FUNCTION NAME: clx_netif_destroyProfile
 * PURPOSE:
 *      This API is used to destroy the Network Profile for Rx packets to User Process.
 * INPUT:
 *      unit            -- The unit ID
 *      id              -- The Network Profile ID
 *
 * OUTPUT:
 *      None
 * RETURN:
 *      CLX_E_OK        -- Operation is successful.
 *      CLX_E_OTHERS    -- Fail
 * NOTES:
 *      None
 *
 */
CLX_ERROR_NO_T
clx_netif_destroyProfile(
    const UI32_T            unit,
    const UI32_T            profile_id);

typedef enum
{
    CLX_NETIF_INTF_PROPERTY_IGR_SAMPLING_RATE,
    CLX_NETIF_INTF_PROPERTY_EGR_SAMPLING_RATE,
    CLX_NETIF_INTF_PROPERTY_SKIP_PORT_STATE_EVENT,
    CLX_NETIF_INTF_PROPERTY_ADMIN_STATE,
    CLX_NETIF_INTF_PROPERTY_PDMA_RX_CNT,
    CLX_NETIF_INTF_PROPERTY_RX_RCH_STATUS,
    CLX_NETIF_INTF_PROPERTY_LAST
} CLX_NETIF_INTF_PROPERTY_T;

/* FUNCTION NAME:   clx_netif_setIntfProperty
 * PURPOSE:
 *      Set Port property.
 * INPUT:
 *      unit                -- Device unit number
 *      intf_id             -- Network Interface ID
 *      property            -- Property type
 *      param0              -- First parameter
 *      param1              -- Second parameter
 * OUTPUT:
 *      None
 * RETURN:
 *      CLX_E_OK            -- Operation success
 *      CLX_E_BAD_PARAMETER -- Bad parameter
 * NOTES:
 *
 */
CLX_ERROR_NO_T
clx_netif_setIntfProperty(
    const UI32_T                        unit,
    const UI32_T                        intf_id,
    const CLX_NETIF_INTF_PROPERTY_T     property,
    const UI32_T                        param0,
    const UI32_T                        param1);

/* FUNCTION NAME:   clx_netif_getIntfProperty
 * PURPOSE:
 *      Get port property.
 * INPUT:
 *      unit                -- Device unit number
 *      intf_id             -- Network Interface ID
 *      property            -- Property type
 * OUTPUT:
 *      *ptr_param0         -- Ptr of first parameter
 *      *ptr_param1         -- Ptr of second parameter
 * RETURN:
 *      CLX_E_OK            -- Operation success
 *      CLX_E_BAD_PARAMETER -- Bad parameter
 * NOTES:
 *
 */
CLX_ERROR_NO_T
clx_netif_getIntfProperty(
    const UI32_T                        unit,
    const UI32_T                        intf_id,
    const CLX_NETIF_INTF_PROPERTY_T     property,
    UI32_T                              *ptr_param0,
    UI32_T                              *ptr_param1);



#define CLX_NETIF_NETLINK_NUM_MAX                   (256)
#define CLX_NETIF_NETLINK_MC_GROUP_NUM_MAX          (32)

typedef struct
{
    C8_T                                name[CLX_NETIF_NAME_LEN];

} CLX_NETIF_NETLINK_MC_GROUP_T;

typedef struct
{
    UI32_T                              id;
    C8_T                                name[CLX_NETIF_NAME_LEN];
    CLX_NETIF_NETLINK_MC_GROUP_T        mc_group[CLX_NETIF_NETLINK_MC_GROUP_NUM_MAX];
    UI32_T                              mc_group_num;

} CLX_NETIF_NETLINK_T;

/* FUNCTION NAME:   clx_netif_createNetlink
 * PURPOSE:
 *      Create Netlink.
 * INPUT:
 *      unit                -- Device unit number
 *      ptr_netlink         -- Pointer of the Netlink
 * OUTPUT:
 *      ptr_netlink_id      -- Pointer of the Netlink ID
 * RETURN:
 *      CLX_E_OK            -- Operation success
 *      CLX_E_BAD_PARAMETER -- Bad parameter
 * NOTES:
 *
 */
CLX_ERROR_NO_T
clx_netif_createNetlink(
    const UI32_T                        unit,
    CLX_NETIF_NETLINK_T                 *ptr_netlink,
    UI32_T                              *ptr_netlink_id);

/* FUNCTION NAME:   clx_netif_destroyNetlink
 * PURPOSE:
 *      Destroy Netlink.
 * INPUT:
 *      unit                -- Device unit number
 *      netlink_id          -- The Netlink ID
 * RETURN:
 *      CLX_E_OK            -- Operation success
 *      CLX_E_BAD_PARAMETER -- Bad parameter
 * NOTES:
 *
 */
CLX_ERROR_NO_T
clx_netif_destroyNetlink(
    const UI32_T                        unit,
    const UI32_T                        netlink_id);

/* FUNCTION NAME:   clx_netif_getNetlink
 * PURPOSE:
 *      Get the Netlink.
 * INPUT:
 *      unit                -- Device unit number
 *      netlink_id          -- The Netlink ID
 * OUTPUT:
 *      ptr_netlink         -- Pointer of the Netlink
 * RETURN:
 *      CLX_E_OK            -- Operation success
 *      CLX_E_BAD_PARAMETER -- Bad parameter
 * NOTES:
 *
 */
CLX_ERROR_NO_T
clx_netif_getNetlink(
    const UI32_T                        unit,
    const UI32_T                        netlink_id,
    CLX_NETIF_NETLINK_T                 *ptr_netlink);


#endif  /* end of CLX_NETIF_H */
