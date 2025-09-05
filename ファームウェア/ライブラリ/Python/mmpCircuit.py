
# mmpCircuit.py - CircuitPython entry point
#
# Example (GPIO UART):
#   import board
#   from mmpCircuit import MmpClient, CircuitPyAdapter
#   cli = MmpClient(CircuitPyAdapter(tx_pin=board.TX, rx_pin=board.RX))
#   cli.ConnectAutoBaud()
#
# Example (USB CDC secondary channel enabled in boot.py):
#   from mmpCircuit import MmpClient, CircuitPyAdapter
#   cli = MmpClient(CircuitPyAdapter(use_usb_cdc=True))
#   cli.ConnectAutoBaud()

from .mmp_core import MmpClient
from .mmp_adapter_circuitpy import CircuitPyAdapter

__all__ = ["MmpClient", "CircuitPyAdapter"]
