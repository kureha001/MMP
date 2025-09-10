#======================================================
# Mech04_Tornado.py （共通UTのみ使用）
#======================================================
import time
from 共通ユーティリティ import (
    通信接続,
    通信切断,
    アナログ入力測定,
    MP3_再生        ,
    PWM_電源     ,
    PWM_出力
    )

#======================================================
def テスト実行():

    print("#####################")
    print("#                   #")
    print("#  04:MECH TORNADO  #")
    print("#                   #")
    print("#####################")

    try:
        通信接続()

        print("\n--------------")
        print("Switch(ANA-IN)")
        print("--------------")
        アナログ入力測定(
            スイッチ数  = 4     , # 押しスイッチ×4
            参加人数    = 1     , # 1人専用
            丸め        = 50    , # アナログ値の丸め
            繰返回数    = 100   , # アドレス切替回数
            待時間      = 0.1   , # ウェイト(秒)
        )

        print("\n---------")
        print("Doll Legs")
        print("---------")
        基盤CH = (0)
        print("  1.Lift (DC Motor:ON)")
        PWM_電源(基盤CH, True)
        time.sleep(2)
        print("  2.Down (DC Motor:OFF)")
        PWM_電源(基盤CH, False)

        print("\n--------")
        print("Bar Lock")
        print("--------")
        print("  1.Free (Servo Motor:HIGH)")
        PWM_出力(1, 160)
        time.sleep(2)
        print("  2.Lock (Servo Motor:LOW)")
        PWM_出力(1, 100)
        time.sleep(2)
        print("  3.Free (Servo Motor:HIGH)")
        PWM_出力(1, 160)

        print("\n--------")
        print("BGM(MP3)")
        print("--------")
        MP3_再生(
            arg再生一覧 = [
                (4,1),  # メインBGM：タイトル画面
                (4,2),  # メインBGM：プレイ画面
                (4,3)   # メインBGM：終了画面
                ]
            )

    finally:
        try: PWM_電源(MMP, 基盤CH, False)
        except Exception: pass
        通信切断()
