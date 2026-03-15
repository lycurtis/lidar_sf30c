// drivers/devices/lidar.c

#include "lidar.h"
#include <string.h>

void sf_parser_init(sf_parse_ctx_t* p){
    memset(p, 0, sizeof(sf_parse_ctx_t));
    p->state = SF_PARSE_BYTE_HIGH;
}

bool sf_parser_feed(sf_parse_ctx_t* p, uint8_t byte){
    switch(p->state){
        case SF_PARSE_BYTE_HIGH:
            if(byte & 0x80){
                p->byte_h = byte & 0x7F;
                p->state = SF_PARSE_BYTE_LOW;
            }
            return false;

        case SF_PARSE_BYTE_LOW:
            if(byte & 0x80){
                p->byte_h = byte & 0x7F;
                return false;
            }

            p->byte_l = byte & 0x7F;
            p->distance_cm = ((uint16_t)p->byte_h << 7) | p->byte_l;
            p->state = SF_PARSE_BYTE_HIGH;
            return true;
    }

    return false;
}

uint16_t sf_build_cmd(char mnemonic, int value, uint8_t* buf, uint16_t buf_size){
    uint16_t i = 0;
    if (buf_size < 4) return 0;

    buf[i++] = '#';
    buf[i++] = (uint8_t)mnemonic;

    if (value >= 0) {
        char tmp[8];
        uint16_t n = 0;
        int v = value;
        do {
            tmp[n++] = '0' + (v % 10);
            v /= 10;
        } while (v > 0 && n < sizeof(tmp));

        for (int j = n - 1; j >= 0 && i < buf_size - 1; j--) {
            buf[i++] = tmp[j];
        }
    }

    buf[i++] = ':';
    return i;
}