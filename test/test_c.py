from burst_link_protocol import BurstInterfaceC


def test_c_encoding_decoding():
    c_interface = BurstInterfaceC()

    packets = [b"Hello, world!", b"Goodbye, world!"]

    decoded = c_interface.decode(c_interface.encode(packets))
    print(decoded)
    assert packets == decoded, f"Expected {packets}, got {decoded}"


def test_c_encoding_decoding_fail():
    c_interface = BurstInterfaceC()

    packets = [b"\0Hello, world!"]

    decoded = c_interface.decode(c_interface.encode(packets))
    print(decoded)
    assert packets == decoded, f"Expected {packets}, got {decoded}"

if __name__ == "__main__":
    test_c_encoding_decoding()
