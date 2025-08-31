#============================================================
# CircuitPython用アダプタ（busio.UART）
#------------------------------------------------------------
#【ファイル配置方法】
# 同一 または libフォルダー(推奨)
#============================================================
from mmp_adapter_base import UartAdapterBase
import time
import board
import busio

class CircuitPyAdapter(UartAdapterBase):
    #------------------------------------------------------------
    # 初期化
    #------------------------------------------------------------
    def __init__(
        self                ,
        tx_pin      = None  ,
        rx_pin      = None  ,
        baud        = 115200,
        timeout_ms  = 100   ,
        rxbuf       = 64    ,
        ):

        # 
        self._tx_pin    = tx_pin
        self._rx_pin    = rx_pin
        self.baud       = int(baud)
        self.timeout_ms = int(timeout_ms)
        self.timeout_s  = float(timeout_ms / 1000.0)
        self.rxbuf      = int(rxbuf)
        self.uart       = None

    #------------------------------------------------------------
    # ピン解決
    #------------------------------------------------------------
    def _resolve_pin(self, pin, is_tx):

        # 指定のピンがボード情報に存在すれば確定する。
        try:
            if hasattr(pin, "name") or "board." in repr(pin): return pin
        except Exception: pass

        # ピン未指定であればボード情報の標準で確定する。
        if pin is None: return getattr(board, "TX" if is_tx else "RX")

        # 指定ピンを文字型で解決する
        if isinstance(pin, str):

            # 指定のピンがボード情報に存在すれば確定する。
            name = pin
            if hasattr(board, name)         : return getattr(board, name)
            if hasattr(board, name.upper()) : return getattr(board, name.upper())
            if name.isdigit():
                for prefix in ("GP", "D", "IO"):
                    cand = f"{prefix}{name}"
                    if hasattr(board, cand) : return getattr(board, cand)
            raise ValueError("指定ピン(文字型)の解決に失敗: '%s'" % pin)

        # 指定ピンを数値型で解決する
        if isinstance(pin, int):
            for prefix in ("GP", "D", "IO"):
                cand = f"{prefix}{pin}"
                if hasattr(board, cand)     : return getattr(board, cand)
            raise ValueError("指定ピン(数値型)の解決に失敗: %s" % pin)

        return pin
    #------------------------------------------------------------
    # UARTオープン
    #------------------------------------------------------------
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
    #------------------------------------------------------------
    # 受信データの有無確認
    #------------------------------------------------------------
    def rx_ready(self):
        try:
            return int(self.uart.in_waiting) or 0
        except Exception:
            return 0

    #------------------------------------------------------------
    # データ読み込み
    #------------------------------------------------------------
    def read(self, n):
        try                 : return self.uart.read(n)
        except Exception:
            try             : return self.uart.read(1)
            except Exception: return b""
    #------------------------------------------------------------
    # データ書き込み
    #------------------------------------------------------------
    def write(self, b):
        try             : return self.uart.write(b)
        except Exception: return 0
    #------------------------------------------------------------
    # 通信バッファを消去
    #------------------------------------------------------------
    def flush(self):
        try:
            if hasattr(self.uart, "reset_input_buffer"):
                try:
                    self.uart.reset_input_buffer()
                    return
                except Exception: pass

            deadline = time.monotonic() + (30 / 1000.0)

            while time.monotonic() < deadline:
                n = 0

                if hasattr(self.uart, "in_waiting"):
                    try             : n = int(self.uart.in_waiting) or 0
                    except Exception: n = 0

                if n > 0:
                    try: self.uart.read(n)
                    except Exception:
                        try             : self.uart.read()
                        except Exception: break
                else:

                    try             : b = self.uart.read(1)
                    except Exception: b = None
                    if not b: break

                time.sleep(0.001)

        except Exception: pass
    #------------------------------------------------------------
    # UARTクローズ
    #------------------------------------------------------------
    def close(self):
        try:
            if self.uart: self.uart.deinit()
        except Exception: pass
        self.uart = None
