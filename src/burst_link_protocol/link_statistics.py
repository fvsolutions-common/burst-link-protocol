import time

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


class BurstLinkStatistics(BaseModel):
    last_update_timestamp: float = Field(default_factory=time.time)

    bytes_handled: int = 0
    bytes_processed: int = 0
    packets_processed: int = 0
    crc_errors: int = 0
    overflow_errors: int = 0
    decode_errors: int = 0

    handled_bytes_per_second: float = 0.0
    processed_bytes_per_second: float = 0.0
    processed_packets_per_second: float = 0.0

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

            self.handled_bytes_per_second = (bytes_handled - self.bytes_handled) / delta_time
            self.processed_bytes_per_second = (bytes_processed - self.bytes_processed) / delta_time
            self.processed_packets_per_second = (packets_processed - self.packets_processed) / delta_time

            self.bytes_handled = bytes_handled
            self.bytes_processed = bytes_processed
            self.packets_processed = packets_processed
            self.crc_errors = crc_errors
            self.overflow_errors = overflow_errors
            self.decode_errors = decode_errors

        return self

    def __str__(self):
        return (
            f"Byte Raw: {to_si(self.bytes_handled, 'B')} ({to_si(self.handled_bytes_per_second * 8, 'bps')}), "
            f"Bytes processed: {to_si(self.bytes_processed, 'B')} ({to_si(self.processed_bytes_per_second * 8, 'bps')}), "
            f"Packets processed: {self.packets_processed} ({to_si(self.processed_packets_per_second, 'packets/s')}), "
            f"Errors (CRC: {self.crc_errors}, Overflow: {self.overflow_errors}, Decode: {self.decode_errors})"
        )

    def to_dict(self):
        return self.model_dump(exclude={"last_update_timestamp"})
