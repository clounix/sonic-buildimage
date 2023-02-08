/*******************************************************************
 * Copyright (c) 2011 -2021 Anlogic Inc.
 * The Software is distributed in source code form and is open to
 * re-distribution and modification where applicable
*******************************************************************/

#define _CRT_SECURE_NO_WARNINGS

#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <assert.h>

#include "opcode.h"
//#define AJE_DEBUG
/* jtag instruction macro list */
#define INS_REFRESH     0x01
#define INS_SAMPLE      0x05
#define INS_READ_STATUS 0x20
#define INS_BYPASS      0xFF

#define INS_PROG_SPI    0x39

extern unsigned int g_hdr_size;
extern unsigned int g_hir_size;
extern unsigned int g_tdr_size;
extern unsigned int g_tir_size;

extern unsigned char* g_hdr_data;
extern unsigned char* g_hir_data;
extern unsigned char* g_tdr_data;
extern unsigned char* g_tir_data;

/* some static & global variables */
static char s_compress = FULL_MODE;     /* compress mode */
static long int s_cfg_start_pos = 0;    /* the start cfg position in aje file */
static int s_done_status = 1;           /* done pin status */
static int s_find_refresh_ins = 0;      /* the flag that refresh instrcution has been found or not */
static int s_cur_level   = 0;           /* for daisy chain, current level */
static int s_total_level = 0;           /* for daisy chain, total level */

FILE*       g_aje_file;
char        g_aje_crc[3];
long int    g_freq = 0;
unsigned char s_idcode_pub = IDCODE_PUB;
static unsigned int s_cascade = 0;      /* cascade flag */

/* function declared in decode.c */
extern unsigned char* Anlogic_BytesDecode(long bytes);
extern unsigned char* Anlogic_NibbleDecode(long bytes);
extern unsigned char* Anlogic_HuffmanDecode(long bytes);
extern unsigned char* Anlogic_LzwDecode(long bytes);

/* function declared in ajeutil.c */
extern int Anlogic_GetBit(unsigned char* data, int id);
extern void Anlogic_SetBit(unsigned char* data, int id, int val);
extern char* Anlogic_HexStrToBinStr(char* src_str, int bin_len);
extern unsigned char Anlogic_ReverseChar(unsigned char byte);

/* function declared in vec.c */
extern enum TAP_STATE Anlogic_TapState(unsigned char opcode);
extern const char* Anlogic_TapState2Str(enum TAP_STATE tap_state);
extern void Anlogic_SetEndDRState(enum TAP_STATE end_st);
extern void Anlogic_SetEndIRState(enum TAP_STATE end_st);
extern int Anlogic_TapTransist(enum TAP_STATE state);
extern int Anlogic_ExeSirCommand(unsigned char command, int cur_lvl, int total_lvl);
extern int Anlogic_ExeSdrCommand(unsigned int data_size, int cur_lvl, int total_lvl,
                                 unsigned char* tdi_data, unsigned char* tdo_data);
extern void Anlogic_Init(void);
extern void Anlogic_ProcessRunTestTck(int num);
extern int Anlogic_SendData(unsigned char* tdi_data, unsigned int data_size, int cascade);
extern int Anlogic_SaveData(unsigned char* tdi_data, unsigned int data_size, unsigned char** tdo_data);
extern int Anlogic_ProcessShiftCmd(unsigned int op_code,
                                   unsigned int cascade,
                                   unsigned int read,
                                   unsigned int data_size,
                                   unsigned char* tdi_data,
                                   unsigned char* tdo_data,
                                   unsigned char* mask,
                                   unsigned char* rmask);

/* function forward declaration */
int Anlogic_ExeSirCommands(unsigned int data_size, unsigned char* tdi_data, unsigned char* tdo_data, unsigned char* mask);
int Anlogic_ExeSDRCommands(unsigned int cascade, unsigned int data_size,
                           unsigned char* tdi_data, unsigned char* tdo_data, unsigned char* mask);
int Anlogic_ExeRSDRCommands(unsigned int cascade, unsigned int data_size,
    unsigned char* tdi_data, unsigned char* rmask_data);
/* list, use for saving the tdo output value*/
struct RdbkTdoData {
    unsigned int len_;
    unsigned char* data_;
    struct RdbkTdoData* next_;
};
static struct RdbkTdoData  rdbk_tdo_data = { 0, NULL, NULL }; // the start node
static struct RdbkTdoData* rdbk_tdo_data_p = &rdbk_tdo_data;  // point to current node

long Anlogic_ReadNumber()
{
    long number = 0;
    char byte = 0, cnt = 0;
    do {
        byte = fgetc(g_aje_file);
        if (byte == EOF) { break; }
        number += (byte & 0x007F) << (7 * cnt);
        cnt ++;
    } while (byte & 0x0080);
    return number;
}

