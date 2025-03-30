#ifndef BURST_DECODER_H
#define BURST_DECODER_H

#include <burst/burst_generic.h>
#include <stdbool.h>

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
} burst_decoder_t;

void burst_decoder_init(burst_decoder_t *ctx, uint8_t *buffer, size_t size);
void burst_decoder_reset(burst_decoder_t *ctx);
burst_status_t burst_decoder_add_byte(burst_decoder_t *ctx, uint8_t byte);
burst_status_t bust_decoder_add_data(burst_decoder_t *ctx, const uint8_t *data, size_t size, size_t *consumed_bytes);
burst_packet_t burst_decoder_get_packet(burst_decoder_t *ctx);

#endif // BURST_DECODER_H