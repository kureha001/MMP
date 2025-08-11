#======================================================
# MECH TORNADO用 テストプログラム
#======================================================
import time
import sys; sys.path.append('..'); import 共通.mmpRottenmeier
MMP = 共通.mmpRottenmeier.mmp()

#───────────────────────────    
def スイッチ():

    MMP.通信接続_自動()

    print("step1. スイッチ確認")
    MMP.アナログ設定(4, 1, 50)
    # 0:脚の屈伸
    # 1:手１
    # 2:手２
    # 3:ゲーム開始
    for cntLoop in range(100):        
        #◎└┐全アドレスから読み取る。
        MMP.アナログ読取()
        time.sleep(0.1)
        print("  %03i" % cntLoop,":", MMP.mmpAnaVal)

    MMP.通信切断()

#───────────────────────────    
def 人形の脚():

    MMP.通信接続_自動()

    print("step1. 脚 上げ")
    MMP.PWM_VALUE(0, 4095)
    time.sleep(1)
    print("step2. 脚 下げ")
    MMP.PWM_VALUE(0, -1)

    MMP.通信切断()

#───────────────────────────    
def 鉄棒ロック():

    MMP.通信接続_自動()

    print("step1. 鉄棒ロック解除")
    MMP.PWM_VALUE(1, 160)
    time.sleep(1)
    print("step2. 鉄棒ロック")
    MMP.PWM_VALUE(1, 100)

    MMP.通信切断()

#====================================================== 
PLUGIN_NAME = "04.MECH TORNADO"
ACTIONS = {
    "コントローラ：各種スイッチの入力"              : スイッチ  ,
    "フィールド：人形の脚(DCモータ)の制御(屈伸有無)": 人形の脚  ,
    "フィールド：鉄棒(サーボ)の制御(ロック/解除)"   : 鉄棒ロック
}
