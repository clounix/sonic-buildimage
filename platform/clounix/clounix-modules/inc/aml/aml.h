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

/* FILE NAME:  aml.h
 * PURPOSE:
 *  1. Provide whole AML resource initialization API.
 *  2. Provide configuration access APIs.
 *  3. Provide ISR registration and deregistration APIs.
 *  4. Provide memory access.
 *  5. Provide DMA management APIs.
 *  6. Provide address translation APIs.
 * NOTES:
 */

#ifndef AML_H
#define AML_H


/* INCLUDE FILE DECLARATIONS
 */
#include <clx_types.h>
#include <clx_error.h>
/* NAMING CONSTANT DECLARATIONS
 */


/* MACRO FUNCTION DECLARATIONS
 */

/* DATA TYPE DECLARATIONS
 */
typedef enum
{
    AML_DEV_TYPE_PCI,
    AML_DEV_TYPE_I2C,
    AML_DEV_TYPE_SPI,
    AML_DEV_TYPE_LAST

} AML_HW_IF_T;

typedef CLX_ERROR_NO_T
(*AML_DEV_READ_FUNC_T)(
    const UI32_T    unit,
    const UI32_T    addr_offset,
    UI32_T          *ptr_data,
    const UI32_T    len);

typedef CLX_ERROR_NO_T
(*AML_DEV_WRITE_FUNC_T)(
    const UI32_T    unit,
    const UI32_T    addr_offset,
    const UI32_T    *ptr_data,
    const UI32_T    len);

typedef CLX_ERROR_NO_T
(*AML_DEV_ISR_FUNC_T)(
    void            *ptr_data);

/* To mask the chip interrupt in kernel interrupt routine. */
typedef struct
{
    UI32_T                  mask_addr;
    UI32_T                  mask_val;

} AML_DEV_ISR_DATA_T;

typedef struct
{
    UI32_T                          unit;
    UI32_T                          msi;
    UI32_T                          valid;
} AML_DEV_MSI_DATA_T;

/* To read or write the HW-intf registers. */
typedef struct
{
    AML_DEV_READ_FUNC_T     read_callback;
    AML_DEV_WRITE_FUNC_T    write_callback;

} AML_DEV_ACCESS_T;

typedef struct
{
    UI32_T                  vendor;
    UI32_T                  device;
    UI32_T                  revision;
} AML_DEV_ID_T;

typedef struct
{
    AML_HW_IF_T             if_type;
    AML_DEV_ID_T            id;
    AML_DEV_ACCESS_T        access;

} AML_DEV_T;

/* EXPORTED SUBPROGRAM SPECIFICATIONS
 */

/* FUNCTION NAME:   aml_getRunMode
 * PURPOSE:
 *      To get current SDK running mode.
 * INPUT:
 *      unit        -- the device unit
 * OUTPUT:
 *      ptr_mode    -- current running mode
 * RETURN:
 *      CLX_E_OK    -- Successfully get the running mode.
 * NOTES:
 *      none
 */
CLX_ERROR_NO_T
aml_getRunMode(
    const UI32_T     unit,
    UI32_T           *ptr_mode);

/* FUNCTION NAME:   aml_init
 * PURPOSE:
 *      To initialize the DMA memory and interface-related kernel source
 *      such as PCIe/I2C/SPI.
 * INPUT:
 *      none
 * OUTPUT:
 *      none
 * RETURN:
 *      CLX_E_OK        -- Successfully initialize AML module.
 *      CLX_E_OTHERS    -- Failed to initialize AML module.
 * NOTES:
 *      none
 */
CLX_ERROR_NO_T
aml_deinit(void);

/* FUNCTION NAME:   aml_init
 * PURPOSE:
 *      To initialize the DMA memory and interface-related kernel source
 *      such as PCIe/I2C/SPI.
 * INPUT:
 *      none
 * OUTPUT:
 *      none
 * RETURN:
 *      CLX_E_OK        -- Successfully initialize AML module.
 *      CLX_E_OTHERS    -- Failed to initialize AML module.
 * NOTES:
 *      none
 */
