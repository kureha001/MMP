# -*- coding: utf-8 -*-
# filename : mmp_core_PWM.py
#============================================================
# ＭＭＰコマンド：ＰＷＭ出力
# バージョン：0.5.02 (2025/10/21) DELETEをRESETに変更
#------------------------------------------------------------
# [インストール方法]
# ・ＰＣ：[PYTHONPASTH] ※環境変数をセットしておく
# ・マイコン：[LIB]
# ・プロジェクトと同一ディレクトリ
#============================================================
from mmp_com import _getValue

class _Pwm:
#━━━━━━━━━━━━━━━
# コンストラクタ
#━━━━━━━━━━━━━━━
    def __init__(self, p:'MmpClient', argTimeOut):
        self._p      = p
        self.TimeOut = argTimeOut
        self.INFO    = self._Info    (p, self.TimeOut)
        self.ANGLE   = self._Angle   (p, self.TimeOut)
        self.ROTATE  = self._Rotation(p, self.TimeOut)

#━━━━━━━━━━━━━━━
# コマンド
#━━━━━━━━━━━━━━━
    #─────────────
    # ＰＷＭ値出力
    #─────────────
    def OUTPUT(self,
        chId      :int          , # チャンネルID
        pwmVal    :int          , # ＰＷＭ値
    ) -> bool:
        cmd = f"PWM/OUTPUT:{chId}:{pwmVal}!"
        res = self._p._send_command(cmd, self.TimeOut)
        return res == "!!!!!"

    #─────────────
    # サブ：インフォメーション
    #─────────────
    class _Info:
        #─────────────
        # インストラクタ
        #─────────────
        def __init__(self, p, argTimeOut):
            self._p = p
            self.TimeOut = argTimeOut

        #─────────────
        # 接続状況
        #─────────────
        def CONNECT(self) -> int:
            cmd = f"PWM/INFO/CONNECT!"
            res = self._p._send_command(cmd, self.TimeOut)
            ok, v = _getValue(res)
            return v if ok else -1

    #─────────────
    # サブ：角度指定
    #─────────────
    class _Angle:
        #─────────────
        # インストラクタ
        #─────────────
        def __init__(self, p, argTimeOut):
            self._p = p
            self.TimeOut = argTimeOut

        #─────────────
        # プリセット登録
        #─────────────
        def SETUP(self,
            chIDfrom    :int        , # チャンネルID(開始)
            chIDto      :int = -1   , # チャンネルID(終了) ※-1：単一
            degTo       :int = 180  , # 角度(最大)
            pwmFrom     :int = 300  , # ＰＷＭ値(0度)
            pwmTo       :int = 600  , # ＰＷＭ値(最大角度)
        ) -> bool:
            cmd = ("PWM/ANGLE/SETUP:"
                f"{chIDfrom}:{chIDto}:"
                f"{degTo}:"
                f"{pwmFrom}:{pwmTo}!"
            )
            res = self._p._send_command(cmd, self.TimeOut)
            return res == "!!!!!"

        #─────────────
        # プリセット削除
        #─────────────
        def RESET(self,
            chIDfrom:int        , # チャンネルID(開始)
            chIDto  :int = -1   , # チャンネルID(終了) ※-1：単一
        ) -> bool:
            cmd = f"PWM/ANGLE/RESET:{chIDfrom}:{chIDto}!"
            res = self._p._send_command(cmd, self.TimeOut)
            return res == "!!!!!"

        #─────────────
        # ＰＷＭ出力
        #─────────────
        def OUTPUT(self,
            chId    :int,   # チャンネルID
            angle   :int,   # 角度（0～degTo）
        ) -> int:           # 戻値：ＰＷＭ値
            cmd = f"PWM/ANGLE/OUTPUT:{int(chId)}:{angle}!"
            res = self._p._send_command(cmd, self.TimeOut)
            ok, v = _getValue(res)
            return v if ok else -1

    #─────────────
    # サブ：回転型サーボ
    #─────────────
    class _Rotation:
        #─────────────
        # インストラクタ
        #─────────────
        def __init__(self, p, argTimeOut):
            self._p = p
            self.TimeOut = argTimeOut

        #─────────────
        # プリセット登録
        #─────────────
        def SETUP(self,
            chIDfrom    :int        , # チャンネルID(開始)
            chIDto      :int = -1   , # チャンネルID(終了) ※-1：単一
            rightLow    :int = 397  , # ＰＷＭ値(左周り最大)
            rightHigh   :int = 430  , # ＰＷＭ値(右周り最大)
            leftLow     :int = 358  , # ＰＷＭ値(左周り最大)
            leftHigh    :int = 330  , # ＰＷＭ値(右周り最大)
        ) -> bool:
            cmd = ("PWM/ROTATE/SETUP:"
                f"{chIDfrom}:{chIDto}:"
                f"{rightLow}:{rightHigh}:"
                f"{leftLow}:{leftHigh}!"
            )
            res    = self._p._send_command(cmd, self.TimeOut)
            return res == "!!!!!"

        #─────────────
        # プリセット削除
        #─────────────
        def RESET(self,
            chIDfrom    :int        , # チャンネルID(開始)
            chIDto      :int = -1   , # チャンネルID(終了) ※-1：単一
        ) -> bool:
            cmd = "PWM/ROTATE/RESET"
            cmd = f"{cmd}:{chIDfrom}:{chIDto}!"
            res = self._p._send_command(cmd, self.TimeOut)
            return res == "!!!!!"

        #─────────────
        # ＰＷＭ出力
        #─────────────
        def OUTPUT(self,
            chId    :int,   # チャンネルID
            rate    :int,   # 速度（-100～+100）
        ) -> int:           # 戻値：ＰＷＭ値
            cmd = "PWM/ROTATE/OUTPUT"
            cmd = f"{cmd}:{chId}:{rate}!"
            res = self._p._send_command(cmd, self.TimeOut)
            ok, v = _getValue(res)
            return v if ok else -1
