#ifndef BURST_ENCODER_H
#define BURST_ENCODER_H

#include <stdint.h>
#include <burst/burst_generic.h>

typedef struct
{
    uint8_t *buffer;    // Output buffer for encoded packets.
    size_t buffer_size; // Total size of the output buffer.
    size_t out_head;    // Current offset (number of bytes written).
} burst_encoder_t;

// Encoder
void burst_encoder_init(burst_encoder_t *ctx, uint8_t *buffer, size_t size);
burst_status_t burst_encoder_add_packet(burst_encoder_t *ctx, const uint8_t *data, size_t size);
burst_packet_t burst_encoder_flush(burst_encoder_t *ctx);

#endif // BURST_ENCODER_H