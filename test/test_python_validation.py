from burst_link_protocol import BurstInterfacePy
import pytest


def test_python():
    interface = BurstInterfacePy()
    packets = [b"Hello, world!", b"Goodbye, world!"]

    data = interface.encode(packets)
    decoded = interface.decode(data)
    assert packets == decoded


def test_python_crc_validation():
    interface = BurstInterfacePy()
    packets = [b"Hello, world!", b"Goodbye, world!"]

    data = bytearray(interface.encode(packets))

    for i in range(len(data)):
        data_copy = data.copy()

        # modify byte x
        data_copy[i] = (data_copy[i] + 1) % 256
        with pytest.raises(Exception):
            decoded = interface.decode(bytes(data_copy))
            print(decoded)
            assert len(decoded) == len(packets), f"Expected {len(packets)} packets, got {len(decoded)}"
