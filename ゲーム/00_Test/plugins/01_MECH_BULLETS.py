#====================================================== 
# 01.MECH BULLETS 用 プラグイン
#====================================================== 
from dataclasses import dataclass
from typing import List, Type, Optional, Callable
import time

#───────────────────────────    
PLUGIN_NAME = "01.MECH BULLETS"

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
# コントローラ：ジョイパッドの入力判定
#======================================================
class アナログ入力測定(基底アクション):

    name        = "コントローラ：ジョイパッドの入力判定"
    description = "ジョイパッドを繰返しスキャンする"

    @dataclass
    class Params:
        使用HC個数: int  = 4      # 使用するHC4067の個数(1～4)
        使用Pin数: int   = 1      # 使用するHC4067のPin数(1～16)
        丸め閾値 : int   = 100    # アナログ値の丸め（この数値以下は0扱い）
        繰返回数 : int   = 100    # 読み取り回数
        待時間秒 : float = 0.05   # 1回ごとのウェイト(秒)

    def run(self, svc, p: "アナログ入力測定.Params", stop_event, emit=None):

        _出力("<<ジョイパッドを測定>>", emit)
        mmp = svc.mmp
        mmp.アナログ設定(p.使用HC個数, p.使用Pin数, p.丸め閾値)

        _出力("step1. ゲームパッド入力", emit)
        for cntLoop in range(p.繰返回数):
            mmp.アナログ読取()
            if p.待時間秒 > 0: time.sleep(p.待時間秒)
            _出力(f"  {cntLoop:03d} : {mmp.mmpAnaVal}", emit)

#======================================================
# 要素アクションを登録
#======================================================
ACTIONS: List[Type[基底アクション]] = [
    アナログ入力測定,
    ]