CLX_ERROR_NO_T
aml_init(void);

/* FUNCTION NAME:   aml_getNumberOfChip
 * PURPOSE:
 *      To get the number of chips connected to host CPU.
 * INPUT:
 *      none
 * OUTPUT:
 *      ptr_num     -- pointer for the chip number
 * RETURN:
 *      CLX_E_OK    -- Successfully get the number of chips.
 * NOTES:
 *      none
 */
CLX_ERROR_NO_T
aml_getNumberOfChip(
    UI32_T  *ptr_num);

/* FUNCTION NAME:   aml_connectIsr
 * PURPOSE:
 *      To enable the system intterupt and specify the ISR handler.
 * INPUT:
 *      unit        -- the device unit
 *      handler     -- the ISR hanlder
 *      ptr_cookie  -- pointer for the data as an argument of the handler
 * OUTPUT:
 *      none
 * RETURN:
 *      CLX_E_OK     -- Successfully connect the ISR handler to the system.
 *      CLX_E_OTHERS -- Failed to connect the ISR handler to the system.
 * NOTES:
 *      none
 */
CLX_ERROR_NO_T
aml_connectIsr(
    const UI32_T        unit,
    AML_DEV_ISR_FUNC_T  handler,
    AML_DEV_ISR_DATA_T  *ptr_cookie);

/* FUNCTION NAME:   aml_disconnectIsr
 * PURPOSE:
 *      To disable the system intterupt notification.
 * INPUT:
 *      unit        -- the device unit
 * OUTPUT:
 *      none
 * RETURN:
 *      CLX_E_OK     -- Successfully disconnect the ISR handler to the system.
 *      CLX_E_OTHERS -- Failed to disconnect the ISR handler to the system.
 * NOTES:
 *      none
 */
CLX_ERROR_NO_T
aml_disconnectIsr(
    const UI32_T unit);

/* FUNCTION NAME:   aml_getDeviceId
 * PURPOSE:
 *      To get the vendor/device/revision ID of the specified chip unit.
 * INPUT:
 *      unit        -- the device unit
 * OUTPUT:
 *      ptr_vendor_id   -- pointer for the vendor ID
 *      ptr_device_id   -- pointer for the device ID
 *      ptr_revision_id -- pointer for the revision ID
 * RETURN:
 *      CLX_E_OK     -- Successfully get the IDs.
 *      CLX_E_OTHERS -- Failed to get the IDs.
 * NOTES:
 *      none
 */
CLX_ERROR_NO_T
aml_getDeviceId(
    const UI32_T    unit,
    UI32_T          *ptr_vendor_id,
    UI32_T          *ptr_device_id,
    UI32_T          *ptr_revision_id);

/* FUNCTION NAME:   aml_readReg
 * PURPOSE:
 *      To read data from the register of the specified chip unit.
 * INPUT:
 *      unit        -- the device unit
 *      addr_offset -- the address of register
 *      len         -- data size read
 * OUTPUT:
 *      ptr_data    -- pointer for the register data
 * RETURN:
 *      CLX_E_OK     -- Successfully read the data.
 *      CLX_E_OTHERS -- Failed to read the data.
 * NOTES:
 *      none
 */
CLX_ERROR_NO_T
aml_readReg(
    const UI32_T    unit,
    const UI32_T    addr_offset,
    UI32_T          *ptr_data,
    const UI32_T    len);

/* FUNCTION NAME:   aml_writeReg
 * PURPOSE:
 *      To write data to the register of the specified chip unit.
 * INPUT:
 *      unit        -- the device unit
 *      addr_offset -- the address of register
 *      ptr_data    -- pointer for the written data
 *      len         -- data size read
 * OUTPUT:
 *      none
 * RETURN:
 *      CLX_E_OK     -- Successfully write the data.
 *      CLX_E_OTHERS -- Failed to write the data.
 * NOTES:
 *      none
 */
