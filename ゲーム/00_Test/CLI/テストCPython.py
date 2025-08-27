#======================================================
# ラウンチャー（CPython版）
#======================================================
#import sys; sys.path.append('..'); import 共通.MMP
import sys; sys.path.append("..")
import 共通.mmpRottenmeier as ライブラリ

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
    テスト実行(lambda: ライブラリ.mmp())
    # 例）通信速度等を変えるなら：
    # 実行_接続込み(lambda: drv.mmp(baud=115200))
