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

    print("------")
    print("ペダル")
    print("------")

    繰返回数 = 50   # アドレス切替回数
    待時間   = 0.2  # ウェイト(秒)

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

#───────────────────────────    
def 通過センサ():

    print("----------")
    print("通過センサ")
    print("----------")

    繰返回数 = 50   # アドレス切替回数
    待時間   = 0.2  # ウェイト(秒)

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

#───────────────────────────    
def 恐竜ランド():

    print("----------")
    print("恐竜ランド")
    print("----------")

    モータ = (3,15) #モータドライバ:DCモータ
    基盤   = (2,14) #モータドライバ:基盤
    最小   = 1300
    最大   = 3500 
    間隔S  = 50
    間隔E  = -100
    停止   = 0.2

    print("-> 基盤：電源ＯＮ")
    for i in range(2): MMP.PWM_VALUE(基盤[i], 4095)

    print("-> DCモータ：速度アップ")
    for val in range(最小,最大,間隔S):
        for i in range(2): MMP.PWM_VALUE(モータ[i], val )
        time.sleep(停止)

    time.sleep(1)

    print("-> DCモータ：速度ダウン")
    for val in range(最大,最小,間隔E):
        for i in range(2): MMP.PWM_VALUE(モータ[i], val )
        time.sleep(停止)

    print("-> 基盤：電源ＯＮ")
    for i in range(2):
        MMP.PWM_VALUE(モータ[i], -1)
        MMP.PWM_VALUE(基盤[i], -1)

#───────────────────────────    
def 小屋の恐竜():

    print("------------")
    print("小屋の恐竜")
    print("------------")

    モータ = (1,13) #サーボモータ：恐竜
    最小 = 120
    最大 = 380

    print("-> 恐竜の首 : 最小から最大")
    サーボ(モータ,最小,最大)

#───────────────────────────    
def 小屋のメータ():

    print("------------")
    print("小屋のメータ")
    print("------------")

    メータ = (0,12) #サーボモータ：骨メータ
    最小 = 120
    最大 = 370
    print("-> 骨メータ: 最小から最大")
    サーボ(メータ,最小,最大)

#───────────────────────────    
def ジオラマ():

    print("--------")
    print("ジオラマ")
    print("--------")

    砂時計 = 5    #サーボモータ:砂時計
    砂時計_最小   = 136 #110～
    砂時計_最大   = 616 #～660
    砂時計_増分   = 1
    砂時計_間隔   = 0.002

    print("-> 砂時計 PWM : 最小から最大")
    for val in range(砂時計_最小,砂時計_最大,砂時計_増分):
        MMP.PWM_VALUE(砂時計,val)
        time.sleep(砂時計_間隔)
    time.sleep(1)

    print("-> 恐竜の首 : 最小から最大")
    恐竜 = (7,6) #サーボモータ:恐竜
    最小 = 230
    最大 = 370
    増分 = 2
    間隔 = 0.005
    サーボ(恐竜,最小,最大,増分,間隔)

    time.sleep(1)

    print("-> 砂時計 PWM : 最大から最小")
    for val in range(砂時計_最大,砂時計_最小,-砂時計_増分):
        MMP.PWM_VALUE(砂時計,val)
        time.sleep(砂時計_間隔)

#───────────────────────────    
def サーボ(
        モータ      ,   # サーボモータのアドレス
        最小        ,   # 110～
        最大        ,   # ～660
        増分 = 2    ,   # 増分
        間隔 = 0.02 ,   # 間隔
        待ち = 1    ,    # 待ち時間(秒)
        中央補正 = -18  
        ):

    print("->   PWM : 中央へ")
    中央 = int((最大 + 最小) / 2) + 中央補正
    for i in モータ:MMP.PWM_VALUE(i,中央)
    time.sleep(待ち)

    print("->   PWM : 中央～最大へ")
    for val in range(中央,最大,増分):
        for i in モータ:MMP.PWM_VALUE(i,val)
        time.sleep(間隔)
    time.sleep(待ち)

    print("->   PWM : 最大～最小へ")
    for val in range(最大,最小,-増分):
        for i in モータ:MMP.PWM_VALUE(i,val)
        time.sleep(間隔)
    time.sleep(待ち)

    print("->   PWM : 最小～中央へ")
    for val in range(最小,中央,増分):
        for i in モータ:MMP.PWM_VALUE(i,val)
        time.sleep(間隔)

#====================================================== 
# メイン処理
#====================================================== 
menu = {
    "1": ("フットスイッチの入力"                            , ぺダル        ),
    "2": ("通過センサーの入力"                              , 通過センサ    ),
    "3": ("フィールド(基盤電源,運搬機[DCモータ])の制御"     , 恐竜ランド    ),
    "4": ("フィールド(小屋の恐竜)のサーボ回転"              , 小屋の恐竜    ),
    "5": ("フィールド(小屋のメータ)のサーボ回転"            , 小屋のメータ  ),
    "6": ("フィールド(ジオラマの恐竜・砂時計)のサーボ回転"  , ジオラマ      )
    }

def clear():
    os.system("cls" if os.name == "nt" else "clear")

def main():
    #┬
    #○MMPを接続する。
    MMP.通信接続_自動()
    #│
    #◎メニュー
    while True:
        clear()
        print("=== メニュー ===")
        for key in sorted(menu, key=lambda x: int(x)):
            print(f"{int(key):02d}. {menu[key][0]}")
        print("00. 終了")

        choice = input("番号を入力してください: ").strip()
        if choice == "0":
            print("終了します。")
            MMP.通信切断()
            break

        choice_key = choice.lstrip("0") or "0"
        if choice_key in menu:
            clear()
            menu[choice_key][1]()  # 関数実行
            print("\nEnterキーでメニューに戻ります。")
            input()
        else:
            print("無効な入力です。Enterキーで続行。")
            input()

if __name__ == "__main__": main()