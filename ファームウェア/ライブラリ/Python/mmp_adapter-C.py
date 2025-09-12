#============================================================
# CircuitPython用：ＵＡＲＴ接続アダプタ
#------------------------------------------------------------
# ＭＭＰシリアルコマンドを直接扱うコア処理
#------------------------------------------------------------
# [インストール方法]
# １．PYTHONPASTH(推奨)か、プロジェクトと動ディレクトリに格納
# ２．動作環境には「mmp_adapter.py」にリネーム
#============================================================
import time
from mmp_adapter_base import MmpAdapterBase


#=================
# アダプタ クラス
#=================
class MmpAdapter(MmpAdapterBase):

    #========================================================
    # コンストラクタ
    #========================================================
    def __init__(self,
        port: str       = None, # ① ポート名
        preferred_ports = None, # ② ポートリスト
    ) -> None:                  # 戻り値：なし

        super().__init__()

        # 通信条件を設定する。
        self._port_name = port
        self._preferred = preferred_ports or []

        # 接続情報を更新する。
        self.uart            = None
        self._connected_baud = None
        self._connected_port = None


    #========================================================
    # 内部ヘルパ
    #========================================================
    #------------------
    # ＣＯＭポート取得
    #------------------
    def _iter_ports(self
    ) -> tuple: # 戻り値：ポート名一覧

        # 遅延インポートでインストール時間を短縮
        try             : from serial.tools import list_ports
        except Exception: return []     # 戻り値 → [失敗]
        ports = [p.device for p in list_ports.comports()]

        # 優先ポートを昇格する（それ以外の場合は相対順序を維持）
        if self._preferred:
            pref   = [p for p in self._preferred if p in     ports]
            others = [p for p in ports           if p not in pref ]
            return pref + others        # 戻り値 → [成功]
        return ports                    # 戻り値 → [成功]


    #========================================================
    # アダプタ共通コマンド：通信関連
    #========================================================
    #------------------------------
    # UART を指定COMポート名で開く
    #------------------------------
    def _open_named(self,
        name,   # ① COMポート名
        baud,   # ② ボーレート
    ) -> bool:  # 戻り値：True=成功／False=失敗

        import serial

        try:
            ser = serial.Serial(
                name                    ,
                baudrate        = baud  ,
                timeout         = 0     ,
                write_timeout   = 0     ,
            )

            # DTR/RTS設定
            try:
                ser.dtr = True
                ser.rts = True
            except Exception: pass

            # 接続情報を更新する。
            self._ser           = ser
            self.connected_baud = baud
            self.connected_port = name

            # 受信バッファをクリア
            self.clear_input()

            return True                 # 戻り値 → [成功]

        except Exception: return False  # 戻り値 → [失敗]

    #------------------------------
    # UART を指定ボーレートで開く
    #------------------------------
    def open_baud(self,
        baud: int   # ① ボーレート
    ) -> bool:      # 戻り値：True=成功／False=失敗

        # ボーレートの指定がある場合：
        if self._port_name:
            # 接続されたCOMポートで接続する。
                                # 戻り値 → [成功]｜失敗
            return self._open_named(self._port_name, baud)

        # ボーレートの指定がない場合：
        # COMポートのリストを用いて総当たりで接続する。
        for p in self._iter_ports():
            if self._open_named(p, baud): return True # 戻り値 → [成功]
        return False                                  # 戻り値 → [失敗]

    #------------------------------
    # 受信バッファを消去する
    #------------------------------
    def clear_input(self
    ) -> None:  # 戻り値：なし

        # シリアルが無効な場合は処理を中断
        if not self._ser: return

        try             : n = self._ser.in_waiting
        except Exception: n = 0

        if n:
            try             : self._ser.read(n)
            except Exception: pass

    #------------------------------
    # ASCII文字を送信
    #------------------------------
    def write_ascii(self,
        s: str  # ① 送信する文字列
    ) -> None:  # 戻り値：なし

        # シリアルが無効な場合は処理を中断
        if not self._ser: return

        # 引数の文字列をASCII変換して送信
        try             : self._ser.write(s.encode("ascii", "ignore"))
        except Exception: pass

    #------------------------------
    # 受信バッファから１文字取得
    #------------------------------
    def read_one_char(self
    ) -> str:   # 戻り値：１文字=取得文字／None=バッファが空

        # シリアルが無効な場合は処理を中断
        if not self._ser: return None

        # 受信バッファから１文字取得する
        try:
            # 受信バッファから空以外の場合：
            if self._ser.in_waiting > 0:
                # 受信バッファの先頭１文字を取得する
                b = self._ser.read(1)
                if b:
                    # 取得文字をASCIIから復元して返す
                                                    # 戻り値 → [成功]
                    try             : return b.decode("ascii", "ignore")
                    except Exception: return None   # 戻り値 → [失敗]
        except Exception            : return None   # 戻り値 → [失敗]

        return None                                 # 戻り値 → [失敗]

    #------------------------------
    # 送信フラッシュ
    # (未対応実装もあるため未使用)
    #------------------------------
    def flush(self
    ) -> None:  # 戻り値：なし

        # シリアルが無効な場合は処理を中断
        if not self._ser: return None

        try             : self._ser.flush()
        except Exception: pass

    #------------------------------
    # 通信を切断
    #------------------------------
    def close(self
    ) -> None:  # 戻り値：なし

        if self._ser:
            try             : self._ser.close()
            except Exception: pass

        # 接続情報を更新する。
        self._ser           = None
        self.connected_baud = None
        self._port_name     = None


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
    # 現在時刻
    #------------------------------
    def now_ms(self
    ) -> int:   # 戻り値：現在時刻(単位：ミリ秒)
        import time
        return int(time.monotonic() * 1000)
