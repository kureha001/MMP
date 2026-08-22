# -*- coding: utf-8 -*-
# filename : mmp_core_SYS.py
#============================================================
# ＭＭＰコマンド：システム情報
# バージョン：0.5
#------------------------------------------------------------
# [インストール方法]
# ・ＰＣ：[PYTHONPASTH] ※環境変数をセットしておく
# ・マイコン：[LIB]
# ・プロジェクトと同一ディレクトリ
#============================================================
class _System:
#━━━━━━━━━━━━━━━
# コンストラクタ
#━━━━━━━━━━━━━━━
    def __init__(self, p:'MmpClient', argTimeOut):
        self._p      = p
        self.TimeOut = argTimeOut

#━━━━━━━━━━━━━━━
# コマンド
#━━━━━━━━━━━━━━━
    #─────────────
    # バージョン取得
    #─────────────
    def VERSION(self) -> str:
        return self._p._send_command("SYS/VERSION!", self.TimeOut)

    #─────────────
    # ＭＭＰ再起動
    #─────────────
    def BOOT(self) -> bool:
        res = self._p._send_command("SYS/BOOT!", self.TimeOut)
        return res == "!!!!!"

    #─────────────
    # ログレベル設定
    #─────────────
    def LOG(self, val:int) -> bool:
        cmd = f"SYS/SET_LOG:{val}!"
        res = self._p._send_command(cmd, self.TimeOut)
        return res == "!!!!!"
