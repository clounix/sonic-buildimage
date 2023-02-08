/*******************************************************************
 * Copyright (c) 2011 -2021 Anlogic Inc.
 * The Software is distributed in source code form and is open to
 * re-distribution and modification where applicable
 *******************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>    
#include <unistd.h>
#include <sys/io.h>
#include "opcode.h"
#include "gpio.h"
#include "string.h"
#include "sys/ioctl.h"
#include "fcntl.h"


#define PCA9555_PATH	"/dev/pca9555"
#define FAN_BOARD  1
#define MAIN_BOARD  2
#define UPGRADE_END  3
/* function declared in aje2vec.c */
extern int Anlogic_AjeToVec(const char* aje_file);

extern int Anlogic_AjeRdbkValue(unsigned int* len, char*** val_list);
extern void Anlogic_Calibration(void);
/* valiables used to read back tdo data */
unsigned int val_len = 0;
char** val_list = NULL;

struct data_msg {
       unsigned char reg;
       unsigned char val;
};

/* reports error information */
void Anlogic_ReportError(int rtn_val) {
    switch (rtn_val) {
    case AJE_FILE_OPEN_FAIL:
        printf("Error: can not open the file\n");
        break;
    case AJE_INVALID_COMMAND:
        printf("Error: invalid command appear\n");
        break;
    case AJE_INVALID_VALUE:
        printf("Error: invalid data appear\n");
        break;
    case AJE_INVALID_TAP_STATE:
        printf("Error: invalid tap state appear\n");
        break;
    case AJE_CHIP_VALIDATION_FAIL:
        printf("Error: chip validation failed, please check the connnection\n");
        break;
    case AJE_TRANSFER_FAIL:
        printf("Error: data tranfer failed\n");
        break;
    case AJE_VERIFY_FAIL:
        printf("Error: data verify failed\n");
        break;
    default:
        printf("Unknown Error\n");
        break;
    }
}

void Anlogic_PrintRdbkValue(void) {
    unsigned int id  = 0;
    if (val_len == 0 || val_list == NULL) {
        return;
    }
    printf("RECEIVED TDO \n");
    for (; id < val_len; ++id) {
        printf("%s",val_list[id]);
        printf("\n");
    }
    printf("\n");
}

int config_jtag_switch(int type){
	int fd;
	unsigned char databuf[8];
/*	unsigned short inportreg, outportreg, dirreg;*/
	int ret = 0;
    struct data_msg writebuf[8];
   
	fd = open(PCA9555_PATH, O_RDWR);
	if(fd < 0) {
		printf("can't open file %s\r\n", PCA9555_PATH);
		return -1;
	}

    memset(databuf, 0, sizeof(databuf));
	ret = read(fd, databuf, 8);
#if 0
	if(ret == 0) { 			/* 数据读取成功 */
		inportreg =  (databuf[1] << 8)|databuf[0]; 	/* input port register数据 */
		outportreg = (databuf[3] << 8)|databuf[2]; 	/* output port register数据 */
		dirreg  =  (databuf[7] << 8)|databuf[6]; 	/* port direction config数据 */
		printf("inportreg = 0x%x, outportreg = 0x%x, dirreg = 0x%x\r\n", inportreg, outportreg, dirreg);
	}
#endif
	usleep(20000); /*100ms */

    /*printf("Config PCA9555 register!");*/
    
	writebuf[0].reg =  7;      
	writebuf[0].val = (databuf[7] & 0xFC);    /*config P1.1 and P1.0 pin as outport*/
	writebuf[1].reg =  3;  
	if(type==FAN_BOARD)    
		writebuf[1].val = (databuf[3] & 0xFC)|0x02;    /*enable FAN CPLD program update*/    
    else if(type==MAIN_BOARD)
        writebuf[1].val = (databuf[3] & 0xFC)|0x01;    /*enable FAN CPLD program update*/
    else if(type==UPGRADE_END)
        writebuf[1].val = (databuf[3] & 0xFC)|0x03;    /* disable CPLD jtag*/
	else {
		 close(fd);	/* 关闭文件 */	
         return -1;
	}
	 
    ret = write(fd, writebuf, 2);
    if(ret != 2)
        printf("write fail.\r\n");
        
	close(fd);	/* 关闭文件 */

    return 0;
}


int main(int argc, char *argv[])
{
    /* Input AJE file */
    u8 val=0;
    int bdtype=0;
    const char* aje_file = "test.aje";
	int rtn_code = AJE_OK;
    int ret_io = -1;
	double duration = 0.0;
	clock_t start, finish;  

	start = clock(); 
  
    Cfg_UIO();  /*Init jtag port*/

	aje_file = argv[1];
	printf(   "Input File :   %s\n"   ,   argv[1]   );

    if (argc != 3) {
        printf("Error: parameters not enough!\r\n");
        return -1;
    }

	if(strcmp(argv[2],"fb")==0){    
		bdtype = FAN_BOARD;    /*enable FAN CPLD program update*/  
        printf("Update fan board cpld grogram....\r\n");
        }  
    else if(strcmp(argv[2],"mb")==0){
        bdtype = MAIN_BOARD;    /*enable FAN CPLD program update*/
        printf("Update main board cpld grogram....\r\n");
        }
	else {
         printf("Update cpld code fail,input error parameters\r\n");
         return 0;
	}
    config_jtag_switch(bdtype);
  /*  Anlogic_Calibration();*/
#if 1
    ret_io = iopl(3);
    if(ret_io < 0)
    {
       perror("iopl set error");
       return ret_io;
    }  
	rtn_code = Anlogic_AjeToVec(aje_file);
    if (rtn_code != AJE_OK) {
        Anlogic_ReportError(rtn_code);
    } else {
        /* get the tdo readback value */
        Anlogic_AjeRdbkValue(&val_len, &val_list);

        /* print the tdo readback value */
        Anlogic_PrintRdbkValue();
    }
    ret_io = iopl(0);
    if(ret_io < 0)
    {
        perror("iopl set error");
    }     
#endif
    config_jtag_switch(UPGRADE_END);   /*disable jtag*/

	finish = clock();   
	duration = (double)(finish - start) / CLOCKS_PER_SEC; 
	printf("Elapsed time (%f seconds)\n\n", duration);  

	if (rtn_code == AJE_OK) {
		printf("Success!\n\n");
	} else {
		printf("Fail!\n\n");
	}

	return rtn_code;
}
