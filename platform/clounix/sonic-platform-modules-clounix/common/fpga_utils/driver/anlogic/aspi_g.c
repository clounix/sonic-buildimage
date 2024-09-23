
/*******************************************************************
*
* CAUTION: This file is automatically generated by HSI.
* Version: 2020.2
* DO NOT EDIT.
*
* Copyright (C) 2010-2021 Xilinx, Inc. All Rights Reserved.
* SPDX-License-Identifier: MIT 

* 
* Description: Driver configuration
*
*******************************************************************/

#include "aspi.h"

/*
* The configuration table for devices
*/

ASpi_Config ASpi_ConfigTable[] =
{
	{
		APAR_SPI_0_DEVICE_ID,
		APAR_SPI_0_BASEADDR,
		APAR_SPI_0_FIFO_EXIST,
		APAR_SPI_0_SPI_SLAVE_ONLY,
		APAR_SPI_0_NUM_SS_BITS,
		APAR_SPI_0_NUM_TRANSFER_BITS,
		APAR_SPI_0_SPI_MODE,
		APAR_SPI_0_TYPE_OF_AXI4_INTERFACE,
		APAR_SPI_0_AXI4_BASEADDR,
		APAR_SPI_0_XIP_MODE,
		APAR_SPI_0_USE_STARTUP,
		APAR_SPI_0_FIFO_DEPTH
	},	{
		APAR_SPI_1_DEVICE_ID,
		APAR_SPI_1_BASEADDR,
		APAR_SPI_1_FIFO_EXIST,
		APAR_SPI_1_SPI_SLAVE_ONLY,
		APAR_SPI_1_NUM_SS_BITS,
		APAR_SPI_1_NUM_TRANSFER_BITS,
		APAR_SPI_1_SPI_MODE,
		APAR_SPI_1_TYPE_OF_AXI4_INTERFACE,
		APAR_SPI_1_AXI4_BASEADDR,
		APAR_SPI_1_XIP_MODE,
		APAR_SPI_1_USE_STARTUP,
		APAR_SPI_1_FIFO_DEPTH
	},	{
		APAR_SPI_2_DEVICE_ID,
		APAR_SPI_2_BASEADDR,
		APAR_SPI_2_FIFO_EXIST,
		APAR_SPI_2_SPI_SLAVE_ONLY,
		APAR_SPI_2_NUM_SS_BITS,
		APAR_SPI_2_NUM_TRANSFER_BITS,
		APAR_SPI_2_SPI_MODE,
		APAR_SPI_2_TYPE_OF_AXI4_INTERFACE,
		APAR_SPI_2_AXI4_BASEADDR,
		APAR_SPI_2_XIP_MODE,
		APAR_SPI_2_USE_STARTUP,
		APAR_SPI_2_FIFO_DEPTH
	},{
        APAR_SPI_3_DEVICE_ID,
		APAR_SPI_3_BASEADDR,
		APAR_SPI_3_FIFO_EXIST,
		APAR_SPI_3_SPI_SLAVE_ONLY,
		APAR_SPI_3_NUM_SS_BITS,
		APAR_SPI_3_NUM_TRANSFER_BITS,
		APAR_SPI_3_SPI_MODE,
		APAR_SPI_3_TYPE_OF_AXI4_INTERFACE,
		APAR_SPI_3_AXI4_BASEADDR,
		APAR_SPI_3_XIP_MODE,
		APAR_SPI_3_USE_STARTUP,
		APAR_SPI_3_FIFO_DEPTH
	},{
	    APAR_SPI_4_DEVICE_ID,
		APAR_SPI_4_BASEADDR,
		APAR_SPI_4_FIFO_EXIST,
		APAR_SPI_4_SPI_SLAVE_ONLY,
		APAR_SPI_4_NUM_SS_BITS,
		APAR_SPI_4_NUM_TRANSFER_BITS,
		APAR_SPI_4_SPI_MODE,
		APAR_SPI_4_TYPE_OF_AXI4_INTERFACE,
		APAR_SPI_4_AXI4_BASEADDR,
		APAR_SPI_4_XIP_MODE,
		APAR_SPI_4_USE_STARTUP,
		APAR_SPI_4_FIFO_DEPTH
	}
};
ASpi_Config *ASpi_LookupConfig(u16 DeviceId)
{
	ASpi_Config *CfgPtr = NULL;
	u32 Index;

	for (Index = 0; Index < APAR_SPI_NUM_INSTANCES; Index++) {
		if (ASpi_ConfigTable[Index].DeviceId == DeviceId) {
			CfgPtr = &ASpi_ConfigTable[Index];
			break;
		}
	}

	return CfgPtr;
}
