# -*- coding: utf-8 -*-
"""
MMP.py
- 上位アプリのエントリポイント。
- 1つの接続指定子（conn）で、各プラットフォームのアダプタを自動選択します。
- 既存の設計（mmp_core.MmpClient + 各アダプタ）には変更を加えません。

使い方（例）:
    from mmp_core import MmpClient
    from MMP      import MmpAdapterFactory

    # TCP（ser2net）:
    client = MmpClient(MmpAdapterFactory(conn="tcp://192.168.2.113:3331?timeout=0.2"))

    # 直結（自動）: 実行系に応じて CPython/Micro/Circuit のシリアルアダプタを選択
    client = MmpClient(MmpAdapterFactory(conn="auto"))
"""

import sys

# mmp_core は既存のまま利用
from mmp_core import MmpClient  # re-export したい場合は __all__ に含める

__all__ = [
    "MmpClient",
    "MmpAdapterFactory",
    "new_client",
]

def _parse_tcp_conn(conn):
    """
    'tcp://host:port?timeout=0.2' を解析して (host, port, timeout_s) を返す。
    Micro/Circuit では urllib.parse が無い可能性があるため、ここで遅延 import する。
    """
    # Lazy import（CPythonでTCPを使う時だけ必要）
    from urllib.parse import urlparse, parse_qs

    u  = urlparse(conn)
    if u.scheme.lower() != "tcp":
        raise ValueError("conn は 'tcp://host:port[?timeout=...]' を指定してください")
    host = u.hostname
    port = u.port
    if not host or not port:
        raise ValueError("tcp 接続には host と port が必須です（例: tcp://192.168.2.113:3331）")
    qs   = parse_qs(u.query or "")
    timeout_s = float(qs.get("timeout", [0.2])[0])
    return host, port, timeout_s


def MmpAdapterFactory(conn="auto"):
    """
    接続指定子（ConnSpec）から適切なアダプタを生成して返すファクトリ。
    - conn="tcp://host:port[?timeout=0.2]" : TCPブリッジ（ser2net, socket://）
    - conn="auto"                         : 実行系（CPython/MicroPython/CircuitPython）に応じて
                                             シリアル前提の従来アダプタを自動選択
    戻り値: アダプタのインスタンス（MmpAdapterBaseの実装）
    """

    # 1) TCP（ser2net, socket://）— 決め打ちの内部処理
    if isinstance(conn, str) and conn.lower().startswith("tcp://"):
        host, port, timeout_s = _parse_tcp_conn(conn)
        # 固有名のTCPアダプタを遅延 import（他環境への影響を避ける）
        from mmp_adapter_Tcp import MmpAdapter
        return MmpAdapter(host=host, port=port, timeout_s=timeout_s)

    # 2) “auto” の場合：実行系名で分岐（Micro/Circuit/CPython）
    impl = getattr(sys, "implementation", None)
    name = (getattr(impl, "name", "") or "").lower()

    if name == "micropython":
        # MicroPython 用アダプタ
        from mmp_adapter_Micro import MmpAdapter
        return MmpAdapter()

    if name == "circuitpython":
        # CircuitPython 用アダプタ
        from mmp_adapter_Circuit import MmpAdapter
        return MmpAdapter()

    # 既定: CPython（従来の COM 総当たりロジックのアダプタ）
    from mmp_adapter_C import MmpAdapter
    return MmpAdapter()


def new_client(conn="auto"):
    """
    便宜メソッド：上位からはこれ一発で MmpClient を用意可能。
    例:
        from MMP import new_client
        client = new_client("tcp://192.168.2.113:3331")
    """
    return MmpClient(MmpAdapterFactory(conn))
