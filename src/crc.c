#include <stddef.h>
#include <stdint.h>

/*
 * Calculate CRC16-CCITT (polynomial 0x1021, initial value 0xFFFF) over
 * the provided data.
 */
uint16_t crc16_ccitt(const uint8_t *data, size_t length) {
  uint16_t crc = 0xFFFF;
  for (size_t i = 0; i < length; i++) {
    crc ^= ((uint16_t)data[i]) << 8;
    for (int j = 0; j < 8; j++) {
      if (crc & 0x8000)
        crc = (crc << 1) ^ 0x1021;
      else
        crc <<= 1;
    }
  }
  return crc;
}