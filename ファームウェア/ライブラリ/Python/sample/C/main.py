# -*- coding: utf-8 -*-
#============================================================
# CPython用（PC）
#  接続先はアダプタ側の既定動作（自動列挙/自動検出）に依存
#============================================================
TARGET = "CPython"

from mmpC import MmpClient
from mmpC import CpyAdapter

mmp = MmpClient(CpyAdapter())

from test_common import run_all

if __name__ == "__main__":
    run_all(mmp, TARGET)
