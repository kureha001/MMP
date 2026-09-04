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

    #━━━━━━━━━━━━━━━
    # サブ：ブリッジ設定
    #━━━━━━━━━━━━━━━
    class _Bridge:
        #─────────────
        # コンストラクタ
        #─────────────
        def __init__(self, p, argTimeOut):
            self._p = p
            self.TimeOut = argTimeOut

        #─────────────
        # TCP
        #─────────────
        def TCP(self,
            ip      :str = "192.168.2.99",   # ① IPアドレス
            port    :int = 8081,             # ② ポート番号
        ) -> bool:
            cmd = f"@TCP:{ip}:{port}!"
            res = self._p._send_command(cmd, self.TimeOut)
            return res == "!!!!!"

        #─────────────
        # WebSocket
        #─────────────
        def WSOC(self,
            ip      :str = "192.168.2.99",   # ① IPアドレス
            port    :int = 8082,             # ② ポート番号
        ) -> bool:
            cmd = f"@WSOC:{ip}:{port}!"
            res = self._p._send_command(cmd, self.TimeOut)
            return res == "!!!!!"

        #─────────────
        # Web API
        #─────────────
        def WAPI(self,
            ip      :str = "192.168.2.99",   # ① IPアドレス
            port    :int = 8080,             # ② ポート番号
        ) -> bool:
            cmd = f"@WAPI:{ip}:{port}!"
            res = self._p._send_command(cmd, self.TimeOut)
            return res == "!!!!!"