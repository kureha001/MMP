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
import time
import board
import busio
from mmp_adapter_base import MmpAdapterBase


#=================
# アダプタ クラス
#=================
class MmpAdapter(MmpAdapterBase):

    # 接続情報を初期化する。
    _uart           = None
    _is_open        = False
    _connected_port = None
    _connected_baud = None
    _lastError      = None

    #========================================================
    # コンストラクタ
    #========================================================
    def __init__(self,
        tx_pin      = None  ,   # ① UARTのTxに用いるGPIO番号
        rx_pin      = None  ,   # ② UARTのRxに用いるGPIO番号
        timeout_s   = 0.05  ,   # ③ タイムアウト(単位：ミリ秒)
        buffer_size = 128   ,   # ④ バッファサイズ(単位：バイト)
    ) -> None:                  # 戻り値：なし

        # UARTピンを設定する。
        self.tx_pin = (tx_pin) if not tx_pin else (getattr(board, "GP0"))
        self.rx_pin = (rx_pin) if not rx_pin else (getattr(board, "GP1"))

        # 通信条件を設定する。
        self.timeout_s   = float(timeout_s)
        self.buffer_size = int(buffer_size)


    #========================================================
    # アダプタ共通コマンド：通信関連
    #========================================================
    #------------------------------
    # UART を指定ボーレートで開く
    #------------------------------
    def open_baud(self,
        baud: int   # ① ボーレート
    ) -> bool:      # 戻り値：True=成功／False=失敗

        # 接続情報を更新する。
        self._is_open           = False
        self._connected_port    = None
        self._connected_baud    = None
        self._lastError         = None

        try:
            # 既に接続がある場合は、切断する。
            # ※deinitを実装しない場合もある
            if self._uart:
                try             : self._uart.deinit()
                except Exception: pass
                self._uart           = None
                self._connected_baud = None
                self.sleep_ms(2)

            # 指定の通信速度で接続する。
            self._uart = busio.UART(
                self.tx_pin                             ,
                self.rx_pin                             ,
                baudrate             = int(baud)        ,
                timeout              = self.timeout_s   ,
                receiver_buffer_size = self.buffer_size ,
            )
            self.sleep_ms(2)

            # 入力バッファを消去する。
            self.clear_input()

            # 接続情報を更新する。
            self._is_open           = True
            self._connected_port    = f"UART(Tx={self.tx_pin}/Rx={self.rx_pin})"
            self._connected_baud    = int(baud)
            self._lastError         = None

            return True         # 戻り値 → [成功]

        except Exception:            
            # 既に接続がある場合は、切断する。
            # ※deinitを実装しない場合もある
            if self._uart:
                try             : self._uart.deinit()
                except Exception: pass

            # 接続情報を更新する。
            self._uart           = None
            self._is_open        = False
            self._connected_port = None
            self._connected_baud = None

            return False        # 戻り値 → [失敗]

    #------------------------------
    # 受信バッファを消去する
    #------------------------------
    def clear_input(self
    ) -> None:  # 戻り値：なし

        # シリアルが無効な場合は処理を中断
        if not self._uart: return

        while True:
            data = self._uart.read(64)
            if not data: break

    #------------------------------
    # ASCII文字を送信
    #------------------------------
    def write_ascii(self,
        s: str  # ① 送信する文字列
    ) -> None:  # 戻り値：なし

        # シリアルが無効 または 空文字指定 の場合は処理を中断
        if not self._uart or not s: return

        # 引数の文字列をASCII変換して送信
        try             : self._uart.write(s.encode("ascii", "ignore"))
        except Exception: pass

    #------------------------------
    # 受信バッファから１文字取得
    #------------------------------
    def read_one_char(self
    ) -> str:   # 戻り値：１文字=取得文字／None=バッファが空

        # シリアルが無効な場合は処理を中断
        if not self._uart: return

        # 受信バッファから１文字取得する
            # 受信バッファから空以外の場合：
        data = self._uart.read(1)
        if data and len(data) > 0:
            try: return chr(data[0])                # 戻り値 → [成功]
            except Exception:
                    # 取得文字をASCIIから復元して返す
                                                    # 戻り値 → [成功]
                try             : return data.decode("ascii", "ignore")[:1] or None
                except Exception: return None       # 戻り値 → [失敗]

        return None                                 # 戻り値 → [失敗]

    #------------------------------
    # 送信バッファを消去する
    #------------------------------
    def flush(self
    ) -> None:  # 戻り値：なし
        return

    #------------------------------
    # 通信を切断
    #------------------------------
    def close(self
    ) -> None:  # 戻り値：なし

        try:
            if not self._uart:
                try             : self._uart.deinit()
                except Exception: pass
        finally:
            # 接続情報を更新する。
            self._uart           = None
            self._is_open        = False
            self._connected_port = None
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
        time.sleep(ms / 1000.0)

    #------------------------------
    # 現在時刻ミリ秒で返す
    #------------------------------
    def now_ms(self
    ) -> int:   # 戻り値：現在時刻(単位：ミリ秒)
        return int(time.monotonic() * 1000)

    #------------------------------
    # 経過時間をミリ秒で返す
    #------------------------------
    def ticks_diff(self,
        a: int, # ① 終了時刻
        b: int, # ② 開始時刻
    ) -> int:   # 戻り値：経過時間(単位：ミリ秒)
        return a - b