# -*- coding: utf-8 -*-
# filename : mmp_core_PWM.py
#============================================================
# ＭＭＰコマンド：ＰＷＭ出力
# バージョン：0.5
#------------------------------------------------------------
# [インストール方法]
# ・ＰＣ：[PYTHONPASTH] ※環境変数をセットしておく
# ・マイコン：[LIB]
# ・プロジェクトと同一ディレクトリ
#============================================================
from mmp_com import _DECtoHEX2, _HEX4toDEC

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
        cmd = "PWM/OUTPUT"
        cmd = f"{cmd}:{int(chId):02X}:{int(pwmVal):04X}!"
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
            cmd = "PWM/INFO/CONNECT"
            res = self._p._send_command(f"{cmd}!", self.TimeOut)
            ok, v = _HEX4toDEC(res)
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
        # 初期化
        #─────────────
        def SETUP(self,
            chIDfrom    :int        , # チャンネルID(開始)
            chIDto      :int = -1   , # チャンネルID(終了) ※-1：単一
            degTo       :int = 180  , # 角度(最大)
            pwmFrom     :int = 300  , # ＰＷＭ値(0度)
            pwmTo       :int = 600  , # ＰＷＭ値(最大角度)
            pwmMiddle   :int = -1   , # ＰＷＭ値(中間) ※-1：自動
            *,timeoutMs :int = 0    ,
        ) -> bool:
            cmd = "PWM/ANGLE/SETUP"
            cmd = (
                f"{cmd}:{_DECtoHEX2(int(chIDfrom))}:{_CHtoHEX2(chIDto)}:"
                f"{_DEGtoHEX3(degTo)}:"
                f"{_PWMtoHEX4(pwmFrom)}:{_PWMtoHEX4(pwmTo)}:{_PWMtoHEX4U(pwmMiddle)}!"
            )
            res = self._p._send_command(cmd, self.TimeOut)
            return res == "!!!!!"

        #─────────────
        # 設定削除
        #─────────────
        def DELETE(self,
            chIDfrom    :int        , # チャンネルID(開始)
            chIDto      :int = -1   , # チャンネルID(終了) ※-1：単一
        ) -> bool:
            cmd = "PWM/ANGLE/DELETE"
            cmd = f"{cmd}:{_DECtoHEX2(int(chIDfrom))}:{_CHtoHEX2(chIDto)}!"
            res = self._p._send_command(cmd, self.TimeOut)
            return res == "!!!!!"

        #─────────────
        # ＰＷＭ出力：通常
        #─────────────
        def OUTPUT(self,
            chId        :int        , # チャンネルID
            angle       :int        , # 角度（0～degTo）
        ) -> bool:
            cmd = "PWM/ANGLE/OUTPUT"
            cmd = f"{cmd}:{_DECtoHEX2(int(chId))}:{_DEGtoHEX3(angle)}!"
            res = self._p._send_command(cmd, self.TimeOut)
            return res == "!!!!!"

        #─────────────
        # ＰＷＭ出力：中間
        #─────────────
        def CENTER(self,
            chId        :int        , # チャンネルID
        ) -> bool:
            cmd = "PWM/ANGLE/CENTER"
            cmd = f"{cmd}:{_DECtoHEX2(int(chId))}!"
            res = self._p._send_command(cmd, self.TimeOut)
            return res == "!!!!!"

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
            pwmLow      :int = 300  , # ＰＷＭ値(左周り最大)
            pwmHigh     :int = 600  , # ＰＷＭ値(右周り最大)
            pwmMiddle   :int = -1   , # ＰＷＭ値(停止) ※-1：自動
        ) -> bool:
            cmd = "PWM/ROTATE/SETUP"
            cmd = (
                f"{cmd}:{_DECtoHEX2(int(chIDfrom))}:{_CHtoHEX2(chIDto)}:"
                f"{_PWMtoHEX4(pwmLow)}:{_PWMtoHEX4(pwmHigh)}:{_PWMtoHEX4U(pwmMiddle)}!"
            )
            res    = self._p._send_command(cmd, self.TimeOut)
            return res == "!!!!!"

        #─────────────
        # プリセット削除
        #─────────────
        def DELETE(self,
            chIDfrom    :int        , # チャンネルID(開始)
            chIDto      :int = -1   , # チャンネルID(終了) ※-1：単一
        ) -> bool:
            cmd = "PWM/ROTATE/DELETE"
            cmd = f"{cmd}:{_DECtoHEX2(int(chIDfrom))}:{_CHtoHEX2(chIDto)}!"
            res = self._p._send_command(cmd, self.TimeOut)
            return res == "!!!!!"

        #─────────────
        # ＰＷＭ出力：通常
        #─────────────
        def OUTPUT(self,
            chId        :int        , # チャンネルID
            rate        :int        , # 速度（-100～+100）
        ) -> bool:
            cmd = "PWM/ROTATE/OUTPUT"
            cmd = f"{cmd}:{_DECtoHEX2(int(chId))}:{_DECtoHEX2(int(rate + 100))}!"
            res = self._p._send_command(cmd, self.TimeOut)
            return res == "!!!!!"

        #─────────────
        # ＰＷＭ出力：停止
        #─────────────
        def STOP(self,
            chId    :int    , # チャンネルID
        ) -> bool:
            cmd = "PWM/ROTATE/STOP"
            cmd = f"{cmd}:{_DECtoHEX2(int(chId))}!"
            res = self._p._send_command(cmd, self.TimeOut)
            return res == "!!!!!"

#━━━━━━━━━━━━━━━
# 内部ヘルパ
#━━━━━━━━━━━━━━━
def _DEGtoHEX3(deg: int) -> str:
    try             : v = int(deg)
    except Exception: v = 0
    return f"{v & 0xFFF:03X}"
#───────────────
def _PWMtoHEX4(pwm: int) -> str:
    try             : v = int(pwm)
    except Exception: v = 0
    return f"{v & 0xFFFF:04X}"
#───────────────
def _PWMtoHEX4U(val: int) -> str:
    try             : v = int(val)
    except Exception: return "FFFF"
    return "FFFF" if v == -1 else f"{v & 0xFFFF:04X}"
#───────────────
def _CHtoHEX2(toCh: int) -> str:
    try             : v = int(toCh)
    except Exception: return "FF"
    return "FF" if v == -1 else f"{v & 0xFF:02X}"
