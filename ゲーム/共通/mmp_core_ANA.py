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
        self._p      = p
        self.TimeOut = argTimeOut

#━━━━━━━━━━━━━━━
# コマンド
#━━━━━━━━━━━━━━━
    #─────────────
    # 使用範囲設定
    #─────────────
    def SETUP(self,
        chs   :int,   # チャンネル数
        devs  :int,   # デバイス数
    ) -> bool:
        cmd = "ANALOG/SETUP"
        res = self._p._send_command(
            f"{cmd}:{_DECtoHEX2(chs)}:{_DECtoHEX2(devs)}!",
            self.TimeOut
            )
        return res == "!!!!!"

    #─────────────
    # 信号入力(バッファ格納)
    #─────────────
    def INPUT(self) -> bool:
        cmd = "ANALOG/INPUT"
        res = self._p._send_command(f"{cmd}!", self.TimeOut)
        return res == "!!!!!"

    #─────────────
    # バッファ読取：丸めなし
    #─────────────
    def READ(self, 
        ch  :int,   # ① チャンネルID
        dev :int,   # ② デバイスID
    ) -> int:       # 戻値：アナログ値
        cmd = "ANALOG/READ"
        res = self._p._send_command(
            f"{cmd}:{_DECtoHEX2(ch)}:{_DECtoHEX2(dev)}!",
            self.TimeOut
            )
        ok, v = _HEX4toDEC(res); 
        return v if ok else -1

    #─────────────
    # バッファ読取：四捨五入
    #─────────────
    def ROUND(self,
        ch  :int,   # ① チャンネルID
        dev :int,   # ② デバイスID
        step:int,   # ③ 丸め単位
        bits:int,   # ④ ビット数
    ) -> int:       # 戻値：算出値
        値 = self.READ(ch, dev)
        if 値 < 0: return 値
        ub = self._ビット補正(bits)
        return self._四捨五入(値, step, ub)

    #─────────────
    # バッファ読取：切り上げ
    #─────────────
    def ROUNDU(self,
        ch  :int,   # ① チャンネルID
        dev :int,   # ② デバイスID
        step:int,   # ③ 丸め単位
        bits:int,   # ④ ビット数
    ) -> int:       # 戻値：算出値
        値 = self.READ(ch, dev)
        if 値 < 0: return 値
        ub = self._ビット補正(bits)
        return self._切り上げ(値, step, ub)

    #─────────────
    # バッファ読取：切り下げ
    #─────────────
    def ROUNDD(self,
        ch  :int,   # ① チャンネルID
        dev :int,   # ② デバイスID
        step:int,   # ③ 丸め単位
        bits:int,   # ④ ビット数
    ) -> int:       # 戻値：算出値
        値 = self.READ(ch, dev)
        if 値 < 0: return 値
        ub = self._ビット補正(bits)
        return self._切り下げ(値, step, ub)

#━━━━━━━━━━━━━━━
# 内部ヘルパ
#━━━━━━━━━━━━━━━
    #─────────────
    def _ビット補正(self,
        bits:int    ,   # ビット数
    ) -> int:
        判定 = isinstance(bits, int) and (1 <= bits <= 16)
        ub = bits if 判定 else self._p.Settings.AnalogBits
        if not (1 <= ub <= 16): ub = 10
        return ub
    #─────────────
    # ・偶数step： r>=中間 で切り上げ
    # ・奇数step： r<=中間 で切り捨て（= r>=mid+1 で切り上げ）
    #─────────────
    def _四捨五入(self,
        raw :int,   # 元の値
        step:int,   # 丸め単位
        bits:int,   # ビット数
    ) -> int:
        if raw < 0 or step <= 0: return raw
        最大   = (1 << bits) - 1
        値     = raw if raw <= 最大 else 最大
        商, 余 = divmod(値, step)
        中間   = step // 2
        if 余 > 中間 or (step%2 == 0 and 余 == 中間): 商 += 1
        値     = 商 * step
        if 値 > 最大: 値 = 最大
        return 値
    #─────────────
    def _切り上げ(self,
        raw :int    ,   # 元の値
        step:int    ,   # 丸め単位
        bits:int = 0,   # ビット数
    ) -> int:
        if raw < 0 or step <= 0: return raw
        最大   = (1 << bits) - 1
        値     = raw if raw <= 最大 else 最大
        商, 余 = divmod(値, step)
        if 余 > 0   : 値 = (商 + 1) * step
        if 値 > 最大: 値 = 最大
        return 値
    #─────────────
    def _切り下げ(self,
        raw :int    ,   # 元の値
        step:int    ,   # 丸め単位
        bits:int = 0,   # ビット数
    ) -> int:
        if raw < 0 or step <= 0: return raw
        最大 = (1 << bits) - 1
        値   = raw if raw <= 最大 else 最大
        値   = (値 // step) * step
        if 値 < 0: 値 = 0
        return 値
