
# mmp_adapter_circuitpy.py
# CircuitPython adapter using busio.UART (or board.USB_CDC for CDC)

from .mmp_adapter_base import MmpAdapterBase

class CircuitPyAdapter(MmpAdapterBase):
    def __init__(self, tx_pin=None, rx_pin=None, use_usb_cdc=False):
        super().__init__()
        self._tx = tx_pin
        self._rx = rx_pin
        self._use_usb = use_usb_cdc
        self._uart = None
        self._serial = None

    def open_baud(self, baud: int) -> bool:
        try:
            if self._use_usb:
                import usb_cdc
                self._serial = usb_cdc.data   # use secondary CDC channel if enabled
                if not self._serial:
                    return False
                # CircuitPython CDC baud is symbolic; store for reference only
                self.connected_baud = baud
                self.clear_input()
                return True
            else:
                import busio, board
                if self._tx is None or self._rx is None:
                    # Try common pins if user didn't specify (best-effort)
                    tx = getattr(board, "TX", None)
                    rx = getattr(board, "RX", None)
                else:
                    tx = self._tx
                    rx = self._rx
                if tx is None or rx is None:
                    return False
                self._uart = busio.UART(tx, rx, baudrate=baud, timeout=0)
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
        self._serial = None

    def clear_input(self) -> None:
        if self._uart:
            try:
                while True:
                    b = self._uart.read(1)
                    if not b:
                        break
            except Exception:
                pass
        elif self._serial:
            try:
                while self._serial.in_waiting:
                    self._serial.read(1)
            except Exception:
                pass

    def write_ascii(self, s: str) -> None:
        try:
            data = s.encode("ascii")
            if self._uart:
                self._uart.write(data)
            elif self._serial:
                self._serial.write(data)
        except Exception:
            pass

    def read_one_char(self):
        try:
            if self._uart:
                b = self._uart.read(1)
                if b:
                    return b.decode("ascii", "ignore")
            elif self._serial and self._serial.in_waiting:
                b = self._serial.read(1)
                if b:
                    return b.decode("ascii", "ignore")
        except Exception:
            return None
        return None

    def sleep_ms(self, ms: int) -> None:
        import time
        time.sleep(ms / 1000.0)

    def now_ms(self) -> int:
        import time
        # monotonic_ns available on CircuitPython
        return int(time.monotonic_ns() // 1_000_000)