unsigned long Anlogic_ProcessRuntestCmd()
{
    unsigned long ulTime = 0;
    unsigned long num = 0;
    char opcode = 0;
    char pre_opcode = 0;
    char done = 0;

    if (g_freq == 0) {
        g_freq = 1000000;
    }

    do {
        pre_opcode = opcode;
        opcode = fgetc(g_aje_file);
        switch (opcode) {
        case TCK:
            num = Anlogic_ReadNumber();
            if (num > 0x8000) { num -= 0x8000; }
            ulTime += num;
            ulTime -= 10;
            break;
        case WAIT:
            num = Anlogic_ReadNumber();
            num -= 10;
            if (num > 0x8000) {
                num -= 0x8000;
                num = g_freq * num / 1000 ;
            } else {
                num = g_freq * num / 1000000 ;
            }
            if (TCK == pre_opcode) {
                ulTime = 0;
            }
            ulTime += num;
            break;
        case RUNTEST:
            break;
        default:
            done = 1;
            break;
        }
    } while (!done);

    if (opcode == BEGINLINE) {
        fseek(g_aje_file, -1, SEEK_CUR);
    }
    return ulTime;
}

unsigned char* Anlogic_ProcessData(unsigned int byte_num) {
    unsigned int index = 0;
    unsigned char* data = NULL;
    char mode = '\0';

    if (s_compress == (char)FULL_MODE) {
        data = (unsigned char*)calloc((byte_num+1), sizeof(unsigned char));
        for(index = 0; index < byte_num; ++index) {
            data[index] = fgetc(g_aje_file);
        }
    } else { // compress mode
        mode = fgetc(g_aje_file);
        if (mode == 0) {           // no compress
            data = (unsigned char*)calloc((byte_num+1), sizeof(unsigned char));
            for(index = 0; index < byte_num; ++index) {
                data[index] = fgetc(g_aje_file);
            }
        } else if (mode == 1) {    // byte compress
            data = Anlogic_BytesDecode(byte_num);
        } else if (mode == 2) {  // nibble compress
            data = Anlogic_NibbleDecode(byte_num);
        } else if (mode == 3) {  // huffman compress
            data = Anlogic_HuffmanDecode(byte_num);
        } else if (mode == 4) {  // LZW compress
            data = Anlogic_LzwDecode(byte_num);
        } else {
            assert(0 && "Error: Unkown compress mode.");
        }
    }
    return data;
}

void Anlogic_PrintData(unsigned int byte_size, unsigned char* data) {
    int byte_id = 0;
    unsigned int bit_id = 0;
    unsigned char byte = '\0';
    unsigned char flip_byte = '\0';

    printf( " (" );
    for(byte_id = byte_size-1; byte_id >=0; --byte_id) {
        byte = data[byte_id];
        flip_byte = 0x00;
        for (bit_id = 0; bit_id < 8; bit_id++) {
            flip_byte <<= 1;
            if (byte & 0x1) {
                flip_byte |= 0x1;
            }
            byte >>= 1;
        }
        printf("%02X", flip_byte);
    }
    printf(") ");
}

/* Get the SIR Instruction */
unsigned char Anlogic_ParseSirIns(unsigned int len, unsigned char* tdi_data) {
    /* default value : bypass instruction */
    unsigned char sir_ins = INS_BYPASS;

    unsigned int byte_id = 0;
    unsigned int bit_id = 0;
    unsigned char byte = '\0';
    unsigned char flip_byte = '\0';

    if (len % 8 != 0) {
        return sir_ins;
    }

    for(byte_id = 0; byte_id < len/8; ++byte_id) {
        byte = tdi_data[byte_id];
        flip_byte = 0x00;
        for (bit_id = 0; bit_id < 8; bit_id++) {
            flip_byte <<= 1;
            if (byte & 0x1) {
                flip_byte |= 0x1;
            }
            byte >>= 1;
        }

        if (flip_byte != INS_BYPASS) {
            sir_ins = flip_byte;
            break;
        }
    }

    return sir_ins;
}

/* read the head and trailer bit number in sir instruction */
int Anlogic_ReadSirInfo(int* header_num, int* trailer_num) {
    unsigned int bin_num = 0;
    unsigned int flip_byte = 0;
    unsigned int bit_id = 0;

    int byte_id = 0;
    int byte_num = 0;

    int* tmp_val = trailer_num;

    unsigned char* tdi = NULL;
    char byte = '\0';

    *header_num = 0;
    *trailer_num = 0;

    bin_num = Anlogic_ReadNumber();
    byte_num = bin_num / 8;
    if (bin_num % 8 != 0) {
        byte_num++;
    }

    if (fgetc(g_aje_file) != TDI) {
        printf("Error Invalid format when executing read sir information!\n");
        fseek(g_aje_file, bin_num-1, SEEK_CUR);
        return 0;
    }

    tdi = Anlogic_ProcessData(byte_num);
    for(byte_id = byte_num-1; byte_id >= 0; --byte_id) {
        byte = tdi[byte_id];
        flip_byte = 0x00;
        for (bit_id = 0; bit_id < 8; bit_id++) {
            flip_byte <<= 1;
            if (byte & 0x1) {
                flip_byte |= 0x1;
            }
            byte >>= 1;
        }

        if (flip_byte == BYPASS) {
            *tmp_val += 1;
        } else if (flip_byte != BYPASS) {
            tmp_val = header_num;
            if(flip_byte == IDCODE_PUB_2) {
                s_idcode_pub = IDCODE_PUB_2;
            }
        }
    }

    if (CONTINUE != fgetc(g_aje_file)) {
        printf("Error Invalid format when executing read sir information!\n");
        return 0;
    }

    return (tmp_val == trailer_num) ? 0 : 1;
}

