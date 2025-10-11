# -*- coding: utf-8 -*-
# filename : mmp_core_ANA.py
#============================================================
# ＭＭＰコマンド：アナログ入力
#------------------------------------------------------------
# [インストール方法]
# ・ＰＣ：[PYTHONPASTH] ※環境変数をセットしておく
# ・マイコン：[LIB]
# ・プロジェクトと同一ディレクトリ
#============================================================
from mmp_core_COM import _resolve, _hex2, _try_parse_hex4_bang

class _AnalogModule:
    #----------------------
    # コンストラクタ
    #----------------------
    def __init__(self, p:'MmpClient'): self._p = p

    #------------------
    # 内部ヘルパ
    #------------------
    def _clamp_bits(self, bits:int) -> int:
        ub = bits if (isinstance(bits, int) and 1 <= bits <= 16) else self._p.Settings.AnalogBits
        if not (1 <= ub <= 16): ub = 10
        return ub
    #----------------------
    def _clamp_raw(self, raw:int, bits:int) -> int:
        if raw < 0: return raw
        maxv = (1 << bits) - 1
        return 0 if raw < 0 else (maxv if raw > maxv else raw)
    #----------------------
    def _round_mid(self, raw:int, step:int, bits:int) -> int:
        if raw < 0 or step <= 0: return raw
        maxv = (1 << bits) - 1
        v = raw if raw <= maxv else maxv
        q, r = divmod(v, step)
        mid = step // 2
        # 中央値の扱い：偶数stepは r>=mid で切り上げ、奇数stepは r<=mid で切り捨て（= r>=mid+1 で切り上げ）
        if r < mid: v2 = q * step
        elif r > mid or (step % 2 == 0 and r == mid): v2 = (q + 1) * step
        else: v2 = q * step
        if v2 > maxv: v2 = maxv
        return v2
    #----------------------
    def _round_up(self, raw:int, step:int, bits:int) -> int:
        if raw < 0 or step <= 0: return raw
        maxv = (1 << bits) - 1
        v = raw if raw <= maxv else maxv
        q, r = divmod(v, step)
        v2 = v if r == 0 else (q + 1) * step
        if v2 > maxv: v2 = maxv
        return v2
    #----------------------
    def _round_down(self, raw:int, step:int, bits:int) -> int:
        if raw < 0 or step <= 0: return raw
        maxv = (1 << bits) - 1
        v = raw if raw <= maxv else maxv
        q = v // step
        v2 = q * step
        if v2 < 0: v2 = 0
        return v2
    #----------------------

    #-----------------------
    # １．使用範囲設定
    #-----------------------
    def Configure(self, hc4067chTtl:int, hc4067devTtl:int, timeoutMs:int=0) -> bool:
        t = _resolve(timeoutMs, self._p.Settings.TimeoutAnalog)
        if not (1 <= hc4067chTtl <= 16) or not (1 <= hc4067devTtl <= 4): return False
        resp = self._p._send_command(f"ANS:{_hex2(hc4067chTtl)}:{_hex2(hc4067devTtl)}!", t)
        return resp == "!!!!!"

    #-----------------------
    # ２．測定値バッファ更新
    #-----------------------
    def Update(self, timeoutMs:int=0) -> bool:
        t = _resolve(timeoutMs, self._p.Settings.TimeoutAnalog)
        resp = self._p._send_command("ANU!", t)
        return resp == "!!!!!"

    #---------------------------------
    # ３．測定値バッファ読取（丸めなし）
    #---------------------------------
    def Read(self, hc4067ch0to15:int, hc4067dev0to3:int, timeoutMs:int=0) -> int:
        t = _resolve(timeoutMs, self._p.Settings.TimeoutAnalog)
        if not (0 <= hc4067ch0to15 <= 15) or not (0 <= hc4067dev0to3 <= 3):
            return -1
        resp = self._p._send_command(f"ANR:{_hex2(hc4067ch0to15)}:{_hex2(hc4067dev0to3)}!", t)
        ok, v = _try_parse_hex4_bang(resp); 
        return v if ok else -1

    #-----------------------------------------
    # ４．測定値バッファ読取（中央値基準の丸め）
    #-----------------------------------------
    def ReadRound(self, hc4067ch0to15:int, hc4067dev0to3:int, step:int, bits:int=0, timeoutMs:int=0) -> int:
        raw = self.Read(hc4067ch0to15, hc4067dev0to3, timeoutMs)
        if raw < 0: return raw
        ub = self._clamp_bits(bits)
        return self._round_mid(raw, step, ub)

    #-----------------------------------------
    # ５．測定値バッファ読取（切り上げ丸め）
    #-----------------------------------------
    def ReadRoundUp(self, hc4067ch0to15:int, hc4067dev0to3:int, step:int, bits:int=0, timeoutMs:int=0) -> int:
        raw = self.Read(hc4067ch0to15, hc4067dev0to3, timeoutMs)
        if raw < 0: return raw
        ub = self._clamp_bits(bits)
        return self._round_up(raw, step, ub)

    #-----------------------------------------
    # ６．測定値バッファ読取（切り捨て丸め）
    #-----------------------------------------
    def ReadRoundDown(self, hc4067ch0to15:int, hc4067dev0to3:int, step:int, bits:int=0, timeoutMs:int=0) -> int:
        raw = self.Read(hc4067ch0to15, hc4067dev0to3, timeoutMs)
        if raw < 0: return raw
        ub = self._clamp_bits(bits)
        return self._round_down(raw, step, ub)