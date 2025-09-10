#┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
#┃アプリ共通：ＭＭＰコネクション
#┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
# [アプリ共通]
#import bak_mmpC as ライブラリ
#import MMP # アプリとのコネクション(このモジュール)
#接続 = None  # MMPオブジェクト
from mmpC import MmpClient
from mmpC import CpyAdapter
#mmp = MmpClient(CpyAdapter())
接続 = MmpClient(CpyAdapter())  # MMPオブジェクト

#────────────────────────────────────    
def 初期化(引数_個数,引数_Ch数):
    #┬
    print("<< ＭＭＰライブラリ >>\n")
    print("接続中...", end="")
    #│
    #●MMPを実体化する。
    #接続 = MmpClient(CpyAdapter())
    #│
    if not 接続.ConnectAutoBaud():
        print("ＭＭＰとの接続に失敗しました...")
        return
    #│
    print("[システム情報]")
    print("　・バージョン  : {}".format(接続.Info.Version()))
    print("　・PCA9685 [0] : 0x{:04X}".format(接続.Info.Dev.Pwm(0)))
    print("　・DFPlayer[1] : 0x{:04X}".format(接続.Info.Dev.Audio(1)))
    #│
    #●アナログ入力を設定する
    ok = 接続.Analog.Configure(
            引数_個数, # 使用するHC4067の個数(1～4)
            引数_Ch数, # 使用するHC4067のPin数(1～16)
            )
    #┴    
#────────────────────────────────────    
"""
def 初期化(引数_個数,引数_Ch数,引数_丸め,):
    #┬
    #●MMPを実体化する。
    MMP.接続 = ライブラリ.mmp()
    #│
    #●MMPを接続する
    MMP.接続.通信接続()
    #│
    #●アナログ入力を設定する
    MMP.接続.アナログ設定(
        引数_個数, # 使用するHC4067の個数(1～4)
        引数_Ch数, # 使用するHC4067のPin数(1～16)
        引数_丸め  # アナログ値の丸め(この数値以下は切り捨て)
        )
    #┴
"""
