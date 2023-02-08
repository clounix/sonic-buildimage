#include <stdio.h>
#include <unistd.h>
#include <sys/io.h>
#include "gpio.h"

int  iopl (int level);


#define OUTPUT_LEVEL  1 

#define GPIOBASE  0x500
#define GPIO_USE_SEL  0x00
#define GPIO_IO_SEL   0x04
#define GPIO_LVL      0x0c
#define GPIO_USE_SEL2  0x30
#define GPIO_IO_SEL2   0x34
#define GPIO_LVL2      0x38
#define GPIO_USE_SEL3  0x40
#define GPIO_IO_SEL3   0x44
#define GPIO_LVL3      0x48

#define INDEXPORT4E    0x2E
#define DATAPORT4F     0x2F
#define BIT0 0x01
#define BIT1 0x02
#define BIT2 0x04
#define BIT3 0x08
#define BIT4 0x10
#define BIT5 0x20
#define BIT6 0x40
#define BIT7 0x80
//////////////////////////////////////////////

int gpio_init(void)
{
    int ret = -1;
	unsigned char Dtemp;
    ret = iopl(3);
    if(ret < 0)
    {
       perror("iopl set error");
       return ret;
    }   

	/*printf("demo for gpio read/write...............\n");*/
	Dtemp=inb(GPIOBASE+GPIO_USE_SEL+2);
	
	Dtemp=inb(GPIOBASE+GPIO_IO_SEL+2);	
	Dtemp=inb(GPIOBASE+GPIO_IO_SEL+2); 	 
	Dtemp &=  ~BIT6;   //low
	outb(Dtemp, GPIOBASE+GPIO_IO_SEL+2);  
	Dtemp=inb(GPIOBASE+GPIO_IO_SEL+2);
	
	Dtemp=inb(GPIOBASE+GPIO_LVL+2);
	

		
	Dtemp=inb(GPIOBASE+GPIO_USE_SEL+3);

	Dtemp=inb(GPIOBASE+GPIO_IO_SEL+3);
		
	Dtemp=inb(GPIOBASE+GPIO_LVL+3);


	Dtemp=inb(GPIOBASE+GPIO_USE_SEL3);

	Dtemp=inb(GPIOBASE+GPIO_IO_SEL3); 	

	Dtemp=inb(GPIOBASE+GPIO_IO_SEL3); 	 
	Dtemp &=  ~BIT7;   //low
	outb(Dtemp, GPIOBASE+GPIO_IO_SEL3); 	
	Dtemp=inb(GPIOBASE+GPIO_IO_SEL3); 	
	
	Dtemp=inb(GPIOBASE+GPIO_LVL3);



	Dtemp=inb(GPIOBASE+GPIO_USE_SEL2);
	/*printf("GPIO_USE_SEL2 is %x!\n",Dtemp);	*/
	Dtemp=inb(GPIOBASE+GPIO_IO_SEL2);
	/*printf("GPIO_IO_SEL2 is %x!\n",Dtemp);	*/
	Dtemp=inb(GPIOBASE+GPIO_IO_SEL2); 	 
	Dtemp &=  ~BIT6;   //low
	outb(Dtemp, GPIOBASE+GPIO_IO_SEL2); 	
	Dtemp=inb(GPIOBASE+GPIO_LVL2);
	/*printf("GPIO_LVL2 is %x!\n",Dtemp);*/

    /*set r/w permission of  all 65536 ports*/
    ret = iopl(0);
    if(ret < 0)
    {
        perror("iopl set error");
        return 0;
    } 

    return ret ;
}

int  gpio71_wr(u8 val)
{
    int ret = -1;
	unsigned char Dtemp;
  /*  ret = iopl(3);
    if(ret < 0)
    {
       perror("iopl set error");
       return ret;
    }  */
    if(val){
        Dtemp=inb(GPIOBASE+GPIO_LVL3);
        Dtemp |=  BIT7;
        outb(Dtemp, GPIOBASE+GPIO_LVL3);  
	/*	printf("Set GPIO71 high!\n");*/
    }
    else{
        Dtemp=inb(GPIOBASE+GPIO_LVL3);
        Dtemp &=  ~BIT7; 
        outb(Dtemp, GPIOBASE+GPIO_LVL3);  
	/*	printf("Set GPIO71 low!\n");*/
    }
    /*set r/w permission of  all 65536 ports*/
   /* ret = iopl(0);
    if(ret < 0)
    {
        perror("iopl set error");
        return 0;
    } */

    return ret;
}


int  gpio38_wr(u8 val)
{
    int ret = -1;
	unsigned char Dtemp;
   /* ret = iopl(3);
    if(ret < 0)
    {
       perror("iopl set error");
       return ret;
    }  */
    if(val){
        Dtemp=inb(GPIOBASE+GPIO_LVL2);
        Dtemp |=  BIT6;
        outb(Dtemp, GPIOBASE+GPIO_LVL2);  
   	/*    printf("Set GPIO38 high!\n");	*/
    }
    else{
        Dtemp=inb(GPIOBASE+GPIO_LVL2);
        Dtemp &=  ~BIT6; 
        outb(Dtemp, GPIOBASE+GPIO_LVL2);  
   	/*    printf("Set GPIO38 low!\n");*/
    }
    /*set r/w permission of  all 65536 ports*/
    /*ret = iopl(0);
    if(ret < 0)
    {
        perror("iopl set error");
        return 0;
    } */

    return ret;
}

int  gpio22_wr(u8 val)
{
    int ret = -1;
	unsigned char Dtemp;
    /*ret = iopl(3);
    if(ret < 0)
    {
       perror("iopl set error");
       return ret;
    }  */
    if(val){
        Dtemp=inb(GPIOBASE+GPIO_LVL+2);
        Dtemp |=  BIT6;    //high
        outb(Dtemp, GPIOBASE+GPIO_LVL+2);  
   	/*    printf("Set GPIO22 high!\n");	*/
    }
    else{
        Dtemp=inb(GPIOBASE+GPIO_LVL+2);
        Dtemp &=  ~BIT6;     //high
        outb(Dtemp, GPIOBASE+GPIO_LVL+2);  
   	/*    printf("Set GPIO22 low!\n"); */
    }
    /*set r/w permission of  all 65536 ports*/
    /*ret = iopl(0);
    if(ret < 0)
    {
        perror("iopl set error");
        return 0;
    } */

    return ret;
}

u8  gpio29_rd(void)
{
    int ret = -1;
	u8  Dtemp;
    u8  level;
    /*ret = iopl(3);
    if(ret < 0)
    {
       perror("iopl set error");
       return 0xFF;
    } */ 
         Dtemp=inb(GPIOBASE+GPIO_LVL+3);
        if(Dtemp&BIT5)
 		  level = 1;
        else
          level = 0;

    /*set r/w permission of  all 65536 ports*/
    /*ret = iopl(0);
    if(ret < 0)
    {
        perror("iopl set error");
        return 0;
    } */

    return level;
}

#if 0
//���붨��

int main(int argc, char* argv[])
{
	//unsigned char value;
	u8 Dtemp;
	int ret;
    
    gpio_init();
    
    printf("Set GPIO71 GPIO38 GPIO22 low!\n");
    gpio71_wr(0);

    gpio38_wr(0);

    gpio22_wr(0);
   while(1){
    Dtemp = gpio29_rd();
 
      sleep(3);
      if(Dtemp)
 		  printf("GPIO29 is high!\n");
        else
          printf("GPIO29 is low!\n");
    }
	return 0;

}
#endif
