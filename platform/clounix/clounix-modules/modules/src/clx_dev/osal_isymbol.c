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

/* FILE NAME:  osal_isymbol.c
* PURPOSE:
*      It provide global OSAL symbol export for linux kernel module
* NOTES:
*/
#include <linux/init.h>
#include <linux/module.h>

/* ----------------------------------------------------- */
#include <osal/osal_mdc.h>
/* dma */
extern struct pci_dev   *_ptr_ext_pci_dev;
EXPORT_SYMBOL(_ptr_ext_pci_dev);

#if defined(CLX_LINUX_KERNEL_MODE)
#include <clx_init.h>
extern CLX_INIT_WRITE_FUNC_T   _ext_dsh_write_func;
EXPORT_SYMBOL(_ext_dsh_write_func);
#endif

#if defined(CLX_LINUX_USER_MODE)
EXPORT_SYMBOL(osal_mdc_readPciReg);
EXPORT_SYMBOL(osal_mdc_writePciReg);
#if defined(CLX_EN_NETIF)
/* intr */
/* for kernel module, this API will be exported by script with other OSAL functions in osal_symbol.c */
EXPORT_SYMBOL(osal_mdc_registerIsr);
#endif
#endif
