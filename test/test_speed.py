
from burst.burst_interface import BurstFramingInterface
from nanobind_example_ext import Decoder
import pytest
from cobs import cobs
import tqdm
import numpy as np
import time

def prepare_packets(packet_size:int) -> bytes:
    python_interface =  BurstFramingInterface()
    return python_interface.encode([np.zeros(packet_size).tobytes()])



@pytest.mark.parametrize("n", [1, 10, 100, 1000])
def test_performance_c(benchmark, n):
    c_interface =  Decoder()
    benchmark(c_interface.decode, prepare_packets(n))

@pytest.mark.parametrize("n", [1, 10, 100, 1000])
def test_performance_python(benchmark, n):
    c_interface =  BurstFramingInterface()
    benchmark(c_interface.decode, prepare_packets(n))
