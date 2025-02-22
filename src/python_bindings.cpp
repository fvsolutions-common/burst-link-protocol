#include <nanobind/nanobind.h>

#include <decoder.h>

namespace nb = nanobind;
using namespace nb::literals;

struct Decoder
{
    decoder_t decoder;
    uint8_t decoder_buffer[1024] = {0};

    Decoder()
    {
        my_decoder_init(&decoder, decoder_buffer, sizeof(decoder_buffer));
    }

    nb::list decode(nb::bytes data, bool fail_on_crc_error = false)
    {
        nb::list result;
        uint8_t *data_ptr = (uint8_t *)data.data();
        size_t data_size = data.size();

        size_t bytes_consumed = 0;
        while (bytes_consumed < data_size)
        {
            cobs_status_t status = decoder_ingest(&decoder, data_ptr + bytes_consumed, data_size - bytes_consumed, &bytes_consumed);

            if (status == COBS_PACKET_READY)
            {
                packet_t packet = decoder_get_packet(&decoder);
                nb::bytes packet_bytes(reinterpret_cast<const char *>(packet.data), packet.size);
                result.append(packet_bytes);
            }

            if (fail_on_crc_error)
            {
                if (status == COBS_CRC_ERROR)
                {
                    throw std::runtime_error("CRC error");
                }
                if (status == COBS_DECODE_ERROR)
                {
                    throw std::runtime_error("Decode error");
                }
                if (status == COBS_OVERFLOW_ERROR)
                {
                    throw std::runtime_error("Overflow error");
                }
            }
        }
        return result;
    }
};

NB_MODULE(nanobind_example_ext, m)
{

    nb::class_<Decoder>(m, "Decoder")
        .def(nb::init<>())
        .def("decode", &Decoder::decode, "data"_a, "fail_on_crc_error"_a = false);
}