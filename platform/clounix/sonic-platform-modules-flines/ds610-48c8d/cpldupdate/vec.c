/*******************************************************************
 * Copyright (c) 2011 -2021 Anlogic Inc.
 * The Software is distributed in source code form and is open to
 * re-distribution and modification where applicable
*******************************************************************/

/***********************************************************************
Filename:    vec.c
Description: generate vec
Log:         initial version, July 2019
 ***********************************************************************/

#define _CRT_SECURE_NO_WARNINGS

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include "gpio.h"
#include "opcode.h"

/* global variables about hdr/hir/tdr/tir */
unsigned int g_hdr_size = 0;
unsigned int g_hir_size = 0;
unsigned int g_tdr_size = 0;
unsigned int g_tir_size = 0;

unsigned char* g_hdr_data = NULL;
unsigned char* g_hir_data = NULL;
unsigned char* g_tdr_data = NULL;
unsigned char* g_tir_data = NULL;

static enum TAP_STATE cur_tap_state = TAP_RESET; /* current tap state */
static enum TAP_STATE end_dr_state  = TAP_IDLE;  /* the tap state that device goes after sdr */
static enum TAP_STATE end_ir_state  = TAP_IDLE;  /* the tap state that device goes after sir */

/* function declared in ajeutil.c */
extern int Anlogic_GetBit(unsigned char* data, int id);
extern void Anlogic_SetBit(unsigned char* data, int id, int val);
extern unsigned char Anlogic_ReverseChar(unsigned char byte);

/* Function forward declaration */
void Anlogic_WritePulseTck(void);

/* Tap Opcode To TAP_STATE */
enum TAP_STATE Anlogic_TapState(unsigned char opcode) {
    enum TAP_STATE state = TAP_RESET;
    switch (opcode) {
    case RESET:     state = TAP_RESET;     break;
    case IDLE:      state = TAP_IDLE;      break;
    case DRSELECT:  state = TAP_DRSELECT;  break;
    case DRCAPTURE: state = TAP_DRCAPTURE; break;
    case DRSHIFT:   state = TAP_DRSHIFT;   break;
    case DREXIT1:   state = TAP_DREXIT1;   break;
    case DRPAUSE:   state = TAP_DRPAUSE;   break;
    case DREXIT2:   state = TAP_DREXIT2;   break;
    case DRUPDATE:  state = TAP_DRUPDATE;  break;
    case IRSELECT:  state = TAP_IRSELECT;  break;
    case IRCAPTURE: state = TAP_IRCAPTURE; break;
    case IRSHIFT:   state = TAP_IRSHIFT;   break;
    case IREXIT1:   state = TAP_IREXIT1;   break;
    case IRPAUSE:   state = TAP_IRPAUSE;   break;
    case IREXIT2:   state = TAP_IREXIT2;   break;
    case IRUPDATE:  state = TAP_IRUPDATE;  break;
    default:
        printf("Error: Illegal tap state opcode %u\n", (unsigned int)opcode);
    }
    return state;
}

/* TAP_STATE to TapState String */
const char* Anlogic_TapState2Str(enum TAP_STATE tap_state) {
#define X(_w) if (tap_state == TAP_ ## _w) return #_w
    X(RESET);
    X(IDLE);
    X(DRSELECT);
    X(DRCAPTURE);
    X(DRSHIFT);
    X(DREXIT1);
    X(DRPAUSE);
    X(DREXIT2);
    X(DRUPDATE);
    X(IRSELECT);
    X(IRCAPTURE);
    X(IRSHIFT);
    X(IREXIT1);
    X(IRPAUSE);
    X(IREXIT2);
    X(IRUPDATE);
#undef X
    return "TapState2Str: unknown state";
}

