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
            cobs_status_t status = decoder_ingest(&decoder, data_ptr+bytes_consumed, data_size-bytes_consumed, &bytes_consumed);

            if (status == COBS_PACKET_READY)
            {
                packet_t packet = decoder_get_packet(&decoder);
                nb::bytes packet_bytes(reinterpret_cast<const char *>(packet.data), packet.size);
                result.append(packet_bytes);
            }

            if (status == COBS_CRC_ERROR)
            {
                if (fail_on_crc_error)
                    throw std::runtime_error("CRC error");
            }
        }

        return result;
    }
};

// NB_MODULE(nanobind_example_ext, m)
// {
// }

// nb::list decode(nb::bytes data)
// {

//     return result;
// }

// decoder_t decoder;
// uint8_t decoder_buffer[5000] = {0};
// int read_data_from_uart(uint8_t *data, uint32_t size) { return 0; }

// int main() {

//   my_decoder_init(&decoder, decoder_buffer, sizeof(decoder_buffer));

//   while (1) {
//     uint8_t bytes[1000] = {0};
//     uint32_t bytes_read = read_data_from_uart(bytes, sizeof(bytes));

//     // If no data is read, continue
//     if (bytes_read == 0) {
//       continue;
//     }

//     size_t bytes_consumed = 0;
//     while (bytes_consumed < bytes_read) {

//       cobs_status_t result =
//           decoder_ingest(&decoder, decoder_buffer, bytes_read, &bytes_consumed);

//       if (result == COBS_PACKET_READY) {
//         packet_t packet = decoder_get_packet(&decoder);
//         printf("received a packet of size %d\n", (int)packet.size);
//       }

//       if (result == COBS_CRC_ERROR) {
//         printf("CRC error\n");
//       }
//     }
//   }
// }

int add(int a, int b)
{
    printf("Addinrrg %d and %d\n", a, b);
    return a + b;
}

NB_MODULE(nanobind_example_ext, m)
{

    m.def("add", &add);

    m.def("add_Test", [](float a, int b)
          { return a + b; }, "test"_a, "b"_a);

    nb::class_<Decoder>(m, "Decoder")
        .def(nb::init<>())
        .def("decode", &Decoder::decode, "data"_a, "fail_on_crc_error"_a = false);
}