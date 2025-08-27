# -*- coding: utf-8 -*-
#======================================================
# ラウンチャー（CPython版）
#   - ドライバ: 共通.mmpRottenmeier（PC用）
#   - 実行したいテスト本体（MechXX_...）だけをコメント解除
#======================================================
import sys
sys.path.append("..")  # 例：上位フォルダにドライバがある前提
import 共通.mmpRottenmeier as drv

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
    # 例）通信速度等を変えるなら：
    # 実行_接続込み(lambda: drv.mmp(baud=115200))
