#======================================================
# MECH HUNTER用 テストプログラム
#======================================================
import time
import sys; sys.path.append('..'); import 共通.mmpRottenmeier
MMP = 共通.mmpRottenmeier.mmp()

#───────────────────────────    
def 電源投入():

    MMP.通信接続_自動()
    print("step1. 電源ＯＮ")
    for 装置No in range(2): MMP.PWM_VALUE(装置No,4095)
    time.sleep(5)
    print("step2. 電源ＯＦＦ")
    for 装置No in range(2): MMP.PWM_VALUE(装置No,-1)
    MMP.通信切断()

#───────────────────────────    
def 命中確認():

    MMP.通信接続_自動()
    MMP.アナログ設定(1, 2, 50)

    繰返回数 = 100    # アドレス切替回数
    待時間   = 0.05  # ウェイト(秒)

    print("step1. 電源ON")
    for 装置No in range(2): MMP.PWM_VALUE(装置No,4095)
    time.sleep(3)

    print("step1. スイッチ確認")
    for 回数 in range(繰返回数):        
        MMP.アナログ読取()
        if 待時間 > 0 : time.sleep(待時間)
        print("  %03i" % 回数,":", MMP.mmpAnaVal)

    print("step3. 電源OFF")
    for 装置No in range(2): MMP.PWM_VALUE(装置No,-1)

    MMP.通信切断()

#====================================================== 
PLUGIN_NAME = "03.MECH HUNTER"
ACTIONS = {
    "フィールド：電源の制御"        : 電源投入,
    "フィールド：命中スイッチの入力": 命中確認
}
