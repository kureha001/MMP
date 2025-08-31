# CircuitPython ラッパ
from mmp_core import Core
from mmp_adapter_circuitpy import CircuitPyAdapter
import time

class mmp(Core):
    def __init__(self,
                 arg読込調整=128,
                 arg通信速度=115200,
                 arg時間切れ=100,
                 arg割当ピンTX=None,
                 arg割当ピンRX=None):
        adapter = CircuitPyAdapter(tx_pin=arg割当ピンTX,
                                   rx_pin=arg割当ピンRX,
                                   baud=arg通信速度,
                                   timeout_ms=arg時間切れ,
                                   rxbuf=64)
        super().__init__(adapter, 読込調整=arg読込調整, baud=arg通信速度, timeout_ms=int(arg時間切れ))
        print(" - UART     = GPIO-Serial")
        print(f"   - TX     = {arg割当ピンTX}")
        print(f"   - RX     = {arg割当ピンRX}")

    def 通信接続(self):
        self.接続済 = False
        print("<<Connecting(GPIO UART)...>>")
        try:
            self.adapter.open()
        except Exception as e:
            raise Exception(f"UARTに接続失敗: {e}")
        time.sleep(0.005)
        self.バッファ消去()
        if self.バージョン確認():
            print(f"  -> Connected(Ver.{self.version})")
            self.接続済 = True
            return True
        self.通信切断()
        return False

    def バッファ消去(self):
        self.adapter.flush()

    def 通信切断(self):
        try:
            self.adapter.close()
        except Exception:
            pass
        print("<<Disconnected>>")
        self.接続済 = False
