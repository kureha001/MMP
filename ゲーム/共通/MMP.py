# -*- coding: utf-8 -*-
# filename : MMP.py
#=================================================================
# Python共通：アプリ接続
# バージョン：0.6
# ・エラー出力を改行
#-----------------------------------------------------------------
# [インストール方法]
# ・ＰＣ：[PYTHONPASTH] ※環境変数をセットしておく
# ・マイコン：[LIB]
# ・プロジェクトと同一ディレクトリ
#=================================================================
import sys

# mmp_core は既存のまま利用
from mmp_core import MmpClient

接続 = None  # type: ignore[assignment]

__all__ = [
    "MmpClient"         ,
    "ファクトリ別接続"  ,
    "通信接続"          ,
    "接続"              ,
]

#=================================================================
# 'tcp://host:port?timeout=0.2'を解析し(host,port,timeout_s)を返す
# urllib.parseはMicro/Circuitでは無いことを考慮しここで遅延import
#=================================================================
def TCPブリッジ情報取得(conn: str):

    s = (conn or "").strip()
    if not s.lower().startswith("tcp://"):
        raise ValueError("conn は 'tcp://host:port[?timeout=...]' を指定してください")

    rest = s[6:]  # "tcp://"
    if "?" in rest  : hostport, query = rest.split("?", 1)
    else            : hostport, query = rest, ""

    if ":" not in hostport:
        raise ValueError("tcp 接続には host と port が必須です")

    host, port_str = hostport.rsplit(":", 1)
    try: port = int(port_str)
    except Exception: raise ValueError("port は整数で指定してください")

    timeout_s = 0.2
    if query:
        for kv in query.split("&"):
            if kv.startswith("timeout="):
                try: timeout_s = float(kv.split("=", 1)[1])
                except Exception: pass

    return host, port, timeout_s


#=================================================================
# 指定モジュールから MmpAdapter を動的 import して返す。
# 分かりやすいエラーにラップする。
#=================================================================
def アダプタ生成(module_name: str):
    try:
        mod = __import__(module_name)
        Adp = getattr(mod, "MmpAdapter")
        return Adp
    except Exception as e:
        raise ImportError(f"'{module_name}.py' の読込失敗: {e}") from e

#=================================================================
# 接続指定子（ConnSpec）から適切なアダプタを生成して返すファクトリ
#-----------------------------------------------------------------
# -  conn="tcp://host:port[?timeout=0.2]" :
#    TCPブリッジ（ser2net, socket://）
# - conn="auto" :
#    実行系（CPython/MicroPython/CircuitPython）に応じて
#    シリアル前提の従来アダプタを自動選択
#-----------------------------------------------------------------
# 戻り値: アダプタのインスタンス（mmp_adapter_base の実装）
#=================================================================
def ファクトリ別接続(conn="auto"):

    print(conn)

    # 0) usb4a
    if isinstance(conn, str) and conn.lower().startswith("usb4a://"):
        idx_str = conn.split("://", 1)[1]
        try: index = int(idx_str) if idx_str else 0
        except Exception: index = 0
        Adapter = アダプタ生成("mmp_adapter_Usb4a")
        return Adapter(device_index=index)

    # 1) TCP（ser2net）
    if isinstance(conn, str) and conn.lower().startswith("tcp://"):
        host, port, timeout_s = TCPブリッジ情報取得(conn)
        impl = getattr(sys, "implementation", None)
        name = (getattr(impl, "name", "") or "").lower()
        if   name == "micropython"  : Adapter = アダプタ生成("mmp_adapter_Tcp_Micro"    )
        elif name == "circuitpython": Adapter = アダプタ生成("mmp_adapter_Tcp_Circuit"  )
        else                        : Adapter = アダプタ生成("mmp_adapter_Tcp_C"        )
        return Adapter(host=host, port=port, timeout_s=timeout_s)

    # 2) auto の場合：実行系名で分岐（Micro/Circuit/CPython）
    impl = getattr(sys, "implementation", None)
    name = (getattr(impl, "name", "") or "").lower()

    # 2-1) MicroPythonのコンストラクタの引数：
    # ① uart_id = 0：UART番号
    # ② tx_pin  = 0：UARTのTxに用いるGPIO番号
    # ③ rx_pin  = 1：UARTのRxに用いるGPIO番号 
    if   name == "micropython":
        Adapter = アダプタ生成("mmp_adapter_Ser_Micro")
        return Adapter()

    # 2-2) CircuitPythonのコンストラクタの引数：
    # ① tx_pin      = None：UARTのTxに用いるGPIO番号
    # ② rx_pin      = None：UARTのRxに用いるGPIO番号
    # ③ timeout_s   = 0.05：タイムアウト(単位：ミリ秒)
    # ④ buffer_size = 128 ：バッファサイズ(単位：バイト)
    elif name == "circuitpython":
        Adapter = アダプタ生成("mmp_adapter_Ser_Circuit")
        return Adapter()

    # 2-3) CPythonのコンストラクタの引数：
    # ① port            = None：ポート名
    # ② preferred_ports = None：ポートリスト
    else:
        # 既定: CPython
        Adapter = アダプタ生成("mmp_adapter_Ser_C")
        return Adapter()


#=================================================================
def 通信接続(conn="auto"):

    print("<< ＭＭＰライブラリ for Python>>")
    print("")
    print("接続中...")

    global 接続
    try:
        接続 = MmpClient(ファクトリ別接続(conn))
        if 接続.ConnectAutoBaud():
            print(f"　・通信ポート　: {接続.ConnectedPort}")
            print(f"　・通信速度　　: {接続.ConnectedBaud}bps")
            print( "　・バージョン  : {}".format(接続.INFO.VERSION()))
            print( "　・PCA9685 [0] : 0x{:04X}".format(接続.PWM.INFO.CONNECT()))
            print( "　・DFPlayer[1] : 0x{:04X}".format(接続.MP3.INFO.CONNECT(1)))
            return True

        else:
            print("ＭＭＰとの接続に失敗しました...")
            print(接続.LastError)
            接続 = None
            return False

    except Exception as e:
        接続 = None
        print("例外:", e)
        return False

