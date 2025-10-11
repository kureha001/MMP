# -*- coding: utf-8 -*-
# filename : mmp_core_I2C.py
#============================================================
# ＭＭＰコマンド：Ｉ２Ｃ通信
#------------------------------------------------------------
# [インストール方法]
# ・ＰＣ：[PYTHONPASTH] ※環境変数をセットしておく
# ・マイコン：[LIB]
# ・プロジェクトと同一ディレクトリ
#============================================================
from mmp_core_COM import _resolve, _hex2, _try_parse_hex4_bang

class _I2cModule:
    #----------------------
    # コンストラクタ
    #----------------------
    def __init__(self, p:'MmpClient'): self._p = p

    #----------------------
    # １．書き込み
    #----------------------
    def Write(self, addr:int, reg:int, val:int, timeoutMs:int=0) -> bool:
        t = _resolve(timeoutMs, self._p.Settings.TimeoutI2c)
        resp = self._p._send_command(f"I2W:{_hex2(addr)}:{_hex2(reg)}:{_hex2(val)}!", t)
        return resp == "!!!!!"

    #----------------------
    # ２．読み出し
    #----------------------
    def Read(self, addr:int, reg:int, timeoutMs:int=0) -> int:
        t = _resolve(timeoutMs, self._p.Settings.TimeoutI2c)
        resp = self._p._send_command(f"I2R:{_hex2(addr)}:{_hex2(reg)}!", t)
        ok, v = _try_parse_hex4_bang(resp); 
        return v if ok else 0

