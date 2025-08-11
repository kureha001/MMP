#====================================================== 
# 02.MECH SAURUS 用 プラグイン
#====================================================== 
from dataclasses import dataclass
from typing import List, Type, Optional, Callable
import time

#───────────────────────────    
PLUGIN_NAME = "02.MECH SAURUS"

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
# サーボ制御共通
#======================================================
def サーボ制御(
    mmp,
    モータ  : [int, ...],
    最小    : int,
    最大    : int,
    増分    : int   = 2,
    間隔    : float = 0.02,
    待ち    : float = 1.0,
    中央補正: int   = -18,
    emit    : Optional[Callable[[str], None]] = None
    ):

    _出力("    -> PWM : 中央へ", emit)
    中央 = int((最大 + 最小) / 2) + 中央補正
    for ch in モータ: mmp.PWM_VALUE(ch, 中央)
    time.sleep(待ち)

    _出力("    -> PWM : 中央～最大へ", emit)
    for val in range(中央, 最大, 増分):
        for ch in モータ: mmp.PWM_VALUE(ch, val)
        time.sleep(間隔)
    time.sleep(待ち)

    _出力("    -> PWM : 最大～最小へ", emit)
    for val in range(最大, 最小, -増分):
        for ch in モータ: mmp.PWM_VALUE(ch, val)
        time.sleep(間隔)
    time.sleep(待ち)

    _出力("    -> PWM : 最小～中央へ", emit)
    for val in range(最小, 中央, 増分):
        for ch in モータ: mmp.PWM_VALUE(ch, val)
        time.sleep(間隔)

#======================================================
# コントローラ：フットスイッチの入力判定
#======================================================
class フットスイッチ入力(基底アクション):

    name        = "コントローラ：フットスイッチの入力判定"
    description = "フットスイッチを繰返しスキャンする"

    @dataclass
    class Params:
        使用HC個数  : int   = 2
        使用Pin数   : int   = 2
        丸め閾値    : int   = 200
        繰返回数    : int   = 50
        待時間秒    : float = 0.1

    def run(self, svc, p: "フットスイッチ入力.Params", stop_event, emit=None):

        _出力("<<フットスイッチを測定>>", emit)
        mmp = svc.mmp
        mmp.アナログ設定(p.使用HC個数, p.使用Pin数, p.丸め閾値)

        _出力("step1. フットスイッチ入力", emit)
        for cntLoop in range(p.繰返回数):
            if stop_event.is_set():
                _出力("[フットスイッチ] 停止指示により中断しました。", emit)
                break
            mmp.アナログ読取()
            if p.待時間秒 > 0: time.sleep(p.待時間秒)
            _出力(f"  {cntLoop:03d} : {mmp.mmpAnaVal}", emit)

#======================================================
# フィールド：通過センサーの入力判定
#======================================================
class 通過センサ入力(基底アクション):

    name        = "フィールド：通過センサの入力判定"
    description = "通過センサを繰返しスキャンする"

    @dataclass
    class Params:
        使用HC個数  : int   = 2
        使用Pin数   : int   = 10
        丸め閾値    : int   = 100
        繰返回数    : int   = 50
        待時間秒    : float = 0.1
        参照Index1  : int   = 8
        参照Index2  : int   = 9

    def run(self, svc, p: "通過センサ入力.Params", stop_event, emit=None):

        _出力("<<通過センサを測定>>", emit)
        mmp = svc.mmp
        mmp.アナログ設定(p.使用HC個数, p.使用Pin数, p.丸め閾値)

        _出力("step1. 通過センサ入力", emit)
        for cntLoop in range(p.繰返回数):
            if stop_event.is_set():
                _出力("[通過センサ] 停止指示により中断しました。", emit)
                break
            mmp.アナログ読取()
            if p.待時間秒 > 0: time.sleep(p.待時間秒)
            try:
                センサ値1 = (mmp.mmpAnaVal[p.参照Index1][0], mmp.mmpAnaVal[p.参照Index1][1])
                センサ値2 = (mmp.mmpAnaVal[p.参照Index2][0], mmp.mmpAnaVal[p.参照Index2][1])
            except Exception:
                センサ値1 = センサ値2 = ("N/A", "N/A")
            _出力(f"  {cntLoop:03d} : 1= {センサ値1} ／ 2= {センサ値2}", emit)

#======================================================
# フィールド：基板の電源ON/OFF,運搬機の動作
#======================================================
class 基板電源_DC運搬(基底アクション):

    name        = "フィールド：基板の電源ON/OFF,運搬機の動作"
    description = "『基板電源のON/OFF』『運搬機のDCモータ』をPWM制御で制御する"

    @dataclass
    class Params:
        DCモータ: [int, int]    = (3, 15)   # DCモータのチャネル
        基板    : [int, int]    = (2, 14)   # 基板電源のチャネル
        最小    : int           = 1300
        最大    : int           = 3500 
        間隔S   : int           = 50
        間隔E   : int           = -100
        停止    : float         = 0.2
        速度アップ後待機: float = 1.0

    def run(self, svc, p: "基板電源_DC運搬.Params", stop_event, emit=None):

        _出力("<<基板電源をON/OFF、運搬機のDCモータ動かす>>", emit)
        mmp = svc.mmp
        モータ = tuple(p.DCモータ)
        基板   = tuple(p.基板)

        _出力("step1. 基板：電源ＯＮ", emit)
        for ch in 基板: mmp.PWM_VALUE(ch, 4095)

        _出力("step2. DCモータ：速度アップ", emit)
        for val in range(p.最小, p.最大, p.間隔S):
            if stop_event.is_set():
                _出力("[DCモータ] 停止指示により中断しました。", emit)
                break
            for ch in モータ: mmp.PWM_VALUE(ch, val)
            time.sleep(p.停止)

        time.sleep(p.速度アップ後待機)

        _出力("step3. DCモータ：速度ダウン", emit)
        for val in range(p.最大, p.最小, p.間隔E):
            if stop_event.is_set():
                _出力("[DCモータ] 停止指示により中断しました。", emit)
                break
            for ch in モータ: mmp.PWM_VALUE(ch, val)
            time.sleep(p.停止)

        _出力("step4. 基板：電源ＯＦＦ", emit)
        for ch in モータ: mmp.PWM_VALUE(ch, -1)
        for ch in 基板: mmp.PWM_VALUE(ch, -1)