int Anlogic_ProcessShiftCommand(char op_code)
{
    int rtn_val = AJE_OK;
    int id = 0;
    unsigned int i=0;
    unsigned int bin_num = 0;
    unsigned int byte_num = 0;
    unsigned char* tdi = NULL;
    unsigned char* tdo = NULL;
    unsigned char* mask = NULL;
    unsigned char* rmask = NULL;
    unsigned char* rtdo = NULL;
    unsigned char ins = 0xFF;
    char byte = '\0';

    bin_num = Anlogic_ReadNumber();
    byte_num = bin_num / 8;
    if (bin_num % 8 != 0) {
        byte_num++;
    }

    while((byte = fgetc(g_aje_file)) != CONTINUE) {
        switch (byte) {
        case TDI:
            tdi = Anlogic_ProcessData(byte_num);
            break;
        case TDO:
            tdo = Anlogic_ProcessData(byte_num);
            break;
        case MASK:
            mask = Anlogic_ProcessData(byte_num);
            break;
        case RMASK:
            rmask = Anlogic_ProcessData(byte_num);
            break;
        default:
            printf("Error: Invalid format when executing process sir or sdr instruction!\n");
            return AJE_INVALID_COMMAND;
            break;
        }
    }

    /* exe sir or sdr instruction */
    if (op_code == SIR) {

        /* if done=0, 'refresh' must be executed before 'program_spi' */
        if (s_done_status == 0 && s_find_refresh_ins == 0) {
            ins = Anlogic_ParseSirIns(bin_num, tdi);
            if (ins == INS_REFRESH) {
                s_find_refresh_ins = 1;
            } else if (ins == INS_PROG_SPI) {
#ifdef AJE_DEBUG
                printf("SIR %d TDI (", s_total_level*8);
                for (id = 1; id <= s_total_level; ++id) {
                    if (id == s_cur_level) {
                        printf("%02X", INS_REFRESH);
                    } else {
                        printf("FF");
                    }
                }
                printf(") ;\n");
                printf("SIR %d TDI (", s_total_level*8);

                for (id = 1; id <= s_total_level; ++id) {
                    printf("FF");
                }
                printf(") ;\n");
#endif
                Anlogic_ExeSirCommand(INS_REFRESH, s_cur_level, s_total_level);
                Anlogic_ExeSirCommand(INS_BYPASS, s_cur_level, s_total_level);
                s_done_status = 1;
            }
        }

#ifdef AJE_DEBUG
        printf("SIR %d TDI", bin_num);
        Anlogic_PrintData(byte_num, tdi);
        if (tdo != NULL) {
            printf("TDO");
            Anlogic_PrintData(byte_num, tdo);
        }
        if (mask != NULL) {
            printf("MASK");
            Anlogic_PrintData(byte_num, mask);
        }
        printf(";\n");
#endif

        rtn_val = Anlogic_ExeSirCommands(bin_num, tdi, tdo, mask);
    } 
    else if (op_code == RSDR) {
#ifdef AJE_DEBUG
        printf("//XSDR %d TDI", bin_num);
        Anlogic_PrintData(byte_num, tdi);
        if (tdo != NULL) {
            printf("TDO");
            Anlogic_PrintData(byte_num, tdo);
        }
        if (mask != NULL) {
            printf("MASK");
            Anlogic_PrintData(byte_num, mask);
        }
        printf(";\n");

        printf("SDR %d TDI", bin_num);
        Anlogic_PrintData(byte_num, tdi);
        if (mask != NULL) {
            printf("RMASK");
            Anlogic_PrintData(byte_num, mask);
        }
        printf(";\n");
#endif
        rtn_val = Anlogic_ExeRSDRCommands(s_cascade, bin_num, tdi, mask);
        if (AJE_OK != rtn_val)
            return rtn_val;

        rtdo = (unsigned char*)malloc((rdbk_tdo_data_p->len_ + 1) * sizeof(unsigned char)); 
        rtdo[rdbk_tdo_data_p->len_] = '\0';
        for (i = 0; i < rdbk_tdo_data_p->len_; i++)
            rtdo[i] = (rdbk_tdo_data_p->data_[i]);

#ifdef AJE_DEBUG
        printf("SDR %d TDI", bin_num);
        Anlogic_PrintData(byte_num, rtdo);
        printf(";\n");
#endif
        rtn_val = Anlogic_ExeSDRCommands(s_cascade, bin_num, rtdo, NULL, NULL);
    }
    else {
#ifdef AJE_DEBUG
        printf("SDR %d TDI", bin_num);
        Anlogic_PrintData(byte_num, tdi);
        if (tdo != NULL) {
            printf("TDO");
            Anlogic_PrintData(byte_num, tdo);
        }
        if (mask != NULL) {
            printf("MASK");
            Anlogic_PrintData(byte_num, mask);
        }
        if (rmask != NULL) {
            printf("RMASK");
            Anlogic_PrintData(byte_num, rmask);
        }
        printf(";\n");
#endif
        if (rmask == NULL) {
            rtn_val = Anlogic_ExeSDRCommands(s_cascade, bin_num, tdi, tdo, mask);
        } else {
            rtn_val = Anlogic_ExeRSDRCommands(s_cascade, bin_num, tdi, rmask);
        }
    }

    //free tdi tdo msk
    if (tdi != NULL) { free(tdi); tdi = NULL; }
    if (tdo != NULL) { free(tdo); tdo = NULL; }
    if (mask != NULL) { free(mask); mask = NULL; }
    if (rmask != NULL) { free(rmask); rmask = NULL; }
    if (rtdo != NULL) { free(rtdo); rtdo = NULL; }

    return rtn_val;
}

