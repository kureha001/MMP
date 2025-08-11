#======================================================
# MECH HUNTER用 プラグイン
#======================================================
from dataclasses import dataclass
from typing import List, Type, Optional, Callable
import time

#───────────────────────────    
PLUGIN_NAME = "03.MECH HUNTER"

#───────────────────────────    
class 基底アクション:

    name: str = "Unnamed"
    description: str = ""
    @dataclass
    class Params: pass
    def run(self, svc, p, stop_event, emit: Optional[Callable[[str], None]] = None):
        raise NotImplementedError

#───────────────────────────    
def _出力(msg: str, emit: Optional[Callable[[str], None]]):
    if emit : emit(msg)
    else    : print(msg)

#======================================================
# フィールド：カモロボの電源制御
#======================================================
class 電源制御(基底アクション):

    name        = "フィールド：カモロボの電源制御"
    description = "カモロボをPWM制御でON/OFFする"

    @dataclass
    class Params:

        duration_sec: float = 5.0

    def run(self, svc, p: "電源制御.Params", stop_event, emit=None):

        _出力("<<電源をON/OFF>>", emit)
        mmp = svc.mmp

        _出力("step1. 電源ON(5秒間)", emit)
        for 装置No in range(2): mmp.PWM_VALUE(装置No,4095)
        time.sleep(5)

        _出力("step2. 電源OFF", emit)
        for 装置No in range(2): mmp.PWM_VALUE(装置No,-1)


#======================================================
# フィールド：カモロボの命中判定
#======================================================
class アナログ入力測定(基底アクション):

    name        = "フィールド：カモロボの命中判定"
    description = "カモロボのLEDを繰返しスキャンする"

    @dataclass
    class Params:

        使用HC個数  : int   = 1      # 使用するHC4067の個数(1～4)
        使用Pin数   : int   = 2      # 使用するHC4067のPin数(1～16)
        丸め閾値    : int   = 50    # アナログ値の丸め（この数値以下は0扱い）
        繰返回数    : int   = 100    # 読み取り回数
        待時間秒    : float = 0.05   # 1回ごとのウェイト(秒)
        電源ON秒    : float = 3.0

    def run(self, svc, p: "アナログ入力測定.Params", stop_event, emit=None):

        _出力("<<羽のLED電圧を測定>>", emit)
        mmp = svc.mmp
        mmp.アナログ設定(p.使用HC個数, p.使用Pin数, p.丸め閾値)

        _出力("step1. 電源ON(3秒間まつ)", emit)
        for 装置No in range(2): mmp.PWM_VALUE(装置No,4095)
        time.sleep(p.電源ON秒)

        try:
            _出力("step2. アナログ読取", emit)
            for i in range(p.repeat):
                mmp.アナログ読取()
                if p.wait_sec > 0: time.sleep(p.wait_sec)
                _出力(f"  {i:03d} : {mmp.mmpAnaVal}", emit)

        finally:
            _出力("step3. 電源OFF", emit)
            for 装置No in range(2): mmp.PWM_VALUE(装置No,-1)

#======================================================
# 要素アクションを辞書に登録
#======================================================
ACTIONS: List[Type[基底アクション]] = [
    電源制御        ,
    アナログ入力測定,
    ]
