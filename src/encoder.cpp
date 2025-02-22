#include "burst_interface.h"
#include "crc.h"  // Assumes crc16_ccitt() is available.
#include <stddef.h>
#include <stdint.h>

// COBS encoding with CRC appending.
// The input to be encoded is the raw packet data followed by the two CRC bytes.
// The algorithm works by maintaining a "code" (the count of nonzero bytes)
// and inserting that count at the start of each block. When a zero is encountered
// (or when the block length reaches COBS_MAX_CODE) the block is terminated.
burst_status_t burst_encoder_add_packet(burst_encoder_t *ctx, const uint8_t *data, size_t size)
{
    // Compute the CRC over the raw packet data.
    uint16_t crc = crc16_ccitt(data, size);
    uint8_t crc_high = (crc >> 8) & 0xFF;
    uint8_t crc_low  = crc & 0xFF;
    // The total number of bytes to encode: raw data + 2 bytes of CRC.
    size_t total_bytes = size + CRC_SIZE;

    // Initialize COBS block state.
    uint8_t code = 1; // Code value starts at 1.
    // Reserve space for the code byte.
    if (ctx->out_head >= ctx->buffer_size)
        return BURST_OVERFLOW_ERROR;
    size_t code_index = ctx->out_head;
    ctx->buffer[ctx->out_head++] = 0; // Placeholder for the code.

    // Process each byte from the raw data and then the CRC.
    for (size_t i = 0; i < total_bytes; i++) {
        uint8_t byte;
        if (i < size)
            byte = data[i];
        else if (i == size)
            byte = crc_high;
        else // i == size + 1
            byte = crc_low;

        if (byte == 0) {
            // Write the current code to the reserved position.
            ctx->buffer[code_index] = code;
            // Start a new block.
            code = 1;
            if (ctx->out_head >= ctx->buffer_size)
                return BURST_OVERFLOW_ERROR;
            code_index = ctx->out_head;
            ctx->buffer[ctx->out_head++] = 0; // Reserve placeholder for new code.
        } else {
            // Append the nonzero byte.
            if (ctx->out_head >= ctx->buffer_size)
                return BURST_OVERFLOW_ERROR;
            ctx->buffer[ctx->out_head++] = byte;
            code++;
            // If the maximum code value is reached, finish the block.
            if (code == COBS_MAX_CODE) {
                ctx->buffer[code_index] = code;
                code = 1;
                if (ctx->out_head >= ctx->buffer_size)
                    return BURST_OVERFLOW_ERROR;
                code_index = ctx->out_head;
                ctx->buffer[ctx->out_head++] = 0; // Reserve new placeholder.
            }
        }
    }
    
    // Finalize the last block.
    ctx->buffer[code_index] = code;
    
    // Append the packet delimiter.
    if (ctx->out_head >= ctx->buffer_size)
         return BURST_OVERFLOW_ERROR;
    ctx->buffer[ctx->out_head++] = COBS_DELIMITER;
    
    return BURST_PACKET_READY;
}

void burst_encoder_init(burst_encoder_t *ctx, uint8_t *buffer, size_t size)
{
    ctx->buffer = buffer;
    ctx->buffer_size = size;
    ctx->out_head = 0;
}

burst_packet_t burst_encoder_flush(burst_encoder_t *ctx)
{
    burst_packet_t packet;
    packet.data = ctx->buffer;
    packet.size = ctx->out_head;
    // Reset the encoder context for the next use.
    ctx->out_head = 0;
    return packet;
}