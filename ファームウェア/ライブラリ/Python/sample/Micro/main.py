# -*- coding: utf-8 -*-
#============================================================
# MicroPython用
# （例: RP2040/2350） UART0, TX=0, RX=1 想定
#============================================================
TARGET = "MicroPython"
from mmpMicro import MmpClient, MicroPyAdapter
mmp = MmpClient(MicroPyAdapter(uart_id=0, tx=0, rx=1))

from test_common import run_all

if __name__ == "__main__":
    run_all(mmp, TARGET)