/* Tap State Transistions */
int Anlogic_TapTransist(enum TAP_STATE state) {
    int id = 0;
    int count = 0;

    if (cur_tap_state == state && state == TAP_RESET) {
        for (id = 0; id < 6; ++id) {
            TMS_Wr(1);
            Anlogic_WritePulseTck();
        }
        return AJE_OK;
    }

    while (cur_tap_state != state) {
        switch (cur_tap_state) {
        case TAP_RESET:
            TMS_Wr(0);
            cur_tap_state = TAP_IDLE;
            break;
        case TAP_IDLE:
            TMS_Wr(1);
            cur_tap_state = TAP_DRSELECT;
            break;
            /* DR STATE Transistion */
        case TAP_DRSELECT:
            if (state >= TAP_IRSELECT || state == TAP_RESET) {
                TMS_Wr(1);
                cur_tap_state = TAP_IRSELECT;
            } else {
                TMS_Wr(0);
                cur_tap_state = TAP_DRCAPTURE;
            }
            break;
        case TAP_DRCAPTURE:
            if (state == TAP_DRSHIFT) {
                TMS_Wr(0);
                cur_tap_state = TAP_DRSHIFT;
            } else {
                TMS_Wr(1);
                cur_tap_state = TAP_DREXIT1;
            }
            break;
        case TAP_DRSHIFT:
            TMS_Wr(1);
            cur_tap_state = TAP_DREXIT1;
            break;
        case TAP_DREXIT1:
            if (state == TAP_DRPAUSE) {
                TMS_Wr(0);
                cur_tap_state = TAP_DRPAUSE;
            } else {
                TMS_Wr(1);
                cur_tap_state = TAP_DRUPDATE;
            }
            break;
        case TAP_DRPAUSE:
            TMS_Wr(1);
            cur_tap_state = TAP_DREXIT2;
            break;
        case TAP_DREXIT2:
            if (state == TAP_DRSHIFT) {
                TMS_Wr(0);
                cur_tap_state = TAP_DRSHIFT;
            } else {
                TMS_Wr(1);
                cur_tap_state = TAP_DRUPDATE;
            }
            break;
        case TAP_DRUPDATE:
            if (state == TAP_IDLE) {
                TMS_Wr(0);
                cur_tap_state = TAP_IDLE;
            } else {
                TMS_Wr(1);
                cur_tap_state = TAP_DRSELECT;
            }
            break;
            /* IR STATE Transistion */
        case TAP_IRSELECT:
            if (state == TAP_RESET) {
                TMS_Wr(1);
                cur_tap_state = TAP_RESET;
            } else {
                TMS_Wr(0);
                cur_tap_state = TAP_IRCAPTURE;
            }
            break;
        case TAP_IRCAPTURE:
            if (state == TAP_IRSHIFT) {
                TMS_Wr(0);
                cur_tap_state = TAP_IRSHIFT;
            } else {
                TMS_Wr(1);
                cur_tap_state = TAP_IREXIT1;
            }
            break;
        case TAP_IRSHIFT:
            TMS_Wr(1);
            cur_tap_state = TAP_IREXIT1;
            break;
        case TAP_IREXIT1:
            if (state == TAP_IRPAUSE) {
                TMS_Wr(0);
                cur_tap_state = TAP_IRPAUSE;
            } else {
                TMS_Wr(1);
                cur_tap_state = TAP_IRUPDATE;
            }
            break;
        case TAP_IRPAUSE:
            TMS_Wr(1);
            cur_tap_state = TAP_IREXIT2;
            break;
        case TAP_IREXIT2:
            if (state == TAP_IRSHIFT) {
                TMS_Wr(0);
                cur_tap_state = TAP_IRSHIFT;
            } else {
                TMS_Wr(1);
                cur_tap_state = TAP_IRUPDATE;
            }
            break;
        case TAP_IRUPDATE:
            if (state == TAP_IDLE) {
                TMS_Wr(0);
                cur_tap_state = TAP_IDLE;
            } else {
                TMS_Wr(1);
                cur_tap_state = TAP_IRSELECT;
            }
            break;
        default:
            printf("Error: invalid tap sate.\n");
            return AJE_ERROR;
        }
        Anlogic_WritePulseTck();

        if (++count > 10) {
            printf("Error: Loop in Tap Transistion.\n");
            return AJE_ERROR;
        }
    }
    TDI_Wr(0);

    return AJE_OK;
}

/* Init function, set current tap state to reset */
void Anlogic_Init(void) {
    cur_tap_state = TAP_RESET;
    Anlogic_TapTransist(TAP_RESET);
}

/* set the tap state that deevice goes after sdr */
void Anlogic_SetEndDRState(enum TAP_STATE end_st) {
    end_dr_state = end_st;
#ifdef AJE_DEBUG
    printf("ENDDR %s;\n", Anlogic_TapState2Str(end_st));
#endif
}

/* set the tap state that deevice goes after sir */
void Anlogic_SetEndIRState(enum TAP_STATE end_st) {
    end_ir_state = end_st;
#ifdef AJE_DEBUG
    printf("ENDIR %s;\n", Anlogic_TapState2Str(end_st));
#endif
}

