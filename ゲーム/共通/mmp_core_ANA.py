# -*- coding: utf-8 -*-
# filename : mmp_core_ANA.py
#============================================================
# ＭＭＰコマンド：アナログ入力
# バージョン：0.5
#------------------------------------------------------------
# [インストール方法]
# ・ＰＣ：[PYTHONPASTH] ※環境変数をセットしておく
# ・マイコン：[LIB]
# ・プロジェクトと同一ディレクトリ
#============================================================
from mmp_com import  _DECtoHEX2, _HEX4toDEC

class _Analog:
#━━━━━━━━━━━━━━━
# コンストラクタ
#━━━━━━━━━━━━━━━
    def __init__(self, p:'MmpClient', argTimeOut):
        self._p         = p
        self.TimeOut    = argTimeOut

#━━━━━━━━━━━━━━━
# 内部ヘルパ
#━━━━━━━━━━━━━━━
    #─────────────
    def _ビット補正(self, bits:int) -> int:
        ub = bits if (isinstance(bits, int) and 1 <= bits <= 16) else self._p.Settings.AnalogBits
        if not (1 <= ub <= 16): ub = 10
        return ub
    #─────────────
    def _四捨五入(self, raw:int, step:int, bits:int) -> int:
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
    #─────────────
    def _切り上げ(self, raw:int, step:int, bits:int) -> int:
        if raw < 0 or step <= 0: return raw
        maxv = (1 << bits) - 1
        v = raw if raw <= maxv else maxv
        q, r = divmod(v, step)
        v2 = v if r == 0 else (q + 1) * step
        if v2 > maxv: v2 = maxv
        return v2
    #─────────────
    def _切り下げ(self, raw:int, step:int, bits:int) -> int:
        if raw < 0 or step <= 0: return raw
        maxv = (1 << bits) - 1
        v = raw if raw <= maxv else maxv
        q = v // step
        v2 = q * step
        if v2 < 0: v2 = 0
        return v2

#━━━━━━━━━━━━━━━
# コマンド
#━━━━━━━━━━━━━━━
    #─────────────
    # 使用範囲設定
    #─────────────
    def Configure(self,
        chs   :int,   # チャンネル数
        devs  :int,   # デバイス数
    ) -> bool:
        cmd     = "ANALOG/SETUP"
        resp    = self._p._send_command(f"{cmd}:{_DECtoHEX2(chs)}:{_DECtoHEX2(devs)}!", self.TimeOut)
        return resp == "!!!!!"

    #─────────────
    # 信号入力(バッファ格納)
    #─────────────
    def Update(self) -> bool:
        cmd     = "ANALOG/INPUT"
        resp    = self._p._send_command(f"{cmd}!", self.TimeOut)
        return resp == "!!!!!"

    #─────────────
    # バッファ読取：丸めなし
    #─────────────
    def Read(self, 
        ch  :int,   # チャンネルID
        dev :int    # デバイスID
    ) -> int:
        cmd     = "ANALOG/READ"
        resp    = self._p._send_command(f"{cmd}:{_DECtoHEX2(ch)}:{_DECtoHEX2(dev)}!", self.TimeOut)
        ok, v   = _HEX4toDEC(resp); 
        return v if ok else -1

    #─────────────
    # バッファ読取：四捨五入
    #─────────────
    def ReadRound(self,
        ch  :int    ,   # チャンネルID
        dev :int    ,   # デバイスID
        step:int    ,   #
        bits:int = 0    #
    ) -> int:
        raw = self.Read(ch, dev)
        if raw < 0: return raw
        ub = self._ビット補正(bits)
        return self._四捨五入(raw, step, ub)

    #─────────────
    # バッファ読取：切り上げ
    #─────────────
    def ReadRoundUp(self,
        ch  :int    ,   # チャンネルID
        dev :int    ,   # デバイスID
        step:int    ,   #
        bits:int = 0    #
    ) -> int:
        raw = self.Read(ch, dev)
        if raw < 0: return raw
        ub = self._ビット補正(bits)
        return self._切り上げ(raw, step, ub)

    #─────────────
    # バッファ読取：切り下げ
    #─────────────
    def ReadRoundDown(self,
        ch  :int    ,   # チャンネルID
        dev :int    ,   # デバイスID
        step:int    ,   #
        bits:int = 0    #
    ) -> int:
        raw = self.Read(ch, dev)
        if raw < 0: return raw
        ub = self._ビット補正(bits)
        return self._切り下げ(raw, step, ub)