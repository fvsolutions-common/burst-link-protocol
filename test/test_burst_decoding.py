from burst_link_protocol import BurstInterfaceC, BurstInterfacePy

payloads = [
    # Malformed packet
    (
        "04 01 ff 0e 06 d3 23 01 01 01 01 01 01 01 01 01 01 01 01 01 01 01 01 01 01 01 01 01 01 01 01 01 01 01 01 01 01 01 01 01 01 01 01 01 01 01 01 01 01 03 85 1f 00 0b 02 04 02 07 02 0b 02 04 02 08 02 0b 02 04 02 0c 02 0b 02 02 02 0d 02 0b 02 02 03 aa 94 00 04 02 ff 0e 03 d3 23 01 01 01 01 01 01 01 01 01 01 01 01 01 01 01 01 01 01 01 01 01 01 01 01 01 01 01 01 01 01 01 01 01 01 01 01 01 01 01 01 01 01 03 85 1f 00"
    )
    # Good packet
    (
        "04 02 ff 0e 03 e1 23 01 01 01 01 01 01 01 01 01 01 01 01 01 01 01 01 01 01 01 01 01 01 01 01 01 01 01 01 01 01 01 01 01 01 01 01 01 01 01 01 01 01 03 99 fe 00"
    )(
        "04 01 ff 0e 06 ef be ad de c1 02 0b 02 01 02 01 02 0b 02 04 02 02 02 0b 02 04 02 03 02 0b 02 04 02 04 02 0b 02 04 02 05 02 0b 02 04 02 06 02 0b 02 04 02 a6 02 0b 02 04 02 07 02 0b 02 04 02 08 02 0b 02 04 02 0c 02 0b 02 02 02 0d 02 0b 02 02 03 aa 94 00"
    )
]


def test_multi_encoded_payloads():
    c_interface = BurstInterfaceC()
    py_interface = BurstInterfacePy()
    for payload in payloads:
        payload = bytes.fromhex(payload.replace(" ", ""))
        print(f"Testing payload: {payload.hex()}")
        py_decoded_packet = py_interface.decode(payload)
        print(f"Decoded payload: {py_decoded_packet}")
        c_decoded_packet = c_interface.decode(payload, fail_on_crc_error=True)
        print(f"Decoded payload: {c_decoded_packet}")


if __name__ == "__main__":
    test_multi_encoded_payloads()
