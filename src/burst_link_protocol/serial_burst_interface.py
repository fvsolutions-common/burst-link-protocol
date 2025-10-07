import asyncio
import threading
import time

import janus
import serial
from burst_interface_c import BurstInterfaceC

from .link_statistics import BurstLinkStatistics
import io
import socket

class TCPSocket(io.RawIOBase):
    def __init__(self, sock: socket.socket):
        self.sock = sock

    def read(self, size=-1):
        try:
            return self.sock.recv(size)
        except BlockingIOError:
            return b""

    def write(self, b):
        return self.sock.send(b)

    def close(self):
        self.sock.close()
        return super().close()
    

class SerialBurstInterface:
    debug_timings = False
    debug_io = False

    kill = False
    block_size = 1000
    RATE_CHECK_INTERVAL = 1

    interface: BurstInterfaceC
    _statistics: BurstLinkStatistics
    _handle: io.RawIOBase
    last_rate_timestamp: float = 0

    @classmethod
    def from_serial(cls, port: str, bitrate: int):
        serial_handle: serial.Serial = serial.Serial(port, bitrate, timeout=0.5)
        serial_handle.set_buffer_size(rx_size=100 * 1024, tx_size=100 * 1024)  # type: ignore
        return cls(serial_handle)

    @classmethod
    def from_file(cls, file_path: str):
        file_handle = open(file_path, "r+b", buffering=0)
        return cls(file_handle)

    @classmethod
    def from_tcp(cls, host: str, port: int):
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.connect((host, port))
        sock.setblocking(False)
        return cls(TCPSocket(sock))

    def __init__(self, serial_handle: io.RawIOBase):
        self._handle = serial_handle

        if isinstance(self._handle, serial.Serial):
            self._handle.reset_input_buffer()
            self._handle.reset_output_buffer()

        self.current_stats = BurstLinkStatistics()
        self._statistics = BurstLinkStatistics()

        self.receive_task_handle = threading.Thread(target=self.receive_task, daemon=True)
        self.transmit_task_handle = threading.Thread(target=self.transmit_task, daemon=True)

        self.interface = BurstInterfaceC()
        self.transmit_packet_queue = janus.Queue()
        self.receive_packet_queue = janus.Queue()

        self.receive_task_handle.start()
        self.transmit_task_handle.start()

    @property
    def statistics(self):
        return self.current_stats.update(
            self.interface.bytes_handled,
            self.interface.bytes_processed,
            self.interface.packets_processed,
            self.interface.crc_errors,
            self.interface.overflow_errors,
            self.interface.decode_errors,
        )

    def close(self, timeout: float = 1.0):
        self.kill = True
        self.transmit_packet_queue.close()
        self.receive_packet_queue.close()
        self.transmit_task_handle.join(timeout=timeout)

    def receive_task(self):
        try:
            while True:
                # Read incoming data
                data = self._handle.read(self.block_size)

                if self.kill:
                    break

                if data:
                    if self.debug_io:
                        print(f"Received burst frame: {' '.join([f'{x:02X}' for x in data])}, length: {len(data)}")
                    try:
                        decoded_packets = self.interface.decode(data, fail_on_crc_error=True)
                    except Exception as e:
                        print(f"Error decoding: {e}")
                        continue

                    for packet in decoded_packets:
                        # put all packets in the receive queue
                        if self.debug_io:
                            print(f"Received: {packet}")

                        self.receive_packet_queue.sync_q.put(packet)

                time.sleep(0.001)

        except Exception as e:
            print(f"Error in read task: {e}")
            self.close()

    def transmit_task(self):
        try:
            while True:
                packet = self.transmit_packet_queue.sync_q.get()
                if self.debug_io:
                    print(f"Transmitting packet: {' '.join([f'{x:02X}' for x in packet])}")

                data = self.interface.encode([packet])

                if self.debug_io:
                    from cobs import cobs

                    daat = cobs.decode(data[:-1])
                    # print in space separated hex
                    print(f"Transmitting burst frame: {' '.join([f'{x:02X}' for x in daat])}")

                    # print raw frame
                    print(f"Transmitting 'raw' burst frame: {' '.join([f'{x:02X}' for x in data])}")
                self._handle.write(data)

        except Exception as e:
            print(f"Error in transmit task: {e}")
            self.close()

    async def send(self, data: bytes):
        await self.transmit_packet_queue.async_q.put(data)

    async def flush_receive_queue(self):
        while not self.receive_packet_queue.async_q.empty():
            self.receive_packet_queue.async_q.get_nowait()

    async def send_with_response(self, data: bytes):
        # Flush all other packets
        await self.flush_receive_queue()

        start_time = time.time()
        await self.transmit_packet_queue.async_q.put(data)
        response = await self.receive_packet_queue.async_q.get()
        end_time = time.time()

        if self.debug_timings:
            print(f"Time taken: {(end_time - start_time) * 1000} ms for {len(data)} bytes")

        return response

    async def receive(self):
        return await self.receive_packet_queue.async_q.get()

    def receive_all(self):
        packets = []
        while not self.receive_packet_queue.sync_q.empty():
            packets.append(self.receive_packet_queue.sync_q.get())
        return packets


async def main():
    interface = SerialBurstInterface.from_serial("COM4", 115200)

    for i in range(1000):
        response = await interface.send_with_response(10 * f"Hello World {i}!".encode())
        print(f"Received: {response}")
        await asyncio.sleep(0.01)


if __name__ == "__main__":
    asyncio.run(main())
