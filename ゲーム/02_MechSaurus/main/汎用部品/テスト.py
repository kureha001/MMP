#====================================================== 
# ＭＭＰライブラリ Ver 0.03 Rottenmeier用 サンプル
#------------------------------------------------------
# Ver 0.02.005　2025/04/28 By Takanari.Kureha
#       1.PWMエキスパンダ(PCA9685)マルチ対応
#====================================================== 

#====================================================== 
# インクルード
#====================================================== 
import mmpRottenmeier
import time

#────────────────────────────────────    
def ぺダル():

    繰返回数 = 50   # アドレス切替回数
    待時間   = 0.2  # ウェイト(秒)

    print("------")
    print("ペダル")
    print("------")
    MMP.アナログ設定(
            2,  # 使用するHC4067の個数(1～4)
            2,  # 使用するHC4067のPin数(1～16)
            200 # アナログ値の丸め(この数値以下は切り捨て)
            )
    #◎└┐繰り返し読み取る。
    for cntLoop in range(繰返回数):        
        #◎└┐全アドレスから読み取る。
        MMP.アナログ読取()
        if 待時間 > 0 : time.sleep(待時間)
        print("  %03i" % cntLoop,":", MMP.mmpAnaVal)

#────────────────────────────────────    
def 通過センサ():

    繰返回数 = 50   # アドレス切替回数
    待時間   = 0.2  # ウェイト(秒)
    全表示 = True   # True:全件表示／False:先頭末尾のみ表示

    print("----------")
    print("通過センサ")
    print("----------")
    MMP.アナログ設定(
            2,  # 使用するHC4067の個数(1～4)
            10, # 使用するHC4067のPin数(1～16)
            100 # アナログ値の丸め(この数値以下は切り捨て)
            )
    #◎└┐繰り返し読み取る。
    for cntLoop in range(繰返回数):        
        #◎└┐全アドレスから読み取る。
        MMP.アナログ読取()
        if 待時間 > 0 : time.sleep(待時間)
        センサ値1 = (MMP.mmpAnaVal[8][0],MMP.mmpAnaVal[8][1])
        センサ値2 = (MMP.mmpAnaVal[9][0],MMP.mmpAnaVal[9][1])
        print("  %03i" % cntLoop,": 1=", センサ値1,"／ 2=", センサ値2)

#────────────────────────────────────
def 恐竜ランド():
#　├→（ＰＷＤ：エスカレータのＤＣモータ）
    print("----------")
    print("恐竜ランド")
    print("----------")

    基盤  = (1,3) #恐竜ランドの基盤
    モータ  = (0,2) #恐竜ランドのDCモータ
    最小  = 1800
    最大  = 3500 
    間隔S = 50
    間隔E = -100
    停止  = 0.2

    for i in range(2): MMP.PWM_VALUE(基盤[i], 4095)

    for val in range(最小,最大,間隔S):
        print("PWM:",val)
        for i in range(2): MMP.PWM_VALUE(モータ[i], val )
        time.sleep(停止)

    time.sleep(1)

    for val in range(最大,最小,間隔E):
        print("PWM:",val)
        for i in range(2): MMP.PWM_VALUE(モータ[i], val )
        time.sleep(停止)

    for i in range(2):
        MMP.PWM_VALUE(モータ[i], -1)
        MMP.PWM_VALUE(基盤[i], -1)

#────────────────────────────────────
def 小屋の恐竜():
    print("------------")
    print("小屋の恐竜")
    print("------------")
    モータ = (4,6) #小屋の恐竜のサーボモータ
    最小 = 140
    最大 = 380
    サーボ(モータ,最小,最大)

#────────────────────────────────────
def 小屋のメータ():
    print("------------")
    print("小屋のメータ")
    print("------------")
    メータ = (5,7) #小屋の恐竜のサーボモータ
    最小 = 180
    最大 = 380
    サーボ(メータ,最小,最大)

#────────────────────────────────────
def ジオラマ():
    print("--------")
    print("ジオラマ")
    print("--------")
    砂時計 = 8    #ジオラマの砂時計のサーボモータ
    砂時計_最小   = 136 #110～
    砂時計_最大   = 616 #～660
    砂時計_増分   = 1
    砂時計_間隔   = 0.002

    for val in range(砂時計_最小,砂時計_最大,砂時計_増分):
        MMP.PWM_VALUE(砂時計,val)
        time.sleep(砂時計_間隔)
    time.sleep(1)

    恐竜 = (9,10) #時計台の恐竜のサーボモータ
    最小 = 230
    最大 = 370
    増分 = 2
    間隔 = 0.005
    サーボ(恐竜,最小,最大,増分,間隔)
#    サーボ(恐竜,最小,最大)

    time.sleep(1)

    for val in range(砂時計_最大,砂時計_最小,-砂時計_増分):
        MMP.PWM_VALUE(砂時計,val)
        time.sleep(砂時計_間隔)

#────────────────────────────────────
def サーボ(
        モータ      ,   # サーボモータのアドレス
        最小        ,   # 110～
        最大        ,   # ～660
        増分 = 2    ,   # 増分
        間隔 = 0.02 ,   # 間隔
        待ち = 1    ,    # 待ち時間(秒)
        中央補正 = -18  
        ):

    中央 = int((最大 + 最小) / 2) + 中央補正

    for i in モータ:MMP.PWM_VALUE(i,中央)
    time.sleep(待ち)

    for val in range(中央,最大,増分):
        for i in モータ:MMP.PWM_VALUE(i,val)
#        print(val)
        time.sleep(間隔)
    time.sleep(待ち)

    for val in range(最大,最小,-増分):
        for i in モータ:MMP.PWM_VALUE(i,val)
        time.sleep(間隔)
#        print(val)
    time.sleep(待ち)

    for val in range(最小,中央,増分):
        for i in モータ:MMP.PWM_VALUE(i,val)
        time.sleep(間隔)


#====================================================== 
# メイン処理
#====================================================== 
#┬
#○MMPを実体化する。
MMP = mmpRottenmeier.mmp()
#│
#○MMPを接続する。
MMP.通信接続_自動()
#│
#○テストする。
#ぺダル()
#通過センサ()
#恐竜ランド()
#小屋の恐竜()
#小屋のメータ()
ジオラマ()
#│
#○MMPを切断する。
MMP.通信切断()
#┴