/* Send HIR/HDR/TIR/TDR data to device */
void Anlogic_SendBypassData(unsigned int op_code) {
    unsigned char* data = NULL;
    unsigned int size = 0;
    unsigned int index = 0;
    unsigned int bit = 0;

    switch (op_code) {
    case HIR: size = g_hir_size; data = g_hir_data; break;
    case HDR: size = g_hdr_size; data = g_hdr_data; break;
    case TIR: size = g_tir_size; data = g_tir_data; break;
    case TDR: size = g_tdr_size; data = g_tdr_data; break;
    default: break;
    }

    /* no value set, use default */
    if (data == NULL) {
        bit = (op_code == HIR || op_code == TIR) ? 1 : 0;
    }

    for (index = 0; index < size-1; ++index) {
        if (data != NULL) {
            bit = Anlogic_GetBit(data, index);
        }
        TDI_Wr(bit);
        Anlogic_WritePulseTck();
    }

    bit = Anlogic_GetBit(data, index);
    TDI_Wr(bit);
}

/* Send TDI data to device */
int Anlogic_SendData(unsigned char* tdi_data, unsigned int bin_size, int cascade) {
    unsigned int index = 0;
    unsigned int bit = 0;

    for (index = 0; index < bin_size-1; ++index) {
        bit = Anlogic_GetBit(tdi_data, index);
        TDI_Wr(bit);
        Anlogic_WritePulseTck();
    }

    bit = Anlogic_GetBit(tdi_data, index);
    TDI_Wr(bit);

    if (cascade == 1) {
        Anlogic_WritePulseTck();
    }
    return 0;
}

/* Send TDI data and read TDO, Verify */
int Anlogic_ReadData(unsigned char* tdi_data, unsigned char* tdo_data, unsigned char* mask, unsigned int data_size, int cascade) {
    unsigned int index = 0;
    int tdi_bit = 0;
    int tdo_bit = 0;
    int mask_bit = 0;

    for (index = 0; index < data_size-1; ++index) {
        tdi_bit = Anlogic_GetBit(tdi_data, index);
        tdo_bit = Anlogic_GetBit(tdo_data, index);
        mask_bit = Anlogic_GetBit(mask, index);

        if (mask_bit == 1 && tdo_bit != TDO_Rd()) {
            return AJE_VERIFY_FAIL;
        }

        TDI_Wr(tdi_bit);
        Anlogic_WritePulseTck();
    }

    tdi_bit = Anlogic_GetBit(tdi_data, index);
    tdo_bit = Anlogic_GetBit(tdo_data, index);
    mask_bit = Anlogic_GetBit(mask, index);

    if (mask_bit == 1 && tdo_bit != TDO_Rd()) {
        return AJE_VERIFY_FAIL;
    }
    TDI_Wr(tdi_bit);

    if (cascade == 1) {
        Anlogic_WritePulseTck();
    }

    return AJE_OK;
}

/* Send TDI Data and Save TDO */
int Anlogic_SaveData(unsigned char* tdi_data, unsigned int data_size, unsigned char* rmask_data, unsigned char** tdo_data) {
    unsigned int index = 0;
    unsigned int rmask_index = 0;
    unsigned int tdi_bit = 0;
    unsigned int tdo_bit = 0;
    unsigned int rmask_bit = 0;

    for (index = 0; index < data_size-1; ++index) {
        rmask_bit = rmask_data == NULL ? 1 : Anlogic_GetBit(rmask_data, index);
        if (rmask_bit == 1) {
            tdo_bit = TDO_Rd();
            Anlogic_SetBit(*tdo_data, rmask_index++, tdo_bit);
        }
        tdi_bit = Anlogic_GetBit(tdi_data, index);
        TDI_Wr(tdi_bit);
        Anlogic_WritePulseTck();
    }
    rmask_bit = rmask_data == NULL ? 1 : Anlogic_GetBit(rmask_data, index);
    if (rmask_bit == 1) {
        tdo_bit = TDO_Rd();
        Anlogic_SetBit(*tdo_data, rmask_index, tdo_bit);
    }
    tdi_bit = Anlogic_GetBit(tdi_data, index);
    TDI_Wr(tdi_bit);

    return AJE_OK;
}

