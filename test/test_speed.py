
from burst_interface import BurstInterfacePy,BurstInterfaceC
import pytest
from cobs import cobs
import tqdm
import numpy as np
import time

def prepare_packets(packet_size:int) -> bytes:
    python_interface =  BurstInterfacePy()
    return python_interface.encode([np.zeros(packet_size).tobytes()])



@pytest.mark.parametrize("n", [1, 10, 100, 1000])
def test_performance_c(benchmark, n):
    c_interface =  BurstInterfaceC()
    benchmark(c_interface.decode, prepare_packets(n))

@pytest.mark.parametrize("n", [1, 10, 100, 1000])
def test_performance_python(benchmark, n):
    c_interface =  BurstInterfacePy()
    benchmark(c_interface.decode, prepare_packets(n))
