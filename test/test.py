from nanobind_example_ext import Decoder
from cobs import cobs
import numpy as np
import time

def add_crc(data: bytes) -> bytes:
    crc16 = 18604
    return data + crc16.to_bytes(2, byteorder='big')

def generate_payload(length: int) -> bytes:
    return cobs.encode(add_crc(np.random.bytes(length))) + b'\0'

def generate_payloads(length: int, num: int) -> bytes:
    return b''.join([generate_payload(length) for _ in range(num)])


test_payload = generate_payloads(500, 1000)



decoder = Decoder()
start = time.time()
result = decoder.decode(test_payload)
print(len(result))
end = time.time()

print(print(f"Took {end - start} seconds to decode {len(test_payload)} bytes"))