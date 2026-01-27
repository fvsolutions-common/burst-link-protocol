#include "burst_decoder.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

// #define BURST_DECODER_DEBUG

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

#ifdef BURST_DECODER_DEBUG
	printf("BURST Ingest RAW: ");
	for (size_t i = 0; i < size; i++) {
		printf("%02X ", data[i]);
	}
	printf("\n");
#endif

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
#ifdef BURST_DECODER_DEBUG
	printf("	Completed packet: ");
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
#ifdef BURST_DECODER_DEBUG
		printf("	CRC error, computed: %04X, received: %04X\n", computed_crc, received_crc);
#endif
		return BURST_CRC_ERROR;
	}

	// CRC check passed, we can remove it from the packet.
	ctx->out_head -= CRC_SIZE;

	#ifdef BURST_DECODER_DEBUG
	printf("	Final packet: ");
	for (size_t i = 0; i < ctx->out_head; i++) {
		printf("%02X ", ctx->buffer[i]);
	}
	printf("\n");
#endif
	return BURST_PACKET_READY;
}

burst_status_t burst_decoder_add_byte(burst_decoder_t *ctx, uint8_t byte) {
	// Handle 0x00 is ALWAYS a delimiter
	if (byte == 0) {
		// If we have data in the buffer, try to finish the packet
		if (ctx->out_head > 0) {
			burst_status_t res = burst_decoder_complete_packet(ctx);
			return res;
		}
		// If buffer is empty, it's just idle padding zeros; ignore them.
		burst_decoder_reset(ctx);
		return BURST_DATA_CONSUMED;
	}

	// 2. Prevent Buffer Overflow
	if (ctx->out_head >= ctx->buffer_size) {
		return BURST_OVERFLOW_ERROR;
	}

	switch (ctx->state) {
		case COBS_DECODE_READ_CODE:
			// This byte is a Code Byte (1-255)
			// Insert a 0x00 if this isn't the first block of the packet
			// AND the previous block wasn't a full 255-byte run.
			if (ctx->code != 0xFF && ctx->out_head > 0) {
				ctx->buffer[ctx->out_head++] = 0;
				if (ctx->out_head >= ctx->buffer_size) return BURST_OVERFLOW_ERROR;
			}

			ctx->block = ctx->code = byte;

			if (byte == 1) {
				// A code of 1 means a single zero was encoded;
				// the next byte will be another code byte.
				ctx->state = COBS_DECODE_READ_CODE;
			} else {
				ctx->state = COBS_DECODE_RUN;
			}
			break;

		case COBS_DECODE_RUN:
			ctx->buffer[ctx->out_head++] = byte;
			ctx->block--;

			// When the block counter hits 1, the next byte must be a code byte.
			if (ctx->block == 1) {
				ctx->state = COBS_DECODE_READ_CODE;
			}
			break;
	}

	return BURST_DATA_CONSUMED;
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
#ifdef BURST_DECODER_DEBUG
				printf("Decoded packet of size %d bytes\n", packet.size);
#endif
				if (packet.size > 0 && burst_managed_decoder->callback_function != NULL) {
					// Call the callback function with the received data
					burst_managed_decoder->callback_function(packet.data, packet.size, burst_managed_decoder->user_data);
				}

				burst_managed_decoder->statistics.bytes_processed += packet.size;
				burst_managed_decoder->statistics.packets_processed++;

				continue;
			}

			case BURST_CRC_ERROR:
#ifdef BURST_DECODER_DEBUG
				printf("CRC Error on packet\n");
#endif
				burst_managed_decoder->statistics.crc_errors++;
				continue;

			case BURST_DECODE_ERROR:
#ifdef BURST_DECODER_DEBUG
				printf("Decode Error on packet\n");
#endif
				burst_managed_decoder->statistics.decode_errors++;
				continue;

			case BURST_OVERFLOW_ERROR:
#ifdef BURST_DECODER_DEBUG
				printf("Overflow Error on packet\n");
#endif
				burst_managed_decoder->statistics.overflow_errors++;
				continue;

			default:
				continue;
		}
	}
	return bytes_consumed;
}
