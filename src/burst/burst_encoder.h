#ifndef BURST_ENCODER_H
#define BURST_ENCODER_H

#include <burst/burst_generic.h>
#include <stdint.h>

typedef struct {
	uint32_t bytes_ingested;     // Total number of bytes ingested
	uint32_t bytes_processed;    // Total number of bytes after processing
	uint32_t packets_processed;  // Successfully decoded packets

	// Error statistics
	uint32_t overflow_errors;
	uint32_t bytes_discarted;  // Total number of bytes discarded

} burst_encoder_statistics_t;

typedef struct {
	uint8_t *buffer;     // Output buffer for encoded packets.
	size_t buffer_size;  // Total size of the output buffer.
	size_t out_head;     // Current offset (number of bytes written).
} burst_encoder_t;

typedef struct {
	burst_encoder_t encoder;                // Decoder instance
	burst_encoder_statistics_t statistics;  // Statistics for the decoder
} burst_managed_encoder_t;

// Encoder
void burst_encoder_init(burst_encoder_t *ctx, uint8_t *buffer, size_t size);
burst_status_t burst_encoder_add_packet(burst_encoder_t *ctx, const uint8_t *data, size_t size);
burst_packet_t burst_encoder_flush(burst_encoder_t *ctx);
void burst_managed_encoder_init(burst_managed_encoder_t *burst_managed_encoder, uint8_t *buffer, size_t size);
int burst_managed_encoder_add_packet(burst_managed_encoder_t *burst_managed_encoder, const uint8_t *data, size_t len);
burst_packet_t burst_managed_encoder_flush(burst_managed_encoder_t *burst_managed_encoder);
int burst_managed_encoder_free_space(burst_managed_encoder_t *burst_managed_encoder);

#endif  // BURST_ENCODER_H