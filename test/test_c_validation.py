
from burst_link_protocol import BurstInterfacePy,BurstInterfaceC
import pytest
import numpy as np

def test_c_decoder_python():
    python_interface =  BurstInterfacePy()
    c_interface =  BurstInterfaceC()

    packets = [b"Hello, world!", b"Goodbye, world!"]

    data = python_interface.encode(packets)
    decoded = c_interface.decode(data)
    print(decoded)
    assert packets == decoded

def test_python_crc_validation():
    interface =  BurstInterfacePy()
    packets = [b"Hello, world!", b"Goodbye, world!"]

    data = bytearray(interface.encode(packets))

    for i in range( len(data)):
        c_interface =  BurstInterfaceC()

        data_copy = data.copy()

        # modify byte x
        data_copy[i] = (data_copy[i] + 1) % 256
        with pytest.raises(Exception):
            decoded = c_interface.decode(bytes(data_copy),fail_on_crc_error=True)
            print(decoded)
            assert len(decoded) == len(packets), f"Expected {len(packets)} packets, got {len(decoded)}"

def test_max_size_error():
    interface =  BurstInterfacePy()
    c_interface =  BurstInterfaceC()
    packets = np.random.bytes(1500)
    with pytest.raises(Exception):
        data = c_interface.decode(interface.encode([packets]),fail_on_crc_error=True)
    
def test_encoding():
    interface =  BurstInterfacePy()
    c_interface =  BurstInterfaceC()
    packets = [b"Hello, world!", b"Goodbye, world!"]

    assert interface.encode(packets) == c_interface.encode(packets)

# if __name__ == "__main__":
#     # test_c_decoder_python()
#     # test_python_crc_validation()
#     # max_size_error()