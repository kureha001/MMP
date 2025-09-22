# -*- coding: utf-8 -*-
# mmp_adapter_Tcp_Micro.py
#============================================================
# MicroPython用：TCPブリッジアダプタ
#------------------------------------------------------------
# [設置方法]
#  ser2netの"socket://"に接続するため、同一セグメントの
#  ネットワークに接続する
#------------------------------------------------------------
# [インストール方法]
# 同じフォルダ（または lib）に以下の実装ファイルを配置
# [インストール方法]
# ・ＰＣ：[PYTHONPASTH] ※環境変数をセットしておく
# ・マイコン：[LIB]
# ・プロジェクトと同一ディレクトリ
#============================================================
from mmp_adapter_base import MmpAdapterBase

import time
try:
    import usocket as socket
except ImportError:  # 念のため
    import socket


class MmpAdapter(MmpAdapterBase):
    def __init__(self, host: str, port: int, timeout_s: float = 0.3, **kwargs) -> None:
        super().__init__()
        self._host        = host
        self._port        = int(port)
        self._timeout_s   = float(timeout_s)

        # Base で定義される想定のプロパティ
        self._uart           = None   # ← Transport Handle（今回は TCP ソケット）
        self._is_open        = False
        self._connected_port = None   # "tcp://host:port"
        self._connected_baud = None   # 記録のみ（TCPなので実通信に影響しない）
        self._lastError      = None

    # ------------------------------------------------------------
    # 接続・切断
    # ------------------------------------------------------------
    def open_baud(self, baud: int) -> bool:
        """指定ボーレートで開く（TCP なので記録のみ）。成功/失敗を返す。"""
        self._lastError = None
        self.close()  # 念のため前回の残りを掃除

        try:
            s = socket.socket()
            s.settimeout(self._timeout_s)
            s.connect((self._host, self._port))

            self._uart = s
            self._is_open = True
            self._connected_port = f"tcp://{self._host}:{self._port}"
            self._connected_baud = int(baud)

            # 接続直後の受信残渣を掃除
            self.clear_input()
            return True

        except Exception as e:
            self._lastError = f"TCP接続エラー: {e}"
            self._uart = None
            self._is_open = False
            return False

    def close(self) -> None:
        if self._uart:
            try:
                # MicroPython では shutdown 無しで close だけで十分
                self._uart.close()
            except Exception:
                pass
        self._uart = None
        self._is_open = False

    # ------------------------------------------------------------
    # I/O
    # ------------------------------------------------------------
    def clear_input(self) -> None:
        """受信バッファを読み捨ててクリア"""
        s = self._uart
        if not s:
            return

        # 現在のタイムアウトを保持
        try:
            cur_to = self._timeout_s
            s.settimeout(0)  # 非ブロックでできるだけ吸い出す
            while True:
                try:
                    chunk = s.recv(64)
                    if not chunk:
                        break
                except Exception:
                    break
        finally:
            try:
                s.settimeout(cur_to)
            except Exception:
                pass

    def write_ascii(self, s: str) -> None:
        sck = self._uart
        if not sck: return
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
            # 送信失敗時は接続断として扱う
            self._is_open = False

    def read_one_char(self):
        """
        1文字だけ取り出す（無ければ None）。
        - recv(1) が b"" を返したら EOF（切断）と見なして _is_open=False
        - タイムアウト時は None
        """
        s = self._uart
        if not s:
            return None

        try:
            b = s.recv(1)  # settimeout に依存（open 時に設定済み）
            if b is None:
                # 実装によっては未到達時に None を返すことがある
                return None
            if b == b"":
                # EOF（相手切断）
                self._is_open = False
                return None
            try:
                ch = b.decode("ascii", "ignore")
            except Exception:
                ch = ""
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
        # MicroPython なら sleep_ms があるが、互換的に sleep を使ってもよい
        try:
            time.sleep_ms(int(ms))
        except AttributeError:
            time.sleep(max(0.0, ms) / 1000.0)

    def now_ms(self) -> int:
        try:
            return time.ticks_ms()
        except AttributeError:
            return int(time.time() * 1000)
