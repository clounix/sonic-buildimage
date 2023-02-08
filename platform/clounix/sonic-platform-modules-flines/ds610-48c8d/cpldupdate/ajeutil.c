/*******************************************************************
 * Copyright (c) 2011 -2021 Anlogic Inc.
 * The Software is distributed in source code form and is open to
 * re-distribution and modification where applicable
*******************************************************************/

/*******************************************************************
 Filename :		ajeutil.c
 Description:	utility source file
 Log:			initial version, December 2019
*******************************************************************/

#define _CRT_SECURE_NO_WARNINGS

#include <malloc.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

int Anlogic_GetBit(unsigned char* data, int id) {
    return (data[id/8] & (1 << (7-id%8))) ? 1 : 0;
}

void Anlogic_SetBit(unsigned char* data, int id, int val) {
    unsigned char mask = 1 << (7-id%8);
    if (val) {
        data[id/8] |= mask;
    } else {
        data[id/8] &= ~mask;
    }
}

/* reverse char data e.g. 8'b10001101 -> 8'b10110001 */
unsigned char Anlogic_ReverseChar(unsigned char byte) {
    unsigned int bit_id = 0;
    unsigned char flip_byte = 0x00;
    for (bit_id = 0; bit_id < 8; ++bit_id) {
        flip_byte <<= 1;
        if (byte & 0x1) {
            flip_byte |= 0x1;
        }
        byte >>= 1;
    }
    return flip_byte;
}

/* tranfer hex to bin data, e.g. char('A') -> char[4] = {'1', '0', '1', '0' } */
void Anlogic_HexTobin(char hex, char* char_s) {
    switch(hex) {
    case '0': strcpy(char_s ,"0000"); break;
    case '1': strcpy(char_s ,"0001"); break;
    case '2': strcpy(char_s ,"0010"); break;
    case '3': strcpy(char_s ,"0011"); break;
    case '4': strcpy(char_s ,"0100"); break;
    case '5': strcpy(char_s ,"0101"); break;
    case '6': strcpy(char_s ,"0110"); break;
    case '7': strcpy(char_s ,"0111"); break;
    case '8': strcpy(char_s ,"1000"); break;
    case '9': strcpy(char_s ,"1001"); break;
    case 'a':
    case 'A': strcpy(char_s ,"1010"); break;
    case 'b':
    case 'B': strcpy(char_s ,"1011"); break;
    case 'c':
    case 'C': strcpy(char_s ,"1100"); break;
    case 'd':
    case 'D': strcpy(char_s ,"1101"); break;
    case 'e':
    case 'E': strcpy(char_s ,"1110"); break;
    case 'f':
    case 'F': strcpy(char_s ,"1111"); break;
    default:
        printf("Error: HexToChar Invalid Hex value %c", hex);
    }
}

char* Anlogic_HexStrToBinStr(char* src_str, int bin_len) {
    char* bin_value = NULL;
    int hex_idx = 0;
    int char_idx = 0;
    char char_s[5] = { 0 };
    int hex_size = strlen(src_str);
    if (hex_size*4 < bin_len) {
        return NULL;
    }
    bin_value = (char*)malloc((bin_len+1)*sizeof(char));
    bin_value[bin_len] = 0x00;

    for(hex_idx=hex_size-1; hex_idx>=0; --hex_idx) {
        char c = src_str[hex_idx];
        Anlogic_HexTobin(c, char_s);
        for (char_idx = 0; char_idx<4; ++char_idx) {
            bin_value[--bin_len] = char_s[3-char_idx];
            if (bin_len == 0) {
                return bin_value;
            }
        } // end char_idx
    } // end hex_idx
    return bin_value;
}