CLX_ERROR_NO_T
aml_writeReg(
    const UI32_T    unit,
    const UI32_T    addr_offset,
    const UI32_T    *ptr_data,
    const UI32_T    len);

/* FUNCTION NAME:   aml_convertVirtToPhy
 * PURPOSE:
 *      To get the physical address of the corresponding virtual
 *      address input.
 * INPUT:
 *      ptr_virt_addr   -- pointer to the virtual address
 * OUTPUT:
 *      ptr_phy_addr    -- pointer to the physical address
 * RETURN:
 *      CLX_E_OK     -- Successfully convert the address.
 *      CLX_E_OTHERS -- Failed to convert the address.
 *                      The memory might be not allocated by AML.
 * NOTES:
 *      none
 */
CLX_ERROR_NO_T
aml_convertVirtToPhy(
    void            *ptr_virt_addr,
    CLX_ADDR_T      *ptr_phy_addr);

/* FUNCTION NAME:   aml_convertPhyToVirt
 * PURPOSE:
 *      To get the virtual address of the corresponding physical
 *      address input.
 * INPUT:
 *      ptr_virt_addr   -- pointer for the physical address
 * OUTPUT:
 *      pptr_virt_addr  -- pointer for the virtual address pointer
 * RETURN:
 *      CLX_E_OK     -- Successfully convert the address.
 *      CLX_E_OTHERS -- Failed to convert the address.
 *                      The memory might be not allocated by AML.
 * NOTES:
 *      none
 */
CLX_ERROR_NO_T
aml_convertPhyToVirt(
    const CLX_ADDR_T    phy_addr,
    void                **pptr_virt_addr);

/* FUNCTION NAME:   aml_flushCache
 * PURPOSE:
 *      To update the data from CPU cache to the physical memory.
 * INPUT:
 *      ptr_virt_addr   -- pointer for the data
 *      size            -- target data size to be updated
 * OUTPUT:
 *      none
 * RETURN:
 *      CLX_E_OK     -- Successfully update the data from CPU cache
 *                      to the physical memory.
 *      CLX_E_OTHERS -- Failed to pdate the data from CPU cache
 *                      to the physical memory.
 * NOTES:
 *      none
 */
CLX_ERROR_NO_T
aml_flushCache(
    void            *ptr_virt_addr,
    const UI32_T    size);

/* FUNCTION NAME:   aml_invalidateCache
 * PURPOSE:
 *      To update the data from physical memory to the CPU cache.
 * INPUT:
 *      ptr_virt_addr   -- pointer for the data
 *      size            -- target data size to be updated
 * OUTPUT:
 *      none
 * RETURN:
 *      CLX_E_OK     -- Successfully update the data from physical memory
 *                      to the CPU cache.
 *      CLX_E_OTHERS -- Failed to pdate the data from physical memory
 *                      to the CPU cache.
 * NOTES:
 *      none
 */
CLX_ERROR_NO_T
aml_invalidateCache(
    void            *ptr_virt_addr,
    const UI32_T    size);


/* FUNCTION NAME:   aml_savePciConfig
 * PURPOSE:
 *      To save pci configuration space data.
 * INPUT:
 *      unit        -- the device unit
 * OUTPUT:
 *      none
 * RETURN:
 *      CLX_E_OK     -- Successfully save the pci configuration space data.
 *      CLX_E_OTHERS -- Failed to save the pci configuration space data.
 * NOTES:
 *      none
 */
CLX_ERROR_NO_T
aml_savePciConfig(
    const UI32_T    unit);


/* FUNCTION NAME:   aml_restorePciConfig
 * PURPOSE:
 *      To restore the previously saved pci configuration space data.
 * INPUT:
 *      unit        -- the device unit
 * OUTPUT:
 *      none
 * RETURN:
 *      CLX_E_OK     -- Successfully restore the pci configuration space data.
 *      CLX_E_OTHERS -- Failed to restore the pci configuration space data.
 * NOTES:
 *      none
 */
CLX_ERROR_NO_T
aml_restorePciConfig(
    const UI32_T    unit);

#endif  /* #ifndef AML_H */