int Anlogic_ProcessBypassCommand(char op_code) {
    unsigned int bin_num = 0;
    unsigned int byte_num = 0;
    unsigned int index = 0;

    unsigned char* tdi = NULL;
    char byte = '\0';

    char* op_code_str = NULL;

    bin_num = Anlogic_ReadNumber();
    if (bin_num > 0) {
        byte_num = bin_num / 8;
        if (bin_num % 8 != 0) {
            byte_num++;
        }
        byte = fgetc(g_aje_file);
        if (byte == TDI) {
            tdi = (unsigned char*)calloc((byte_num+1), sizeof(unsigned char));
            for(index = 0; index < byte_num; ++index) {
                tdi[index] = fgetc(g_aje_file);
            }
        }
    }

    switch (op_code)
    {
    case TIR: op_code_str = "TIR"; g_tir_size = bin_num; g_tir_data = tdi; break;
    case TDR: op_code_str = "TDR"; g_tdr_size = bin_num; g_tdr_data = tdi; break;
    case HIR: op_code_str = "HIR"; g_hir_size = bin_num; g_hir_data = tdi; break;
    case HDR: op_code_str = "HDR"; g_hdr_size = bin_num; g_hdr_data = tdi; break;
    default:
        printf("Error: invalid Head/Trailer format,  unkown opcode.\n\n");
        return AJE_INVALID_COMMAND;
    }

#ifdef AJE_DEBUG
    printf("%s %d", op_code_str, bin_num);
    if ( bin_num > 0) {
        printf(" TDI ");
        Anlogic_PrintData(byte_num, tdi);
    }
    printf(" ;\n");
#endif

    if (bin_num > 0) {  // Skip CONTINUE command
        fgetc(g_aje_file);
    }
    return AJE_OK;
}

int Anlogic_ExeSirCommands(unsigned int data_size, unsigned char* tdi_data, unsigned char* tdo_data,
                           unsigned char* mask) {
    unsigned int rtn_val = AJE_OK;
    unsigned int op_code = SIR;
    unsigned int cascade = 0;   // no casecade
    unsigned int read = 0;      // send data only
    rtn_val = Anlogic_ProcessShiftCmd(op_code, cascade, read, data_size, tdi_data, tdo_data, mask, NULL);
    return rtn_val;
}

int Anlogic_ExeSDRCommands(unsigned int cascade, unsigned int data_size,
                           unsigned char* tdi_data, unsigned char* tdo_data, unsigned char* mask) {
    unsigned int rtn_val = AJE_OK;
    unsigned int op_code = SDR;
    unsigned int read = 0;      // send data only
    rtn_val = Anlogic_ProcessShiftCmd(op_code, cascade, read, data_size, tdi_data, tdo_data, mask, NULL);
    return rtn_val;
}

