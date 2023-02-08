/*******************************************************************
 * Copyright (c) 2011 -2021 Anlogic Inc.
 * This file is strictly confidential. All rights reserved.
*******************************************************************/

#ifndef OPCODE_H
#define OPCODE_H

//#define AJE_DEBUG

//#define CFG_ENABLE

// SVF OpCode
#define  UNKNOWN    0x00
#define  TRST       0x01
#define  ON         0x02
#define  OFF        0x03
#define  STATE      0x04
#define  ENDIR      0x05
#define  ENDDR      0x06
#define  FREQUENCY  0x07
#define  HZ         0x08
#define  TDR        0x09
#define  TIR        0x0A
#define  HDR        0x0B
#define  HIR        0x0C
#define  SDR        0x0D
#define  SIR        0x0E
#define  TDI        0x0F
#define  TDO        0x10
#define  MASK       0x11
#define  SMASK      0x12
#define  RUNTEST    0x13
#define  TCK        0x14
#define  ENDCMD     0x15
#define  LEFTPAREN  0x16
#define  CONTINUE   0x17
#define  WAIT       0x18
#define  XSDR       0x19
#define  XTDI       0x1A
#define  XTDO       0x1B
#define  RSDR       0x1C    /* It means two SDRs and TDI data of second SDR is the same as TDO of first SDR */
#define  HEADER     0x20    /* Add the specified header to AJE file */
#define  COMMENT    0x21    /* Write comments in SVF file into AJE file */
#define  SETFLOW    0x22    /* Change the flow control register. */
#define  RESETFLOW  0x23    /* Clear the flow control register. */
#define  LOOP		0x24
#define  ENDLOOP	0x25
#define  RMASK      0x26

// Header
#define  FILECRC    0x61
#define  MEM        0x62    // The maximum memory needed to allocate in order hold one row of data.
#define  ENDFILE    0x7F    // End of the file
#define  BEGINLINE  0x5E    // Begining of line
#define  ENDLINE    0x24    // End of line

// JTAG OpCode
#define  RESET      0x00
#define  IDLE       0x01
#define  IRSHIFT    0x02
#define  DRSHIFT    0x03
#define  DRPAUSE    0x04
#define  IRPAUSE    0x05
#define  DRSELECT	0x06
#define  IRSELECT	0x07
#define  DRCAPTURE  0x08
#define  IRCAPTURE  0x09
#define  DREXIT1	0x10
#define  IREXIT1    0x11
#define  DREXIT2    0x12
#define  IREXIT2    0x13
#define  DRUPDATE	0x14
#define  IRUPDATE   0x15

// Flow control register bit definitions.
// A set bit indicates that the register currently exhibits the corresponding mode.
#define  INTEL_PRGM 0x0001    /* Intelligent programming is in effect. */
#define  CASCADE    0x0002    /* Currently splitting large SDR. */
#define  REPEATLOOP 0x0008    /* Currently executing a repeat loop. */
#define  SHIFTRIGHT 0x0080    /* The next data stream needs a right shift. */
#define  SHIFTLEFT  0x0100    /* The next data stream needs a left shift. */
#define  VERIFYUES  0x0200    /* Continue if fail is in effect. */


// Vendor Code
#define  VENDOR     0x56
#define  ANLOGIC    0x01
#define  XILINX     0x02
#define  ALTERA     0x03
#define  LATTICE    0x04

// Mode Code
#define  COMP_MODE  0xF1    // compress mode
#define  FULL_MODE  0xF2    // full mode

// SIR Command
#define  BYPASS     0xFF
#define  SAMPLE     0x05
#define  IDCODE_PUB 0x06
#define	 IDCODE_PUB_2   0xE0

// Reading IO Status
#define LINE_MAX_SIZE   1000 /* max size of a line when reading svf file */
#define DATA_MAX_SIZE   300  /* max size of tdi/tdo/mask hex str in svf file */

// return type 
#define AJE_OK                       0
#define AJE_WARNING                 -1
#define AJE_ERROR                   -2
#define AJE_FILE_OPEN_FAIL          -6
#define AJE_FILE_INVALID            -7
#define AJE_INVALID_COMMAND         -10
#define AJE_INVALID_VALUE           -11
#define AJE_INVALID_TAP_STATE       -12
#define AJE_CHIP_VALIDATION_FAIL    -13
#define AJE_TRANSFER_FAIL           -16
#define AJE_VERIFY_FAIL             -17

/* TAP STATE Enumerate */
enum TAP_STATE {
    TAP_RESET	= 0,  /* Initialization state */
    TAP_IDLE = 1,
    TAP_DRSELECT = 2, /* DR STATE: 2~8 */
    TAP_DRCAPTURE = 3,
    TAP_DRSHIFT = 4,
    TAP_DREXIT1 = 5,
    TAP_DRPAUSE = 6,
    TAP_DREXIT2 = 7,
    TAP_DRUPDATE = 8,
    TAP_IRSELECT = 9, /* IR STATE: 9~15 */
    TAP_IRCAPTURE = 10,
    TAP_IRSHIFT = 11,
    TAP_IREXIT1 = 12,
    TAP_IRPAUSE = 13,
    TAP_IREXIT2 = 14,
    TAP_IRUPDATE = 15,
};

#endif // OPCODE_H
