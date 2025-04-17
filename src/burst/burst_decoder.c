#include "burst_decoder.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

void burst_decoder_init(burst_decoder_t *ctx, uint8_t *buffer, size_t size) {
	ctx->buffer = buffer;
	ctx->buffer_size = size;
	burst_decoder_reset(ctx);
}

burst_status_t bust_decoder_add_data(burst_decoder_t *ctx, const uint8_t *data, size_t size, size_t *consumed_bytes) {
	// If the decoder was finished, reset it.
	if (ctx->finished) {
		burst_decoder_reset(ctx);
	}

	for (size_t i = 0; i < size; i++) {
		uint8_t byte = data[i];
		(*consumed_bytes)++;

		burst_status_t result = burst_decoder_add_byte(ctx, byte);

		if (result != BURST_DATA_CONSUMED) {
			ctx->finished = true;
			return result;
		}
	}
	return BURST_DATA_CONSUMED;
}

void burst_decoder_reset(burst_decoder_t *ctx) {
	ctx->out_head = 0;
	ctx->state = COBS_DECODE_READ_CODE;
	ctx->block = 0;
	ctx->code = 0xFF;
	ctx->finished = false;
}

burst_status_t burst_decoder_complete_packet(burst_decoder_t *ctx) {
#if 0
	printf("Completed packet: ");
	for (size_t i = 0; i < ctx->out_head; i++) {
		printf("%02X ", ctx->buffer[i]);
	}
	printf("\n");
#endif
	// Ensure we have at least two bytes for the CRC.
	if (ctx->out_head < CRC_SIZE) {
		return BURST_CRC_ERROR;
	}

	// Calculate the CRC over the packet data excluding the last two CRC bytes.
	uint16_t computed_crc = burst_crc16(ctx->buffer, ctx->out_head - CRC_SIZE);

	// Extract the received CRC from the last two bytes (big-endian).
	uint16_t received_crc = ((uint16_t)ctx->buffer[ctx->out_head - CRC_SIZE] << 8) | ctx->buffer[ctx->out_head - 1];

	// Check if the CRCs match.
	if (computed_crc != received_crc) {
		return BURST_CRC_ERROR;
	}

	// CRC check passed, we can remove it from the packet.
	ctx->out_head -= CRC_SIZE;
	return BURST_PACKET_READY;
}

burst_status_t burst_decoder_add_byte(burst_decoder_t *ctx, uint8_t byte) {
	// Check for space in the buffer
	if (ctx->out_head >= ctx->buffer_size) {
		return BURST_OVERFLOW_ERROR;
	}

	switch (ctx->state) {
		case COBS_DECODE_FINISH_RUN:
			// If the byte is zero, the block is complete
			if (byte == 0) {
				return burst_decoder_complete_packet(ctx);
			}

			/* fallthrough */
		case COBS_DECODE_READ_CODE:

			// If last code was 0xF, its a overhead byte
			if (ctx->code != 0xFF) {
				ctx->buffer[ctx->out_head++] = 0;
			}

			// The data is a new block code
			ctx->block = ctx->code = byte;

			// If the code is 1, the block is already complete
			if (byte != 1) {
				ctx->state = COBS_DECODE_RUN;
			} else {
				ctx->state = COBS_DECODE_FINISH_RUN;
			}

			return BURST_DATA_CONSUMED;

		case COBS_DECODE_RUN:

			// Decrement the block counter
			ctx->block--;

			// If we het a unexpected delimiter, return an error
			if (!byte) {
				return BURST_DECODE_ERROR;
			}

			ctx->buffer[ctx->out_head++] = byte;

			if (ctx->block == 1) {
				ctx->state = COBS_DECODE_FINISH_RUN;
			}

			return BURST_DATA_CONSUMED;
	}

	// This should never happen, but some compilers are dumb
	return BURST_DECODE_ERROR;
}

burst_packet_t burst_decoder_get_packet(burst_decoder_t *ctx) {
	if (!ctx->finished) {
		burst_packet_t packet;
		packet.data = NULL;
		packet.size = 0;
		return packet;
	}

	burst_packet_t packet;
	packet.data = ctx->buffer;
	packet.size = ctx->out_head;
	return packet;
}

void burst_managed_decoder_init(burst_managed_decoder_t *burst_managed_decoder, uint8_t *buffer, size_t size, burst_managed_decoder_callback_t callback,
                                void *user_data) {
	burst_managed_decoder->callback_function = callback;
	burst_managed_decoder->user_data = user_data;
	burst_decoder_init(&burst_managed_decoder->decoder, buffer, size);
}

int burst_managed_decoder_handle_data(burst_managed_decoder_t *burst_managed_decoder, const uint8_t *data, size_t len) {
	if (len == 0) {
		return 0;  // No data to process
	}

	burst_managed_decoder->statistics.bytes_ingested += len;
	size_t bytes_consumed = 0;
	while (bytes_consumed < len) {
		uint8_t *data_ptr = (uint8_t *)data + bytes_consumed;
		size_t data_len = len - bytes_consumed;

		burst_status_t status = bust_decoder_add_data(&burst_managed_decoder->decoder, data_ptr, data_len, &bytes_consumed);

		switch (status) {
			case BURST_PACKET_READY: {
				burst_packet_t packet = burst_decoder_get_packet(&burst_managed_decoder->decoder);

				if (packet.size > 0 && burst_managed_decoder->callback_function != NULL) {
					// Call the callback function with the received data
					burst_managed_decoder->callback_function(packet.data, packet.size, burst_managed_decoder->user_data);
				}

				burst_managed_decoder->statistics.bytes_processed += packet.size;
				burst_managed_decoder->statistics.packets_processed++;

				continue;
			}

			case BURST_CRC_ERROR:
				burst_managed_decoder->statistics.crc_errors++;
				continue;

			case BURST_DECODE_ERROR:
				burst_managed_decoder->statistics.decode_errors++;
				continue;

			case BURST_OVERFLOW_ERROR:
				burst_managed_decoder->statistics.overflow_errors++;
				continue;

			default:
				continue;
		}
	}
	return bytes_consumed;
}
