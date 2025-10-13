# -*- coding: utf-8 -*-
# filename : mmp_core_INF.py
#============================================================
# ＭＭＰコマンド：システム情報
# バージョン：0.4
#------------------------------------------------------------
# [インストール方法]
# ・ＰＣ：[PYTHONPASTH] ※環境変数をセットしておく
# ・マイコン：[LIB]
# ・プロジェクトと同一ディレクトリ
#============================================================
from mmp_core_COM import _resolve, _hex2, _try_parse_hex4_bang

class _InfoModule:
    #----------------------
    # コンストラクタ
    #----------------------
    def __init__(self, p:'MmpClient'): self._p = p; self.Dev = self._DevModule(p)

    #----------------------
    # １．バージョン
    #----------------------
    def Version(self, timeoutMs: int = 0) -> str:
        t = _resolve(timeoutMs, self._p.Settings.TimeoutGeneral)
        return self._p._send_command("INF/VER!", t)

    #----------------------
    # ２．デバイス情報
    #----------------------
    class _DevModule:
        #---------------------------
        # 2-0.コンストラクタ
        #---------------------------
        def __init__(self, p:'MmpClient'): self._p = p

        #---------------------------
        # 2-1.ＰＷＭデバイス
        #---------------------------
        def Pwm(self, devId0to15:int, timeoutMs:int=0) -> int:
            t = _resolve(timeoutMs, self._p.Settings.TimeoutPwm)
            resp = self._p._send_command(f"PWX:{_hex2(devId0to15)}!", t)
            ok, v = _try_parse_hex4_bang(resp); 
            return v if ok else -1

        #---------------------------
        # 2-2.音声デバイス
        #---------------------------
        def Audio(self, devId1to4:int, timeoutMs:int=0) -> int:
            t = _resolve(timeoutMs, self._p.Settings.TimeoutAudio)
            resp = self._p._send_command(f"DPX:{_hex2(devId1to4)}!", t)
            ok, v = _try_parse_hex4_bang(resp); 
            return v if ok else -1