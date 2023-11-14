//**********************************************************************
//**********************************************************************
//**                                                                  **
//**        (C)Copyright 1985-2019, American Megatrends, Inc.         **
//**                                                                  **
//**                       All Rights Reserved.                       **
//**                                                                  **
//**      5555 Oakbrook Parkway, Suite 200, Norcross, GA 30093        **
//**                                                                  **
//**                       Phone: (770)-246-8600                      **
//**                                                                  **
//**********************************************************************
//**********************************************************************

//**********************************************************************
//<AMI_FHDR_START>
//
// Name: amifldrv.h
//
// Description: AMI Linux Generic Driver header file
//
//<AMI_FHDR_END>
//**********************************************************************

#ifndef _AMIFLDRVDEFS__H_
#define _AMIFLDRVDEFS__H_

#define CMD_ALLOC				0x4160
#define CMD_FREE				0x4161
#define CMD_LOCK_KB				0x4162
#define CMD_UNLOCK_KB			0x4163
#define CMD_IOWRITE_BYTE		0x4164
#define CMD_IOWRITE_WORD		0x4165
#define CMD_IOWRITE_DWORD		0x4166
#define CMD_IOREAD_BYTE			0x4167
#define CMD_IOREAD_WORD			0x4168
#define CMD_IOREAD_DWORD		0x4169
#define CMD_GET_DRIVER_INFO		0x416A

/*
 * 2. Data structures
 *
 */
#pragma pack(1)
typedef struct tagAMIFLDRV_ALLOC
{
	long			size;
	unsigned long	kvirtlen;
	void			*kmallocptr;
	void			*kvirtadd;
	void			*kphysadd;
} AMIFLDRV_ALLOC;

typedef struct tagAMIFLDRV_CPU_CONTEXT
{
	unsigned int		Edi;
	unsigned int		Esi;
	unsigned int		Ebp;
	unsigned int		Ebx;
	unsigned int		Edx;
	unsigned int		Ecx;
	unsigned int		Eax;
} AMIFLDRV_CPU_CONTEXT;

typedef struct tagAMIFLDRV_PORTRW
{
	union {
		unsigned char		ValueByte;
		unsigned short		ValueWord;
		unsigned int		ValueDword;
	} Value;
	unsigned short			Port;
	AMIFLDRV_CPU_CONTEXT	CpuContext;
} AMIFLDRV_PORTRW;

typedef struct tagAMIDRV_INFO
{
	unsigned int	InfoVersion;
	unsigned int	InfoLength;
	unsigned int	Major;
	unsigned int	Minor;
} AMIDRV_INFO;
#pragma pack()

#endif	// _AMIFLDRVDEFS__H_

//**********************************************************************
//**********************************************************************
//**                                                                  **
//**        (C)Copyright 1985-2019, American Megatrends, Inc.         **
//**                                                                  **
//**                       All Rights Reserved.                       **
//**                                                                  **
//**      5555 Oakbrook Parkway, Suite 200, Norcross, GA 30093        **
//**                                                                  **
//**                       Phone: (770)-246-8600                      **
//**                                                                  **
//**********************************************************************
//**********************************************************************
