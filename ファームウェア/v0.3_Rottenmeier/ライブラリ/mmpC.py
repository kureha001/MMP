#============================================================
# CPython ラッパ
#------------------------------------------------------------
#【ファイル配置方法】
# 同一 または 共通フォルダー(推奨)
#============================================================
from mmp_core import Core, ConnectException
from mmp_adapter_cpython import CpythonAdapter
import time

#============================================================
# MMPクラス
#============================================================
class mmp(Core):
    #------------------------------------------------------------
    # 初期化
    #------------------------------------------------------------
    def __init__(
        self                ,
        arg読込調整 = 64    ,
        arg通信速度 = 115200,
        arg時間切れ = 0.2   ,
        ):

        # UARTアダプタの初期化
        adapter = CpythonAdapter(
            baud        = arg通信速度       ,
            timeout_s   = float(arg時間切れ),
            )
        self.adapter = adapter  # 明示保持（安全のため）

        # 親クラスの初期化
        super().__init__(
            adapter                                     ,
            読込調整    = arg読込調整                   ,
            baud        = arg通信速度                   ,
            timeout_ms  = int(round(arg時間切れ * 1000)),
            )

        # 初期化完了メッセージ
        print(" - UART     = USB-Serial")

    #------------------------------------------------------------
    # 通信接続
    #------------------------------------------------------------
    def 通信接続(self, comm_num=None):
        return self.通信接続_自動() if comm_num is None else self.通信接続_指定(comm_num)
    #------------------------------------------------------------
    # 通信接続：COMポートを自動に接続
    #------------------------------------------------------------
    def 通信接続_自動(self):

        self.接続済 = False
        print("<<Scanning...>>")

        # COMポートをスキャン
        for COM番号 in range(0, 100):

            # カレントのCOMポートを開く
            port = f"COM{COM番号}"
            ok = False
            try:
                # COMポートを開く
                self.adapter.open_port(port)
                print(f"  - {port}")

                # 通信バッファ消去
                time.sleep(0.005)
                self.バッファ消去()

                # デバイス情報を取得
                # ※失敗した場合は、次のCOMポートへ
                time.sleep(0.2)
                if not self.バージョン確認(): continue

                # デバイス情報の取得に成功ならば、正常で終了する
                print(f"  -> Connected({port})")
                print(f"  -> Connected(Ver.{self.version})")
                self.接続済 = True
                ok = True
                return True

            # 以上の場合、フラグを失敗にセット
            except Exception: ok = False

            # 正常/異常の共通後処理
            finally:
                # 接続失敗ならば、カレントのCOMポートを閉じる
                if not ok:
                    try: self.adapter.close()
                    except Exception: pass

        # COMポートが見つからなかった場合
        self.通信切断()
        return False
    #------------------------------------------------------------
    # 通信接続：COMポートを指定し接続
    #------------------------------------------------------------
    def 通信接続_指定(self, comm_num):
        self.接続済 = False
        print(f"<<Connecting(USB UART {comm_num})...>>")
        try:
            self.adapter.open_port(comm_num)
        except Exception as e:
            raise ConnectException(f"UARTに接続失敗: {e}")

        time.sleep(0.005)
        self.バッファ消去()

        if self.バージョン確認():
            print(f"  -> Connected(Ver.{self.version})")
            self.接続済 = True
            return True

        try             : self.adapter.close()
        except Exception: pass

        self.通信切断()
        return False

    #------------------------------------------------------------
    def バッファ消去(self):
        self.adapter.flush()

    #------------------------------------------------------------
    def 通信切断(self):
        try             : self.adapter.close()
        except Exception: pass
        print("<<Disconnected>>")
        self.接続済 = False
