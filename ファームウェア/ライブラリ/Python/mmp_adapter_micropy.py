#============================================================
# MicroPython 用 UART アダプタ
#  - MmpClient（mmp_core.py）が呼ぶ最小APIを実装
#    * open_baud(baud) -> bool
#    * connected_baud  : 実際に開いたボーレート
#    * clear_input()
#    * write_ascii(s)
#    * flush()
#    * read_one_char() -> 1文字 or None（非ブロッキング）
#    * now_ms() / ticks_diff(a,b)
#    * sleep_ms(ms)
#    * close()
#------------------------------------------------------------
# 既定は UART(0) / ピン未指定（ボードのデフォルト割当を使用）
# 必要に応じて tx_pin / rx_pin を指定してください。
#============================================================

try:
    import machine
    import utime as _time
except ImportError:
    # MicroPython 以外ではインポートできない想定
    machine = None
    _time = None


class MicroPyAdapter:
    def __init__(self, uart_id=0, tx_pin=None, rx_pin=None):
        self._uart_id = uart_id
        self._tx_pin = tx_pin
        self._rx_pin = rx_pin
        self.uart = None
        self.connected_baud = None

    #--------------------------------------------------------
    # UART を指定ボーレートで開く
    #--------------------------------------------------------
    def open_baud(self, baud: int) -> bool:
        try:
            # 既存 UART を解放
            try:
                if self.uart:
                    # MicroPython 実装によっては deinit がある
                    if hasattr(self.uart, "deinit"):
                        self.uart.deinit()
            except Exception:
                pass

            # ピン指定の有無で分岐（未指定ならボード既定）
            if self._tx_pin is not None and self._rx_pin is not None:
                tx = machine.Pin(self._tx_pin)
                rx = machine.Pin(self._rx_pin)
                self.uart = machine.UART(self._uart_id, baudrate=int(baud), tx=tx, rx=rx)
            else:
                self.uart = machine.UART(self._uart_id, baudrate=int(baud))

            # RXバッファを空に
            self.clear_input()

            # 実際に開いたボーレートを保存
            self.connected_baud = int(baud)
            return True

        except Exception:
            self.uart = None
            self.connected_baud = None
            return False

    #--------------------------------------------------------
    # 受信バッファを捨てる
    #--------------------------------------------------------
    def clear_input(self):
        if not self.uart:
            return
        try:
            # .any() で受信済みバイト数を取得し、一括読み捨て
            n = self.uart.any()
            if n and n > 0:
                try:
                    self.uart.read(n)
                except Exception:
                    # 一部実装で read(n) に失敗しても作業継続
                    pass
        except Exception:
            pass

    #--------------------------------------------------------
    # ASCII 送信
    #--------------------------------------------------------
    def write_ascii(self, s: str):
        if not self.uart or s is None:
            return 0
        try:
            # MicroPython の UART.write は bytes/str を受け付ける実装が多い
            return self.uart.write(s)
        except Exception:
            try:
                return self.uart.write(s.encode("ascii", "ignore"))
            except Exception:
                return 0

    #--------------------------------------------------------
    # 送信フラッシュ（未対応実装もあるため no-op）
    #--------------------------------------------------------
    def flush(self):
        # 一部ポートで .flush() がないため、軽い待ちを入れて擬似フラッシュ
        try:
            self.sleep_ms(1)
        except Exception:
            pass

    #--------------------------------------------------------
    # 非ブロッキングで 1 文字読む（なければ None）
    #--------------------------------------------------------
    def read_one_char(self):
        if not self.uart:
            return None
        try:
            if self.uart.any() > 0:
                b = self.uart.read(1)
                if b:
                    # ASCII 1文字へ（不正バイトは無視）
                    try:
                        return b.decode("ascii", "ignore")
                    except Exception:
                        # b が bytes 長さ1なら chr へフォールバック
                        try:
                            return chr(b[0])
                        except Exception:
                            return None
            return None
        except Exception:
            return None

    #--------------------------------------------------------
    # クロック/タイマ
    #--------------------------------------------------------
    def now_ms(self) -> int:
        # MicroPython 標準
        return _time.ticks_ms()

    def ticks_diff(self, a: int, b: int) -> int:
        # a - b（wrap 安全）
        return _time.ticks_diff(a, b)

    def sleep_ms(self, ms: int):
        _time.sleep_ms(int(ms))

    #--------------------------------------------------------
    # クローズ
    #--------------------------------------------------------
    def close(self):
        try:
            if self.uart:
                if hasattr(self.uart, "deinit"):
                    self.uart.deinit()
        except Exception:
            pass
        finally:
            self.uart = None
            self.connected_baud = None
