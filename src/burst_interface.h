#ifndef BURST_INTERFACE_H
#define BURST_INTERFACE_H

#include <stddef.h>
#include <stdint.h>

#define COBS_DELIMITER 0x00
#define COBS_MAX_CODE 0xFF

// Status codes returned by the encoder.
typedef enum
{
    BURST_DATA_CONSUMED,
    BURST_PACKET_READY,
    BURST_OVERFLOW_ERROR,
    BURST_ENCODE_ERROR,
    BURST_CRC_ERROR,
    BURST_DECODE_ERROR
} burst_status_t;

// Encoded packet structure.
typedef struct
{
    uint8_t *data;
    size_t size;
} burst_packet_t;

// Encoder context. This holds the output buffer and the current write position.
typedef struct
{
    uint8_t *buffer;    // Output buffer for encoded packets.
    size_t buffer_size; // Total size of the output buffer.
    size_t out_head;    // Current offset (number of bytes written).
} burst_encoder_t;

// Decoder context for real-time COBS decoding.
typedef struct
{
  uint8_t *buffer;    // Output buffer for decoded data.
  size_t buffer_size; // Size of the output buffer.
  size_t out_head;    // Current count of decoded bytes stored.

  uint8_t current_code;    // Current block’s code byte; 0 indicates a new block is
                           // expected.
  uint8_t bytes_remaining; // Number of data bytes left to copy for the current
                           // block.
  bool pending_zero;       // true if a zero should be inserted before starting the
                           // next block.

  bool finished; // true if the packet is complete and available in the buffer
} burst_decoder_t;

void my_encoder_init(burst_encoder_t *ctx, uint8_t *buffer, size_t size);
burst_status_t encoder_add_packet(burst_encoder_t *ctx, const uint8_t *data, size_t size);
burst_packet_t encoder_flush(burst_encoder_t *ctx);

void my_decoder_init(burst_decoder_t *ctx, uint8_t *buffer, size_t size);
burst_status_t decoder_ingest(burst_decoder_t *ctx, const uint8_t *data, size_t size,
                             size_t *consumed_bytes);
void decoder_reset(burst_decoder_t *ctx);
burst_status_t complete_packet(burst_decoder_t *ctx);

burst_status_t decoder_ingest_byte(burst_decoder_t *ctx, uint8_t byte);

burst_packet_t decoder_get_packet(burst_decoder_t *ctx);

#endif // ENCODER_H
