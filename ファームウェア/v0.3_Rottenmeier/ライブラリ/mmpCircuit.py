#============================================================
# CircuitPython ラッパ
#------------------------------------------------------------
#【ファイル配置方法】
# 同一 または libフォルダー(推奨)
#============================================================
from mmp_core import Core
from mmp_adapter_circuitpy import CircuitPyAdapter
import time

#============================================================
# MMPクラス
#============================================================
class mmp(Core):
    #--------------------------------------------------------
    # 初期化
    #--------------------------------------------------------
    def __init__(
        self                    ,
        arg読込調整     = 128   ,
        arg通信速度     = 115200,
        arg時間切れ     = 100   ,
        arg割当ピンTX   = None  ,
        arg割当ピンRX   = None  ,
        ):

        # UARTアダプタの初期化
        adapter = CircuitPyAdapter(
            tx_pin      = arg割当ピンTX ,
            rx_pin      = arg割当ピンRX ,
            baud        = arg通信速度   ,
            timeout_ms  = arg時間切れ   ,
            rxbuf       = 64            ,
            )

        # 親クラスの初期化
        super().__init__(
            adapter                         ,
            読込調整    = arg読込調整       ,
            baud        = arg通信速度       ,
            timeout_ms  = int(arg時間切れ)  )

        # 初期化完了メッセージ
        print( " - UART     = GPIO-Serial")
        print(f"   - TX     = {arg割当ピンTX}")
        print(f"   - RX     = {arg割当ピンRX}")

    #--------------------------------------------------------
    # 通信接続
    #--------------------------------------------------------
    def 通信接続(self):

        self.接続済 = False
        print("<<Connecting(GPIO UART)...>>")

        # UART接続の確立
        try: self.adapter.open()
        except Exception as e: raise Exception(f"Fail to connect: {e}")

        # 通信バッファをクリア
        time.sleep(0.005)
        self.バッファ消去()

        # バージョン確認
        if self.バージョン確認():
            print(f"  -> Connected(Ver.{self.version})")
            self.接続済 = True
            return True

        # 失敗の場合は通信を切断
        self.通信切断()
        return False

    #--------------------------------------------------------
    def バッファ消去(self):
        self.adapter.flush()

    #--------------------------------------------------------
    def 通信切断(self):
        try             : self.adapter.close()
        except Exception: pass
        print("<<Disconnected>>")
        self.接続済 = False
