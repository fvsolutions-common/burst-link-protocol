#include <stdint.h>

uint16_t crc16_ccitt(const uint8_t *data, size_t length);
#define CRC_SIZE sizeof(uint16_t)