# -*- coding: utf-8 -*-
#============================================================
# CPython用：TCPブリッジアダプタ(ser2netの"socket://"に接続)
#------------------------------------------------------------
# [インストール方法]
# 同じフォルダ（または PYTHONPATH）に以下の実装ファイルを配置
#   mmp_adapter_base.py
# 　mmp_core.py
# 　MMP.py
#============================================================
import socket
import select
import time
from typing           import Optional
from mmp_adapter_base import MmpAdapterBase

#=================
# アダプタ クラス
#=================
class MmpAdapter(MmpAdapterBase):

    _sock           = None
    _is_open        = False
    _connected_port = None
    _connected_baud = None
    _lastError      = None

    #========================================================
    # コンストラクタ
    #--------------------------------------------------------
    #  host      : ser2netの IPv4/IPv6 アドレス or ホスト名
    #  port      : ser2netの待受ポート
    #  timeout_s : 読取時の最長待ち時間（秒）
    #========================================================
    def __init__(self,
            host: str,
            port: int,
            timeout_s: float = 0.2
    ):

        if not host or not isinstance(port, int):
            raise ValueError("host/port は必須です")

        self._host       = host
        self._port       = int(port)
        self._timeout_s  = float(timeout_s)

        self._sock           = None
        self._is_open        = False
        self._connected_port = None
        self._connected_baud = None
        self._lastError      = None

    #==============================================
    # アダプタ共通コマンド：通信関連
    #----------------------------------------------
    #  TCP なのでボーレートの概念はないが、
    #  上位互換のために受け取り、
    #  接続確立後に ConnectedBaud として保持する。
    #==============================================
    def open_baud(self, baud: int) -> bool:
        try             : self.close()
        except Exception: pass

        try:
            s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            # 低レイテンシ優先
            try: s.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
            except Exception: pass

            # 接続時のみタイムアウト（以降はselectで扱う）
            s.settimeout(max(self._timeout_s, 0.01))
            s.connect((self._host, self._port))

            # 接続後はブロッキングに戻す（selectで制御）
            s.settimeout(None)
            self._sock = s

            self._is_open        = True
            self._connected_port = f"tcp://{self._host}:{self._port}"
            self._connected_baud = int(baud) if isinstance(baud, int) else None
            self._lastError      = None
            return True

        except Exception as e:
            self._sock      = None
            self._is_open   = False
            self._lastError = f"TCP接続エラー: {e}"
            return False

    #------------------------------
    # 通信を切断
    #------------------------------
    def close(self) -> None:
        s = self._sock
        self._sock = None
        self._is_open = False
        if s:
            try:
                try : s.shutdown(socket.SHUT_RDWR)
                except Exception: pass
                s.close()
            except Exception: pass

    #------------------------------
    # 受信バッファを消去する
    #------------------------------
    def clear_input(self) -> None:
        s = self._sock
        if not s: return
        try:
            s.setblocking(False)
            for _ in range(64):

                r, _, _ = select.select([s], [], [], 0.0)
                if not r: break

                try: data = s.recv(1024)
                except BlockingIOError: break
                if not data: break

        except Exception: pass

        finally:
            try: s.setblocking(True)
            except Exception: pass

    #------------------------------
    # ASCII文字を送信
    #------------------------------
    def write_ascii(self, argCommand: str) -> None:
        if not self._sock: raise RuntimeError("ソケット未接続です")
        data = argCommand.encode("ascii", "ignore")
        total = 0
        try:
            while total < len(data):
                print(f"　　・{total}：{data[total:]}")
                sent = self._sock.send(data[total:])
                if not sent:
                    self._is_open = False
                    break
                total += sent
        except Exception: self._is_open = False

    #-----------------------------------------
    # 受信バッファから１文字取得
    #-----------------------------------------
    #  1バイトだけ読む。データがなければ None
    #  上位はポーリングで繰り返し呼ぶ前提
    # （mmp_core._send_command）
    #-----------------------------------------
    def read_one_char(self) -> Optional[str]:
        s = self._sock
        if not s: return None
        try:
            r, _, _ = select.select([s], [], [], self._timeout_s)
            if not r: return None

            data = s.recv(1)
            if not data:
                self._is_open = False
                return None
            try: ch = data.decode("ascii", "ignore")
            except Exception: return None

            return ch if ch else None

        except Exception: return None

    #------------------------------
    # 送信フラッシュ
    #------------------------------
    #  未対応実装もあるため未使用
    #------------------------------
    def flush(self) -> None:
        return

    #========================================================
    # ヘルパー
    #========================================================
    #------------------------------
    # 一時停止
    #------------------------------
    def sleep_ms(self, ms: int) -> None:
        time.sleep(max(ms, 0) / 1000.0)

    #------------------------------
    # 現在時刻
    #------------------------------
    def now_ms(self) -> int:
        return int(time.monotonic() * 1000)
