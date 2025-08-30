#======================================================
# Mech02_Saurus.py
#======================================================
import time
from 共通ユーティリティ import (
    アナログ入力測定,
    MP3_再生        ,
    PWM_電源_ON     ,
    PWM_電源_OFF    ,
    PWM_出力        ,
    PWM_スイープ    ,
)

#======================================================
def テスト実行(new_mmp):

    print("####################")
    print("#                  #")
    print("#  02:MECH SAURUS  #")
    print("#                  #")
    print("####################")
    MMP = new_mmp()
    if not MMP.通信接続():
        print(f"Failed to connect !!")
        return

    #======================================================
    try:
        """
        print("-------------------")
        print("Foot pedals(ANA-IN)")
        print("-------------------")
        アナログ入力測定(
            MMP,
            スイッチ数   = 2,      # 各コントローラ 2 スイッチ
            参加人数     = 2,      # 2 プレイヤー
            丸め         = 200,
            繰返回数     = 50,
            待時間       = 0.05,
            全件表示     = True    # そのまま全件表示
        )

        print("---------------")
        print("Sensors(ANA-IN)")
        print("---------------")
        アナログ入力測定(
            MMP,
            スイッチ数      = 2,          # 各センサ 2 スイッチ（0,1）
            参加人数        = 10,         # PL=0..7 は予約、PL=8,9 を使うため 10 まで確保
            丸め            = 100,
            繰返回数        = 50,
            待時間          = 0.05,
            全件表示        = False,      # フィルタ表示に切替えるので無視される
            表示プレイヤー  = (8, 9),     # ← ここで最後の 2 プレイヤーだけ表示
            表示スイッチ    = (0, 1)      # ← スイッチ 0 と 1 を表示
        )

        print("---------------")
        print("Dinosaur Island")
        print("---------------")
        モータCH = (3, 15)   # DCモータ
        基盤CH   = (2, 14)   # 基盤電源
        最小   = 1300
        最大   = 3500
        間隔S  = 50
        間隔E  = -100
        停止s  = 0.2

        print("  1/4.Power ON")
        PWM_電源_ON(MMP, 基盤CH)
        time.sleep(3)

        print("  2/4.DC Motor: PWM min to max")
        for val in range(最小, 最大, 間隔S):
            PWM_出力(MMP, モータCH, val)
            time.sleep(停止s)
        time.sleep(1)

        print("  3/4.DC Motor: PWM max to min")
        for val in range(最大, 最小, 間隔E):
            PWM_出力(MMP, モータCH, val)
            time.sleep(停止s)

        print("  4/4.Power OFF")
        PWM_出力(MMP, モータCH, 0)
        PWM_電源_OFF(MMP, 基盤CH)

        """
        print("------------")
        print("Hut:Dinosaur")
        print("------------")
        恐竜CH       = (1, 13)
        最小v, 最大v = 120, 380
        中央補正     = -18
        増分, 間隔s, 待ちs = 6, 0.02, 0.5
        PWM_スイープ(MMP, 恐竜CH, 最小v, 最大v, 中央補正, 増分, 間隔s, 待ちs)

        print("---------")
        print("Hut:Meter")
        print("---------")
        メータCH = (0, 12)
        最小v, 最大v = 120, 370
        中央補正     = 0
        増分, 間隔s, 待ちs = 4, 0.02, 0.5
        PWM_スイープ(MMP, メータCH, 最小v, 最大v, 中央補正, 増分, 間隔s, 待ちs)
#        PWM_スイープ(MMP, メータCH, 120, 370, 0, 2, 0.02, 1.0)

        print("-------")
        print("Diorama")
        print("-------")
        砂時計CH            = 5
        砂_最小, 砂_最大    = 136, 616
        中央補正            = -18
        砂_増分, 砂_待ちs   = 5, 0.002

        print(">Hourglass: PWM min to max")
        for v in range(砂_最小, 砂_最大, 砂_増分):
            PWM_出力(MMP, 砂時計CH, v); time.sleep(砂_待ちs)
        time.sleep(1)

        print(">Dinosaur")
        最小v, 最大v        = 230, 370
        中央補正            = 0
        増分, 間隔s, 待ちs  = 3, 0.005, 0.5
        ジオラマ恐竜CH = (7, 6)
        PWM_スイープ(MMP, ジオラマ恐竜CH, 最小v, 最大v, 中央補正, 増分, 間隔s, 待ちs)
        time.sleep(1)

        print(">Hourglass: PWM max to min")
        for v in range(砂_最大, 砂_最小, -砂_増分):
            PWM_出力(MMP, 砂時計CH, v); time.sleep(砂_待ちs)

        """
        print("--------")
        print("BGM(MP3)")
        print("--------")
        MP3_再生(
            MMP,
            再生リスト=[
                (2,1),  # メインBGM：タイトル画面
                (2,2),  # メインBGM：プレイ画面
                (2,3)   # メインBGM：終了画面
                ]
            )
        """

    finally:
        try:
            PWM_電源_OFF(MMP, (2, 14))   # 恐竜アイランド基盤
            PWM_出力(MMP, (3, 15), 0)    # DCモータ
        except Exception: pass
        MMP.通信切断()
