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
    def In(self, gpioId:int) -> int:
        cmd     = "DIGITAL/INPUT"
        resp    = self._p._send_command(f"{cmd}:{_DECtoHEX2(gpioId)}!", self.TimeOut)
        ok, v   = _HEX4toDEC(resp); 
        return v if ok else 0

    #─────────────
    # 信号出力
    #─────────────
    def Out(self, gpioId:int, val0or1:int) -> bool:
        cmd     = "DIGITAL/OUTPUT"
        bit     = '1' if (val0or1 & 1) else '0'
        resp    = self._p._send_command(f"{cmd}:{_DECtoHEX2(gpioId)}:{bit}!", self.TimeOut)
        return resp == "!!!!!"