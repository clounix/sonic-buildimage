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

/* FILE NAME:  netif_perf.h
 * PURPOSE:
 *      It provide customer performance test API.
 * NOTES:
 */

#ifndef NETIF_PERF_H
#define NETIF_PERF_H

/* #define PERF_EN_TEST */

/* FUNCTION NAME: perf_rxCallback
 * PURPOSE:
 *      To count the Rx-gpd for Rx-test.
 * INPUT:
 *      len         -- To check if the Rx-gpd length equals to test length.
 * OUTPUT:
 *      None
 * RETURN:
 *      CLX_E_OK    -- Successful operation.
 * NOTES:
 *      None
 */
CLX_ERROR_NO_T
perf_rxCallback(
    const UI32_T                len);

/* FUNCTION NAME: perf_rxTest
 * PURPOSE:
 *      To check if Rx-test is going.
 * INPUT:
 *      None
 * OUTPUT:
 *      None
 * RETURN:
 *      CLX_E_OK    -- Successful operation.
 * NOTES:
 *      None
 */
CLX_ERROR_NO_T
perf_rxTest(
    void);

/* FUNCTION NAME: perf_test
 * PURPOSE:
 *      To do Tx-test or Rx-test.
 * INPUT:
 *      len         -- Test length
 *      tx_channel  -- Test Tx channel numbers
 *      rx_channel  -- Test Rx channel numbers
 *      test_skb    -- Test GPD or SKB
 * OUTPUT:
 *      None
 * RETURN:
 *      CLX_E_OK    -- Successful operation.
 * NOTES:
 *      None
 */
CLX_ERROR_NO_T
perf_test(
    UI32_T                      len,
    UI32_T                      tx_channel,
    UI32_T                      rx_channel,
    BOOL_T                      test_skb);

#endif /* end of NETIF_PERF_H */
