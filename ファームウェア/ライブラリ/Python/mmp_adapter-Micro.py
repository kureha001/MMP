#============================================================
# MicroPython用：ＵＡＲＴ接続アダプタ
#------------------------------------------------------------
# ＭＭＰシリアルコマンドを直接扱うコア処理
#------------------------------------------------------------
# [インストール方法]
# １．PYTHONPASTH(推奨)か、プロジェクトと動ディレクトリに格納
# ２．動作環境には「mmp_adapter.py」にリネーム
#------------------------------------------------------------
# 既定は UART(0)/uart_id=0/tx_pin=1
# uart_id=None/tx_pin=None で、ボードのデフォルトになる
# 必要に応じて tx_pin / rx_pin を指定のこと。
#============================================================
try:
    import machine
    import utime as _time
except ImportError:
    # MicroPython 以外ではインポートできない想定
    machine = None
    _time   = None


#=================
# アダプタ クラス
#=================
class MmpAdapter:

    #========================================================
    # コンストラクタ
    #========================================================
    def __init__(self, 
        uart_id      = 0     ,  # ① UART番号
        tx_pin      = None  ,   # ② UARTのTxに用いるGPIO番号
        rx_pin      = None  ,   # ③ UARTのRxに用いるGPIO番号
    ) -> None:                  # 戻り値：なし

        # UARTピンを設定する。
        self._uart_id   = uart_id
        self._tx_pin    = tx_pin
        self._rx_pin    = rx_pin

        # 通信条件を設定する。

        # 接続情報を更新する。
        self.uart           = None
        self.connected_baud = None


    #========================================================
    # アダプタ共通コマンド：通信関連
    #========================================================
    #------------------------------
    # UART を指定ボーレートで開く
    #------------------------------
    def open_baud(self,
        baud: int   # ① ボーレート
    ) -> bool:      # 戻り値：True=成功／False=失敗

        try:
            # 既に接続がある場合は、切断する。
            # ※deinitを実装しない場合もある
            if self.uart and hasattr(self.uart, "deinit"):
                try             : self.uart.deinit()
                except Exception: pass

            # ピン指定の有無に応じて接続する。
            if not self._tx_pin and not self._rx_pin:
                tx = machine.Pin(self._tx_pin)
                rx = machine.Pin(self._rx_pin)
                self.uart = machine.UART(self._uart_id, baudrate=int(baud), tx=tx, rx=rx)
            else:
                self.uart = machine.UART(self._uart_id, baudrate=int(baud))

            # 入力バッファを消去する。
            self.clear_input()

            # 接続情報を更新する。
            self.connected_baud = int(baud)

            return True         # 戻り値 → [成功]

        except Exception:
            # 既に接続がある場合は、切断する。
            # ※deinitを実装しない場合もある
            if self.uart and hasattr(self.uart, "deinit"):
                try             : self.uart.deinit()
                except Exception: pass

            # 接続情報を更新する。
            self.uart           = None
            self.connected_baud = None

            return False        # 戻り値 → [失敗]

    #------------------------------
    # 受信バッファを消去する
    #------------------------------
    def clear_input(self
    ) -> None:  # 戻り値：なし

        # シリアルが無効な場合は処理を中断
        if not self.uart: return

        try:
            n = self.uart.any()
            if n and n > 0:
                try             : self.uart.read(n)
                except Exception: pass
        except Exception        : pass

    #------------------------------
    # ASCII文字を送信
    #------------------------------
    def write_ascii(self,
        s: str  # ① 送信する文字列
    ) -> None:  # 戻り値：なし

        # シリアルが無効 または 空文字指定 の場合は処理を中断
        if not self.uart  or not s: return

        # 引数の文字列をASCII変換して送信
        #※MicroPython の UART.write は bytes/str を受け付ける実装が多い
        try                 : self.uart.write(s)
        except Exception:
            try             : self.uart.write(s.encode("ascii", "ignore"))
            except Exception: pass

    #------------------------------
    # 受信バッファから１文字取得
    #------------------------------
    def read_one_char(self
    ) -> str:   # 戻り値：１文字=取得文字／None=バッファが空

        # シリアルが無効な場合は処理を中断
        if not self.uart: return

        try:
            if self.uart.any() > 0:
                b = self.uart.read(1)
                if b:
                    # ASCII 1文字へ（不正バイトは無視）
                    try: return b.decode("ascii", "ignore")
                    except Exception:
                        # b が bytes 長さ1なら chr へフォールバック
                        try             : return chr(b[0])
                        except Exception: return None
            return None
        except Exception: return None

    #------------------------------
    # 送信バッファを消去する
    #------------------------------
    def flush(self
    ) -> None:  # 戻り値：なし
        pass

    #------------------------------
    # 通信を切断
    #------------------------------
    def close(self
    ) -> None:  # 戻り値：なし

        try:
            if self.uart:
                if hasattr(self.uart, "deinit"):
                    self.uart.deinit()
        except Exception: pass
        finally:
            # 接続情報を更新する。
            self.uart            = None
            self._connected_baud = None


    #========================================================
    # アダプタ共通コマンド：ヘルパ
    #========================================================
    #------------------------------
    # 一時停止
    #------------------------------
    def sleep_ms(self,
        ms: int # ① 停止する時間(単位：ミリ秒)
    ) -> None:  # 戻り値：なし
        _time.sleep_ms(int(ms))

    #------------------------------
    # 現在時刻ミリ秒で返す
    #------------------------------
    def now_ms(self
    ) -> int:   # 戻り値：現在時刻(単位：ミリ秒)
        return _time.ticks_ms()

    #------------------------------
    # 経過時間をミリ秒で返す
    #------------------------------
    def ticks_diff(self,
        a: int, # ① 終了時刻
        b: int, # ② 開始時刻
    ) -> int:   # 戻り値：経過時間(単位：ミリ秒)
        return _time.ticks_diff(a, b)