int Anlogic_ExeRSDRCommands(unsigned int cascade, unsigned int data_size,
                           unsigned char* tdi_data, unsigned char* rmask_data) {
    unsigned int rtn_val = AJE_OK;
    unsigned int op_code = SDR;
    unsigned int read = 1;
    unsigned int rtdo_byte_len = 0;
    unsigned int rtdo_bin_len = 0;
    unsigned int data_id = 0;
    unsigned int rmask_bit = 0;
    unsigned char* rtdo_data = NULL;
    struct RdbkTdoData* cur_tdo_data = NULL;
    if (rmask_data == NULL) {
        rtdo_byte_len = strlen(tdi_data);
    } else {
        for (data_id = 0; data_id < data_size; ++data_id) {
            rmask_bit = Anlogic_GetBit(rmask_data, data_id);
            if (rmask_bit == 1) {
                ++rtdo_bin_len;
            }
        }
        rtdo_byte_len = (rtdo_bin_len+7)/8;
    }
    rtdo_data = (unsigned char*)malloc((rtdo_byte_len+1)*sizeof(unsigned char));
    rtdo_data[rtdo_byte_len] = '\0';
    rtn_val = Anlogic_ProcessShiftCmd(op_code, cascade, read, data_size, tdi_data, rtdo_data, NULL, rmask_data);

    cur_tdo_data = (struct RdbkTdoData*)malloc(sizeof(struct RdbkTdoData));
    cur_tdo_data->len_ = rtdo_byte_len;
    cur_tdo_data->data_ = rtdo_data;
    cur_tdo_data->next_ = NULL;
    rdbk_tdo_data_p->next_ = cur_tdo_data;
    rdbk_tdo_data_p = cur_tdo_data;
    return rtn_val;
}

/*
 * read the aje header information,
 * include crc, version, compress mode, maximum memory size and vendor
*/
int Anlogic_ReadAjeHeader()
{
    char byte;
    char buffer[100];
    // crc bytes
    if (fgetc(g_aje_file) != FILECRC) {
        return AJE_FILE_INVALID;
    }

    if (fgets(g_aje_crc, 3, g_aje_file) == NULL) {
        return AJE_FILE_INVALID;
    }

    // version
    if (fgets(buffer, 9, g_aje_file) == NULL) {
        return AJE_FILE_INVALID;
    }

    // compress or full
    byte = fgetc(g_aje_file);
    if (byte != (char)COMP_MODE && byte != (char)FULL_MODE) {
        return AJE_FILE_INVALID;
    }
    s_compress = byte;

    // maximum memory size
    if (MEM != fgetc(g_aje_file)) {
        return AJE_FILE_INVALID;
    }
    Anlogic_ReadNumber();

    // Vender
    if (VENDOR != fgetc(g_aje_file )) {
        return AJE_FILE_INVALID;
    }
    if (ANLOGIC != fgetc(g_aje_file)) {;
        return AJE_FILE_INVALID;
    }

    s_cfg_start_pos = ftell(g_aje_file);
    return AJE_OK;
}

/* read the chip id before processing data, need to get the chain information */
int Anlogic_ReadChipIdcode(void) 
{
    int rtn_val = AJE_OK;
    int finish = 0;

    /* get from HDR/TDR Command */
    int hdr_num = 0;
    int tdr_num = 0;

    /* get from SIR Command */
    int trailer_num = 0;
    int header_num = 0;

    long line_length = 0;
    char op_code = 0x00;

    while((finish == 0) && (fgetc(g_aje_file)) == BEGINLINE) {
        line_length = Anlogic_ReadNumber()/8;
        op_code = fgetc(g_aje_file);
        switch (op_code) {
        case HDR:
            hdr_num = Anlogic_ReadNumber();
            if (hdr_num != 0) {
                while (CONTINUE != fgetc(g_aje_file));
            }
            break;
        case TDR:
            tdr_num = Anlogic_ReadNumber();
            if (tdr_num != 0) {
                while (CONTINUE != fgetc(g_aje_file));
            }
            break;
        case SIR:
            finish = Anlogic_ReadSirInfo(&header_num, &trailer_num);
            break;
        default:
            fseek(g_aje_file, line_length-1, SEEK_CUR);
            break;
        }
    }

    if (finish == 0) {
        return AJE_WARNING;
    }

    s_cur_level = tdr_num+trailer_num+1;
    s_total_level = tdr_num+trailer_num+1+header_num+hdr_num;

    rtn_val = Anlogic_ExeSirCommand(s_idcode_pub, s_cur_level, s_total_level);
    if (rtn_val != AJE_OK) {
        return rtn_val;
    }

    unsigned char* tdo_data = (unsigned char*)malloc(5*sizeof(unsigned char));
    tdo_data[4] = '\0';
    rtn_val = Anlogic_ExeSdrCommand(32, s_cur_level, s_total_level, NULL, tdo_data);

    if (rtn_val == AJE_OK) {  /* print 32 bit chip id in terminal  */
        if (tdo_data[0] == 0 && tdo_data[1] == 0 &&
            tdo_data[2] == 0 && tdo_data[3] == 0) {
            rtn_val = AJE_CHIP_VALIDATION_FAIL;
        } else {
            printf("\n Chip Id : ");
            Anlogic_PrintData(4, tdo_data);
            printf("\n");
        }

    }

    free(tdo_data);
    tdo_data = NULL;
    return rtn_val;
}