#======================================================
# フィールド(小屋)：恐竜首を動作
#======================================================
class 小屋_恐竜首(基底アクション):

    name        = "フィールド(小屋)：恐竜首を動作"
    description = "恐竜首を『サーボのPWM制御』で左右に動かす"

    @dataclass
    class Params:
        モータ     : [int, int] = (1, 13)
        最小       : int        = 120
        最大       : int        = 380
        増分       : int        = 2
        間隔       : float      = 0.02
        待ち       : float      = 1.0
        中央補正   : int        = -18

    def run(self, svc, p: "小屋_恐竜首.Params", stop_event, emit=None):

        _出力("<<恐竜首のサーボを動かす>>", emit)
        mmp = svc.mmp

        _出力("step1. 恐竜の首 : 最小から最大", emit)
        サーボ制御(mmp, tuple(p.モータ), p.最小, p.最大, p.増分, p.間隔, p.待ち, p.中央補正, emit)

#======================================================
# フィールド(小屋)：メータのサーボを動作
#======================================================
class 小屋_メータ(基底アクション):

    name        = "フィールド(小屋)：メータのサーボを動作"
    description = "骨メータを『サーボのPWM制御』で左右に動かす"

    @dataclass
    class Params:
        メータ     : [int, int] = (0, 12)
        最小       : int        = 120
        最大       : int        = 370
        増分       : int        = 2
        間隔       : float      = 0.02
        待ち       : float      = 1.0
        中央補正   : int        = -18

    def run(self, svc, p: "小屋_メータ.Params", stop_event, emit=None):

        _出力("<<骨メータのサーボを動かす>>", emit)
        mmp = svc.mmp
        _出力("step1. 骨メータ: 最小から最大", emit)
        サーボ制御(mmp, tuple(p.メータ), p.最小, p.最大, p.増分, p.間隔, p.待ち, p.中央補正, emit)

#======================================================
# フィールド(ジオラマ)：恐竜首・砂時計の動作
#======================================================
class ジオラマ制御(基底アクション):

    name        = "フィールド(ジオラマ)：恐竜首・砂時計の動作"
    description = "『砂時計を左右』『恐竜首を上下』にサーボのPWM制御で動かす"

    @dataclass
    class Params:
        砂時計チャネル: int         = 5
        砂時計_最小   : int         = 136
        砂時計_最大   : int         = 616
        砂時計_増分   : int         = 1
        砂時計_間隔   : float       = 0.002
        恐竜モータ    : [int, int]  = (7, 6)
        恐竜_最小     : int         = 230
        恐竜_最大     : int         = 370
        恐竜_増分     : int         = 2
        恐竜_間隔     : float       = 0.005
        恐竜_待ち     : float       = 1.0
        恐竜_中央補正 : int         = -18

    def run(self, svc, p: "ジオラマ制御.Params", stop_event, emit=None):

        _出力("<<砂時計・恐竜首のサーボを動かす>>", emit)
        mmp = svc.mmp

        _出力("step1. 砂時計 : 最小から最大", emit)
        for val in range(p.砂時計_最小, p.砂時計_最大, p.砂時計_増分):
            if stop_event.is_set():
                _出力("[砂時計] 停止指示により中断しました。", emit)
                return
            mmp.PWM_VALUE(p.砂時計チャネル, val)
            time.sleep(p.砂時計_間隔)
        time.sleep(1.0)

        _出力("step2. 恐竜の首 : 最小から最大", emit)
        サーボ制御(
            mmp, tuple(p.恐竜モータ), p.恐竜_最小, p.恐竜_最大,
            p.恐竜_増分, p.恐竜_間隔, p.恐竜_待ち, p.恐竜_中央補正,
            emit
            )
        time.sleep(1.0)

        _出力("step3. 砂時計 PWM : 最大から最小", emit)
        for val in range(p.砂時計_最大, p.砂時計_最小, -p.砂時計_増分):
            if stop_event.is_set():
                _出力("[砂時計] 停止指示により中断しました。", emit)
                return
            mmp.PWM_VALUE(p.砂時計チャネル, val)
            time.sleep(p.砂時計_間隔)

#======================================================
# 要素アクションを登録
#======================================================
ACTIONS: List[Type[基底アクション]] = [
    フットスイッチ入力  ,
    通過センサ入力      ,
    基板電源_DC運搬     ,
    小屋_恐竜首         ,
    小屋_メータ         ,
    ジオラマ制御        ,
    ]
