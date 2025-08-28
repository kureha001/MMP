#======================================================
# Mech02_Saurus.py
#======================================================
import time
from 共通ユーティリティ import (
    アナログ入力測定,
    MP3_再生        ,
    PWM_電源_ON     ,
    PWM_電源_OFF    ,
    PWM_出力
)

#======================================================
def テスト実行(new_mmp):

    MMP = new_mmp()
    MMP.通信接続()

    try:
        print("-------------------")
        print("Foot pedals(ANA-IN)")
        print("-------------------")
        アナログ入力測定(
            MMP,
            スイッチ数   = 2,      # 各コントローラ 2 スイッチ
            参加人数     = 2,      # 2 プレイヤー
            丸め         = 200,
            繰返回数     = 50,
            待時間       = 0.2,
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
            待時間          = 0.2,
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

        print("  2/4.DC Motor：PWM min to max")
        for val in range(最小, 最大, 間隔S):
            PWM_出力(MMP, モータCH, val)
            time.sleep(停止s)

        time.sleep(1)

        print("  3/4.DC Motor：PWM max to min")
        for val in range(最大, 最小, 間隔E):
            PWM_出力(MMP, モータCH, val)
            time.sleep(停止s)

        print("  4/4.Power OFF")
        PWM_出力(MMP, モータCH, 0)
        PWM_電源_OFF(MMP, 基盤CH)

        print("------------")
        print("Hut:Dinosaur")
        print("------------")
        恐竜CH = (1, 13)
        最小v, 最大v = 120, 380
        中央補正 = -18
        増分, 間隔s, 待ちs = 2, 0.02, 1.0

        サーボスイープ(恐竜CH, 最小v, 最大v, 中央補正, 増分, 間隔s, 待ちs)

        print("---------")
        print("Hut:Meter")
        print("---------")
        メータCH = (0, 12)
        サーボスイープ(メータCH, 120, 370, 0, 2, 0.02, 1.0)

        print("-------")
        print("Diorama")
        print("-------")
        砂時計CH         = 5
        砂_最小, 砂_最大 = 136, 616
        砂_増分, 砂_間隔 = 1, 0.002

        print(">Hourglass: PWM min to max")
        for v in range(砂_最小, 砂_最大, 砂_増分):
            PWM_出力(MMP, 砂時計CH, v); time.sleep(砂_間隔)
        time.sleep(1)

        print(">Dinosaur")
        ジオラマ恐竜CH = (7, 6)
        サーボスイープ(ジオラマ恐竜CH, 230, 370, 0, 2, 0.005, 1.0)
        time.sleep(1)

        print(">Hourglass: PWM max to min")
        for v in range(砂_最大, 砂_最小, -砂_増分):
            PWM_出力(MMP, 砂時計CH, v); time.sleep(砂_間隔)

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

        def サーボスイープ(chs, vmin, vmax, center_offset=0, step=2, wait=0.02, hold=1.0):
            center = int((vmin + vmax) / 2) + center_offset
            print("  1/4.PWM: Cnter")
            PWM_出力(MMP, chs, center); time.sleep(hold)

            print("  2/4.PWM: Cnter to Max")
            for v in range(center, vmax, step):
                PWM_出力(MMP, chs, v); time.sleep(wait)
            time.sleep(hold)

            print("  3/4.PWM: Max   to Min")
            for v in range(vmax, vmin, -step):
                PWM_出力(MMP, chs, v); time.sleep(wait)
            time.sleep(hold)

            print("  4/4.PWM: Min   to Center")
            for v in range(vmin, center, step):
                PWM_出力(MMP, chs, v); time.sleep(wait)

    finally:
        try:
            PWM_電源_OFF(MMP, (2, 14))   # 恐竜アイランド基盤
            PWM_出力(MMP, (3, 15), 0)    # DCモータ
        except Exception: pass
        MMP.通信切断()