/* provide read chip id alone */
int Anlogic_ReadChipIdcodeAlone()
{

    Anlogic_Init();

    int rtn_val = AJE_OK;
 
    int int_cur_level = 1;
    int int_total_level = 1;

    rtn_val = Anlogic_ExeSirCommand(IDCODE_PUB, int_cur_level, int_total_level);
    if (rtn_val != AJE_OK) {
        return rtn_val;
    }

    unsigned char* tdo_data = (unsigned char*)malloc(5 * sizeof(unsigned char));
    tdo_data[4] = '\0';
    rtn_val = Anlogic_ExeSdrCommand(32, int_cur_level, int_total_level, NULL, tdo_data);

    if (rtn_val == AJE_OK) {  /* print 32 bit chip id in terminal  */
        if (tdo_data[0] == 0 && tdo_data[1] == 0 &&
            tdo_data[2] == 0 && tdo_data[3] == 0) {
            rtn_val = AJE_CHIP_VALIDATION_FAIL;
        }
        else {
            printf("\n Chip Id : ");
            Anlogic_PrintData(4, tdo_data);
            printf("\n");
        }

    }

    free(tdo_data);
    tdo_data = NULL;
    return rtn_val;
}

/* Loading Read chip io status instrcution and compare ref_io_status */
int Anlogic_LoadingReadChipIOStatusInstruction(unsigned int bit_size, unsigned char* ref_io_status) {
    int rtn_val = AJE_OK;
    int bit = 0;
    unsigned int io_index = 0;
    unsigned int byte = (bit_size+7)/8;

    rtn_val = Anlogic_ExeSirCommand(SAMPLE, s_cur_level, s_total_level);
    if (rtn_val != AJE_OK) {
        return rtn_val;
    }

    unsigned char* tdo_data = (unsigned char*)malloc((byte+1)*sizeof(unsigned char));
    tdo_data[byte] = '\0';
    rtn_val = Anlogic_ExeSdrCommand(bit_size, s_cur_level, s_total_level, NULL, tdo_data);
    if (rtn_val == AJE_OK) {
        printf("\nIO Status: \n");
        printf("Bs order\t\tRef\t\tRead\t\tVerify\n");

        for (io_index = 0; io_index < bit_size; ++io_index) {
            bit = Anlogic_GetBit(tdo_data, io_index);
            printf("%8d\t\t%c\t\t%d\t\t", io_index, ref_io_status[io_index], bit);
            if (ref_io_status[io_index] - '0' == bit) {
                printf("true\n");
            } else {
                printf("false\n");
            }
        }
    }

    free(tdo_data);
    tdo_data = NULL;
    return rtn_val;
}

