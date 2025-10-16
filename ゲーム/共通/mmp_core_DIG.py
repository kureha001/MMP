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
from mmp_com import _getValue

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
        cmd = f"DIGITAL/INPUT:{gpio}!"
        res = self._p._send_command(cmd, self.TimeOut)
        ok, v = _getValue(res)
        return v if ok else -1


    #─────────────
    # 信号出力
    #─────────────
    def OUTPUT(self, gpio:int, val:int) -> bool:
        cmd = f"DIGITAL/OUTPUT:{gpio}:{'1' if (val & 1) else '0'}!"
        res = self._p._send_command(cmd, self.TimeOut)
        return res == "!!!!!"