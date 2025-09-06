
# mmpC.py - CPython entry point
#
# Usage:
#   from mmpC import MmpClient, CpyAdapter
#   cli = MmpClient(CpyAdapter(port=None, preferred_ports=["COM3","/dev/ttyUSB0"]))
#   cli.ConnectAutoBaud()
#   print(cli.Info.Version())

from mmp_core import MmpClient
from mmp_adapter_cpython import CpyAdapter

__all__ = ["MmpClient", "CpyAdapter"]
