#include <stdint.h>

uint16_t blp_crc16_ccitt(const uint8_t *data, size_t length);
#define CRC_SIZE sizeof(uint16_t)