#============================================================
# CPython用アダプタ（pyserial）
#------------------------------------------------------------
#【ファイル配置方法】
# 同一 または 共通フォルダー(推奨)
# 　共通フォルダには、以下の要領でパスを通します
# 　PYTHONPATH=D:\WS\MMP\ゲーム\共通\
#============================================================
from mmp_adapter_base import UartAdapterBase
import serial

class CpythonAdapter(UartAdapterBase):
    #------------------------------------------------------------
    # 初期化
    #------------------------------------------------------------
    def __init__(
        self                ,
        baud        = 115200,
        timeout_s   = 0.2   ,
        ):

        self.baud       = int(baud)
        self.timeout_s  = float(timeout_s)
        self.uart       = None
    #------------------------------------------------------------
    # UARTオープン
    #------------------------------------------------------------
    def open_port(self, port_name):
        s               = serial.Serial()
        s.baudrate      = self.baud
        s.timeout       = self.timeout_s
        s.write_timeout = self.timeout_s
        s.port          = port_name
 
        s.open()
        self.uart = s
 
        return True
    #------------------------------------------------------------
    # 受信データの有無確認
    #------------------------------------------------------------
    def rx_ready(self):
        try             : return int(self.uart.in_waiting) or 0
        except Exception: return 0
    #------------------------------------------------------------
    # データ読み込み
    #------------------------------------------------------------
    def read(self, n):
        try             : return self.uart.read(n)
        except Exception: return b""
    #------------------------------------------------------------
    # データ書き込み
    #------------------------------------------------------------
    def write(self, b):
        try             : return self.uart.write(b)
        except Exception: return 0
    #------------------------------------------------------------
    # 通信バッファを消去
    #------------------------------------------------------------
    def flush(self):
        try:
            self.uart.reset_input_buffer()
            self.uart.reset_output_buffer()
        except Exception: pass
    #------------------------------------------------------------
    # UARTクローズ
    #------------------------------------------------------------
    def close(self):
        try:
            if self.uart and self.uart.is_open: self.uart.close()
        except Exception: pass
        self.uart = None
