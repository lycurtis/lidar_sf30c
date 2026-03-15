// app/main.c

#include <stdio.h>
#include <string.h>
#include "bsp.h"
#include "bsp_config.h"
#include "bsp_pinmap.h"
#include "lidar.h"
#include "usart.h"

int main(void) {
    BSP_Init();

    usart_config_t lidar = {
        .instance = BSP_USART_LIDAR,
        .baud_rate = BSP_USART_LIDAR_BAUD,
        .stop_bits = 1,
    };
    usart_init(&lidar);

    usart_config_t debug = {
        .instance = BSP_USART_DEBUG,
        .baud_rate = BSP_USART_DEBUG_BAUD,
        .stop_bits = 1,
    };
    usart_init(&debug);

    static sf_parse_ctx_t lidar_ctx;
    sf_parser_init(&lidar_ctx);

    // Configure 5002 Hz Update Rate
    uint8_t cmd[16];
    uint16_t len;
    len = sf_build_cmd('R', 2, cmd, sizeof(cmd)); // "#R2:" --> update rate 5002
    usart_write(BSP_USART_LIDAR, cmd, len);
    len = sf_build_cmd('U', 2, cmd, sizeof(cmd)); // "#U2:" --> serial output rate 5002
    usart_write(BSP_USART_LIDAR, cmd, len);

    uint32_t reading_count = 0;
    uint32_t last_report = BSP_GetTick();

    for (;;) {
        if (usart_rx_ready(BSP_USART_LIDAR)) {
            uint8_t b = usart_read_byte(BSP_USART_LIDAR);
            if (sf_parser_feed(&lidar_ctx, b)) {
                reading_count++;
            }
        }

        uint32_t now = BSP_GetTick();
        if (now - last_report >= 1000) {
            printf("Rate: %lu Hz  Last: %u cm\r\n",
                   reading_count, lidar_ctx.distance_cm);
            reading_count = 0;
            last_report = now;
        }
    }
}