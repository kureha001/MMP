# -*- coding: utf-8 -*-
#============================================================
# TCPブリッジ用アダプタ（ser2net の socket:// に接続）
# I/F契約は既存どおり  :contentReference[oaicite:2]{index=2}
# 依存: 標準ライブラリのみ（socket, select, time）
# 使い方:
#   このファイルを "mmp_adapter.py" にリネームして:
#     from mmp_adapter import MmpAdapter
#     cli = MmpAdapter(host="192.168.2.113", port=3331, timeout_s=0.2)
#     # -> mmp_core.MmpClient から利用（上位は無変更）
#============================================================
import socket
import select
import time
from typing import Optional
from mmp_adapter_base import MmpAdapterBase

#========================================================
# アダプタ クラス
#========================================================
#  ser2net の raw TCP (socket://) に接続するトランスポート。
#  - APIは既存アダプタと互換（上位はmmp_coreのまま動く）
# 　  :contentReference[oaicite:3]{index=3}
#  - 低遅延/安定性: TCP_NODELAY, KEEPALIVE（任意）に対応
#  - 例外は握り潰さずFalse/Noneに落とす
# 　（上位の再試行ロジックに委ねる）
#========================================================
class MmpAdapter(MmpAdapterBase):

    #========================================================
    # コンストラクタ
    #  host/port : ser2netサーバ
    #  timeout_s : 受信待ちの最大待ち時間（秒）; read_one_char で使用
    #  keepalive : OSのTCP KeepAliveを有効化するか（既定: 有効）
    #  ka_*      : 利用OSが対応していれば KeepAlive パラメータを適用
    #              (Linux: TCP_KEEPIDLE/TCP_KEEPINTVL/TCP_KEEPCNT)
    #========================================================
    def __init__(self,
        host        : str                 ,
        port        : int                 ,
        timeout_s   : float         = 0.2 ,
        keepalive   : bool          = True,
        ka_idle     : Optional[int] = None,
        ka_intvl    : Optional[int] = None,
        ka_cnt      : Optional[int] = None
    ):
        if not host or not isinstance(port, int):
            raise ValueError("host/port は必須です")

        self._host      = host
        self._port      = int(port)
        self._timeout_s = float(timeout_s)

        self._keepalive = bool(keepalive)
        self._ka_idle   = ka_idle
        self._ka_intvl  = ka_intvl
        self._ka_cnt    = ka_cnt

        self._sock: Optional[socket.socket] = None
        self._is_open                       = False
        self._connected_port: Optional[str] = None
        self._connected_baud: Optional[int] = None
        self._lastError: Optional[str]      = None


    #========================================================
    # アダプタ共通コマンド（既存I/F互換）
    #========================================================
    #  contentReference[oaicite:4]{index=4}
    #  TCPなので実ボーレートは無関係だが、上位互換のため記録のみ行う。
    #  接続確立: 成功→True / 失敗→False（lastErrorに理由）
    #========================================================
    def open_baud(self, baud: int) -> bool:

        # 既存と同じ契約: 先に既存接続を閉じる
        try: self.close()
        except Exception: pass

        try:
            s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

            # 低遅延（小パケット即送信）
            try: s.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
            except Exception: pass

            # KeepAlive（任意）
            if self._keepalive:
                try:
                    s.setsockopt(socket.SOL_SOCKET, socket.SO_KEEPALIVE, 1)
                    # Linux固有の詳細設定（存在する場合のみ）
                    if self._ka_idle is not None and hasattr(socket, "TCP_KEEPIDLE"):
                        s.setsockopt(socket.IPPROTO_TCP, socket.TCP_KEEPIDLE, int(self._ka_idle))
                    if self._ka_intvl is not None and hasattr(socket, "TCP_KEEPINTVL"):
                        s.setsockopt(socket.IPPROTO_TCP, socket.TCP_KEEPINTVL, int(self._ka_intvl))
                    if self._ka_cnt is not None and hasattr(socket, "TCP_KEEPCNT"):
                        s.setsockopt(socket.IPPROTO_TCP, socket.TCP_KEEPCNT, int(self._ka_cnt))

                # KeepAlive設定はベストエフォート
                except Exception: pass

            # 接続時タイムアウト（以降はselectで制御）
            s.settimeout(max(self._timeout_s, 0.01))
            s.connect((self._host, self._port))
            s.settimeout(None)  # 接続後はブロッキング、I/Oはselectで待つ

            self._sock = s
            self._is_open = True
            self._connected_port = f"tcp://{self._host}:{self._port}"
            self._connected_baud = int(baud) if isinstance(baud, int) else None
            self._lastError = None
            return True

        except Exception as e:
            self._sock = None
            self._is_open = False
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
                try: s.shutdown(socket.SHUT_RDWR)
                except Exception: pass
                s.close()
            except Exception: pass

    #------------------------------
    # 受信バッファを消去する
    #------------------------------
    def clear_input(self) -> None:
        """
        受信側の溜まりを可能な範囲で捨てる。
        上位はコマンド直前に呼び出す前提（mmp_core側） :contentReference[oaicite:5]{index=5}
        """
        s = self._sock
        if not s: return

        try:
            s.setblocking(False)

            for _ in range(64):  # 上限を設けて無限ループを回避

                r, _, _ = select.select([s], [], [], 0.0)
                if not r: break
                try: data = s.recv(1024)
                except (BlockingIOError, InterruptedError): break

                if not data:
                    # peer closed
                    self._is_open = False
                    break

        except Exception: pass
        finally:
            try: s.setblocking(True)
            except Exception: pass

    #------------------------------
    # ASCII文字を送信
    #------------------------------
    def write_ascii(self, s: str) -> None:
        if not self._sock: raise RuntimeError("ソケット未接続です")
        if not s: return
        b = s.encode("ascii", "ignore")
        # sendall は内部で繰り返し送信してくれる
        self._sock.sendall(b)

    #------------------------------
    # 受信バッファから１文字取得
    #------------------------------
    def read_one_char(self) -> Optional[str]:
        """
        1バイトだけ読み取る。無ければ None。
        切断(EOF)時は _is_open=False に落とす。
        """
        s = self._sock
        if not s: return None
        try:
            r, _, _ = select.select([s], [], [], self._timeout_s)
            if not r: return None
            data = s.recv(1)
            if not data:
                # peer closed
                self._is_open = False
                return None
            try: ch = data.decode("ascii", "ignore")
            except Exception: return None
            return ch if ch else None
        except Exception: return None

    #------------------------------
    # 送信フラッシュ
    # (未対応実装もあるため未使用)
    #------------------------------
    def flush(self) -> None:
        return

    #========================================================
    # 時間ヘルパ（既存契約に合わせる） :contentReference[oaicite:6]{index=6}
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

    #========================================================
    # プロパティ
    #========================================================
    # （任意拡張）内部状態参照
    @property
    def last_error(self) -> Optional[str]:
        return self._lastError
