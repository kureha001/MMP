# CPython ラッパ
from mmp_core import Core
from mmp_adapter_cpython import CpythonAdapter
import time

class mmp(Core):
    def __init__(self, arg読込調整=64, arg通信速度=115200, arg時間切れ=0.2):
        adapter = CpythonAdapter(baud=arg通信速度, timeout_s=float(arg時間切れ))
        super().__init__(adapter, 読込調整=arg読込調整, baud=arg通信速度, timeout_ms=int(round(arg時間切れ*1000)))
        print(" - UART     = USB-Serial")

    def 通信接続(self):
        self.接続済 = False
        print("<<Scanning...>>")
        for COM番号 in range(0, 100):
            port = f"COM{COM番号}"
            try:
                self.adapter.open_port(port)
                print(f"  - {port}")
                time.sleep(0.005)
                self.バッファ消去()
                time.sleep(0.2)
                if self.バージョン確認():
                    print(f"  -> Connected({port})")
                    print(f"  -> Connected(Ver.{self.version})")
                    self.接続済 = True
                    return True
            except Exception:
                pass
            try:
                self.adapter.close()
            except Exception:
                pass
        self.通信切断()
        return False

    def 通信接続_指定(self, comm_num):
        self.接続済 = False
        print(f"<<Connecting(USB UART {comm_num})...>>")
        try:
            self.adapter.open_port(comm_num)
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
