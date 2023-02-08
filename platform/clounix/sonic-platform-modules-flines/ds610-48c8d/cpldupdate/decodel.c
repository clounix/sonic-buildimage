/*******************************************************************
 * Copyright (c) 2011 -2021 Anlogic Inc.
 * The Software is distributed in source code form and is open to
 * re-distribution and modification where applicable
*******************************************************************/

/***********************************************************************
Filename:    decode.c
Description: decode the data from aje file
Log:         initial version, July 2019
 ***********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

extern FILE*  g_aje_file;

/* Function delcared in lzw_lib.c */
extern int lzw_decompress (void (*dst)(int), int (*src)(void));

unsigned char* g_pWriteBuffer = NULL;
long int g_iWriteCount = 0;

void Anlogic_WriteBufferByte(int byte)
{
    g_pWriteBuffer[g_iWriteCount++] = (unsigned char)byte;
}

int Anlogic_ReadAjeByte(void)
{
    return fgetc(g_aje_file);
}

long Anlogic_NumberDecode(void) {
    long number = 0;
    char byte = '\0';
    unsigned int count = 0;
    do {
        byte = fgetc(g_aje_file);
        number += (byte & 0x7F) << (7 * count++);
    } while (byte & 0x80);
    return number;
}

unsigned char* Anlogic_BytesDecode(long bytes) {
    long int i = 0;
    long int key_count = 0;
    unsigned char byte = 0, key = 0;
    unsigned char* rtn_val = NULL;
    rtn_val = (unsigned char*)malloc((bytes+1)*sizeof(unsigned char));

    key = fgetc(g_aje_file);

    for (i = 0; i < bytes; i++) {
        if (key_count <= 0) {
            byte = fgetc(g_aje_file);
            rtn_val[i] = byte;
            if (byte == key) {
                key_count = Anlogic_NumberDecode();  // The number of key bytes
            }
        } else {
            key_count--; // Use up the key chain first
            rtn_val[i] = key;
        }
    }
    return rtn_val;
}

unsigned char* Anlogic_NibbleDecode(long bytes) {
    int i = 0, j = 0, byte_time = 0;
    long int num_keys = 0, key_bytes, key_times;
    unsigned char *buffer = 0;
    unsigned char* rtn_val;
    rtn_val = (unsigned char*)malloc((bytes+1)*sizeof(unsigned char));

    num_keys = Anlogic_NumberDecode();
    key_bytes = (num_keys + 1) / 2;
    key_times = (bytes * 2) / num_keys;
    buffer = (unsigned char*)calloc(key_bytes + 1 , sizeof(unsigned char));
    if (!buffer) {
        assert(0 && "nibble decode: calloc fail");
    }

    for (i = 0; i < key_bytes; i++ )
        buffer[i] = fgetc(g_aje_file);

    if (num_keys % 2 == 0) {
        for ( i = 0; i < key_times; i++ ) {
            for ( j = 0; j < key_bytes; j++ ) {
                rtn_val[j+i*key_bytes] = buffer[j];
            }
        }
    } else {
        assert(key_times % 2 == 0);
        buffer = (unsigned char*)realloc(buffer, key_bytes*2*sizeof(unsigned char));
        if (!buffer) {
            assert(0 && "nibble decode: realloc fail");
        }
        //buffer[key_bytes-2 : 0] keep no change
        for(i = key_bytes-1; i < key_bytes*2-1; ++i) {
            if (i == key_bytes-1)
                buffer[i] = (buffer[i] & 0xF0) + (buffer[0] >> 4);
            else
                buffer[i] = ((buffer[i-key_bytes] << 4) & 0xF0) +
                        (buffer[i-key_bytes+1] >> 4);
        }
        byte_time = key_times / 2;
        for(i = 0; i < byte_time; ++i) {
            for(j = 0; j < (key_bytes*2-1); ++j) {
                rtn_val[j+i*(key_bytes*2-1)] = buffer[j];
            }
        }
    }
    return rtn_val;
}

unsigned char* Anlogic_HuffmanDecode(long bytes) {
    int i = 0, j = 0, m = 0, bits = 8;
    unsigned char cur_char = 0, key = 0, byte = 0;
    unsigned char* rtn_val;
    rtn_val = (unsigned char*)malloc((bytes+1)*sizeof(unsigned char));

    key = fgetc(g_aje_file);

    for ( i = 0; i < bytes; i++ ) {
        byte = 0x00;
        if ( bits > 7 ) {
            cur_char = fgetc(g_aje_file);
            bits = 0;
        }
        if ( ( cur_char << bits++ ) & 0x80 ) {
            m = 8;
            for (j = 0; j < m; j++) {
                if (bits > 7) {
                    cur_char = fgetc(g_aje_file);
                    bits = 0;
                }
                byte |= ( ( cur_char << bits++ ) & 0x80 ) >> j;
            }
        } else {
            byte = key;
            m = 0;
        }
        rtn_val[i] = byte;
    }
    return rtn_val;
}

unsigned char* Anlogic_LzwDecode(long bytes) {
    g_pWriteBuffer = (unsigned char*)malloc((bytes+1)*sizeof(unsigned char));
    g_iWriteCount = 0;
    lzw_decompress(Anlogic_WriteBufferByte, Anlogic_ReadAjeByte);
    return g_pWriteBuffer;
}
