# -*- coding: utf-8 -*-
# filename : mmp_core_PWM.py
#============================================================
# ＭＭＰコマンド：ＰＷＭ出力
#------------------------------------------------------------
# [インストール方法]
# ・ＰＣ：[PYTHONPASTH] ※環境変数をセットしておく
# ・マイコン：[LIB]
# ・プロジェクトと同一ディレクトリ
#============================================================
from mmp_core_COM import (
    _resolve, _hex1, _hex2, _hex3, _hex4, _try_parse_hex4_bang,
    enc_ch_end, enc_opt_u16, enc_deg3, enc_pwm4
)

class _PwmModule:
    #----------------------
    # コンストラクタ
    #----------------------
    def __init__(self, p:'MmpClient'):
        self._p       = p
        self.Angle    = self._AngleModule(p)
        self.Rotation = self._RotationModule(p)

    #----------------------
    # ０．基本
    #----------------------
    #----------------------
    # ① ＰＷＭ値出力
    #----------------------
    def Out( self           ,
        chId      :int      , # チャンネルID
        pwmVal    :int      , # ＰＷＭ値
        timeoutMs :int = 0
    ) -> bool:
        timeOut = _resolve(timeoutMs, self._p.Settings.TimeoutPwm)
        cmd     = f"PWM:{int(chId):02X}:{int(pwmVal):04X}!"
        resp    = self._p._send_command(cmd, timeOut)
        return resp == "!!!!!"
    #----------------------
    # ②デバイス情報
    #----------------------
    def Info(self           ,
        devId     :int      , # デバイスID
        timeoutMs :int = 0  ,
    ) -> int:
        timeOut = _resolve(timeoutMs, self._p.Settings.TimeoutPwm)
        resp    = self._p._send_command(f"PWX:{int(devId):02X}!", timeOut)
        ok, v   = _try_parse_hex4_bang(resp)
        return v if ok else -1

    #----------------------
    # １．角度
    #----------------------
    class _AngleModule:
        def __init__(self, p): self._p = p
        #----------------------
        # ① 初期化（PAI）
        #----------------------
        def Init(self               ,
            chIDfrom    :int        , # チャンネルID(開始)
            chIDto      :int = -1   , # チャンネルID(終了) ※-1：単一
            degTo       :int = 180  , # 角度(最大)
            pwmFrom     :int = 300  , # ＰＷＭ値(0度)
            pwmTo       :int = 600  , # ＰＷＭ値(最大角度)
            pwmMiddle   :int = -1   , # ＰＷＭ値(中間) ※-1：自動
            *,timeoutMs :int = 0    ,
        ) -> bool:
            timeOut = _resolve(timeoutMs, self._p.Settings.TimeoutPwm)
            cmd     = (
                f"PAI:{_hex2(int(chIDfrom))}:{enc_ch_end(chIDto)}:"
                f"{enc_deg3(degTo)}:"
                f"{enc_pwm4(pwmFrom)}:{enc_pwm4(pwmTo)}:{enc_opt_u16(pwmMiddle)}!"
            )
            resp    = self._p._send_command(cmd, timeOut)
            return resp == "!!!!!"

        #----------------------
        # ② 設定削除（PAD）
        #----------------------
        def Delete(self             ,
            chIDfrom    :int        , # チャンネルID(開始)
            chIDto      :int = -1   , # チャンネルID(終了) ※-1：単一
            *,timeoutMs :int = 0    ,
        ) -> bool:
            timeOut = _resolve(timeoutMs, self._p.Settings.TimeoutPwm)
            cmd     = f"PAD:{_hex2(int(chIDfrom))}:{enc_ch_end(chIDto)}!"
            resp    = self._p._send_command(cmd, timeOut)
            return resp == "!!!!!"

        #----------------------
        # ③ 出力（PAO）
        #----------------------
        def Out(self            ,
            chId        :int    , # チャンネルID
            angle       :int    , # 角度（0～degTo）
            *,timeoutMs :int = 0,
        ) -> bool:
            timeOut = _resolve(timeoutMs, self._p.Settings.TimeoutPwm)
            cmd     = f"PAO:{_hex2(int(chId))}:{enc_deg3(angle)}!"
            resp    = self._p._send_command(cmd, timeOut)
            return resp == "!!!!!"

        #----------------------
        # ④ 中心に移動（PAN）
        #----------------------
        def Center(self         ,
            chId        :int    , # チャンネルID
            *,timeoutMs :int = 0,
        ) -> bool:
            timeOut = _resolve(timeoutMs, self._p.Settings.TimeoutPwm)
            cmd     = f"PAN:{_hex2(int(chId))}!"
            resp    = self._p._send_command(cmd, timeOut)
            return resp == "!!!!!"

    #----------------------
    # ２．ローテーション
    #----------------------
    class _RotationModule:
        def __init__(self, p): self._p = p
        #----------------------
        # ① 初期化（PRI）
        #----------------------
        def Init(self               ,
            chIDfrom    :int        , # チャンネルID(開始)
            chIDto      :int = -1   , # チャンネルID(終了) ※-1：単一
            pwmLow      :int = 300  , # ＰＷＭ値(左周り最大)
            pwmHigh     :int = 600  , # ＰＷＭ値(右周り最大)
            pwmMiddle   :int = -1   , # ＰＷＭ値(停止) ※-1：自動
            *,timeoutMs :int = 0    ,
        ) -> bool:
            timeOut = _resolve(timeoutMs, self._p.Settings.TimeoutPwm)
            cmd     = (
                f"PRI:{_hex2(int(chIDfrom))}:{enc_ch_end(chIDto)}:"
                f"{enc_pwm4(pwmLow)}:{enc_pwm4(pwmHigh)}:{enc_opt_u16(pwmMiddle)}!"
            )
            resp = self._p._send_command(cmd, timeOut)
            return resp == "!!!!!"

        #----------------------
        # ② 設定削除（PRD）
        #----------------------
        def Delete(self             ,
            chIDfrom    :int        , # チャンネルID(開始)
            chIDto      :int = -1   , # チャンネルID(終了) ※-1：単一
            *,timeoutMs :int = 0    ,
        ) -> bool:
            timeOut = _resolve(timeoutMs, self._p.Settings.TimeoutPwm)
            cmd     = f"PRD:{_hex2(int(chIDfrom))}:{enc_ch_end(chIDto)}!"
            resp    = self._p._send_command(cmd, timeOut)
            return resp == "!!!!!"

        #----------------------
        # ③ 出力（PRO）
        #----------------------
        def Out(self            ,
            chId        :int    , # チャンネルID
            rate        :int    , # 速度（-100～+100）
            *,timeoutMs :int = 0,
        ) -> bool:
            timeOut = _resolve(timeoutMs, self._p.Settings.TimeoutPwm)
            cmd     = f"PRO:{_hex2(int(chId))}:{_hex2(int(rate + 100))}!"
            resp    = self._p._send_command(cmd, timeOut)
            return resp == "!!!!!"

        #----------------------
        # ④ 停止
        #----------------------
        def Stop(self               ,
            chId        :int        , # チャンネルID
            *,timeoutMs :int = 0    ,
        ) -> bool:
            timeOut = _resolve(timeoutMs, self._p.Settings.TimeoutPwm)
            cmd     = f"PRN:{_hex2(int(chId))}!"
            resp    = self._p._send_command(cmd, timeOut)
            return resp == "!!!!!"
