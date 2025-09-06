
# mmp_adapter_base.py
# Common adapter interface for CPython / MicroPython / CircuitPython.
# Each concrete adapter must implement a minimal non-blocking I/O surface.

class MmpAdapterBase:
    """Minimal adapter surface used by mmp_core.MmpClient."""
    def __init__(self):
        self.connected_baud = None

    # --- lifecycle ---
    def open_baud(self, baud: int) -> bool:
        """Open transport at baud. Returns True on success."""
        raise NotImplementedError

    def close(self) -> None:
        """Close transport if open."""
        raise NotImplementedError

    # --- I/O ---
    def clear_input(self) -> None:
        """Non-blocking: drain / discard any pending input bytes."""
        raise NotImplementedError

    def write_ascii(self, s: str) -> None:
        """Write ASCII text (non-blocking or buffered)."""
        raise NotImplementedError

    def read_one_char(self):
        """Return one character (str len 1) if available, else None (non-blocking)."""
        raise NotImplementedError

    def flush(self) -> None:
        """Flush output if the platform supports it."""
        pass

    # --- timing ---
    def sleep_ms(self, ms: int) -> None:
        raise NotImplementedError

    def now_ms(self) -> int:
        """Return a monotonic millisecond counter (wrap allowed)."""
        raise NotImplementedError

    def ticks_diff(self, now_ms: int, start_ms: int) -> int:
        """Return (now_ms - start_ms) in ms, handling wrap if needed."""
        # Default naive implementation works for large monotonic counters.
        return int(now_ms - start_ms)