int Anlogic_ReadChipIOStatus(const char* io_state_file) {
    int rtn_val = AJE_OK;
    FILE* io_stat_stream = NULL;
    char line[LINE_MAX_SIZE];
    char buf[10];
    int sscanf_num = 0;
    char sdr_tdi_str[DATA_MAX_SIZE];
    char sdr_tdo_str[DATA_MAX_SIZE];
    char sdr_mask_str[DATA_MAX_SIZE];

    char* sdr_tdo_bin_str = NULL;
    char* sdr_mask_bin_str = NULL;
    unsigned char* ref_io_status = NULL;

    int sir_num = 0;
    int sir_val = 0;
    int sdr_num = 0;
    int str_idx = 0;

    int find_sample_ins = 0; /* sample instruction flag */

    char* p = strstr(io_state_file, ".svf");
    if (p == NULL || strcmp(p, ".svf") != 0) {
        printf("Error: %s is not a svf file\n", io_state_file);
        return AJE_FILE_INVALID;
    }

    if ((io_stat_stream = fopen(io_state_file, "r")) == NULL) {
        printf("Error: cannot read the svf file %s\n", io_state_file);
        return AJE_FILE_OPEN_FAIL;
    }

    // Parse svf file
    while (!feof(io_stat_stream)) {
        fgets(line, LINE_MAX_SIZE, io_stat_stream);

        // Find "Sample" instruction,  E.G. SIR 8 TDI (05)
        if (find_sample_ins == 0) {
            sscanf_num = sscanf(line, "SIR %d TDI (%d)", &sir_num, &sir_val);
            if (sscanf_num != 2) {  continue;  }
            if (sir_num != 8 || sir_val != INS_SAMPLE) {
                continue;
            }
            find_sample_ins = 1;
        } else {
            if (strncasecmp(line, "SDR", 3) != 0) {
                continue;
            }
            // sample instruction
            // E.G. SDR	426	TDI (3FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF)
            //				TDO (3FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFBAFBFEAAFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF)
            //			   MASK (0000000000000000000000000000000000000000000000AAAAAAAA00000000000000000000000000000000000000000000000000000);
            sscanf_num = sscanf(line, "SDR %d TDI (%[^)]) TDO (%[^)]) MASK (%[^)])", &sdr_num, sdr_tdi_str, sdr_tdo_str, sdr_mask_str);
            if (sscanf_num == 2) { // for line feeds
                fgets(line, LINE_MAX_SIZE, io_stat_stream);
                sscanf_num = sscanf(line, "%s (%[^)])", buf, sdr_tdo_str);
                assert(strcasecmp(buf, "TDO") == 0);
                if (sscanf_num == 2) {
                    fgets(line, LINE_MAX_SIZE, io_stat_stream);
                    sscanf_num = sscanf(line, "%s (%[^ )])",  buf, sdr_mask_str);
                    assert(strcasecmp(buf, "MASK") == 0);
                }
            }

            if (sdr_num == 0 || sdr_tdi_str == NULL || sdr_tdo_str == NULL || sdr_mask_str == NULL) {
                printf("Error: Invalid SDR string %s", line);
                return AJE_FILE_INVALID;
            }

            ref_io_status = (unsigned char*)malloc((sdr_num+1)*sizeof(unsigned char));
            memset(ref_io_status, 'x', sdr_num);
            ref_io_status[sdr_num] = 0x00;

            sdr_tdo_bin_str  = Anlogic_HexStrToBinStr(sdr_tdo_str, sdr_num);
            sdr_mask_bin_str = Anlogic_HexStrToBinStr(sdr_mask_str, sdr_num);

            for(str_idx = 0; str_idx < sdr_num; ++str_idx) {
                if (sdr_mask_bin_str[str_idx] == '1') { // the bit need verify
                    ref_io_status[sdr_num-1-str_idx] = sdr_tdo_bin_str[str_idx];
                }
            }
        } // end read sdr instruction
    } // end of read file

    if (ref_io_status == NULL) {
        printf("Error: Invalid svf file %s, cannot find the sample data", io_state_file);
        return AJE_FILE_INVALID;
    }

    if (sdr_tdo_bin_str  != NULL)  { free(sdr_tdo_bin_str);  sdr_tdo_bin_str  = NULL; }
    if (sdr_mask_bin_str != NULL)  { free(sdr_mask_bin_str); sdr_mask_bin_str = NULL; }

    rtn_val = Anlogic_LoadingReadChipIOStatusInstruction(sdr_num, ref_io_status);

    return rtn_val;
}

int Anlogic_CheckDonePinStatus() {
    int rtn_val = AJE_OK;
    unsigned char* tdo_data = NULL;

    s_done_status = 1;
    rtn_val = Anlogic_ExeSirCommand(INS_READ_STATUS, s_cur_level, s_total_level);

    tdo_data = (unsigned char*)malloc(5*sizeof(unsigned char));
    tdo_data[4] = '\0';
    rtn_val = Anlogic_ExeSdrCommand(32, s_cur_level, s_total_level, NULL, tdo_data);
    if (rtn_val == AJE_OK) {
        s_done_status = Anlogic_GetBit(tdo_data, 26);
    }

    free(tdo_data);
    tdo_data = NULL;
    return rtn_val;
}

/* get the tdo readback value */
int Anlogic_AjeRdbkValue(unsigned int* len, char*** val_list) {
    unsigned int level = 0;
    unsigned int byte_id = 0;
    unsigned char byte;
    char* data = NULL;
    struct RdbkTdoData* cur_node = &rdbk_tdo_data;
    const char hex[16]=
    {'0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f'};
    while ((cur_node = cur_node->next_) != NULL) {
        ++level;
    }
    *len = level;

    *val_list = (char**)malloc(level*sizeof(char*));
    cur_node = &rdbk_tdo_data;
    level = 0;
    while ((cur_node = cur_node->next_) != NULL) {
        // transfer 1 byte to 2 char value
        // e.g. 0xC5 -> ('c', '5')
        data = (char*)malloc((cur_node->len_*2+1)*sizeof(char));
        data[cur_node->len_*2] = '\0';

        for(byte_id = 0; byte_id < cur_node->len_; ++byte_id) {
            byte = cur_node->data_[byte_id];
            data[byte_id*2] = hex[byte/16];
            data[byte_id*2+1] = hex[byte%16];
        }
        (*val_list)[level] = data;
        ++level;
    }

    return AJE_OK;
}

