# -*- coding: utf-8 -*-
# filename : mmp_core_I2C.py
#============================================================
# ＭＭＰコマンド：Ｉ２Ｃ通信
# バージョン：0.5
#------------------------------------------------------------
# [インストール方法]
# ・ＰＣ：[PYTHONPASTH] ※環境変数をセットしておく
# ・マイコン：[LIB]
# ・プロジェクトと同一ディレクトリ
#============================================================
from mmp_com import _DECtoHEX2, _HEX4toDEC

class _I2C:
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
    # 書き込み
    #─────────────
    def WRITE(self,
        addr:int,   # アドレス
        reg :int,   # レジスタ
        val :int,   # 値
    ) -> bool:
        cmd = "I2C/WRITE"
        res = self._p._send_command(
            f"{cmd}:{_DECtoHEX2(addr)}:{_DECtoHEX2(reg)}:{_DECtoHEX2(val)}!",
            self.TimeOut
            )
        return res == "!!!!!"

    #─────────────
    # 読み出し
    #─────────────
    def READ(self,
        addr:int,   # アドレス
        reg :int,   # レジスタ
    ) -> int:
        cmd = "I2C/READ"
        res = self._p._send_command(
            f"{cmd}:{_DECtoHEX2(addr)}:{_DECtoHEX2(reg)}!",
            self.TimeOut
            )
        ok, v = _HEX4toDEC(res); 
        return v if ok else 0

