# -*- coding: utf-8 -*-
# filename : mmp_core_ANA.py
#============================================================
# ＭＭＰコマンド：アナログ入力
# バージョン：0.6
#------------------------------------------------------------
# [インストール方法]
# ・ＰＣ：[PYTHONPASTH] ※環境変数をセットしておく
# ・マイコン：[LIB]
# ・プロジェクトと同一ディレクトリ
#============================================================
from mmp_com import _getValue

class _Analog:
#━━━━━━━━━━━━━━━
# クラス変数
#━━━━━━━━━━━━━━━
    分解能   = 12 # アナログ入力の分解能（ビット数）
    丸め単位 = 10 # 丸める単位

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
        cmd = f"ANALOG/SETUP:{chs}:{devs}!"
        res = self._p._send_command(cmd, self.TimeOut)
        return res == "!!!!!"

    #─────────────
    # 信号入力(バッファ格納)
    #─────────────
    def INPUT(self) -> bool:
        cmd = "ANALOG/INPUT!"
        res = self._p._send_command(cmd, self.TimeOut)
        return res == "!!!!!"

    #─────────────
    # バッファ読取：丸めなし
    #─────────────
    def READ(self, 
        ch  :int,   # ① チャンネルID
        dev :int,   # ② デバイスID
    ) -> int:       # 戻値：アナログ値
        cmd = f"ANALOG/READ:{ch}:{dev}!"
        res = self._p._send_command(cmd, self.TimeOut)
        ok, v = _getValue(res)
        return v if ok else -1

    #─────────────
    # バッファ読取：四捨五入
    #─────────────
    def ROUND(self,
        ch  :int,   # ① チャンネルID
        dev :int,   # ② デバイスID
    ) -> int:       # 戻値：算出値
        値 = self.READ(ch, dev)
        if 値 < 0: return 値
        return self._四捨五入(値)

    #─────────────
    # バッファ読取：切り上げ
    #─────────────
    def ROUNDU(self,
        ch  :int,   # ① チャンネルID
        dev :int,   # ② デバイスID
    ) -> int:       # 戻値：算出値
        値 = self.READ(ch, dev)
        if 値 < 0: return 値
        return self._切り上げ(値)

    #─────────────
    # バッファ読取：切り下げ
    #─────────────
    def ROUNDD(self,
        ch  :int,   # ① チャンネルID
        dev :int,   # ② デバイスID
    ) -> int:       # 戻値：算出値
        値 = self.READ(ch, dev)
        if 値 < 0: return 値
        return self._切り下げ(値)

#━━━━━━━━━━━━━━━
# 内部ヘルパ
#━━━━━━━━━━━━━━━
    #─────────────
    # ・偶数step： r>=中間 で切り上げ
    # ・奇数step： r<=中間 で切り捨て（= r>=mid+1 で切り上げ）
    #─────────────
    def _四捨五入(self,
        raw :int,   # 元の値
    ) -> int:
        if raw < 0 or self.丸め単位 <= 0: return raw
        最大   = (1 << self.分解能) - 1
        値     = raw if raw <= 最大 else 最大
        商, 余 = divmod(値, self.丸め単位)
        中間   = self.丸め単位 // 2
        if 余 > 中間 or (self.丸め単位%2 == 0 and 余 == 中間): 商 += 1
        値     = 商 * self.丸め単位
        if 値 > 最大: 値 = 最大
        return 値
    #─────────────
    def _切り上げ(self,
        raw :int    ,   # 元の値
    ) -> int:
        if raw < 0 or self.丸め単位 <= 0: return raw
        最大   = (1 << self.分解能) - 1
        値     = raw if raw <= 最大 else 最大
        商, 余 = divmod(値, self.丸め単位)
        if 余 > 0   : 値 = (商 + 1) * self.丸め単位
        if 値 > 最大: 値 = 最大
        return 値
    #─────────────
    def _切り下げ(self,
        raw :int    ,   # 元の値
    ) -> int:
        if raw < 0 or self.丸め単位 <= 0: return raw
        最大 = (1 << self.分解能) - 1
        値   = raw if raw <= 最大 else 最大
        値   = (値 // self.丸め単位) * self.丸め単位
        if 値 < 0: 値 = 0
        return 値
