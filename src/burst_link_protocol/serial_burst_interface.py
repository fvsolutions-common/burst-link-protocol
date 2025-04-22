from burst_interface_c import  BurstInterfaceC
import serial
import time
import threading
import asyncio
import janus
from pydantic import BaseModel, Field

def to_si(value: float, suffix: str) -> str:
    """
    Convert a value to a string with SI suffix.
    """
    if value == 0:
        return "0"
    elif value < 1e-3:
        return f"{value:.2f} {suffix}"
    elif value < 1e3:
        return f"{value:.2f} {suffix}"
    elif value < 1e6:
        return f"{value / 1e3:.2f} k{suffix}"
    elif value < 1e9:
        return f"{value / 1e6:.2f} M{suffix}"
    else:
        return f"{value / 1e9:.2f} G{suffix}"


class BurstSerialStatistics(BaseModel):
    last_update_timestamp: float = Field(default_factory=time.time)

    bytes_handled: int = 0
    bytes_processed: int = 0
    packets_processed: int = 0
    crc_errors: int = 0
    overflow_errors: int = 0
    decode_errors: int = 0

    handled_bytes_per_second: float = 0
    processed_bytes_per_second: float = 0
    processed_packets_per_second: float = 0

    def update(
        self,
        bytes_handled,
        bytes_processed,
        packets_processed,
        crc_errors,
        overflow_errors,
        decode_errors,
    ):

        now = time.time()
        if now - self.last_update_timestamp > 1:
            delta_time = now - self.last_update_timestamp
            self.last_update_timestamp = now

            self.handled_bytes_per_second = (
                (bytes_handled - self.bytes_handled) / delta_time
            )
            self.processed_bytes_per_second = (
                (bytes_processed - self.bytes_processed) / delta_time
            )
            self.processed_packets_per_second = (
                (packets_processed - self.packets_processed) / delta_time
            )

            self.bytes_handled = bytes_handled
            self.bytes_processed = bytes_processed
            self.packets_processed = packets_processed
            self.crc_errors = crc_errors
            self.overflow_errors = overflow_errors
            self.decode_errors = decode_errors

        return self

    def __str__(self):
        return (
            f"Byte Raw: {to_si(self.bytes_handled, 'B')} ({to_si(self.handled_bytes_per_second*8, 'bps')}), "
            f"Bytes processed: {to_si(self.bytes_processed, 'B')} ({to_si(self.processed_bytes_per_second*8, 'bps')}), "
            f"Packets processed: {self.packets_processed} ({to_si(self.processed_packets_per_second, 'packets/s')}), "
            f"Errors (CRC: {self.crc_errors}, Overflow: {self.overflow_errors}, Decode: {self.decode_errors})"
        )

class SerialBurstInterface:
    debug_timings = False
    debug_io = False

    kill = False
    block_size = 1000
    RATE_CHECK_INTERVAL = 1

    interface: BurstInterfaceC
    last_rate_timestamp: float = 0

    statitsics: BurstSerialStatistics

    @classmethod
    def from_serial(cls, port: str, bitrate: int):
        serial_handle: serial.Serial = serial.Serial(port, bitrate, timeout=0.5)
        serial_handle.set_buffer_size(rx_size=100 * 1024, tx_size=100 * 1024)  # type: ignore
        return cls(serial_handle)

    def __init__(self, serial_handle: serial.Serial):
        self.handle = serial_handle
        self.handle.reset_input_buffer()
        self.handle.reset_output_buffer()

        self.current_stats = BurstSerialStatistics()
        self.statitsics = BurstSerialStatistics()

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

    def close(self):
        self.kill = True
        self.handle.close()
        self.transmit_packet_queue.close()
        self.receive_packet_queue.close()

    def receive_task(self):
        try:
            while True:
                # Read incoming data
                data = self.handle.read(self.block_size)

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
                self.handle.write(data)

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
