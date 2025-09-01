#============================================================
# MicroPython用アダプタ（machine.UART）
#------------------------------------------------------------
#【ファイル配置方法】
# 同一 または libフォルダー(推奨)
#============================================================
from mmp_adapter_base import UartAdapterBase
from machine import UART, Pin
import time

#============================================================
#  プラットフォーム別アダプタ
#============================================================
class MicroPyAdapter(UartAdapterBase):
    #--------------------------------------------------------
    # 初期化
    #--------------------------------------------------------
    def __init__(
        self                ,
        uart_id     = 0     ,
        tx_pin      = 0     ,
        rx_pin      = 1     ,
        baud        = 115200,
        timeout_ms  = 100   ,
        ):

        self.uart_id    = int(uart_id)
        self.tx_pin     = int(tx_pin)
        self.rx_pin     = int(rx_pin)
        self.baud       = int(baud)
        self.timeout_ms = int(timeout_ms)
        self.uart       = None

    #--------------------------------------------------------
    # UARTオープン
    #--------------------------------------------------------
    def open(self):
        t_char = max(1, self.timeout_ms // 10)
        self.uart = UART(
            self.uart_id                        ,
            baudrate        = self.baud         ,
            timeout         = self.timeout_ms   ,
            timeout_char    = t_char            ,
            bits            = 8                 ,
            parity          = None              ,
            stop            = 1                 ,
            tx              = Pin(self.tx_pin)  ,
            rx              = Pin(self.rx_pin)  ,
            )
        return True

    #--------------------------------------------------------
    # 受信データの有無確認
    #--------------------------------------------------------
    def rx_ready(self):
        try             : return int(self.uart.any()) or 0
        except Exception: return 0

    #--------------------------------------------------------
    # データ読み込み
    #--------------------------------------------------------
    def read(self, n):
        try                 : return self.uart.read(n)
        except Exception:
            try             : return self.uart.read(1)
            except Exception: return b""

    #--------------------------------------------------------
    # データ書き込み
    #--------------------------------------------------------
    def write(self, b):
        try             : return self.uart.write(b)
        except Exception: return 0

    #--------------------------------------------------------
    # 通信バッファを消去
    #--------------------------------------------------------
    def flush(self):
        try:
            t0 = time.ticks_ms()
            while time.ticks_diff(time.ticks_ms(), t0) < 30:
                n = self.uart.any()
                if not n: break
                try: self.uart.read(n)
                except Exception:
                    try             : self.uart.read()
                    except Exception: break
                time.sleep_ms(1)
        except Exception: pass

    #--------------------------------------------------------
    # UARTクローズ
    #--------------------------------------------------------
    def close(self):
        try:
            if self.uart: self.uart.deinit()
        except Exception: pass
        self.uart = None
