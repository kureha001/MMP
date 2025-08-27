# -*- coding: utf-8 -*-
#======================================================
# ラウンチャー（MicroPython版）
#   - ドライバ: mmpRottenmeier_micro（マイコン用）
#   - 実行したいテスト本体（MechXX_...）だけをコメント解除
#======================================================
import mmpRottenmeier_micro as drv

#------------------------------------------------------
# テストしたいアプリをコメントアウトで選択
# （1本だけ有効にしてください）
#------------------------------------------------------
from Mech01_Bullets import テスト実行
# from Mech02_Saurus  import テスト実行
# from Mech03_Hunter  import テスト実行
# from Mech04_Tornado import テスト実行

if __name__ == "__main__":
    # ミドルウェア生成関数だけ渡す（必要なら引数をここで指定可能）
    実行_接続込み(lambda: drv.mmp())
    # 例）UARTを変えるなら：
    # 実行_接続込み(lambda: drv.mmp(uart_id=1, tx_pin=0, rx_pin=1, baud=115200))
