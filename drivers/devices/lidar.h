// drivers/devices/lidar.h

#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    SF_PARSE_BYTE_HIGH,
    SF_PARSE_BYTE_LOW,
} sf_parse_state_t;

typedef struct {
    sf_parse_state_t state;
    uint8_t byte_h;
    uint8_t byte_l;
    uint16_t distance_cm;
} sf_parse_ctx_t;

void sf_parser_init(sf_parse_ctx_t* p);
bool sf_parser_feed(sf_parse_ctx_t* p, uint8_t byte);
