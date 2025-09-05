
# mmp_adapter_micropy.py
# MicroPython adapter using machine.UART

from .mmp_adapter_base import MmpAdapterBase

class MicroPyAdapter(MmpAdapterBase):
    def __init__(self, uart_id=0, tx=None, rx=None):
        super().__init__()
        self._uart_id = uart_id
        self._tx = tx
        self._rx = rx
        self._uart = None

    def open_baud(self, baud: int) -> bool:
        try:
            from machine import UART, Pin
            kwargs = {"baudrate": baud}
            if self._tx is not None:
                kwargs["tx"] = self._tx
            if self._rx is not None:
                kwargs["rx"] = self._rx
            self._uart = UART(self._uart_id, **kwargs)
            self.connected_baud = baud
            self.clear_input()
            return True
        except Exception:
            return False

    def close(self) -> None:
        try:
            if self._uart:
                self._uart.deinit()
        except Exception:
            pass
        self._uart = None

    def clear_input(self) -> None:
        if not self._uart:
            return
        try:
            while self._uart.any():
                self._uart.read(1)
        except Exception:
            pass

    def write_ascii(self, s: str) -> None:
        if not self._uart:
            return
        try:
            self._uart.write(s.encode("ascii"))
        except Exception:
            pass

    def read_one_char(self):
        if not self._uart:
            return None
        try:
            if self._uart.any():
                b = self._uart.read(1)
                if b:
                    try:
                        return b.decode("ascii")
                    except Exception:
                        return None
        except Exception:
            return None
        return None

    def sleep_ms(self, ms: int) -> None:
        import time
        time.sleep_ms(ms)

    def now_ms(self) -> int:
        import time
        return time.ticks_ms()

    def ticks_diff(self, now_ms: int, start_ms: int) -> int:
        import time
        return time.ticks_diff(now_ms, start_ms)
