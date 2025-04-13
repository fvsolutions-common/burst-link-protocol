#ifndef BURST_DECODER_H
#define BURST_DECODER_H

#include <burst/burst_generic.h>
#include <stdbool.h>
typedef int (*burst_managed_decoder_callback_t)(const uint8_t *data, size_t length, void *user_data);

typedef struct
{
	uint32_t bytes_ingested;     // Total number of bytes ingested
	uint32_t bytes_processed;    // Total number of bytes after processing
	uint32_t packets_processed;  // Successfully decoded packets

    // Error statistics
    uint32_t crc_errors;
    uint32_t overflow_errors;
    uint32_t decode_errors;
} burst_decoder_statistics_t;

typedef struct
{
    uint8_t *buffer;    // Output buffer for decoded data.
    size_t buffer_size; // Size of the output buffer.
    size_t out_head;    // Current count of decoded bytes stored.
    enum cobs_decode_inc_state
    {
        COBS_DECODE_READ_CODE,
        COBS_DECODE_RUN,
        COBS_DECODE_FINISH_RUN
    } state;
    uint8_t block, code;

    bool finished; // true if the packet is complete and available in the buffer

    burst_decoder_statistics_t statistics; // Statistics for the decoder

} burst_decoder_t;

typedef struct
{
    burst_decoder_t decoder;               // Decoder instance
    burst_decoder_statistics_t statistics; // Statistics for the decoder
    burst_managed_decoder_callback_t callback_function; // Callback function for decoded packets
    void *user_data;                       // User data for the callback function
} burst_managed_decoder_t;

void burst_decoder_init(burst_decoder_t *ctx, uint8_t *buffer, size_t size);
void burst_decoder_reset(burst_decoder_t *ctx);
burst_status_t burst_decoder_add_byte(burst_decoder_t *ctx, uint8_t byte);
burst_status_t bust_decoder_add_data(burst_decoder_t *ctx, const uint8_t *data, size_t size, size_t *consumed_bytes);
burst_packet_t burst_decoder_get_packet(burst_decoder_t *ctx);

void burst_managed_decoder_init(burst_managed_decoder_t *burst_managed_decoder, uint8_t *buffer, size_t size, burst_managed_decoder_callback_t callback, void *user_data);
int burst_managed_decoder_handle_data(burst_managed_decoder_t *burst_managed_decoder, const uint8_t *data, size_t len);

#endif // BURST_DECODER_H