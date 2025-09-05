
# mmpMicro.py - MicroPython entry point
#
# Example:
#   from mmpMicro import MmpClient, MicroPyAdapter
#   cli = MmpClient(MicroPyAdapter(uart_id=0, tx=0, rx=1))
#   cli.ConnectAutoBaud()

from .mmp_core import MmpClient
from .mmp_adapter_micropy import MicroPyAdapter

__all__ = ["MmpClient", "MicroPyAdapter"]
