#====================================================== 
# MECH SAURUS用 テストプログラム
#====================================================== 
import os
import time
import sys; sys.path.append('..'); import 共通.mmpRottenmeier

#○MMPを実体化する。
MMP = 共通.mmpRottenmeier.mmp()

#───────────────────────────    
def ぺダル():
    MMP.通信接続_自動()
    MMP.アナログ設定(2, 2, 200)

    繰返回数 = 50
    待時間   = 0.1

    print("step1. フットスイッチ入力")
    for cntLoop in range(繰返回数):        
        MMP.アナログ読取()
        if 待時間 > 0 : time.sleep(待時間)
        print("  %03i" % cntLoop,":", MMP.mmpAnaVal)

    MMP.通信切断()

#───────────────────────────    
def 通過センサ():

    MMP.通信接続_自動()
    MMP.アナログ設定(2, 10, 100)

    繰返回数 = 50
    待時間   = 0.1

    print("step1. 通過センサ入力")
    for cntLoop in range(繰返回数):        
        MMP.アナログ読取()
        if 待時間 > 0 : time.sleep(待時間)
        センサ値1 = (MMP.mmpAnaVal[8][0],MMP.mmpAnaVal[8][1])
        センサ値2 = (MMP.mmpAnaVal[9][0],MMP.mmpAnaVal[9][1])
        print("  %03i" % cntLoop,": 1=", センサ値1,"／ 2=", センサ値2)

    MMP.通信切断()

#───────────────────────────    
def 恐竜ランド():

    MMP.通信接続_自動()

    モータ = (3,15)
    基盤   = (2,14)
    最小   = 1300
    最大   = 3500 
    間隔S  = 50
    間隔E  = -100
    停止   = 0.2

    print("step1. 基盤：電源ＯＮ")
    for i in range(2): MMP.PWM_VALUE(基盤[i], 4095)

    print("step2. DCモータ：速度アップ")
    for val in range(最小,最大,間隔S):
        for i in range(2): MMP.PWM_VALUE(モータ[i], val )
        time.sleep(停止)
    time.sleep(1)

    print("step3. DCモータ：速度ダウン")
    for val in range(最大,最小,間隔E):
        for i in range(2): MMP.PWM_VALUE(モータ[i], val )
        time.sleep(停止)

    print("step4. 基盤：電源ＯＦＦ")
    for i in range(2):
        MMP.PWM_VALUE(モータ[i], -1)
        MMP.PWM_VALUE(基盤[i], -1)

    MMP.通信切断()

#───────────────────────────    
def 小屋の恐竜():

    MMP.通信接続_自動()

    モータ = (1,13)
    最小 = 120
    最大 = 380

    print("step1. 恐竜の首 : 最小から最大")
    サーボ(モータ,最小,最大)

    MMP.通信切断()

#───────────────────────────    
def 小屋のメータ():

    MMP.通信接続_自動()

    メータ = (0,12)
    最小 = 120
    最大 = 370

    print("step1. 骨メータ: 最小から最大")
    サーボ(メータ,最小,最大)

    MMP.通信切断()

#───────────────────────────    
def ジオラマ():

    MMP.通信接続_自動()

    砂時計 = 5
    砂時計_最小   = 136
    砂時計_最大   = 616
    砂時計_増分   = 1
    砂時計_間隔   = 0.002

    print("step1. 砂時計 : 最小から最大")
    for val in range(砂時計_最小,砂時計_最大,砂時計_増分):
        MMP.PWM_VALUE(砂時計,val)
        time.sleep(砂時計_間隔)
    time.sleep(1)

    print("step2. 恐竜の首 : 最小から最大")
    恐竜 = (7,6)
    最小 = 230
    最大 = 370
    増分 = 2
    間隔 = 0.005
    サーボ(恐竜,最小,最大,増分,間隔)
    time.sleep(1)

    print("step3. 砂時計 PWM : 最大から最小")
    for val in range(砂時計_最大,砂時計_最小,-砂時計_増分):
        MMP.PWM_VALUE(砂時計,val)
        time.sleep(砂時計_間隔)

    MMP.通信切断()

#───────────────────────────    
def サーボ(モータ, 最小, 最大, 増分 = 2, 間隔 = 0.02, 待ち = 1, 中央補正 = -18):

    print("    -> PWM : 中央へ")
    中央 = int((最大 + 最小) / 2) + 中央補正
    for i in モータ:MMP.PWM_VALUE(i,中央)
    time.sleep(待ち)

    print("    -> PWM : 中央～最大へ")
    for val in range(中央,最大,増分):
        for i in モータ:MMP.PWM_VALUE(i,val)
        time.sleep(間隔)
    time.sleep(待ち)

    print("    -> PWM : 最大～最小へ")
    for val in range(最大,最小,-増分):
        for i in モータ:MMP.PWM_VALUE(i,val)
        time.sleep(間隔)
    time.sleep(待ち)

    print("    -> PWM : 最小～中央へ")
    for val in range(最小,中央,増分):
        for i in モータ:MMP.PWM_VALUE(i,val)
        time.sleep(間隔)

#====================================================== 
PLUGIN_NAME = "02.MECH SAURUS"
ACTIONS = {
    "コントローラ：フットスイッチの入力"               : ぺダル,
    "フィールド：通過センサーの入力"                   : 通過センサ    ,
    "フィールド：基盤電源,運搬機(DCモータ)の制御"      : 恐竜ランド    ,
    "フィールド-小屋：恐竜首(サーボ)の制御"            : 小屋の恐竜    ,
    "フィールド-小屋：メータのサーボ回転"              : 小屋のメータ  ,
    "フィールド-ジオラマ：恐竜首・砂時計(サーボ)の制御": ジオラマ
    }
