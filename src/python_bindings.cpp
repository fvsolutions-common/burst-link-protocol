#include <nanobind/nanobind.h>
extern "C" {
#include <burst_link_protocol.h>
}

namespace nb = nanobind;
using namespace nb::literals;

struct BurstInterface {
	burst_managed_decoder_t decoder;
	uint8_t decoder_buffer[1024] = {0};
	burst_encoder_t encoder;
	uint8_t encoder_buffer[1024] = {0};

	// Create buffer to hold the the vector of bytes likt a list

	nb::list result;

	BurstInterface() {
		burst_managed_decoder_init(&decoder, decoder_buffer, sizeof(decoder_buffer), add_packet, this);
		burst_encoder_init(&encoder, encoder_buffer, sizeof(encoder_buffer));
	}

	static int add_packet(const uint8_t *data, size_t size, void *user_data) {
		// Create a bytes object from the data and append it to the result list
		// nb::bytes packet_bytes(reinterpret_cast<const char *>(data), size);
		// result.append(packet_bytes);
		BurstInterface *self = static_cast<BurstInterface *>(user_data);
		nb::bytes packet_bytes(reinterpret_cast<const char *>(data), size);
		self->result.append(packet_bytes);  // Append the packet to the result list
		return 0;                           // Return 0 to indicate success
	}
	nb::list decode(nb::bytes data, bool fail_on_crc_error = false) {
		result.clear();  // Clear the result list before returning
		burst_managed_decoder_handle_data(&decoder, (uint8_t *)data.data(), data.size());

		return result;  // Return the result list containing the decoded packets
	}

	nb::bytes encode(nb::list data) {
		for (size_t i = 0; i < data.size(); i++) {
			nb::bytes data_bytes = data[i];
			burst_encoder_add_packet(&encoder, (uint8_t *)data_bytes.data(), data_bytes.size());
		}
		// flush the encoder
		burst_packet_t packet = burst_encoder_flush(&encoder);
		return nb::bytes(reinterpret_cast<const char *>(packet.data), packet.size);
	}

	uint32_t get_bytes_handled() {
		return decoder.statistics.bytes_handled;
	}
	uint32_t get_packets_handled() {
		return decoder.statistics.packets_processed;
	}

	uint32_t get_crc_error_count() {
		return decoder.statistics.crc_errors;
	}
	uint32_t get_overrun_count() {
		return decoder.statistics.overflow_errors;
	}
	uint32_t get_packet_error_count() {
		return decoder.statistics.decode_errors;
	}
	

	// Statistics
};

NB_MODULE(burst_interface_c, m) {
	nb::set_leak_warnings(false);

	nb::class_<BurstInterface>(m, "BurstInterfaceC")
		.def(nb::init<>())
		.def("decode", &BurstInterface::decode, "data"_a, "fail_on_crc_error"_a = false)
		.def("encode", &BurstInterface::encode, "packets"_a)
		// Use a lambda
		.def_prop_ro("bytes_handled", &BurstInterface::get_bytes_handled);
}