# アダプタの抽象ベース（最小I/F）

class UartAdapterBase:
    def rx_ready(self):
        return 0

    def read(self, n):
        return b""

    def write(self, b):
        return 0

    def flush(self):
        pass

    def close(self):
        pass
