#======================================================
# Mech04_Tornado.py （共通UTのみ使用）
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

        print("\n--------------")
        print("Switch(ANA-IN)")
        print("--------------")
        アナログ入力測定(
            MMP,
            スイッチ数  = 4     , # 押しスイッチ×4
            参加人数    = 1     , # 1人専用
            丸め        = 50    , # アナログ値の丸め
            繰返回数    = 100   , # アドレス切替回数
            待時間      = 0.1   , # ウェイト(秒)
        )

        print("\n---------")
        print("Doll Legs")
        print("---------")
        print("  1.Lift (DC Motor:ON)")
        PWM_電源_ON(MMP, 0)
        time.sleep(2)
        print("  2.Down (DC Motor:OFF)")
        PWM_電源_OFF(MMP, 0)

        print("\n--------")
        print("Bar Lock")
        print("--------")
        print("  1.Free (Servo Motor:HIGH)")
        PWM_出力(MMP, 1, 160)
        time.sleep(2)
        print("  2.Lock (Servo Motor:LOW)")
        PWM_出力(MMP, 1, 100)
        time.sleep(2)
        print("  3.Free (Servo Motor:HIGH)")
        PWM_出力(MMP, 1, 160)

        print("\n--------")
        print("BGM(MP3)")
        print("--------")
        MP3_再生(
            MMP,
            再生リスト=[
                (4,1),  # メインBGM：タイトル画面
                (4,2),  # メインBGM：プレイ画面
                (4,3)   # メインBGM：終了画面
                ]
            )

    finally:
        MMP.通信切断()
