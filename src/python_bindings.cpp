#include <nanobind/nanobind.h>
extern "C"
{
#include <burst_interface.h>
}

namespace nb = nanobind;
using namespace nb::literals;

struct BurstInterface
{
    burst_decoder_t decoder;
    uint8_t decoder_buffer[1024] = {0};
    burst_encoder_t encoder;
    uint8_t encoder_buffer[1024] = {0};

    BurstInterface()
    {
        burst_decoder_init(&decoder, decoder_buffer, sizeof(decoder_buffer));
        burst_encoder_init(&encoder, encoder_buffer, sizeof(encoder_buffer));
    }

    nb::list decode(nb::bytes data, bool fail_on_crc_error = false)
    {
        nb::list result;
        uint8_t *data_ptr = (uint8_t *)data.data();
        size_t data_size = data.size();

        size_t bytes_consumed = 0;
        while (bytes_consumed < data_size)
        {
            burst_status_t status = bust_decoder_add_data(&decoder, data_ptr + bytes_consumed, data_size - bytes_consumed, &bytes_consumed);

            if (status == BURST_PACKET_READY)
            {
                burst_packet_t packet = burst_decoder_get_packet(&decoder);
                nb::bytes packet_bytes(reinterpret_cast<const char *>(packet.data), packet.size);
                result.append(packet_bytes);
            }

            if (fail_on_crc_error)
            {
                if (status == BURST_CRC_ERROR)
                {
                    throw std::runtime_error("CRC error");
                }
                if (status == BURST_DECODE_ERROR)
                {
                    throw std::runtime_error("Decode error");
                }
                if (status == BURST_OVERFLOW_ERROR)
                {
                    throw std::runtime_error("Overflow error");
                }
            }
        }
        return result;
    }

    nb::bytes encode(nb::list data)
    {
        for (size_t i = 0; i < data.size(); i++)
        {
            nb::bytes data_bytes = data[i];
            burst_encoder_add_packet(&encoder, (uint8_t *)data_bytes.data(), data_bytes.size());
        }
        // flush the encoder
        burst_packet_t packet = burst_encoder_flush(&encoder);
        return nb::bytes(reinterpret_cast<const char *>(packet.data), packet.size);
    }
};

NB_MODULE(burst_interface_c, m)
{

    nb::class_<BurstInterface>(m, "BurstInterfaceC")
        .def(nb::init<>())
        .def("decode", &BurstInterface::decode, "data"_a, "fail_on_crc_error"_a = false)
        .def("encode", &BurstInterface::encode, "packets"_a);
}