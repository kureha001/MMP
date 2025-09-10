#============================================================
# CircuitPython用：ＵＡＲＴ接続アダプタ
#------------------------------------------------------------
# ＭＭＰシリアルコマンドを直接扱うコア処理
#------------------------------------------------------------
# [インストール方法]
# １．PYTHONPASTH(推奨)か、プロジェクトと動ディレクトリに格納
# ２．動作環境には「mmp_adapter.py」にリネーム
#------------------------------------------------------------
# 'uart_id' パラメータがありません。
# ※CircuitPython busio.UART はこれをサポートしていません。
#============================================================
from mmp_adapter_base import MmpAdapterBase

import time
import board
import busio


class MmpAdapter(MmpAdapterBase):
    def __init__(self, tx_pin=None, rx_pin=None, timeout_s=0.05, buffer_size=128):
        # Default pins (RP2040/2350 typical wiring: TX=GP0, RX=GP1)
        self.tx_pin = tx_pin if tx_pin is not None else getattr(board, "GP0")
        self.rx_pin = rx_pin if rx_pin is not None else getattr(board, "GP1")

        self.timeout_s = float(timeout_s)
        self.buffer_size = int(buffer_size)

        self.uart = None
        self._connected_baud = None

    # ---------- time helpers ----------
    def now_ms(self) -> int:
        return int(time.monotonic() * 1000)

    def ticks_diff(self, a: int, b: int) -> int:
        # monotonic-based difference (ms)
        return a - b

    def sleep_ms(self, ms: int) -> None:
        time.sleep(ms / 1000.0)

    # ---------- transport ----------
    @property
    def connected_baud(self):
        return self._connected_baud

    def open_baud(self, baud: int) -> bool:
        # Re-open UART cleanly for each baud attempt
        try:
            if self.uart is not None:
                try:
                    self.uart.deinit()
                except Exception:
                    pass
                self.uart = None
                self._connected_baud = None
                self.sleep_ms(2)  # small settle

            # CircuitPython UART: busio.UART(tx, rx, baudrate=..., timeout=..., receiver_buffer_size=...)
            self.uart = busio.UART(
                self.tx_pin,
                self.rx_pin,
                baudrate=int(baud),
                timeout=self.timeout_s,
                receiver_buffer_size=self.buffer_size,
            )
            # small settle + drain any line noise
            self.sleep_ms(2)
            self.clear_input()

            self._connected_baud = int(baud)
            return True
        except Exception:
            # Ensure closed state on failure
            try:
                if self.uart is not None:
                    self.uart.deinit()
            except Exception:
                pass
            self.uart = None
            self._connected_baud = None
            return False

    def clear_input(self) -> None:
        if self.uart is None:
            return
        # Drain until empty (non-blocking reads)
        while True:
            data = self.uart.read(64)  # type: ignore[arg-type]
            if not data:
                break

    def write_ascii(self, s: str) -> None:
        if self.uart is None:
            return
        data = s.encode("ascii", "ignore")
        self.uart.write(data)

    def read_one_char(self):
        if self.uart is None:
            return None
        data = self.uart.read(1)
        if data and len(data) > 0:
            # Return single-character string (core expects str, not bytes)
            try:
                return chr(data[0])
            except Exception:
                # Fallback safe path
                try:
                    return data.decode("ascii", "ignore")[:1] or None
                except Exception:
                    return None
        return None

    def flush(self) -> None:
        # busio.UART has no flush; writes are immediate. Keep as no-op.
        return

    def close(self) -> None:
        try:
            if self.uart is not None:
                try:
                    self.uart.deinit()
                except Exception:
                    pass
        finally:
            self.uart = None
            self._connected_baud = None
            self.sleep_ms(2)
