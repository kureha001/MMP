
# mmp_adapter_cpython.py
# CPython adapter using pyserial

from mmp_adapter_base import MmpAdapterBase

class CpyAdapter(MmpAdapterBase):
    def __init__(self, port: str = None, preferred_ports=None):
        super().__init__()
        self._port_name = port
        self._preferred = preferred_ports or []
        self._ser = None

    # -- helpers --
    def _iter_ports(self):
        # Lazily import to keep install-time light
        try:
            from serial.tools import list_ports
        except Exception:
            return []
        ports = [p.device for p in list_ports.comports()]
        # Promote preferred ports (keep relative order otherwise)
        if self._preferred:
            pref = [p for p in self._preferred if p in ports]
            others = [p for p in ports if p not in pref]
            return pref + others
        return ports

    def _open_named(self, name, baud):
        import serial
        try:
            ser = serial.Serial(name, baudrate=baud, timeout=0, write_timeout=0)
            # DTR/RTS toggles can help some USB bridges
            try:
                ser.dtr = True
                ser.rts = True
            except Exception:
                pass
            self._ser = ser
            self.connected_baud = baud
            # Drain any pending input
            self.clear_input()
            return True
        except Exception:
            return False

    # --- lifecycle ---
    def open_baud(self, baud: int) -> bool:
        # If explicit port provided, try it only
        if self._port_name:
            return self._open_named(self._port_name, baud)

        # Else enumerate and try in order
        for p in self._iter_ports():
            if self._open_named(p, baud):
                # Remember the port actually used
                self._port_name = p
                return True
        return False

    def close(self) -> None:
        if self._ser:
            try:
                self._ser.close()
            except Exception:
                pass
        self._ser = None

    # --- I/O ---
    def clear_input(self) -> None:
        if not self._ser:
            return
        try:
            n = self._ser.in_waiting
        except Exception:
            n = 0
        if n:
            try:
                self._ser.read(n)
            except Exception:
                pass

    def write_ascii(self, s: str) -> None:
        if not self._ser:
            return
        try:
            self._ser.write(s.encode("ascii", "ignore"))
        except Exception:
            pass

    def read_one_char(self):
        if not self._ser:
            return None
        try:
            if self._ser.in_waiting > 0:
                b = self._ser.read(1)
                if b:
                    try:
                        return b.decode("ascii", "ignore")
                    except Exception:
                        return None
        except Exception:
            return None
        return None

    def flush(self) -> None:
        if self._ser:
            try:
                self._ser.flush()
            except Exception:
                pass

    # --- timing ---
    def sleep_ms(self, ms: int) -> None:
        import time
        time.sleep(ms / 1000.0)

    def now_ms(self) -> int:
        import time
        return int(time.monotonic() * 1000)
