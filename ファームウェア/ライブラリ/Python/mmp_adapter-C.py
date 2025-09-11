#============================================================
# CircuitPython用：ＵＡＲＴ接続アダプタ
#------------------------------------------------------------
# ＭＭＰシリアルコマンドを直接扱うコア処理
#------------------------------------------------------------
# [インストール方法]
# １．PYTHONPASTH(推奨)か、プロジェクトと動ディレクトリに格納
# ２．動作環境には「mmp_adapter.py」にリネーム
#============================================================
from mmp_adapter_base import MmpAdapterBase


class MmpAdapter(MmpAdapterBase):

    # 接続したCOMポート
    PortName = None

    #--------------------------------------------------------
    # コンストラクタ
    #--------------------------------------------------------
    def __init__(self, port: str = None, preferred_ports=None):
        super().__init__()
        self._port_name = port
        self._preferred = preferred_ports or []
        self._ser = None

    #--------------------------------------------------------
    # ＣＯＭポート取得ヘルパ
    #--------------------------------------------------------
    def _iter_ports(self):
        # Lazily import to keep install-time light
        try             : from serial.tools import list_ports
        except Exception: return []
        ports = [p.device for p in list_ports.comports()]
        # Promote preferred ports (keep relative order otherwise)
        if self._preferred:
            pref = [p for p in self._preferred if p in ports]
            others = [p for p in ports if p not in pref]
            return pref + others
        return ports

    #--------------------------------------------------------
    # UART を指定COMポートで開く
    #--------------------------------------------------------
    def _open_named(self, name, baud):
        import serial
        try:
            ser = serial.Serial(name, baudrate=baud, timeout=0, write_timeout=0)

            # DTR/RTS設定
            try:
                ser.dtr = True
                ser.rts = True
            except Exception: pass

            # 通信確立情報を反映
            self._ser           = ser
            self.connected_baud = baud
            self.PortName       = name

            # 受信バッファをクリア
            self.clear_input()

            return True

        except Exception: return False

    #--------------------------------------------------------
    # 通信確立したCOMポート名を返す
    #--------------------------------------------------------
    def get_port_name(self): return self.PortName

    #--------------------------------------------------------
    # UART を指定ボーレートで開く
    #--------------------------------------------------------
    def open_baud(self, baud: int) -> bool:

        # 接続されたCOMポートのリストを取得する。
        if self._port_name:
            return self._open_named(self._port_name, baud)

        # COMポートのリストを用いて総当たりで接続する。
        for p in self._iter_ports():
            if self._open_named(p, baud):
                # Remember the port actually used
                self._port_name = p
                return True

        return False

    #--------------------------------------------------------
    # クローズ
    #--------------------------------------------------------
    def close(self) -> None:
        if self._ser:
            try             : self._ser.close()
            except Exception: pass
        self._ser = None

    #--------------------------------------------------------
    # 受信バッファを捨てる
    #--------------------------------------------------------
    def clear_input(self) -> None:
        if not self._ser: return
        try             : n = self._ser.in_waiting
        except Exception: n = 0

        if n:
            try             : self._ser.read(n)
            except Exception: pass

    #--------------------------------------------------------
    # ASCII 送信
    #--------------------------------------------------------
    def write_ascii(self, s: str) -> None:
        if not self._ser: return
        try             : self._ser.write(s.encode("ascii", "ignore"))
        except Exception: pass

    #--------------------------------------------------------
    # 非ブロッキングで 1 文字読む（なければ None）
    #--------------------------------------------------------
    def read_one_char(self):

        if not self._ser: return None

        try:
            if self._ser.in_waiting > 0:
                b = self._ser.read(1)
                if b:
                    try             : return b.decode("ascii", "ignore")
                    except Exception: return None
        except Exception: return None

        return None

    #--------------------------------------------------------
    # 送信フラッシュ（未対応実装もあるため no-op）
    #--------------------------------------------------------
    def flush(self) -> None:
        if self._ser:
            try             : self._ser.flush()
            except Exception: pass

    #--------------------------------------------------------
    # クロック/タイマ
    #--------------------------------------------------------
    def sleep_ms(self, ms: int) -> None:
        import time
        time.sleep(ms / 1000.0)
    def now_ms(self) -> int:
        import time
        return int(time.monotonic() * 1000)
