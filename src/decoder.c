#include "burst_interface.h"
#include "crc.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

void burst_decoder_init(burst_decoder_t *ctx, uint8_t *buffer, size_t size)
{
  ctx->buffer = buffer;
  ctx->buffer_size = size;
  burst_decoder_reset(ctx);
}

burst_status_t bust_decoder_add_data(burst_decoder_t *ctx, const uint8_t *data, size_t size,
                             size_t *consumed_bytes)
{
  // If the decoder was finished, reset it.
  if (ctx->finished)
  {
    burst_decoder_reset(ctx);
  }

  for (size_t i = 0; i < size; i++)
  {
    uint8_t byte = data[i];
    (*consumed_bytes)++;

    burst_status_t result = burst_decoder_add_byte(ctx, byte);

    if (result != BURST_DATA_CONSUMED)
    {
      ctx->finished = true;
      return result;
    }
  }
  return BURST_DATA_CONSUMED;
}
void burst_decoder_reset(burst_decoder_t *ctx)
{
  ctx->out_head = 0;
  ctx->current_code = 0;
  ctx->bytes_remaining = 0;
  ctx->pending_zero = false;
  ctx->finished = false;
}

burst_status_t burst_decoder_complete_packet(burst_decoder_t *ctx)
{
  // Ensure we have at least two bytes for the CRC.
  if (ctx->out_head < CRC_SIZE)
  {
    return BURST_CRC_ERROR;
  }

  // Calculate the CRC over the packet data excluding the last two CRC bytes.
  uint16_t computed_crc = crc16_ccitt(ctx->buffer, ctx->out_head - CRC_SIZE);

  // Extract the received CRC from the last two bytes (big-endian).
  uint16_t received_crc =
      ((uint16_t)ctx->buffer[ctx->out_head - CRC_SIZE] << 8) |
      ctx->buffer[ctx->out_head - 1];

  // Check if the CRCs match.
  if (computed_crc != received_crc)
  {
    return BURST_CRC_ERROR;
  }

  // CRC check passed, we can remove it from the packet.
  ctx->out_head -= CRC_SIZE;
  return BURST_PACKET_READY;
}

// If byte is a delimiter but a block is not complete, return COBS_DECODE_ERROR
// If the buffer is full, return COBS_OVERFLOW_ERROR
// If the byte is consumed, but the packet is not complete, return COBS_DATA_CONSUMED
// If the packet is complete, return COBS_PACKET_READY
burst_status_t burst_decoder_add_byte(burst_decoder_t *ctx, uint8_t byte)
{
    // Check if there is space for more data.
    if (ctx->out_head >= ctx->buffer_size) {
        return BURST_OVERFLOW_ERROR;
    }
    
    // If the byte is a delimiter, decide if it terminates the packet.
    if (byte == COBS_DELIMITER) {
        // If in the middle of a block, a delimiter is not allowed.
        if (ctx->current_code != 0) {
            return BURST_DECODE_ERROR;
        }
        // Otherwise, the packet is complete.
        return burst_decoder_complete_packet(ctx);
    }
    
    // If a zero is pending from a previous block, insert it now.
    if (ctx->pending_zero) {
        if (ctx->out_head >= ctx->buffer_size) {
            return BURST_OVERFLOW_ERROR;
        }
        ctx->buffer[ctx->out_head++] = COBS_DELIMITER;
        ctx->pending_zero = false;
        // Now, treat the current byte as a new block code.
        ctx->current_code = byte;
        ctx->bytes_remaining = (byte > 0 ? byte - 1 : 0);
        return BURST_DATA_CONSUMED;
    }
    
    // If not currently in a block, this byte is the new block code.
    if (ctx->current_code == 0) {
        ctx->current_code = byte;
        ctx->bytes_remaining = (byte > 0 ? byte - 1 : 0);
        return BURST_DATA_CONSUMED;
    }
    
    // Otherwise, we are in the middle of a block so treat the byte as data.
    ctx->buffer[ctx->out_head++] = byte;
    if (ctx->bytes_remaining > 0) {
        ctx->bytes_remaining--;
    }
    
    // When the block is complete...
    if (ctx->bytes_remaining == 0) {
        // If the block's code is less than COBS_MAX_CODE, a zero is pending.
        if (ctx->current_code < COBS_MAX_CODE) {
            ctx->pending_zero = true;
        }
        ctx->current_code = 0;
    }
    
    return BURST_DATA_CONSUMED;
}


burst_packet_t burst_decoder_get_packet(burst_decoder_t *ctx)
{

  if (!ctx->finished)
  {
    burst_packet_t packet;
    packet.data = NULL;
    packet.size = 0;
    return packet;
  }

  burst_packet_t packet;
  packet.data = ctx->buffer;
  packet.size = ctx->out_head-1;
  return packet;
}
