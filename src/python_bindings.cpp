#include <nanobind/nanobind.h>
#include <burst_interface.h>

namespace nb = nanobind;
using namespace nb::literals;

struct Decoder
{
    burst_decoder_t decoder;
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
            burst_status_t status = decoder_ingest(&decoder, data_ptr + bytes_consumed, data_size - bytes_consumed, &bytes_consumed);

            if (status == BURST_PACKET_READY)
            {
                burst_packet_t packet = decoder_get_packet(&decoder);
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
};

NB_MODULE(burst_interface_c, m)
{

    nb::class_<Decoder>(m, "BurstInterfaceC")
        .def(nb::init<>())
        .def("decode", &Decoder::decode, "data"_a, "fail_on_crc_error"_a = false);
}