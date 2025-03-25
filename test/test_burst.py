from burst_link_protocol import BurstInterfaceC, BurstInterfacePy

payloads = [
    ("00"),
    ("01 01 01 00 00 00 00 00 00 00 00"),
    ("00 00 00 00 00 00 00 00 00 00 00"),
    ("00 00 00 00 00 00 00 00 00 00 00"),
    ("00 00 00 00 00 00 00 00 00 00 00"),
    ("00 00 00 00 00 00 00 00 00 00 00"),
    ("0A 0F 0A 0B 08 80 80 88 50 12 04 26 00 00 00 10 00"),
    ("01"),
    ("FF"),
    ("00" * 200),
    ("00" * 300),
    ("AA" * 200),
    ("AA" * 500),
    ("AA" * 600),
]


def test_multi_payload():
    c_interface = BurstInterfaceC()
    py_interface = BurstInterfacePy()
    # payload = bytes.fromhex("0A 0F 0A 0B 08 80 80 88 50 12 04 26 00 00 00 10 00")
    for payload in payloads:
        payload = bytes.fromhex(payload.replace(" ", ""))
        
        if len(payload) < 20:
            print(f"Testing payload: {payload.hex()}")
        else:
            print(f"Testing payload: {payload[:20].hex()}... {len(payload)} bytes")

        c_payload = c_interface.encode([payload])
        py_payload = py_interface.encode([payload])

        assert c_payload == py_payload, f"Expected {py_payload}, got {c_payload}"

        py_decoded_packet = py_interface.decode(c_payload)
        c_decoded_packet = c_interface.decode(py_payload, fail_on_crc_error=True)

        assert len(c_decoded_packet) == 1, (
            f"Expected 1 packet, got {len(c_decoded_packet)}"
        )
        assert len(py_decoded_packet) == 1, (
            f"Expected 1 packet, got {len(py_decoded_packet)}"
        )

        assert py_decoded_packet[0] == c_decoded_packet[0], (
            f"Expected {py_decoded_packet}, got {c_decoded_packet}"
        )


if __name__ == "__main__":
    test_multi_payload()
