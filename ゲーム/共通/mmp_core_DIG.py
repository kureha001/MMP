# -*- coding: utf-8 -*-
# filename : mmp_core_DIG.py
#============================================================
# ＭＭＰコマンド：デジタル入出力
# バージョン：0.5
#------------------------------------------------------------
# [インストール方法]
# ・ＰＣ：[PYTHONPASTH] ※環境変数をセットしておく
# ・マイコン：[LIB]
# ・プロジェクトと同一ディレクトリ
#============================================================
from mmp_com import _DECtoHEX2, _HEX4toDEC

class _Digital:
#━━━━━━━━━━━━━━━
# コンストラクタ
#━━━━━━━━━━━━━━━
    def __init__(self, p:'MmpClient', argTimeOut):
        self._p         = p
        self.TimeOut    = argTimeOut

#━━━━━━━━━━━━━━━
# コマンド
#━━━━━━━━━━━━━━━
    #─────────────
    # 信号入力
    #─────────────
    def INPUT(self, gpio:int) -> int:
        cmd = "DIGITAL/INPUT"
        res = self._p._send_command(f"{cmd}:{_DECtoHEX2(gpio)}!", self.TimeOut)
        ok, v = _HEX4toDEC(res); 
        return v if ok else 0

    #─────────────
    # 信号出力
    #─────────────
    def OUTPUT(self, gpio:int, val:int) -> bool:
        cmd = "DIGITAL/OUTPUT"
        bit = '1' if (val & 1) else '0'
        res = self._p._send_command(f"{cmd}:{_DECtoHEX2(gpio)}:{bit}!", self.TimeOut)
        return res == "!!!!!"