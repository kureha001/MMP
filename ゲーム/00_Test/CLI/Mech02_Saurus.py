#======================================================
# Mech02_Saurus.py
#======================================================
import time
from 共通ユーティリティ import (
    通信接続,
    通信切断,
    アナログ入力測定,
    MP3_再生        ,
    PWM_電源        ,
    PWM_出力        ,
    PWM_移動        ,
    PWM_移動_上中下 ,
)

#======================================================
def テスト実行():

    print("####################")
    print("#                  #")
    print("#  02:MECH SAURUS  #")
    print("#                  #")
    print("####################")

    try:
        通信接続()

        print("-------------------")
        print("Foot pedals(ANA-IN)")
        print("-------------------")
        アナログ入力測定(
            スイッチ数   = 2,      # 各コントローラ 2 スイッチ
            参加人数     = 2,      # 2 プレイヤー
            丸め         = 200,
            繰返回数     = 50,
            待時間       = 0.05,
        )

        print("---------------")
        print("Sensors(ANA-IN)")
        print("---------------")
        アナログ入力測定(
            スイッチ数      = 2,          # 各センサ 2 スイッチ（0,1）
            参加人数        = 10,         # PL=0..7 は予約、PL=8,9 を使うため 10 まで確保
            丸め            = 100,
            繰返回数        = 50,
            待時間          = 0.05,
            表示プレイヤー  = (8, 9),     # ← ここで最後の 2 プレイヤーだけ表示
            表示スイッチ    = (0, 1)      # ← スイッチ 0 と 1 を表示
        )

        print("---------------")
        print("Dinosaur Island")
        print("---------------")
        モータCH     = (3, 15)   # DCモータ
        基盤CH       = (2, 14)   # 基盤電源
        最小v, 最大v = 1300, 3500
        間隔S, 間隔E = 80, 120
        待ちs        = 0.2

        print("  1/4.Power ON")
        PWM_電源(基盤CH, True)   # 恐竜アイランド基盤
        time.sleep(1)

        print("  2/4.DC Motor: PWM min to max")
        PWM_移動(モータCH ,最小v, 最大v, 間隔S, 待ちs)
        time.sleep(1)

        print("  3/4.DC Motor: PWM max to min")
        PWM_移動(モータCH ,最大v, 最小v, 間隔E, 待ちs)

        print("  4/4.Power OFF")
        PWM_出力(モータCH, 0)
        PWM_電源(基盤CH, False)   # 恐竜アイランド基盤

        print("------------")
        print("Hut:Dinosaur")
        print("------------")
        恐竜CH       = (1, 13)
        最小v, 最大v = 120, 380
        中央補正     = -18
        増分, 間隔s, 待ちs = 5, 0.01, 0.5
        PWM_移動_上中下(恐竜CH, 最小v, 最大v, 中央補正, 増分, 間隔s, 待ちs)

        print("---------")
        print("Hut:Meter")
        print("---------")
        メータCH = (0, 12)
        最小v, 最大v = 120, 370
        中央補正     = 0
        増分, 間隔s, 待ちs = 5, 0.01, 0.5
        PWM_移動_上中下(メータCH, 最小v, 最大v, 中央補正, 増分, 間隔s, 待ちs)

        print("-------")
        print("Diorama")
        print("-------")
        砂時計CH            = (5)
        砂_最小v, 砂_最大v  = 136, 616
        中央補正            = -18
        砂_増分, 砂_待ちs   = 5, 0.002

        print(">Hourglass: PWM min to max")
        PWM_移動(砂時計CH ,砂_最小v, 砂_最大v, 砂_増分, 砂_待ちs)
        time.sleep(1)

        print(">Dinosaur")
        最小v, 最大v        = 230, 370
        中央補正            = 0
        増分, 間隔s, 待ちs  = 3, 0.005, 0.2
        ジオラマ恐竜CH = (7, 6)
        PWM_移動_上中下(ジオラマ恐竜CH, 最小v, 最大v, 中央補正, 増分, 間隔s, 待ちs)
        time.sleep(1)

        print(">Hourglass: PWM max to min")
        PWM_移動(砂時計CH ,砂_最大v, 砂_最小v, 砂_増分, 砂_待ちs)

        print("--------")
        print("BGM(MP3)")
        print("--------")
        MP3_再生(
            arg再生一覧 = [
                (2,1),  # メインBGM：タイトル画面
                (2,2),  # メインBGM：プレイ画面
                (2,3)   # メインBGM：終了画面
                ]
            )

    finally:
        try:
            PWM_電源(基盤CH, False)   # 恐竜アイランド基盤
        except Exception: pass
        try:
            PWM_出力(モータCH, 0)     # DCモータ
        except Exception: pass
        通信切断()
