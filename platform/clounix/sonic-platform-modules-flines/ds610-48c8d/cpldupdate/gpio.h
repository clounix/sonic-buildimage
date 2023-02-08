/*******************************************************************
* Copyright (c) 2011 -2021 Anlogic Inc.
 * The Software is distributed in source code form and is open to
 * re-distribution and modification where applicable
*******************************************************************/
 
typedef unsigned int u32;
typedef unsigned char u8;

#define GPIO_TRI          0x0004
#define GPIO_DATA         0x0000
#define GPIO_INPUT_01BITS 0x0001
#define GPIO_INPUT_32BITS 0xFFFF
#define GPIO_OUPUT        0x0000

#define JTAG_TMS          0x41200000
#define JTAG_TCK          0x41210000
#define JTAG_TDI          0x41220000
#define JTAG_TDO          0x41230000

#define GPIO_MAP_SIZE     0x1000    // Page size is 4Kb

 

volatile u32 *mapped_jtag_tck    ;//   
volatile u32 *mapped_jtag_tdi    ;//   
volatile u32 *mapped_jtag_tdo    ;//   
volatile u32 *mapped_jtag_tms    ;//   
volatile u32 *mapped_cclk        ;//   
volatile u32 *mapped_cfg_data    ;//   
volatile u32 *mapped_cfg_data_dir;//   
volatile u32 *mapped_cfg_data_in ;//   
volatile u32 *mapped_csn         ;//   
volatile u32 *mapped_done        ;//   
volatile u32 *mapped_initn       ;//   
volatile u32 *mapped_prog        ;//   
volatile u32 *mapped_rdwrn       ;//   
volatile u32 *mapped_spicsn_busy ;//   
volatile u32 *mapped_mpms_dma_start;
volatile u32 *mapped_mpms_scale    ;
volatile u32 *mapped_mpms_bak      ;


volatile u32 *mapped_base;

//Linux operation function ----------------------------------------
void Open_LinuxDev(void);
void Close_LinuxDev(void);

void Print_Logo(void);


void TMS_Wr(u8 value);
void TCK_Wr(u8 value);
u8 TDO_Rd(void);
void TDI_Wr(u8 value);
void Cfg_UIO(void);
