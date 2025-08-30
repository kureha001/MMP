#======================================================
# ラウンチャー（CircuitPython版)
#======================================================
import mmpCircuit as ライブラリ

#------------------------------------------------------
# テストしたいアプリをコメントアウトで選択
# （1本だけ有効にしてください）
#------------------------------------------------------
from Mech01_Bullets import テスト実行
#from Mech02_Saurus  import テスト実行
#from Mech03_Hunter  import テスト実行
#from Mech04_Tornado import テスト実行

if __name__ == "__main__":
    # 既定のTX/RX/ボーレートをライブラリ側で持たせている前提
    # 必要に応じて下記のようにboardピンを明示指定できます:
    # import board
    # テスト実行(lambda: ライブラリ.mmp(tx_pin=board.TX, rx_pin=board.RX, baud=115200, timeout_ms=100))
    テスト実行(lambda: ライブラリ.mmp())
