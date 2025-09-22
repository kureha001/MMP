# -*- coding: utf-8 -*-
# mmp_adapter_Tcp_Circuit.py
#============================================================
# CircuitPython用：TCPブリッジアダプタ
#------------------------------------------------------------
# [設置方法]
#  ser2netの"socket://"に接続するため、同一セグメントの
#  ネットワークに接続する
#------------------------------------------------------------
# [インストール方法]
# ・ＰＣ：[PYTHONPASTH] ※環境変数をセットしておく
# ・マイコン：[LIB]
# ・プロジェクトと同一ディレクトリ
#============================================================
from mmp_adapter_base import MmpAdapterBase

import time
import wifi
import socketpool


class MmpAdapter(MmpAdapterBase):
    def __init__(self, host: str, port: int, timeout_s: float = 0.6, **kwargs) -> None:
        """host/port/timeout_s は MMP.py の Factory から渡される想定"""
        super().__init__()
        self._host        = host
        self._port        = int(port)
        self._timeout_s   = float(timeout_s)

        # Transport（プールは GC されないように保持）
        self._pool        = socketpool.SocketPool(wifi.radio)
        self._sock        = None
        self._rx1         = bytearray(1)   # 1文字読み用
        self._rxbuf       = bytearray(64)  # クリア用

        # Base 想定のプロパティ
        self._uart           = None        # Transport Handle として _sock を参照
        self._is_open        = False
        self._connected_port = None        # "tcp://host:port"
        self._connected_baud = None        # 記録のみ（TCPなので実通信に影響なし）
        self._lastError      = None

    # ------------------------------------------------------------
    # 接続・切断
    # ------------------------------------------------------------
    def open_baud(self, baud: int) -> bool:
        """指定ボーレートで開く（TCP なので記録のみ）。成功/失敗を返す。"""
        self._lastError = None
        self.close()  # 念のため前回の残りを掃除

        try:
            s = self._pool.socket(self._pool.AF_INET, self._pool.SOCK_STREAM)
            s.settimeout(self._timeout_s)
            s.connect((self._host, self._port))

            self._sock = s
            self._uart = s           # Base 互換
            self._is_open = True
            self._connected_port = f"tcp://{self._host}:{self._port}"
            self._connected_baud = int(baud)

            # 接続直後の受信残渣を掃除
            self.clear_input()
            return True

        except Exception as e:
            self._lastError = f"TCP接続エラー: {e}"
            self._sock = None
            self._uart = None
            self._is_open = False
            return False

    def close(self) -> None:
        if self._sock:
            try:
                self._sock.close()
            except Exception:
                pass
        self._sock = None
        self._uart = None
        self._is_open = False

    # ------------------------------------------------------------
    # I/O
    # ------------------------------------------------------------
    def clear_input(self) -> None:
        """受信バッファを読み捨ててクリア（非ブロック）"""
        s = self._sock
        if not s:
            return

        try:
            cur_to = self._timeout_s
            s.settimeout(0)  # 非ブロッキングで吸い出す
            while True:
                try:
                    n = s.recv_into(self._rxbuf)
                    if not n:
                        break
                except Exception:
                    break
        finally:
            try:
                s.settimeout(cur_to)
            except Exception:
                pass

    def write_ascii(self, s: str) -> None:
        """ASCII を送信（例外は上位に影響しないよう握りつぶし）"""
        sck = self._sock
        if not sck:
            return
        data = s.encode("ascii", "ignore")
        total = 0
        try:
            while total < len(data):
                sent = sck.send(data[total:])
                if not sent:
                    # 相手が閉じた等
                    self._is_open = False
                    break
                total += sent
        except Exception:
            self._is_open = False

    def read_one_char(self):
        """
        1文字だけ取り出す（無ければ None）。
        - recv_into(_rx1) が 0 を返したら EOF/未到達として扱う
        - タイムアウト時は例外が出ることがあるので None を返す
        """
        s = self._sock
        if not s:
            return None

        try:
            n = s.recv_into(self._rx1)
            if n is None:
                # 実装によっては未到達で None のことがある
                return None
            if n == 0:
                # EOF（相手切断 or まだ来ていない）
                # 明確な切断は send/次回の I/O で検知されることもある
                return None
            ch = self._rx1[:n].decode("ascii", "ignore")
            return ch if ch else None
        except Exception:
            # タイムアウト or 一時的な受信失敗
            return None

    def flush(self) -> None:
        """TCP には明示 flush 相当がないので no-op"""
        return

    # ------------------------------------------------------------
    # ヘルパ
    # ------------------------------------------------------------
    def sleep_ms(self, ms: int) -> None:
        time.sleep(max(0.0, ms) / 1000.0)

    def now_ms(self) -> int:
        return int(time.monotonic() * 1000)
