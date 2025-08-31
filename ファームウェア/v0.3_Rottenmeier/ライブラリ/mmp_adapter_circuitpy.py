# CircuitPython用アダプタ（busio.UART）

from mmp_adapter_base import UartAdapterBase
import time
import board
import busio

class CircuitPyAdapter(UartAdapterBase):
    def __init__(self, tx_pin=None, rx_pin=None, baud=115200, timeout_ms=100, rxbuf=64):
        self._tx_pin = tx_pin
        self._rx_pin = rx_pin
        self.baud = int(baud)
        self.timeout_ms = int(timeout_ms)
        self.timeout_s = float(timeout_ms / 1000.0)
        self.rxbuf = int(rxbuf)
        self.uart = None

    def _resolve_pin(self, pin, is_tx):
        try:
            if hasattr(pin, "name") or "board." in repr(pin):
                return pin
        except Exception:
            pass
        if pin is None:
            return getattr(board, "TX" if is_tx else "RX")
        if isinstance(pin, str):
            name = pin
            if hasattr(board, name):
                return getattr(board, name)
            if hasattr(board, name.upper()):
                return getattr(board, name.upper())
            if name.isdigit():
                for prefix in ("GP", "D", "IO"):
                    cand = f"{prefix}{name}"
                    if hasattr(board, cand):
                        return getattr(board, cand)
            raise ValueError("指定ピンを解決できません: '%s'" % pin)
        if isinstance(pin, int):
            for prefix in ("GP", "D", "IO"):
                cand = f"{prefix}{pin}"
                if hasattr(board, cand):
                    return getattr(board, cand)
            raise ValueError("数値ピンを解決できません: %s" % pin)
        return pin

    def open(self):
        tx = self._resolve_pin(self._tx_pin, True)
        rx = self._resolve_pin(self._rx_pin, False)
        self.uart = busio.UART(
            baudrate=self.baud,
            timeout=self.timeout_s,
            bits=8, parity=None, stop=1,
            tx=tx, rx=rx,
            receiver_buffer_size=self.rxbuf,
        )
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
            try:
                return self.uart.read(1)
            except Exception:
                return b""

    def write(self, b):
        try:
            return self.uart.write(b)
        except Exception:
            return 0

    def flush(self):
        try:
            if hasattr(self.uart, "reset_input_buffer"):
                try:
                    self.uart.reset_input_buffer()
                    return
                except Exception:
                    pass
            deadline = time.monotonic() + (30 / 1000.0)
            while time.monotonic() < deadline:
                n = 0
                if hasattr(self.uart, "in_waiting"):
                    try:
                        n = int(self.uart.in_waiting) or 0
                    except Exception:
                        n = 0
                if n > 0:
                    try:
                        self.uart.read(n)
                    except Exception:
                        try:
                            self.uart.read()
                        except Exception:
                            break
                else:
                    try:
                        b = self.uart.read(1)
                    except Exception:
                        b = None
                    if not b:
                        break
                time.sleep(0.001)
        except Exception:
            pass

    def close(self):
        try:
            if self.uart:
                self.uart.deinit()
        except Exception:
            pass
        self.uart = None
