/*******************************************************************
* Copyright (c) 2011 -2021 Anlogic Inc.
 * The Software is distributed in source code form and is open to
 * re-distribution and modification where applicable
*******************************************************************/
 
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <malloc.h>
#include <time.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/mman.h>
#include <errno.h>
#include "gpio.h"
#include <sys/io.h>
 
extern void Anlogic_ProcessRunTestTck(int num);
extern int gpio_init(void);
extern int  gpio71_wr(u8 val);   /*TCK*/
extern int  gpio38_wr(u8 val);   /*TDI*/
extern int  gpio22_wr(u8 val);   /*TMS*/
extern u8  gpio29_rd(void);      /*TDO*/

// Set GPIO Direction*******************************************
void GPIO_SetDir(volatile u32 *Gpio_Baseaddr, volatile u32 Value)
{
	*(Gpio_Baseaddr + GPIO_TRI) = Value;
}

 
//TMS Write function
void TMS_Wr(u8 value)
{
	gpio22_wr(value);
}
//TCK Write function
void TCK_Wr(u8 value)
{
	gpio71_wr(value);
}
//TDI Write function
void TDI_Wr(u8 value)
{
	gpio38_wr(value);
}
//TDO Read function
u8 TDO_Rd(void)
{
	return gpio29_rd();
}



void Cfg_UIO(void)
{
	printf( "\n\n" );
    printf(" *********************************************************************************************** \n\r" );		
	printf(" ***********************************   Open Linux Device  ************************************** \n\r" );	
	// From Linux DTS 


 #if 0
    fd00_jtag_tck      = open("/dev/uio0" ,O_RDWR); 
	mapped_base = mmap(0, GPIO_MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd00_jtag_tck      , 0);
	if (mapped_base == (u32 *)-1)  
		printf("\n Error opening JTAG_TCK     device file \n");
	else 
		printf("\n Opened device JTAG_TCK     file successfully %d\n",fd00_jtag_tck    );
	mapped_jtag_tck    = mapped_base ;  printf("\n JTAG_TCK    :\n Memory mapped at address %p.\n", mapped_jtag_tck    );  


	
    fd01_jtag_tdi      = open("/dev/uio1" ,O_RDWR); 
	mapped_base = mmap(0, GPIO_MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd01_jtag_tdi      , 0);  
	if (mapped_base == (u32 *)-1)  
		printf("\n Error opening JTAG_TDI     device file \n");
	else 
		printf("\n Opened device JTAG_TDI     file successfully %d\n",fd01_jtag_tdi    );
	mapped_jtag_tdi    = mapped_base ;  printf("\n JTAG_TDI    :\n Memory mapped at address %p.\n", mapped_jtag_tdi    );	
	
	
	
    fd02_jtag_tdo      = open("/dev/uio2" ,O_RDWR); 
	mapped_base = mmap(0, GPIO_MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd02_jtag_tdo      , 0);
	if (mapped_base == (u32 *)-1)  
		printf("\n Error opening JTAG_TDO     device file \n");
	else 
		printf("\n Opened device JTAG_TDO     file successfully %d\n",fd02_jtag_tdo    );
	mapped_jtag_tdo    = mapped_base ;  
	printf("\n JTAG_TDO    :\n Memory mapped at address %p.\n", mapped_jtag_tdo    );	
	
	
	
    fd03_jtag_tms      = open("/dev/uio3" ,O_RDWR); 
	mapped_base = mmap(0, GPIO_MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd03_jtag_tms      , 0);
	if (mapped_base == (u32 *)-1)  
		printf("\n Error opening JTAG_TMS     device file \n");
	else 
		printf("\n Opened device JTAG_TMS     file successfully %d\n",fd03_jtag_tms    ); 
	mapped_jtag_tms    = mapped_base ;  printf("\n JTAG_TMS    :\n Memory mapped at address %p.\n", mapped_jtag_tms    );	
	
 

 	printf("\n JTAG_TCK         GPIO Direction JTAG_TCK      : -> 0x0000 Output\n\r ");
 	printf("\n JTAG_TDI         GPIO Direction JTAG_TDI      : -> 0x0000 Output\n\r ");
 	printf("\n JTAG_TDO         GPIO Direction JTAG_TDO      : -> 0xFFFF Input \n\r ");
 	printf("\n JTAG_TMS         GPIO Direction JTAG_TMS      : -> 0x0000 Output\n\r ");



 	
    GPIO_SetDir(mapped_jtag_tck    ,0x0000   );//offset=0x00, 0=Output 1=Input;
    GPIO_SetDir(mapped_jtag_tdi    ,0x0000   );//offset=0x00, 0=Output 1=Input;
    GPIO_SetDir(mapped_jtag_tdo    ,0xFFFF   );//offset=0x00, 0=Output 1=Input;
    GPIO_SetDir(mapped_jtag_tms    ,0x0000   );//offset=0x00, 0=Output 1=Input;
#endif

    gpio_init();

	printf(" *********************************************************************************************** \n\r" );	
	printf(" ************************************ JTAG port init done ************************************** \n\r" );	
	printf( "\n\n" );

}


//SVF TCK default Freq: 1MHz��
//RunTest Delay= the Number * 1us
void Anlogic_Calibration(void)
{
    int ret = -1;
    ret = iopl(3);
    if(ret < 0)
    {
       perror("iopl set error");
       return ;
    }  
	/*Apply 2 pulses to TCK.*/
	TCK_Wr( 0x01 );
	TCK_Wr( 0x01 );
	TCK_Wr( 0x00 );
	TCK_Wr( 0x01 );
	TCK_Wr( 0x00 );
	TCK_Wr( 0x01 );
	TCK_Wr( 0x01 );
	/*Delay for 1 millisecond. Pass on 1000 = 1ms delay.*/

	Anlogic_ProcessRunTestTck(1000000) ;
	/*Apply 2 pulses to TCK*/
	TCK_Wr( 0x01 );
	TCK_Wr( 0x01 );
	TCK_Wr( 0x00 );
	TCK_Wr( 0x01 );
	TCK_Wr( 0x00 );
	TCK_Wr( 0x01 );
	TCK_Wr( 0x01 );
    ret = iopl(0);
    if(ret < 0)
    {
        perror("iopl set error");
        return ;
    } 
}

void Close_LinuxDev(void)
{
	printf( "\n\n" );
	printf(" *********************************************************************************************** \n\r" );		
	printf(" *********************************************************************************************** \n\r" );		
	printf(" ***********************************  Close Linux Device  ************************************** \n\r" );	 
#if 0
	close(fd00_jtag_tck    );
	close(fd01_jtag_tdi    );
	close(fd02_jtag_tdo    );
	close(fd03_jtag_tms    );
#endif

	
    printf(" ---- Close All Device.\n");
	printf(" *********************************************************************************************** \n\r" );		
	printf(" *********************************************************************************************** \n\r" );		
	printf( "\n\n" );    
}
