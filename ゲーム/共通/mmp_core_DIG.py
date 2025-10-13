# -*- coding: utf-8 -*-
# filename : mmp_core_DIG.py
#============================================================
# ＭＭＰコマンド：デジタル入出力
# バージョン：0.4
#------------------------------------------------------------
# [インストール方法]
# ・ＰＣ：[PYTHONPASTH] ※環境変数をセットしておく
# ・マイコン：[LIB]
# ・プロジェクトと同一ディレクトリ
#============================================================
from mmp_core_COM import _resolve, _hex2, _try_parse_hex4_bang

class _DigitalModule:
    #----------------------
    # コンストラクタ
    #----------------------
    def __init__(self, p:'MmpClient'): self._p = p

    #----------------------
    # １．入力
    #----------------------
    def In(self, gpioId:int, timeoutMs:int=0) -> int:
        t = _resolve(timeoutMs, self._p.Settings.TimeoutDigital)
        resp = self._p._send_command(f"POR:{_hex2(gpioId)}!", t)
        ok, v = _try_parse_hex4_bang(resp); 
        return v if ok else 0

    #----------------------
    # ２．出力
    #----------------------
    def Out(self, gpioId:int, val0or1:int, timeoutMs:int=0) -> bool:
        t = _resolve(timeoutMs, self._p.Settings.TimeoutDigital)
        bit = '1' if (val0or1 & 1) else '0'
        resp = self._p._send_command(f"POW:{_hex2(gpioId)}:{bit}!", t)
        return resp == "!!!!!"