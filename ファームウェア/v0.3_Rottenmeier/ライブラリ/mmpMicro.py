#============================================================
# MicroPython ラッパ
#------------------------------------------------------------
#【ファイル配置方法】
# 同一 または libフォルダー(推奨)
#============================================================
from mmp_core import Core
from mmp_adapter_micropy import MicroPyAdapter
import time

#============================================================
# MMPクラス
#============================================================
class mmp(Core):
    #------------------------------------------------------------
    # 初期化
    #------------------------------------------------------------
    def __init__(
        self                    ,
        arg読込調整     = 64    ,
        arg通信速度     = 115200,
        arg時間切れ     = 100   ,
        arg割当ピンTX   = 0     ,
        arg割当ピンRX   = 1     ,
        argシリアル番号 = 0     ,
        ):

        # UARTアダプタの初期化
        adapter = MicroPyAdapter(
            uart_id     = argシリアル番号   ,
            tx_pin      = arg割当ピンTX     ,
            rx_pin      = arg割当ピンRX     ,
            baud        = arg通信速度       ,
            timeout_ms  = arg時間切れ       ,
            )

        # 親クラスの初期化
        super().__init__(
            adapter                         ,
            読込調整    = arg読込調整       ,
            baud        = arg通信速度       ,
            timeout_ms  = int(arg時間切れ)  ,
            )

        # 初期化完了メッセージ
        print(f" - UART     = GPIO-Serial[{argシリアル番号}]")
        print(f"   - TX     = {arg割当ピンTX}")
        print(f"   - RX     = {arg割当ピンRX}")

    #------------------------------------------------------------
    # 通信接続
    #------------------------------------------------------------
    def 通信接続(self):
        self.接続済 = False
        print(f"<<Connecting(GPIO UART{self.adapter.uart_id})...>>")
        try:
            self.adapter.open()
        except Exception as e:
            raise Exception(f"UARTに接続失敗: {e}")
        time.sleep_ms(5)
        self.バッファ消去()
        if self.バージョン確認():
            print(f"  -> Connected(Ver.{self.version})")
            self.接続済 = True
            return True
        self.通信切断()
        return False

    #------------------------------------------------------------
    def バッファ消去(self):
        self.adapter.flush()

    #------------------------------------------------------------
    def 通信切断(self):
        try             :self.adapter.close()
        except Exception:pass
        print("<<Disconnected>>")
        self.接続済 = False
