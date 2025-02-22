from cobs import cobs
from crc import Calculator,Crc16

crc = Calculator(Crc16.IBM_3740)

class BurstFramingInterface:
    buffer = b""

    def __init__(self):
        pass

    @staticmethod
    def crc16( data: bytes) -> bytes:
        return crc.checksum(data).to_bytes(2, "big")
    

    @staticmethod
    def encode_packet(packet: bytes) -> bytes:
        packet_with_crc = packet + BurstFramingInterface.crc16(packet)
        return cobs.encode(packet_with_crc) + b"\x00"

    @staticmethod
    def decode_packet(packet: bytes) -> bytes:
        # decode and check crc
        decoded = cobs.decode(packet)

        if BurstFramingInterface.crc16(decoded[:-2]) != decoded[-2:]:
            raise ValueError("CRC mismatch")

        return decoded[:-2]

    def encode(self, packets: list[bytes]) -> bytes:
        return b"".join([self.encode_packet(packet) for packet in packets])

    def decode(self, steam: bytes) -> list[bytes]:
        self.buffer += steam
        separated_packets = self.buffer.split(b"\x00")
        # Add last packet to buffer
        self.buffer = separated_packets.pop()
        return [self.decode_packet(packet) for packet in separated_packets]


if __name__ == "__main__":
    bfi = BurstFramingInterface()
    packets = [b"Hello, world!", b"Goodbye, world!"]
    encoded = bfi.encode(packets)
    print(encoded)
    decoded = bfi.decode(encoded)
    print(decoded)
    
    
    
