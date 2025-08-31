# CPython用アダプタ（pyserial）

from mmp_adapter_base import UartAdapterBase
import serial

class CpythonAdapter(UartAdapterBase):
    def __init__(self, baud=115200, timeout_s=0.2):
        self.baud = int(baud)
        self.timeout_s = float(timeout_s)
        self.uart = None

    def open_port(self, port_name):
        s = serial.Serial()
        s.baudrate = self.baud
        s.timeout = self.timeout_s
        s.write_timeout = self.timeout_s
        s.port = port_name
        s.open()
        self.uart = s
        return True

    def rx_ready(self):
        try:
            return int(self.uart.in_waiting) or 0
        except Exception:
            return 0

    def read(self, n):
        try:
            return self.uart.read(n)
        except Exception:
            return b""

    def write(self, b):
        try:
            return self.uart.write(b)
        except Exception:
            return 0

    def flush(self):
        try:
            self.uart.reset_input_buffer()
            self.uart.reset_output_buffer()
        except Exception:
            pass

    def close(self):
        try:
            if self.uart and self.uart.is_open:
                self.uart.close()
        except Exception:
            pass
        self.uart = None
