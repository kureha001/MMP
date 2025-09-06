# -*- coding: utf-8 -*-
#============================================================
# CircuitPython用
# （例: RP2040/2350） TX=0, RX=1 想定
# ※ Adapterのtx_pin/rx_pin指定がboardピンオブジェクトの場合は、
# 適宜 board.GP0, board.GP1 などに変更してください。
#============================================================
TARGET = "CircuitPython"
from mmpCircuit import MmpClient, CircuitPyAdapter


# 引数：tx_pin=None, rx_pin=None, timeout_s=0.05, buffer_size=128
mmp = MmpClient(CircuitPyAdapter())

from test_common import run_all

if __name__ == "__main__":
    run_all(mmp, TARGET)
