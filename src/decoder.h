#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define COBS_DELIMITER 0x00
#define COBS_MAX_CODE 0xFF

typedef enum
{
  COBS_PACKET_READY = 0,
  COBS_DATA_CONSUMED = 1,
  COBS_CRC_ERROR = 2,
  COBS_DECODE_ERROR = 3,
  COBS_OVERFLOW_ERROR = 4
} cobs_status_t;

// Packet structure: holds a pointer to decoded data and its size.
typedef struct
{
  uint8_t *data;
  size_t size;
  cobs_status_t status;
} packet_t;

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
} decoder_t;

void my_decoder_init(decoder_t *ctx, uint8_t *buffer, size_t size);
cobs_status_t decoder_ingest(decoder_t *ctx, const uint8_t *data, size_t size,
                             size_t *consumed_bytes);
void decoder_reset(decoder_t *ctx);
cobs_status_t complete_packet(decoder_t *ctx);

cobs_status_t decoder_ingest_byte(decoder_t *ctx, uint8_t byte);

packet_t decoder_get_packet(decoder_t *ctx);