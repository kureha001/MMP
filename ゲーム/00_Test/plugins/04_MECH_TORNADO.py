#======================================================
# MECH TORNADO用 プラグイン
#======================================================
from dataclasses import dataclass
from typing import List, Type, Optional, Callable
import time

#───────────────────────────    
PLUGIN_NAME = "04.MECH TORNADO"

#───────────────────────────    
class 基底アクション:

    name: str = "Unnamed"
    description: str = ""

    @dataclass
    class Params:
        pass

    def run(self, svc, p, stop_event, emit: Optional[Callable[[str], None]] = None):
        raise NotImplementedError

#───────────────────────────    
def _出力(msg: str, emit: Optional[Callable[[str], None]]):
    if emit : emit(msg)
    else    : print(msg)

#======================================================
# コントローラ：各種スイッチの入力
#======================================================
class 各種スイッチ(基底アクション):

    name        = "コントローラ：各種スイッチの入力"
    description = "スイッチ(脚,手1,手2,ゲーム開始)を繰返しスキャンする"

    @dataclass
    class Params:
        使用HC個数  : int   = 4      # 使用するHC4067の個数(1～4)
        使用Pin数   : int   = 1      # 使用するHC4067のPin数(1～16)
        丸め閾値    : int   = 50     # アナログ値の丸め（この数値以下は0扱い）
        反復回数    : int   = 100    # 読み取り回数
        待機秒数    : float = 0.05   # 1回ごとの待機(秒)

    def run(self, svc, p: "各種スイッチ.Params", stop_event, emit=None):

        _出力("<スイッチ入力を監視>>", emit)
        mmp = svc.mmp
        mmp.アナログ設定(p.使用HC個数, p.使用Pin数, p.丸め閾値)

        _出力("step1. スイッチ確認", emit)
        for 回数 in range(p.反復回数):
            mmp.アナログ読取()
            if p.待機秒数 > 0: time.sleep(p.待機秒数)
            _出力(f"  {回数:03d} : {mmp.mmpAnaVal}", emit)

#======================================================
# フィールド：人形の脚を動作
#======================================================
class 人形の脚(基底アクション):

    name        = "フィールド：人形の脚を動作"
    description = "人形の脚を『DCモータのPWM制御』で動かす"
 
    @dataclass
    class Params:
        PWMチャネル : int   = 0      # DCモータのPWMチャネル
        上げ秒数    : float = 1.0    # 上げ保持時間(秒)
        上げ値      : int   = 4095   # 脚上げ（ON）デューティ
        下げ値      : int   = -1     # 脚下げ（OFF）デューティ

    def run(self, svc, p: "人形の脚.Params", stop_event, emit=None):

        _出力("<DCモータをON/OFF>>", emit)
        mmp = svc.mmp

        _出力("step1. 脚 上げ(1秒間)", emit)
        mmp.PWM_VALUE(p.PWMチャネル, p.上げ値)
        time.sleep(1)

        _出力("step2. 脚 下げ", emit)
        mmp.PWM_VALUE(p.PWMチャネル, p.下げ値)

#======================================================
# フィールド：鉄棒のロックスイッチを動作
#======================================================
class 鉄棒ロックと解除(基底アクション):

    name        = "フィールド：鉄棒のロックスイッチを動作"
    description = "鉄棒を『サーボのPWM制御』でロック/解除する"

    @dataclass
    class Params:
        サーボチャネル  : int   = 1     # サーボのPWMチャネル
        解除位置HIGH    : int   = 160   # 解除(HIGH)位置
        ロック位置LOW   : int   = 100   # ロック(LOW)位置
        待機秒数        : float = 1.0    # 各位置での待機(秒)

    def run(self, svc, p: "鉄棒ロックと解除.Params", stop_event, emit=None):

        _出力("<<サーボを制御>>", emit)
        mmp = svc.mmp
        ch = p.サーボチャネル

        _出力("step1. 鉄棒ロック解除", emit)
        mmp.PWM_VALUE(ch, p.解除位置HIGH)
        time.sleep(1)

        _出力("step2. 鉄棒ロック", emit)
        mmp.PWM_VALUE(ch, p.ロック位置LOW)

#======================================================
# 要素アクションを辞書に登録
#======================================================
ACTIONS: List[Type[基底アクション]] = [
    各種スイッチ    ,
    人形の脚        ,
    鉄棒ロックと解除,
    ]
