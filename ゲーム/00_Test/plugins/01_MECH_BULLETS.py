#====================================================== 
# MECH BULLETS用 テストプログラム
#====================================================== 
import time
import sys; sys.path.append('..'); import 共通.mmpRottenmeier

# MMPを実体化
MMP = 共通.mmpRottenmeier.mmp()

#───────────────────────────    
def アナログ入力テスト():

    MMP.通信接続_自動()
    MMP.アナログ設定(4, 1, 100)

    繰返回数 = 100   # アドレス切替回数
    待時間   = 0.05  # ウェイト(秒)

    print("step1. ゲームパッド入力")
    for cntLoop in range(繰返回数):        
        MMP.アナログ読取()
        if 待時間 > 0: time.sleep(待時間)
        print("  %03i" % cntLoop, ":", MMP.mmpAnaVal)

    MMP.通信切断()

#====================================================== 
PLUGIN_NAME = "01.MECH BULLETS"
ACTIONS = {
    "コントローラ：ジョイパッドの入力": アナログ入力テスト
    }