/* Process SIR/SDR */
int Anlogic_ProcessShiftCmd(unsigned int op_code, unsigned int cascade, unsigned int read, unsigned int data_size,
                       unsigned char* tdi_data, unsigned char* tdo_data, unsigned char* mask, unsigned char* rmask) {
    int rtn_val = AJE_OK;
    // process header data
    switch (op_code) {
    case SIR:
        if (cascade != 1) {
            Anlogic_TapTransist(TAP_IRSHIFT);
            if (g_hir_size > 0) {
                Anlogic_SendBypassData(HIR);
                Anlogic_WritePulseTck();
            }
        }
        break;
    case SDR:
        if (TAP_DRSHIFT != cur_tap_state) {
            if (1 == cascade) {
                if (TAP_DRPAUSE == cur_tap_state) {
                    Anlogic_TapTransist(TAP_DRSHIFT);
                    if (g_hdr_size > 0) {
                        Anlogic_SendBypassData(HDR);
                        Anlogic_WritePulseTck();
                    }
                }
                else {
                    Anlogic_TapTransist(TAP_DRSHIFT);
                }
            }
            else {
                Anlogic_TapTransist(TAP_DRSHIFT);
                if (g_hdr_size > 0) {
                    Anlogic_SendBypassData(HDR);
                    Anlogic_WritePulseTck();
                }
            }
        }
        break;
    default:
        break;
    }

    if (read == 1) {
        rtn_val = Anlogic_SaveData(tdi_data, data_size, rmask, &tdo_data);
    } else if (mask == NULL) {
        rtn_val = Anlogic_SendData(tdi_data, data_size, cascade);
    } else {
        rtn_val= Anlogic_ReadData(tdi_data, tdo_data, mask, data_size, cascade);
    }

    // process tailer data
    switch (op_code) {
    case SIR:
        if (cascade != 1) {
            if (g_tir_size > 0) {
                Anlogic_WritePulseTck();
                Anlogic_SendBypassData(TIR);
            }
            Anlogic_TapTransist(end_ir_state);
        }
        break;
    case SDR:
        if (cascade != 1) {
            if (g_tdr_size > 0) {
                Anlogic_WritePulseTck();
                Anlogic_SendBypassData(TDR);
            }
            Anlogic_TapTransist(end_dr_state);
        }
        break;
    default:
        break;
    }

    return rtn_val;
}

/* process single sir command, do not use bypass data (hir/tir) */
int Anlogic_ExeSirCommand(unsigned char command, int cur_lvl, int total_lvl) {
    int rtn_val = AJE_OK;
    unsigned char* tdi_data = NULL;
    int id = 0;
    Anlogic_TapTransist(TAP_IRSHIFT);

    tdi_data = (unsigned char*)calloc((total_lvl*8+1), sizeof(unsigned char));
    for(id = 0; id < total_lvl; ++id) {
        if (id == (total_lvl - cur_lvl)) {
            tdi_data[id] = Anlogic_ReverseChar(command);
        } else {
            tdi_data[id] = 0xFF;
        }
    }
    rtn_val = Anlogic_SendData(tdi_data, total_lvl*8, 0 /*cascade*/);
    Anlogic_TapTransist(end_ir_state);

    return rtn_val;
}

/* process sdr command, do not use bypass data (hdr/tdr) */
int Anlogic_ExeSdrCommand(unsigned int bit_size, int cur_lvl, int total_lvl,
                     unsigned char* tdi_data, unsigned char* tdo_data) {
    int rtn_val = AJE_OK;
    int head_num = total_lvl - cur_lvl;
    int tailer_num = cur_lvl-1;
    int id = 0;
    unsigned int byte_size = (bit_size + 7)/8;

    Anlogic_TapTransist(TAP_DRSHIFT);
    if (head_num > 0) {
        for (id = 0; id < head_num; ++id) {
            TDI_Wr(0);
            Anlogic_WritePulseTck();
        }
    }

    if (tdi_data == NULL) {
        tdi_data = (unsigned char*)calloc((byte_size+1), sizeof(unsigned char));
    }

    if (tdo_data == NULL) {
        rtn_val = Anlogic_SendData(tdi_data, bit_size, 0 /*cascade*/);
    } else { /* read and save tdo readback data */
        rtn_val = Anlogic_SaveData(tdi_data, bit_size, NULL, &tdo_data /*cascade*/);
    }

    if (tailer_num > 0) {
        for (id = 0; id < tailer_num; ++id) {
            Anlogic_WritePulseTck();
            TDI_Wr(0);
        }
    }

    Anlogic_TapTransist(end_dr_state);
    return rtn_val;
}

/* process runtest num tck */
void Anlogic_ProcessRunTestTck(int num) {
    int i = 0;
    TDI_Wr(0);
    TMS_Wr(0);
    for(i = 0; i < num; ++i) {
        TCK_Wr(1);
        TCK_Wr(0);
    }
#ifdef AJE_DEBUG
    printf("RUNTEST %d TCK;\n", num);
#endif
}

void Anlogic_WritePulseTck(void) {
    TCK_Wr(1);
    TCK_Wr(0);
}