/* core function, parse aje file and process data */
int Anlogic_ProcessCmd() {
    int rtn_val = AJE_OK;
    int tck_num = 0;        /* for runtest num tck command */
    int loop_num = 0;       /* for loop command */
    long cmd_length = 0;
    long cur_pos = 0;
    long freq = 0;
    char op_code = '\0';
    char enterLoop = 0x00;

    enum TAP_STATE tap_state = TAP_RESET;

    fseek(g_aje_file, s_cfg_start_pos, SEEK_SET); /* restore to the start position of configuration */

    int temp = 0;
    int flag = 0x00;
    while( 0x01 == flag || (temp = fgetc(g_aje_file)) == BEGINLINE) {
        if (0x00 == flag )
          cmd_length = Anlogic_ReadNumber()/8;
SPLITSDR:
        op_code = fgetc(g_aje_file);
        switch (op_code) {
        case STATE:
            if (0x01 == flag) {
                flag = 0x00;
                cmd_length = 0;
            }
            tap_state = Anlogic_TapState(fgetc(g_aje_file));
            Anlogic_TapTransist(tap_state);
#ifdef AJE_DEBUG
            if (cmd_length == 2) {
                printf("STATE %s;\n", Anlogic_TapState2Str(tap_state));
            }
#endif
            if (TAP_DRPAUSE == tap_state) {
              goto SPLITSDR;
            }
			
            if (cmd_length >= 2 && tap_state == TAP_IDLE) {
                tck_num = Anlogic_ProcessRuntestCmd();
                if (tck_num > 0) {
                    Anlogic_ProcessRunTestTck(tck_num);
                }
            }
            break;
        case FREQUENCY:
            freq = Anlogic_ReadNumber();
            if (g_freq == 0 || g_freq > freq) {
                g_freq = freq;
#ifdef AJE_DEBUG
                printf("FREQUENCY %.2E HZ;\n", (float)g_freq);
#endif
            }
            break;
        case SIR:
            rtn_val = Anlogic_ProcessShiftCommand(op_code);
            break;
        case RSDR:
        case SDR:
            rtn_val = Anlogic_ProcessShiftCommand(op_code);
            if (0x01 == enterLoop) {
                if (AJE_OK == rtn_val) {
                    enterLoop = 0x00;
                    loop_num = 0;
                } 
                else {
                    rtn_val = (loop_num != 1) ? AJE_OK : rtn_val;
                }
            }
            break;
        case LOOP:
            loop_num = Anlogic_ReadNumber();
            cur_pos = ftell(g_aje_file);
            enterLoop = 0x01;
            break;
        case ENDLOOP:
            if (loop_num > 1) {
                fseek(g_aje_file, cur_pos, SEEK_SET);
                loop_num--;
            }
            break;
        case TDR:
        case TIR:
        case HDR:
        case HIR:
            rtn_val = Anlogic_ProcessBypassCommand(op_code);
            break;
        case ENDIR:
            tap_state = Anlogic_TapState(fgetc(g_aje_file));
            Anlogic_SetEndIRState(tap_state);
            break;
        case ENDDR:
            tap_state = Anlogic_TapState(fgetc(g_aje_file));
            Anlogic_SetEndDRState(tap_state);
            break;
        case SETFLOW:
            if (fgetc(g_aje_file) == CASCADE) {
                s_cascade = 1;
				flag = 0x01;
            }
            break;
        case RESETFLOW:
            if (fgetc(g_aje_file) == CASCADE) {
                s_cascade = 0;
            }
            break;
        case TRST:
            if (fgetc(g_aje_file) == ON) {
#ifdef AJE_DEBUG
                printf("TRST ON;\n");
#endif
            } else {
#ifdef AJE_DEBUG
                printf("TRST OFF;\n");
#endif
            }
            break;
        case RUNTEST:
            fseek(g_aje_file, cmd_length-1, SEEK_CUR);
            break;
        default:
            printf("Error: invalid file format,  unkown opcode.\n\n");
            return AJE_INVALID_COMMAND;
        }

        if (rtn_val != AJE_OK) {
            return rtn_val;
        }
    }
    return rtn_val;
}


int Anlogic_AjeToVec(const char* aje_file) {
    int rtn_code = AJE_OK;
    const char* io_state_file = NULL; /* input svf file, use to read io status */

    if ((g_aje_file = fopen(aje_file, "rb")) == NULL) {
        return AJE_FILE_OPEN_FAIL;
    }

    /* read header content */
    rtn_code = Anlogic_ReadAjeHeader();
    if (rtn_code != AJE_OK) {
        return rtn_code;
    }

    /* execute "state reset" first, 2019/12/20 */
    Anlogic_Init();

    /* read chip id code */
    rtn_code = Anlogic_ReadChipIdcode();
    if (rtn_code != AJE_OK) {
        return rtn_code;
    }

    /* read io status */
    if (io_state_file != NULL) {
        rtn_code = Anlogic_ReadChipIOStatus(io_state_file);
        if (rtn_code != AJE_OK) {
            return rtn_code;
        }
    }

    /* check done pin status */
    Anlogic_CheckDonePinStatus();

    /* core process function */
    rtn_code = Anlogic_ProcessCmd();

    fclose(g_aje_file);
    return rtn_code;
}
